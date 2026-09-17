# SPARK SDK 概觀 —— 移植前的背景

> 建立於 2026-09-11。基準 `rel/v2.4.1_rc2`。
> **用途**：[bes_mcu_port_analysis.md](bes_mcu_port_analysis.md) 的前置閱讀。該文件自 Step 0 的
> ABI 比對切入，預設讀者已具備 SWC、facade 與 prebuilt library 的基本概念。
> 本文僅涵蓋理解該文件六個 Step 所需的範圍，**並非 SDK 的完整導覽**。
> 官方文件：<https://sparkmicro.com/sdk-docs/index.html>（repo 根目錄的
> `SPARK SDK Documentation.html` 為導向該站的跳轉頁）。

---

## 1. SPARK 在系統中的位置

SR1120 是一顆 **UWB transceiver**，**並非具備獨立 CPU 的模組**。它不含韌體、不提供 AT 介面，
亦不會自行維持連線。SPARK 交付的是一套**執行於目標 MCU 上的 host 端無線協定堆疊**，
透過 SPI 驅動此收發器。

```
        ┌──────────────────────────┐              ┌──────────┐
        │        目標 MCU          │   SPI ≤40MHz │          │
        │  ┌────────────────────┐  │◄────────────►│ SR1120   │──╢ antenna
        │  │ SPARK Wireless Core│  │   IRQ pin    │   UWB    │
        │  │  + PHY (prebuilt)  │  │◄─────────────│transceiver│
        │  └────────────────────┘  │   RESET pin  │          │
        │  ┌────────────────────┐  │─────────────►│          │
        │  │ 目標平台 BSP/backend│  │              └──────────┘
        │  └────────────────────┘  │
        │  ┌────────────────────┐  │
        │  │ 原廠 SDK / RTOS    │  │
        │  └────────────────────┘  │
        └──────────────────────────┘
```

**此點決定了整項工作的性質**：並非「外接一顆 IC」，而是「將一套協定堆疊移入目標 MCU，
並確保其取得所需的時序」。中斷優先權、DMA 完成時序、context switch 等要求皆源於此
—— 協定狀態機執行於目標 CPU 上，與既有 RTOS 及 BT stack 共用同一顆核心。

硬體介面共三項：**SPI（含 CS）、一根 IRQ 輸入、一根 RESET 輸出**。
完整清單見 port 文件附錄 A。

---

## 2. 五層架構與 facade 介面

| 層 | 位置 | 內容 | 移植時需異動 |
|---|---|---|---|
| Application | `app/example/<app>/`、`app/tool/<tool>/` | `main()`、狀態機、pipeline 接線。可攜 C，不含暫存器存取 | ❌ |
| **Facade**（僅標頭檔）| `app/example/<app>/facade/*.h`、`core/wireless/facade/swc_hal_facade.h` | **app 與 core 得以向平台要求的介面定義** | ❌ |
| **Backend** | `backend/<board>_backend/<app>_backend/` | 上述介面在特定板級上的實作 | ✅ **移植的主要工作** |
| BSP | `bsp/<board>/` | `<board>_*` 週邊驅動、pin map | ✅ |
| Cores | `core/wireless`（SWC）、`core/audio`（SAC）| 原廠無線／音訊堆疊 | ❌ |

**facade 為編譯期契約，不含 vtable 或執行期多型。** app 呼叫 `facade_*()` /
`swc_hal_*()`，由建置系統 link 進**恰好一份**定義這些符號的 backend。

> 支援新板級的方式是撰寫一份 backend，而非修改 app。

移植工作量因此可事先估算：計算 facade 標頭檔中的函式數量即可得出。
以本階段（BSP-validator + Profiler、不含 audio）而言約 45 個，多數為數行的轉接實作，
明細見 port 文件的里程碑表。

repo 內已有兩套完整的參考實作：`bsp/quasar`（STM32U5A5）與 `bsp/quasar-u535`（STM32U535），
可逐檔對照作為範本。`backend/quasar_backend/common_backend/common_backend.c` 中的
**18 個 facade 均標註 `__attribute__((weak))`** —— 此為 SDK 明示的覆寫點，
實作者可僅覆寫所需項目，其餘沿用預設。

---

## 3. Prebuilt 與原始碼的交付形態

**本節為 port 文件 Step 0.1 的前提。**

| 元件 | 形態 | 位置 |
|---|---|---|
| Wireless Core（SWC）| **prebuilt `.a`** | `core/wireless/prebuilt/`（20 個變體）|
| PHY | **prebuilt `.a`** | `driver/spark_radio/sr_phy/prebuilt/`（10 個變體）|
| Audio Core（SAC）| 原始碼 | `core/audio/` |
| BSP / backend / app / library | 原始碼 | `bsp/`、`backend/`、`app/`、`library/` |

