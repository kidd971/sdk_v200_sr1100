# 斷音調查總帳 —— 已排除的項目與剩下的候選

> 對象：`app/example/puretone_unidirectional`｜Quasar U5A5 + SR1100，single radio｜SDK v2.4.0-rc2
> 對照組：SPARK 官方 audio demo `SPARK_AUDIO_DEMO_eng-v1.0.0-ext_codec_support-r1`（同一塊 EVK）
> 前一世代的同一問題見 [fallback_mono_rung_rationale.md](fallback_mono_rung_rationale.md)
> 日期：2026-08-18｜基準 commit：`ce44437`（+ 兩支未 commit 的 lab arm）

---

## 0. 一句話

**近端已解，遠端未解。**
近端的答案是 ISI 2 + 重傳餘裕 + 40 ms buffer 三個一起上（tag `v240-unidir-fbk5-ok`）。
遠端把「輸出功率」這條線從頭走到底（coordinator level 4 → 天花板 → 連 node ACK 一起拉滿）都沒動到掉線距離。
**唯一還沒測的旋鈕只剩 PHY rate，而它理論上是往反方向走的** —— 見 §5。

---

## 1. 判別工具：兩種失效長得一樣，修法完全相反

| 計數器 | 誰的 | 意思 | 對應解法 |
|---|---|---|---|
| `cca_fail` | Coordinator | 通道被判定為忙，封包**根本沒發出去**（本連線 `SWC_CCA_ABORT_TX` 是丟棄不是延後） | 加 CCA try count |
| `rx_rej` | Node | 封包**有發、有到、解不開** —— multipath 造成的 symbol 塗抹 | ISI mitigation |

`[DG]` / `[HS]` 每秒一行的 compact log 就是為了讀這兩個數（commit `fb8a248`）。

### 目前的量測結果

| 情境 | `cca_fail` | `rx_rej` | 判讀 |
|---|---|---|---|
| 近端遮擋掉音 | **0** | 爬升 | multipath 解碼失敗 |
| 遠端掉線 | **0** | — | **封包也有上空中** |

> 遠端的 `cca_fail = 0` 是 2026-08-18 量到的，把 CCA 這條線**完全**關掉了：
> 通道從來沒被判定為忙，`try_count` 設 2 或 15 走的是同一條路徑。
> `SWC_CCA_AUDIO_FBK_4_TRY_COUNT` 維持 14（demo 是 15，差這一格不用管）。

**兩種距離的失效都不是「發不出去」。** 剩下的問題永遠在「送出去之後」。

---

## 2. 有效的（現行配置，tag `v240-unidir-fbk5-ok` / `slim3`）

| 項目 | 值 | 為什麼有效 |
|---|---|---|
| **ISI mitigation** | `SWC_ISI_MITIG_2`（兩端必須一致） | 唯一直接對付 multipath 塗抹的旋鈕。Level 1（SPARK 出貨值）不夠，Level 3 無遮擋就爆音 |
| **Accumulator（重傳餘裕）** | mode 3 = 23/10（3.7 次）<br>mode 4 = 40/10（6.4 次） | Accumulator 在 resampler **之前**，是唯一決定「一包能被送幾次」的旋鈕。slot 是按**包**花的不是按 byte |
| **底部兩階 buffer** | 40 ms（30 ms 不夠，近端斷音會回來） | 遮擋持續數百 ms；queue 排空之後重送再多次也沒用，封包已經過了播放時限 |
| **mode 3 縮回 54 B** | acc 4.6× → 2.3× | 100 B 的 mode 3 + ISI 2 的長 preamble 塞不進 250 µs slot → 長距離持續遮擋會當機。重傳餘裕改由 mode 4 扛 |

**這四項缺一不可**：ISI 提高「單次送達的機率」，accumulator 提供「足夠的次數讓機率兌現」，
buffer 提供「撐過突發遮擋的時間」。三個各補一個獨立的資源，任何一個單獨上都無效。

---

## 3. 已測無效 —— 本世代（puretone_unidirectional，5.25 ms 排程）

