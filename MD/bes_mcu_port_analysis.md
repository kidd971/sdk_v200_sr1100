# 移植 SPARK SDK 至目標 MCU —— 以絡達 AB1595 為例

> 建立於 2026-09-11。基準 `rel/v2.4.1_rc2`。
> **前提**：目標 MCU 平台已存在且可運行（clock / startup / DMA / UART / flash 均已具備），
> 要將 SPARK SDK 作為元件整合至既有的原廠 SDK。本文件以 **絡達（Airoha）AB1595** 為實例，
> 步驟與驗收里程碑對任何 Cortex-M 目標都適用；AB1595 專屬的只有 **BT stack 共存**與 ABI 變體選擇。
> **本階段目標**：**停用 BT**，完成 **BSP-validator** 與 **Profiler**。
> **與 BT stack 並存、Audio，作為下一階段。**
> SPARK Porting Guide：<https://sparkmicro.com/sdk-docs/index.html>
> **前置閱讀**：[spark_sdk_overview.md](spark_sdk_overview.md) —— SPARK SDK 的架構、prebuilt 形態、
> SWC 基本概念與兩支驗證工具。未接觸過本 SDK 者建議先行閱讀，Step 0 方有脈絡。
> 相關：[unidir_port_todo.md](unidir_port_todo.md)、[console_at_uart_routing.md](console_at_uart_routing.md)。

---

## 驗收里程碑

| # | 里程碑 | 驗收條件 | Step |
|---|---|---|---|
| M1 | SPI 通訊建立 | 可讀取 SR1120 chip ID | 3 |
| M2 | **BSP-validator 全數通過** | 11 項測試全數 OK | 4 |
| M3 | 無線連線建立 | 兩板配對成功、可收發、PER 合理 | 5 |
| M4 | **Profiler 量測完成** | DUT + Helper 輸出三情境處理時間 | 6 ← **本階段至此** |
| — | 與 BT stack 並存穩定 | 下一階段 N1 | — |
| — | Audio | 下一階段 N2 | — |

**本階段全程停用 BT**，一次僅引入一個變因。M2 和 M4 兩支工具**都不 link `audio_core`**，
因此「暫不實作 audio」為合法組態（SPARK 定義 Audio Core backend 為
*"only required for audio applications"*）。

**需實作的函式約 45 個**，多數為數行的轉接實作：
`swc_hal_facade.h` 20（單 radio SPI）、`common_facade.h` 14、
`bsp_validator_facade.h` 6（其中 1 個可為空實作）、`profiler_facade.h` 11（7 個與 common 重疊）。

工時不在這 45 個函式，而在 Step 0 的決策和 Step 4 的中斷／DMA 時序。

---

## 檔案清單一覽

### 一、要新增／修改

```
bsp/airoha/src/airoha.c                           板級 init                          Step 1
bsp/airoha/src/airoha_def.h                       pin map                            Step 1
bsp/airoha/src/airoha_spi.c                       radio SPI（自寫暫存器級）          Step 3
bsp/airoha/src/airoha_gpio.c                      CS / RESET pin                     Step 3
bsp/airoha/src/airoha_radio.c                     IRQ / NVIC pend                    Step 3-4
bsp/airoha/src/airoha_it.c                        中斷分派                           Step 4
bsp/airoha/src/airoha_timer_ext.c                 free running timer                 Step 4
bsp/airoha/CMakeLists.txt                         target 須名為 hardware（CMake 路線）Step 1

backend/airoha_backend/wireless_core_backend/     20 函式                            Step 1,4
backend/airoha_backend/common_backend/            14 函式，含衝突 A 的兩個 weak 覆寫 Step 2
backend/airoha_backend/bsp_validator_backend/      6 函式                            Step 4
backend/airoha_backend/profiler_backend/          11 函式（7 個沿用既有）            Step 6

app/tool/bsp_validator/sr1100/bsp_validator.c     main() → thread entry（衝突 C）   Step 4
app/example/hello_world/hello_world_coord.c       main() → thread entry（衝突 C）   Step 5
```

### 二、只 link，不修改