library 的選擇軸直接反映於檔名：

```
swc_api_ sr1100 _ single _ m33 _ stats_band _ qspi _lib.a
         └ 收發器  └ 單/雙   └ 核心   └ 每頻段統計   └ QSPI
           sr1000    dual      m4       （選配）      （選配）
```

**以 `.a` 形式交付**衍生兩項限制：

1. **編譯選項不相符時僅能向原廠索取對應版本，無法自行重新編譯。** 此即 Step 0.1 要求在撰寫
   任何程式碼之前先完成 ABI 比對的理由 —— 不相符屬於交期問題，而非工程工時。
2. **`MCU_DRIVER` 僅有 `m4` / `m33` 兩個分支**，不含其他平台選項；但其作用僅止於選擇檔名，
   手動指定同樣成立（Step 0.2）。

`RELEASE_MODE` 預設為 ON，使用的即為這些 `.a`；原始碼建置為 SPARK 內部組態，本套件不具備此條件。

---

## 4. SWC 基本概念

以下僅涵蓋 port 文件 Step 5 所需的部分。

- **Coordinator / Node** —— 每個網路中恰有一個 coordinator 負責提供時間基準，其餘為 node。
  兩者為**不同的執行檔**。
- **Connection** —— 具方向性的邏輯通道（`swc_connection_t`）。一對裝置之間可並行多條，
  各自具備 payload 大小、通道清單、callback 與統計數據。
- **Timeslot sequence** —— 整個網路共用一張以微秒為單位的時槽表（`timeslot_sequence` /
  `timeslot_sequence_length`），每條 connection 宣告其佔用的時槽（`timeslot_id`）。
  **此表為 TDMA 排程，而非「有資料即發送」的機制。**
- **Channel list** —— 跳頻用的頻率清單，由 `swc_channel_list_init_from_base()` 產生。
- **PER / CCA** —— 統計面。`swc_connection_get_stats()` 提供封包錯誤率、時槽使用率與
  CCA 通過／失敗次數，為 M3「PER 合理」的判定依據。

典型初始化順序（取自 `app/example/hello_world/hello_world_coord.c:159`）：

```c
swc_init(core_cfg, node_cfg, facade_context_switch_trigger, &err);
radio_handle = swc_radio_module_calib(SWC_RADIO_ID_1, &err);
swc_radio_module_init(radio_handle, false, &err);
conn = swc_connection_init(conn_cfg, &err);
swc_connection_set_channels(conn, channels, count, &err);
swc_connection_set_tx_success_callback(conn, cb, NULL, &err);
swc_setup(&err);          /* 此前尚無射頻動作 */
swc_connect(&err);        /* 連線於此呼叫後建立 */
```

其中 `facade_context_switch_trigger` 於 `swc_init` 階段即需提供：Wireless Core 需要一套
「將自身排入較低優先權執行」的機制，而該機制由移植方實作。此即 port 文件衝突 A 的來源。

> **本階段不涉及**：fallback ladder、SAC audio pipeline、pairing、ranging。
> M2 與 M4 兩支工具均不 link `audio_core`，「暫不實作 audio」為原廠認可的合法組態
> —— SPARK 文件明列 Audio Core backend 為 *"only required for audio applications"*。

---

## 5. SDK 內建的兩支驗證工具

**此二者即 M2 與 M4 的驗收基準，無須自行撰寫。** 兩者皆為 SDK 內的 app（`app/tool/`），
移植時應作為第一批執行目標。

### `app/tool/bsp_validator` —— 每顆 radio 執行 11 項

```
[ RUN      ] SPI blocking mode
[ RUN      ] SPI chip select
[ RUN      ] Transceiver reset pin
[ RUN      ] Transceiver IRQ pin and event
[ RUN      ] SPI DMA and transfer complete event
[ RUN      ] Disabling transceiver IRQ event
[ RUN      ] Disabling SPI DMA complete IRQ event
[ RUN      ] Context Switch event
[ RUN      ] Set pending transceiver ISR
[ RUN      ] Enter / Exit critical section
[ RUN      ] Context Switch event combined with Enter / Exit critical section
```

測試順序具有層次：前 3 項為純 SPI／GPIO，第 4–7 項涵蓋中斷與 DMA，
後 4 項則集中驗證 **context switch 與 critical section 的互動**。
亦即近半數項目所驗證的，正是 port 文件中衝突 A 與 B 所指的問題。
**單板即可執行，無須第二塊板子。**