| # | 嘗試 | 結果 | 為什麼無效 |
|---|---|---|---|
| 1 | **CCA try count 2 → 14**<br>`MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT` | ❌ 無改善 | `rx_rej` 爬而 `cca_fail` 不動 —— 是解碼失敗不是通道占用。14 次約 66 µs 吃掉 250 µs slot 的四分之一，還不是免費的。已改回 2（commit `28d1a67`） |
| 2 | **CCA try count（遠端）** | ❌ 不可能有效 | 遠端 `cca_fail` 也是 0，這格在任何距離都是 no-op |
| 3 | **Fallback level 4 輸出功率 = SPARK demo 值**<br>`FBK4_TX_POWER_REF=1`，width 6/6/6/6 gain 1/1/1/0 | ❌ 掉線距離完全沒動 | 量了**兩次**（一次配 ISI 1、一次配 slim3）。加功率不能把反射「解塗抹」—— 反射跟著等比放大。**雖然無效，仍已改為預設值**：這是整張功率表最後一個沒有對照來源的欄位，對齊參考比留著自創值有價值 |
| 4 | **Fallback level 4 輸出功率拉到天花板**<br>`FBK4_TX_POWER_REF=2`，width 7/7/7/7 gain 0/0/0/0 | ❌ *(結果待補)* | 刻意跳過「再加一階」直接測上限：三階 width 都沒動到的東西，多一階不會翻盤。`bin/v240-unidir-slim3-l4max` |
| 5 | **Node ACK 功率拉到天花板**<br>`ACK_TX_POWER_MAX=1`，width 7 gain 0 | ❌ *(結果待補)* | ACK 是全 app 唯一「不隨 ladder 移動」的功率，形狀最符合「鎖 mode 4 與自由跑距離一樣」。`bin/v240-unidir-slim3-allmax` |
| 6 | **ISI level 1**（SPARK 出貨值） | ❌ 近端斷音回來 | 對 multipath 不夠 |
| 7 | **ISI level 3** | ❌ 無遮擋就爆音 | preamble 屬於**連線**不屬於 fallback mode，必須容納 ladder 上**最大**的 payload 進 250 µs slot。注意失效形式是頂階音質壞掉，不是 init 紅燈 |
| 8 | **底部階 buffer 30 ms** | ❌ 近端斷音回來 | 40 ms 才夠 |
| 9 | **Ladder 反應速度**（自由跑 vs 鎖 mode 4） | ❌ 兩者掉線距離相同 | 不是降階不夠快。這一條同時排除了整條 ladder 的門檻調整 |
| 10 | **radio saved calibration**<br>`RADIO_USE_SAVED_CALIB=true`（demo 傳 true，SDK example 傳 false） | ❌ *(結果待補)* | 曾是「消去法剩下的首選」。`bin/v240-unidir-calib-isi1` |

> **§3.4 / §3.5 / §3.10 的結果請確認後補上。** 三支 binary 都已 staged 並在 MANIFEST 裡寫明
> 各種結果代表什麼，但沒有一份寫回實測結論。依口述均為「無改善」，此處不代填。

### 已經比對過、確認**不是**差異來源的項目

同一塊板子上與 SPARK demo 逐項比對，本韌體**相同或更有利**：

| 項目 | 本韌體 | SPARK demo |
|---|---|---|
| 重傳次數（底階） | 6.4 | 5.4 |
| 底階 buffer | 40 ms | 20 ms |
| 底階 payload | 48 B | 50 B |
| chip rate | 20.48 MHz | 20.48 MHz |
| Fallback 輸出功率 level 1–3 | 逐值相同 | 逐值相同 |
| SPI mode / concurrency mode / ACK 預設 | 相同 | 相同 |
| Antenna diversity | 不適用（雙方都是 single radio） | — |

---

## 4. 已測無效 —— 前一世代（puretone_headset，2.25 ms 排程）

摘自 [fallback_mono_rung_rationale.md](fallback_mono_rung_rationale.md) §2，**注意排程與 app 都不同，不要跟 §3 混讀**：

| 嘗試 | 結果 | 說明 |
|---|---|---|
| FEC 1.25 → 2.0 | ❌ 開機紅燈 | 編碼變長吃不下 250 µs timeslot，init assert |
| ISI 0 → 1 / 2 / 3 | ❌ 無改善 | 該世代量到的結果。**本世代 ISI 2 有效**，兩者不衝突：排程、payload、accumulator 全都換過了 |
| 底階改 24 kHz（54 B → 31 B） | ❌ 無改善 | slot 按包花不按 byte，縮小 payload 不增加發送次數 |
| 未改版 baseline | 同樣會斷 | 對照組：頻率、位置完全一樣 |

**該世代的共同結論仍然成立：以上每一項改的都是「單次發送有多耐撞」，沒有一項增加「可以撞幾次」。**

---

## 5. 還沒測的

### 5.1 PHY rate —— 唯一真正沒碰過的旋鈕，而且理論上相反