```
core/wireless/prebuilt/swc_api_sr1100_single_m33_lib.a          ← M4F 核心則取 _m4_ 變體
driver/spark_radio/sr_phy/prebuilt/sr_phy_sr1100_single_m33_lib.a
core/wireless/swc_api.h、swc_error.h、swc_stats.h、facade/swc_hal_facade.h
library/                                       （除 critical_section，下一階段才改）
module/spark_pairing/
```

### 三、本階段用不到，可排除於建置之外

```
core/audio/                                    下一階段 N2 才啟用
bsp/quasar-u535/                               整包
bsp/quasar/                                    保留作參考實作，不編入
third-party/stmicroelectronics/                整包
middleware/tinyusb/、module/tinyusb/           USB_AUDIO_ENABLED=OFF
driver/max98091/                               不使用外掛 codec
app/example/audio_*、puretone_*、star_network 等  本階段僅需 hello_world
```

`bsp/quasar/` 之下個別檔案的取捨與理由見 **[附錄 B](#附錄-b--本階段不需實作的項目)**。

---

## Step 0 — 前置條件

三項須於撰寫程式碼之前確認的事項，其結論決定後續所有步驟的形態。

### 0.1 確認核心並對齊 ABI

**優先序最高，約需一小時。此項未通過則後續工作均無意義。**

Wireless Core 和 PHY **都是 prebuilt `.a`，不是原始碼**，且依 CPU 架構分為 `m33` 與 `m4`
兩種變體。因此第一件事是確認 AB1595 的核心，再據以選定 library 變體。

**步驟一：確認 AB1595 的 Cortex-M 核心。** 向絡達取得規格或對其 SDK 編出的 `.o` 執行
`readelf -A`。兩條分支的目標值如下（均為 2026-09-11 於本 repo 實測）：

| | **Cortex-M33 / M35P** | **Cortex-M4F** |
|---|---|---|
| library 變體 | `swc_api_sr1100_single_m33_lib.a` | `swc_api_sr1100_single_m4_lib.a` |
| `Tag_CPU_arch` | `v8-M.mainline` | `v7E-M` |
| `Tag_FP_arch` | `FPv5/FP-D16 for ARMv8` | `VFPv4-D16` |
| `Tag_ABI_VFP_args` | `VFP registers` | `VFP registers` |
| `Tag_ABI_HardFP_use` | `SP only` | `SP only` |

```
$ arm-none-eabi-readelf -A core/wireless/prebuilt/swc_api_sr1100_single_m33_lib.a
  Tag_CPU_arch:         v8-M.mainline
  Tag_FP_arch:          FPv5/FP-D16 for ARMv8
  Tag_ABI_VFP_args:     VFP registers      ← hard float，不是 softfp
  Tag_ABI_HardFP_use:   SP only            ← 單精度
```

**步驟二：四項逐項比對。** 無論走哪條分支，後兩項都相同：**`-mfloat-abi=hard`、單精度**。
架構與 FP 版本則須與所選變體一致。

最易違反的是浮點 ABI 這項：**很多原廠 SDK 預設 `-mfloat-abi=softfp`** —— 同樣有 FPU，
但參數走整數暫存器而非 VFP。`arm-none-eabi-ld` 通常會攔下，但若被 `--no-warn-mismatch` 掩蓋，
即轉為**執行期參數錯置，且僅在傳遞 float 的路徑上失效**。

**動作**：對 AB1595 SDK 編出的任一 `.o` 執行 `readelf -A`，與上表逐項比對。
**不相符時應停止後續工作，向 SPARK 索取對應 ABI 的 library。此屬交期問題，非工程工時。**

> 本文件後續一律以 `_m33_` 變體舉例。若 AB1595 為 Cortex-M4F，將所有檔名中的
> `_m33_` 換成 `_m4_` 即可，其餘步驟完全相同。

### 0.2 Build 整合方式

**SPARK 建議**：SDK 以外部套件形式引入，**不應修改 SDK 內部檔案**
（*"external package... updated via a drop-in replacement" / "don't modify the SDK's top-level file"*）。

絡達 SDK 若為 Makefile/kbuild 架構，則直接採用 SPARK 的 **Non-CMake 路線**，
可同時迴避「不得修改 SDK」與「`MCU_DRIVER` 無 AB1595 分支」兩項問題：

1. 直接 link `core/wireless/prebuilt/swc_api_sr1100_single_m33_lib.a`
2. 直接 link `driver/spark_radio/sr_phy/prebuilt/sr_phy_sr1100_single_m33_lib.a`
3. include public header（僅透過 `swc_api.h`，不取用 `link/`、`protocol_stack/`、`xlayer/`）
4. 套上等效 compiler flag（hard-float、Thumb、FPU 一致）

`core/wireless/CMakeLists.txt` 裡 `MCU_DRIVER` → `m4`/`m33` 的 `FATAL_ERROR` **不會被執行到** ——
其作用僅止於選擇檔名，手動指定即可。SDK 目錄維持原狀，升級即為整包替換。

### 0.3 整合衝突的處理方式

裸板移植不會遭遇，**整合至既有 RTOS 則必然發生**。

| # | 衝突 | 位置 | 解法 | 本階段 |
|---|---|---|---|---|
| A | **PendSV 被 SPARK 佔用** | `quasar_radio.c:307`、`quasar_it.c:642`、`common_backend.c:35` | 改用**空的 NVIC IRQ 當 SWI** | ✅ 要 |
| C | **每支 app 具備各自的 `main()`** | `bsp_validator.c:188`、`hello_world_coord.c:88` | 改為 thread entry；`facade_wait_for_interrupt()` 改為 task block | ✅ 要 |
| B | **`CRITICAL_SECTION_ENTER` 是 `cpsid i` 全域關中斷** | `library/critical_section/arm_cortex_m/critical_section.c` | 改 **BASEPRI 遮罩** | ⏭ 下一階段 |

**A 與 C 須於本階段處理** —— 只要 RTOS 運行中即會發生，與 BT 是否啟用無關。
兩者僅需修改自有 backend，**SDK 內部無須任何異動**（`common_backend.c` 的 18 個 facade
全標了 `__attribute__((weak))`）。衝突 A 約 15 行，**應使用空置的 NVIC IRQ，而非 semaphore** ——
理由與實作草稿見 **[附錄 D](#附錄-d--衝突-a-的實作細節)**。

**B 可延至下一階段**，但**優先權關係須於現階段確定**：
BASEPRI 的門檻取決於 radio IRQ 和 BT IRQ 的相對位置，該關係於 Step 4 即已固定。

**優先權順序（SPARK 要求）**：

```
multi-radio timer IRQ（雙 radio 才有，全系統最高）
  > transceiver IRQ / SPI DMA IRQ
    > context switch（SWI）
      > 應用層
```

---

## Step 1 — 建立骨架並通過編譯

**異動檔案**（均為新增，SPARK 命名慣例為 `bsp/<new_bsp>/src/`）

```
bsp/airoha/src/airoha.c                         空的初始化
bsp/airoha/src/airoha_def.h                     pin map
bsp/airoha/CMakeLists.txt                       add_library(hardware "") ← target 名稱必須為 hardware
backend/airoha_backend/wireless_core_backend/   swc_hal_facade.h 全數空實作（回傳 0 / false）
backend/airoha_backend/common_backend/
backend/airoha_backend/bsp_validator_backend/
```

**驗收**：`APP=BSP-validator` 可 link 出 `.elf`，**且 prebuilt `.a` 無 ABI 警告**（0.1 的實地驗證）。

---

## Step 2 — Console 輸出

**異動檔案**：`backend/airoha_backend/common_backend/common_backend.c`

覆寫 `facade_board_init` / `facade_print_string` / `facade_delay` / `facade_get_tick_ms`，
轉接至 AB1595 SDK 既有的 API。

**驗收**：可正常輸出字串，且 `facade_delay(1000)` 實測為 1 秒。

> `facade_get_tick_ms`（ms 級系統 tick）和 `swc_hal_get_tick_free_running_timer`
> （Stop-and-Wait 用的高解析自由計數器）為**兩個不同的機制**，後者另需搭配
> `swc_hal_get_free_running_timer_frequency_hz()` 回報真實頻率。

---

## Step 3 — SPI（blocking）　→ M1

**異動檔案**：`bsp/airoha/src/airoha_spi.c`、`airoha_radio.c`、`airoha_gpio.c`、`airoha_def.h`

需實作三項：full-duplex blocking 傳輸、CS set/clear、RESET pin。

**⚠️ radio SPI 不應使用原廠 HAL，須自行撰寫暫存器層級驅動。SPARK 建議**：
*"not recommended to use the MCU manufacturers' peripherals software libraries... may not be
optimized enough."* radio SPI 為系統吞吐瓶頸，原廠 HAL 每次傳輸的額外開銷將直接反映為封包遺失。
**僅 radio SPI 需自行撰寫**，UART / flash / GPIO 沿用既有驅動即可。

| SPI 規格 | 值 |
|---|---|
| 時脈 | **≤ 40 MHz**（同時是上限與最佳值）|
| 模式 | Master、full-duplex、2-line |
| 資料格式 | 8-bit、**MSB first** |
| 極性/相位 | **CPOL = 0、CPHA = 0** |
| inter-byte spacing | SR1000 需要 ≥1 SCK 週期；**SR1100 可停用以提升吞吐** |

時序（SR1020 datasheet）：SCK 週期 typ 25 ns、CS setup to SCK min 12 ns、
MOSI setup min 0 ns、MISO delay after SCK typ 7 ns。

**CS 由 Wireless Core 控制**，BSP 僅提供 assert/deassert
（`swc_hal_radio_1_begin_transfer` / `_end_transfer`）。
**不應使用硬體自動 CS**，其會中斷 Wireless Core 累積的多筆傳輸。

> inter-byte spacing 在 repo 已有對應設定：`bsp/CMakeLists.txt` 在 SR1100 時設
> `QUASAR_SPI_INTERDATA_IDLENESS=..._00CYCLE`（停用）。AB1595 端須作等效設定。

**驗收（M1）**：SPI 讀取 SR1120 chip ID，數值正確。此為電路實際導通的第一個確認點。

---

## Step 4 — IRQ + DMA + context switch　→ M2（本階段主要工作量）

**異動檔案**

```
bsp/airoha/src/airoha_radio.c     IRQ / NVIC pend
bsp/airoha/src/airoha_it.c        中斷分派
bsp/airoha/src/airoha_timer_ext.c free running timer
backend/airoha_backend/wireless_core_backend/wireless_core_backend.c   填實（20 個函式）
backend/airoha_backend/bsp_validator_backend/bsp_validator_backend.c   6 個函式
```

**實作項目**

- radio IRQ pin 的 GPIO 中斷 → `swc_hal_set_radio_1_irq_callback` 註冊的 callback
- SPI DMA 完成中斷 → `swc_hal_set_radio_1_non_blocking_transfer_callback`
- `swc_hal_radio_1_context_switch()` = `NVIC_SetPendingIRQ(radio 自己的 IRQ 號)` + `DSB` + `ISB`
  —— 可直接沿用 `quasar_radio.c:289`，任何 Cortex-M 均適用
  （**產生衝突的是衝突 A 的 PendSV，兩者易於混淆**）
- enable/disable IRQ 四組、free running timer

**中斷優先權於此步驟確定**（依 0.3 的順序配置）。下一階段衝突 B 的 BASEPRI 門檻取決於此，
**配置完成後須記錄下來**。

**bsp_validator backend 僅 6 個函式**：

| facade | 內容 |
|---|---|
| `facade_bsp_init()` | AB1595 init + radio SPI/GPIO init |
| `facade_uart_init()` / `facade_log_io()` | 轉接既有 UART（blocking TX）|
| `facade_time_delay(ms)` | 轉接既有 delay |
| `facade_radio_cs_drive(idx, level)` | GPIO set/clear |
| `facade_debug_led_blink(...)` | **可為空實作** |

**驗收（M2）**：`APP=BSP-validator` **11 項全數通過**：

```
validate_reset_pin                          validate_spi_blocking
validate_transceiver_irq_pin                validate_spi_dma
validate_trigger_transceiver_irq            validate_disable_non_blocking_transfer_irq
validate_disable_transceiver_irq            validate_cs
validate_critical_section                   validate_critical_section_context_switch
validate_wireless_context_switch            ← 衝突 A 於此顯現
```

`validate_critical_section` 兩項於 BT 停用時沿用既有的 `cpsid i` 即可通過，
**無法**偵測衝突 B —— B 須待下一階段啟用 BT 後方會顯現。

**未全數通過前不應進入下一步。** prebuilt library 無 source-level debug，錯誤一旦進入 library 內部，
可用的診斷手段僅剩 `swc_stats_*`、`swc_get_status()`、自己的 log 和 call stack。

---

## Step 5 — 建立無線連線　→ M3

**異動檔案**：`app/example/hello_world/hello_world_coord.c`（`main()` → thread entry，衝突 C）

**強烈建議對端使用既有的 u5a5 板** —— AB1595 對 AB1595 除錯時無法判別故障端。

**驗收（M3）**：配對成功、可正常收發封包、`swc_get_stats()` 的 PER 合理。

> **應先完成單 radio。** 雙 radio 方需 multi-radio timer，硬性要求為：counter up、
> auto-reload preload 關閉、**tick 頻率 = chip rate ±5%（≈20.48 MHz，即 19.46–21.5 MHz）**、
> 每 period 產生中斷、**全系統最高優先權**。雙 radio 的 CPU 負載約單 radio 的 1.5 倍。

---

## Step 6 — Profiler　→ M4

BSP 需求較低 —— profiler 使用的是 ARM core 內建的 DWT cycle counter。
`quasar_profiler.c` 全檔僅三行，**任何 Cortex-M 均可直接沿用**：

```c
QUASAR_SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
QUASAR_SET_BIT(ITM->TCR, ITM_TCR_DWTENA_Msk);
QUASAR_SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
```

**異動檔案**：`backend/airoha_backend/profiler_backend/profiler_backend.c`（11 個函式，其中 7 個沿用既有實作）

| facade | 是否為新實作 |
|---|---|
| `facade_log_init` / `_write` / `_error_string` | 否，轉接既有 UART |
| `facade_packet_generation_timer_*`（4 個）| 否，一般週期性 timer（µs 時基）|
| `facade_profiler_init` | **是**，即上述三行 DWT 設定 |
| `facade_profiler_start` / `_stop` | **是**，讀取 `DWT->CYCCNT` 並相減 |
| `facade_profiler_get_elapsed_us` | **是**，`cycle / 系統時脈 × 1e6` ← 須提供正確的時脈值 |

**需兩片板子**：`profiler_dut.elf` + `profiler_helper.elf`（helper 只在單 radio 組態才建），
因此 **Step 5 須先完成** —— profiler 為隔空量測。

**量測項目**：Wireless Core 收/發封包的處理時間，三種情境（單向送、單向收、雙向）各跑不同 payload。
處理時間**包含 radio 與 MCU 間的 SPI 傳輸**，因此會隨 payload 增長。

**驗收（M4）**：終端輸出三情境的平均處理時間與公式：

```
TX:    base_processing_time_tx + (CMD_BYTE + MAC_HDR + nb_tx_byte) × SPI_time_per_byte
RX:    base_processing_time_rx + max((nb_rx_byte - rx_nb_byte_threshold), 0) × SPI_time_per_byte
RX-TX: 兩者合併取最差情況
```

**SPARK 建議於這些數值上保留 10% 餘裕。**

**此步驟為量化評估，而非僅求執行完成**：數值應隨 payload 與 SPI 速度呈線性且可預測的變化。
曲線跳動或異常偏高，通常指向 Step 3 的 SPI 驅動或 Step 4 的中斷優先權配置。
輸出的數值在下一階段有兩項用途：**BT 開啟前的基準線**、**排 audio 排程的輸入**。

---

# 本階段範圍結束（M1–M4）

## 下一階段（不在本次範圍）

**並存先於 audio** —— audio 的時序預算須建立於「BT 啟用時 radio 尚餘多少處理時間」之上。

**N1 — 與 BT stack 並存**（5–15 人天）
啟用 BT 並長時間運行，觀察中斷延遲、PER 劣化與偶發斷線。
衝突 B 的實作與驗收在這裡，門檻值依 Step 4 確定的優先權關係計算。
衝突 A 的真正驗收也在這裡 —— Step 4 僅驗證其「可運作」，無法驗證 BT 競爭下是否「足夠快」。
以 M4 的 profiler 數值作前後對照。

**N2 — Audio**（10–20 人天）
將 AB1595 的 audio stream 封裝為 `sac_endpoint_interface_t`（producer + consumer），
**而非**將 AB1595 的 pipeline 併入 SAC。
驗收階梯：`SINE_INJECT_HS=1`（本地正弦波出 I2S）→ `SINE_INJECT_DG=1`（過無線）→ 真實音源。
須重新啟用 `core/audio/`、audio BSP，外掛 codec 時另含 I2C。

---

## 工時粗估

| Step | 人天 |
|---|---:|
| 0　ABI 驗證 + 衝突決策 | 1–2 |
| 1　骨架 build | 2–4 |
| 2　console | 1–2 |
| 3　SPI blocking（**自寫驅動**）→ M1 | 3–5 |
| 4　IRQ + DMA + validator 全數通過 → M2 | 6–12 ← **佔比最高** |
| 5　無線連線 → M3 | 3–5 |
| 6　Profiler → M4 | 2–3 |
| **本階段合計（BT 關、無 audio）** | **18–33** |

下一階段 N1 + N2 另計 15–35。**N1 的估計值在 M3 完成之前僅為推測。**

---

## 附錄 A — 硬體資源需求

| 資源 | 單 radio | 雙 radio |
|---|---|---|
| GPIO：transceiver reset | 1 | 2 |
| GPIO：SPI chip select | 1 | 2 |
| 外部中斷腳（transceiver IRQ）| 1 | 2 |
| 軟體中斷（context switch 用）| 1 | 1 |
| **SPI master 控制器** | 1 | **2 個獨立的** |
| **SPI 專用 DMA channel** | **2** | **4** |
| free-running timer | 1 | 1 |
| 16-bit timer @ chip rate ±5%（≈20.48 MHz）| — | 1 |

**此為選型問題，而非實作問題。** BT stack 已佔用部分 SPI 控制器與 DMA channel，
**雙 radio 要 2 個獨立 SPI master + 4 個 DMA channel** —— 無法挪出時僅能採用單 radio。

---

## 附錄 B — 本階段不需實作的項目

| 路徑 | 理由 |
|---|---|
| `quasar_audio.c/h`、`quasar_i2c.c/h`、`driver/max98091/` | 本階段不實作 audio／不使用外掛 codec |
| `quasar_qspi.c/h` | `RADIO_INTERFACE_QSPI=OFF`。**QSPI 不支援 SR1000，也不支援雙 radio** |
| `quasar_adc.c/h` | `BYPASS_BOARD_REV_CHECK=1` 跳過 |
| `quasar_usb.c`、`middleware/tinyusb/`、`module/tinyusb/` | `USB_AUDIO_ENABLED=OFF` |
| `quasar_pwm.c`、`quasar_rgb.c`、`quasar_led.c`、`quasar_button.c` | 暫以空函式替代 |
| `quasar_clock.c`、`quasar_power.c`、`syscalls.c`、`system_stm32u5xx.c`、`GCC/*.ld`、`startup_*.s` | AB1595 SDK 已具備 |
| `third-party/stmicroelectronics/`、`bsp/quasar-u535/` | 整包排除 |
| `core/audio/` | 本階段不 link |
| `core/wireless/`、`module/spark_pairing/`、`library/`（除 `critical_section`）| 完全不異動 |

`bsp/quasar/` **不應就地改為 AB1595 版，應另建 `bsp/airoha/`**，保留 quasar 作為參考實作 ——
`quasar_radio.c` 的 1,017 行扣除 ST HAL 樣板後，實際邏輯約 300 行。

---

## 附錄 C — SPARK Porting Guide 對照

SPARK 的流程是四階段：BSP → Wireless Core backend →（Audio backend）→ App backend。
本文件 Step 1–6 是同一順序，僅切分更細，且每步對應一項可執行的驗收。

**SPARK 沒有涵蓋、而本案需額外處理的是 N1（與 BT 並存）** —— SPARK 假設的情境為「移植至自有板級」，
不涵蓋「整合至既有 RTOS + BT stack 的 SoC」。同理，0.3 的三項衝突中 SPARK 僅涵蓋 A。

**SPARK 明列的禁止事項**

- 不應 include SDK 內部資料夾（`link/`、`protocol_stack/`、`xlayer/`）—— 只用 `swc_api.h`
- 不應直接編譯 SPARK Radio PHY 原始碼
- 不應以 `#define` 變更 prebuilt 行為 —— **應改為選用正確的 library 變體**
  （`swc_api_<transceiver>_<topology>_<arch>[_stats_band][_qspi]_lib.a`）
- 不應修改 SDK 的 top-level CMakeLists（見 0.2）
- 不應使用原廠 HAL 實作 radio SPI（見 Step 3）

**本階段暫不處理**：Optimizations 頁的 ICACHE、CCM（熱點程式搬到 CCM RAM，要改 linker script
+ startup）、`-O2` —— 待 M4 的 profiler 數值取得、確認效能不足後再行處理。

---

## 附錄 D — 衝突 A 的實作細節

**需改寫者僅兩個函式、約 15 行，實作於自有 backend，SDK 內部不予異動。**

### D.1 呼叫鏈

以 `hello_world_coord.c` 為例，所有 app 皆相同：

```
啟動時註冊兩端：
  facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);   :93
  swc_init(core_cfg, node_cfg, facade_context_switch_trigger, &err);                :188
                                └─ 以函式指標形式將 trigger 交予 SWC
執行期：
  radio IRQ（高優先權）→ SWC 偵測到 callback queue 非空
    → facade_context_switch_trigger()               ← 由移植方實作：發出 SWI
      → （降到低優先權）→ swc_connection_callbacks_processing_handler()
        → 使用者的 TX ACK / TX NACK / RX callback
```

`swc_api.h:1265`：*"It is suggested to configure a **software interrupt (SWI)** and put this
function into its IRQ handler."*

### D.2 SWI 與 semaphore 的取捨

| | 做法 | 評估 |
|---|---|---|
| 1 | **空的 NVIC IRQ 當 SWI** | ✅ **採用此方案。** 為 `swc_api.h` 的原始建議，延遲最低，且不佔用 task stack |
| 2 | semaphore → 低優先權 task | SPARK 對 RTOS 的建議。callback 需呼叫 blocking RTOS API 時才需要 |
| 3 | ~~PendSV~~ | ❌ 已為 AB1595 的 RTOS 佔用 |

**關鍵理由：bsp_validator 的兩項測試為同步檢查，semaphore → task 無法通過。**

```c
/* bsp_validator.c:801 — 註解說 "Wait 1ms"，但程式碼沒有任何 delay */
facade_context_switch_trigger();
if (mocked_context_switch_flag) { /* 隨即檢查 */ }

/* :917 更為嚴格 —— 驗證此機制必須可被 critical section 遮蔽 */
CRITICAL_SECTION_ENTER();
facade_context_switch_trigger();
if (mocked_context_switch_flag) { /* 失敗：關中斷期間不應執行 */ }
CRITICAL_SECTION_EXIT();
if (mocked_context_switch_flag) { /* 通過：解除後方才執行 */ }
```

以 NVIC pend 實作的 SWI 天然符合此二項（tail-chaining，觸發後於下一行即已執行完畢；
且受 `cpsid i` 遮蔽）。低優先權 task 則二者皆不符合。此非「semaphore 較差」，
而是**驗收工具預設了 SWI 語意**；後續若 callback 確有呼叫 blocking RTOS API 的需求，
再改採 (2) 並自行調整驗收方式。

### D.3 實作草稿

```c
/* backend/airoha_backend/common_backend/common_backend.c —— 覆寫兩個 weak 函式 */

#define SWC_SWI_IRQn   <AB1595 上未被使用的 IRQ 編號>

static void (*swc_cb)(void);

void facade_set_context_switch_handler(void (*callback)(void))
{
    swc_cb = callback;
    NVIC_SetPriority(SWC_SWI_IRQn, <低於 radio IRQ 和 SPI DMA IRQ>);
    NVIC_EnableIRQ(SWC_SWI_IRQn);
}

void facade_context_switch_trigger(void)
{
    NVIC_SetPendingIRQ(SWC_SWI_IRQn);
    __DSB();                       /* 不可省略，validator 的錯誤訊息會直接指出 */
    __ISB();
}

void <SWC_SWI_IRQn 對應的 handler>(void)
{
    if (swc_cb != NULL) {
        swc_cb();
    }
}
```

**實際難點僅在於尋找 AB1595 確實未使用的 IRQ 編號** —— 須確認 BT stack 與 AB1595 RTOS
現階段及後續均不會使用。**建議於 Step 0 即向絡達原廠確認。**