### `app/tool/profiler` —— DUT + Helper，三個情境

量測 Wireless Core 在最小 payload 條件下的處理時間：

1. DUT 作為發送端（單向 TX）
2. DUT 作為接收端（單向 RX）
3. DUT 雙向收發

**需兩塊板子**（DUT 與 Helper）。其輸出即為 SPARK 在該 MCU 上所佔用的 CPU 時間；
於開啟 BT 前後各執行一次，差值即為共存的代價。

---

## 6. 建置系統：CMake presets 為參考實作

本 repo 的建置組成為 **pixi**（`.pixi/bin/pixi.exe`，內含 CMake 3.31.8 /
gcc-arm-none-eabi 10.3 / Ninja / pyocd）搭配 **CMake presets**，每個 preset 對應
`build/<preset>/` 一棵獨立的建置樹。此組態可作為 backend 與 BSP 組法的對照，
但**並非 SPARK 所要求的建置方式**。

SPARK 的原則僅有一條：

> *"the SDK be included into projects as an external package, meaning that it can be updated
> via a **drop-in replacement**." / "don't modify the SDK's top-level file."*

**SDK 應視為外部元件，升級即為整包替換，因此不應修改 SDK 內部檔案。**
既有原廠 SDK 多採 Makefile／kbuild，可直接採用 SPARK 文件的 **Non-CMake 路線**：
自行指定 `.a`、自行 include public header、自行套用等效的 compiler flag。
四個步驟的細節見 port 文件 Step 0.2。

沿用 CMake 路線時的兩項慣例，Non-CMake 亦須留意：

- BSP 的 CMake target **名稱必須為 `hardware`**（`bsp/quasar/CMakeLists.txt:1`），
  上層依此名稱進行 link。
- 僅透過 **`swc_api.h`** 取用 Wireless Core，不應直接 include `link/`、`protocol_stack/`、
  `xlayer/` 之下的檔案 —— 該等為內部結構，drop-in replacement 不保證其相容性。

---

## 7. SDK 自帶的文件

### 線上文件（無離線副本）

repo 根目錄的 **`SPARK SDK Documentation.html`** 僅是一個五行的轉址頁，
內容為導向 <https://sparkmicro.com/sdk-docs/index.html> 的一段 JavaScript。
**套件本身不含離線副本**，無網路連線時無法閱覽。SPARK Porting Guide 亦位於該站。

### 離線的權威來源是原始碼中的 doxygen 註解

線上文件係由原始碼註解產生，因此標頭檔即為等效且必定同版本的參考來源。
移植期間最常查閱的為以下三類：

| 檔案 | 內容 |
|---|---|
| `core/wireless/facade/swc_hal_facade.h` | 45 個 prototype，依 `@defgroup` 分為四組：context switch 與中斷管理、radio GPIO 控制、SPI/QSPI 通訊、雙 radio timer 管理 |
| `app/example/common/common_facade.h`、`app/tool/bsp_validator/facade/bsp_validator_facade.h`、`app/tool/profiler/facade/profiler_facade.h` | 各 app 與工具所要求的介面 |
| `core/wireless/swc_api.h`、`swc_error.h`、`swc_stats.h` | SWC 公開 API、錯誤碼與統計結構 |

這些註解並非僅有 `@brief`。以 `swc_hal_facade.h` 為例，`swc_hal_radio_2_context_switch()`
的 `@note` 直接載明「單 radio 實作無須提供此函式」—— **哪些函式可略過、哪些必須實作，
是寫在標頭檔裡的**，不必回頭查線上文件或逐一試錯。port 文件里程碑表中那份函式數量統計，
即由此得出。

### 其他隨附文件

- **`EULA.txt`** —— SPARK Microsystems Software EULA v1.0。prebuilt `.a` 與原始碼均受其約束，
  對外交付前須確認條款。
- **`driver/max98091/README.md`** —— 套件中唯一的元件級 README，內容為 MAX98091 codec 專屬，
  與移植無關。

> repo 中的 `MD/` 目錄（含本文件）為本專案自有的設計與量測紀錄，**並非 SPARK 交付物**，
> 更新 SDK 時不受 drop-in replacement 影響。

---

## 後續

移植的實際步驟、驗收里程碑，以及與既有 RTOS／BT stack 的衝突處理，
見 **[bes_mcu_port_analysis.md](bes_mcu_port_analysis.md)**。
該文件以絡達（Airoha）AB1595 為實例，惟 Step 1–6 與 M1–M4 對任何 Cortex-M 目標均適用。