**現況：`swc_cfg_t.chip_rate` 在 `app_swc_core_init()` 裡沒有設定**
（[puretone_unidirectional_coord.c:355](app/example/puretone_unidirectional/puretone_unidirectional_coord.c#L355)），
零初始化 → `SWC_CHIP_RATE_20_48_MHZ`。搭配 `isi_mitig = SWC_ISI_MITIG_2`，
對照 `swc_phy_mode_t` 的命名，本 app 等於跑在五檔裡**最慢的一檔**：

| `swc_phy_mode_t` | 有效速率 | 位置 |
|---|---|---|
| `SWC_CHIP_RATE_20_48_ISI_2` | **10.24 MHz** | ← **本 app 現在在這裡** |
| `SWC_CHIP_RATE_27_30_ISI_2` | 14.15 MHz | |
| `SWC_CHIP_RATE_20_48_ISI_1` | 20.48 MHz | ← SPARK demo 的頂階在這裡 |
| `SWC_CHIP_RATE_27_30_ISI_1` | 27.30 MHz | |
| `SWC_CHIP_RATE_40_96_ISI_1` | 40.96 MHz | |

**為什麼說「理論上相反」**：降速率／加 ISI = 增加 processing gain = 理論上**更遠**。
本 app 已經在最慢的一檔，距離卻比 demo 差。照理論該往下走的方向已經走到底了，
所以剩下能測的只有往**上**走 —— 用距離換回 airtime。

**這不是亂猜，有結構性理由**：10.24 MHz 有效速率表示同樣一個 250 µs slot 只裝得下一半的 bit。
ladder 頂階（mode 0）是 242 B，本來就吃緊；ISI 3 會爆音、mode 3 撐到 100 B 會當機，
都是同一個 slot 預算在說話。**提高 PHY rate 是唯一能同時放鬆 slot 壓力的方向。**

### 5.2 結構性發現：SPARK demo 可以逐階換 PHY preset，本 SDK 不行

demo 的 SDK 在 `swc_connection_fallback_cfg_t` 裡多一個欄位：

```c
/*! Array of preset index. Array size must be equal to fallback_mode_count */
uint8_t *presets;
```

而 preset 是這樣建的（`lib/sdk/core/wireless/api/swc/sr1100/swc_api.c:2073`）：

```
preset 0 = 連線的 fec/chip_code/chip_repetition + node 的 isi_mitig
preset 1 = preset 0，但 isi_mitig 強制 SWC_ISI_MITIG_2
```

demo 的 ladder：**mode 1、2 用 preset 0（ISI 1），mode 3、4 用 preset 1（ISI 2）**。

**本 SDK（v2.4.0-rc2）的 `swc_connection_fallback_cfg_t` 沒有 `presets` 欄位**
（[core/wireless/swc_api.h:275](core/wireless/swc_api.h#L275)），wireless core 是 prebuilt `.a`，
從 app 這一側無法逐階指定 preset。

**這解釋了本 app 一直卡在哪裡**：ISI level 是**整條 ladder 共用**的。
所以「頂階要短 preamble、底階要長 preamble」在這個 SDK 上表達不出來 ——
選 ISI 2 就是全階都付長 preamble（頂階 242 B 因此更緊），選 ISI 1 就是底階近端會斷。
demo 沒有這個限制。**這可能才是「差異在 prebuilt wireless core 裡」的具體內容。**

### 5.3 其他沒測過的

| 項目 | 現值 | 可得 | 備註 |
|---|---|---|---|
| `SR1100_PULSE_COUNT` | 1 | 最大 3 | +3 dB @2、+4.8 dB @3（coherent integration），**比 gain 欄位整個 1.8 dB 的範圍還大**。不隨 ladder 移動。代價是 airtime |
| Schedule | 21 槽 × 250 µs = 5.25 ms（coord 20 / node 1） | demo 是 2.25 ms（7 / 2） | `ce44437` 明列為消去法後剩下的兩個之一 |
| `dynamic_phy_mode_enabled` | `false`（未設定） | `swc_set_phy_mode()` 需要它 | 要做 §5.1 就得先開這個 |

---

## 6. 建議順序

1. **先補 §3 的三個空格**（l4max / allmax / calib）。三支 binary 都在 `bin/` 下，
   結果沒寫回去，下一個人會重跑。
2. **`SR1100_PULSE_COUNT` = 2**。不隨 ladder 移動、+3 dB 比整個 gain 欄位的範圍還大、
   一個 define 就到位。功率這條線要收尾的話這是最後一發。
3. **PHY rate 往上一檔**（`SWC_CHIP_RATE_20_48_ISI_1`，即 ISI 降到 1 但整體有效速率加倍）。
   需要 `dynamic_phy_mode_enabled = true` + `swc_set_phy_mode()`。
   **注意這等於全階 ISI 1，近端斷音會回來** —— 這一支測的是「距離有沒有變遠」，
   不是「能不能出貨」。近端要靠 §5.2 的 preset 機制才能兩全，而那需要 SDK 支援。
4. 若 3 有效，向 SPARK 確認 v2.4.0 正式版是否會補上 fallback `presets`。

---

## 附註：一處註解與實際值不符 —— 已修正

[swc_cfg.h](app/example/puretone_unidirectional/config/sr1100/swc_cfg.h) 的 `NODE_ISI_MITIG`
註解曾寫「Back at 1 to measure it against the current ladder」，但實際值是 `SWC_ISI_MITIG_2`
（`624e046` 改成 1 之後 `e46a386` 又收回 2，註解沒跟上）。已改寫成記錄「2 是現行值、
降到 1 是追當機時的繞路，而真正的解是把 mode 3 縮回 54 B」。

同時：`FBK4_TX_POWER_REF` 預設已從 0 改為 1，level 4 輸出功率改採 SPARK demo 的值。
node 的 ACK 功率經查**本來就等於 demo**（width 5 / gain 1），無需更動。
完整的設定快照見 [link_rf_settings_baseline.md](link_rf_settings_baseline.md)。
