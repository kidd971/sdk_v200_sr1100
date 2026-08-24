# w240 現行設定基準線 —— puretone_unidirectional

> 用途：合併其他功能到 w240 之前的設定快照。任何後續改動若影響鏈路表現，
> 對照這份即可知道基準是什麼。
> 對應 commit：`ce44437`｜tag：`v240-unidir-fbk5-ok`（該 tag 之後只多了診斷與 lab arm）
> 調查過程與已排除項目見 [link_dropout_arms_ledger.md](link_dropout_arms_ledger.md)

---

## 1. 無線電 / SWC

| 項目 | 值 | 備註 |
|---|---|---|
| ISI mitigation | `SWC_ISI_MITIG_2` | **兩端必須一致**，改的是雙方找到對方用的 preamble |
| chip rate | `SWC_CHIP_RATE_20_48_MHZ` | **未明確設定**，`swc_cfg_t` zero-init 得到。搭配 ISI 2 等於有效 10.24 MHz |
| pulse count | `SR1100_PULSE_COUNT` = 1 | 最大 3，未動過 |
| saved calibration | `RADIO_USE_SAVED_CALIB` = false | SDK 範例的預設；SPARK demo 傳 true |
| FEC | `SWC_FEC_1_2_5_0` | 與 demo 相同 |
| 調變 | `SWC_MOD_IOOK` | 與 demo 相同 |
| 並行模式 | `SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE` | 與 demo 相同 |
| 通道 | `{164, 174, 184, 194}`，序列 `{0,1,2,3}` | 與 demo 相同 |
| 排程 | 21 槽 × 250 µs = **5.25 ms**（coord 20 / node 1） | demo 是 9 槽 = 2.25 ms（7 / 2） |
| dynamic phy mode | 未設定（false） | v2.4.0 才有的欄位，demo 的舊 SDK 沒有 |

### CCA

| | 值 |
|---|---|
| 主音訊連線 | try 2、retry 96（4.688 µs） |
| 資料連線 | try 15、retry 160（7.813 µs） |
| fallback 各階 | 7 / 13 / 14 / 14 |

> 遠近兩種距離量到的 `cca_fail` 都是 0 —— CCA 在此設定下不是瓶頸，數值大小不影響行為。

---

## 2. 發射功率

值越小輸出越大（gain 欄位 0 = 最大 0 dB，3 = 最小 −1.8 dB）；width 越大能量越多。

### Coordinator

| 用途 | width | gain |
|---|---|---|
| 音訊主通道 | 1 1 1 1 | 5 5 4 2 |
| 資料 | 6 6 6 6 | 2 2 1 0 |
| 資料 ACK | 6 6 6 6 | 2 2 1 0 |
| ACK | 5 5 5 5 | 2 2 1 1 |

### Coordinator fallback（每列一個 band，欄位 = level 1..4）

| band | width | gain |
|---|---|---|
| 1 | 2 3 3 **6** | 5 4 1 **1** |
| 2 | 2 3 3 **6** | 5 4 1 **1** |
| 3 | 2 3 4 **6** | 4 3 0 **1** |
| 4 | 2 4 5 **6** | 3 3 0 **0** |

> **整張表現在與 SPARK demo 逐值相同。** level 1–3 本來就是；粗體的 level 4 原本是新增
> 24 kHz 那階時複製 level 3 的值，是唯一沒有對照來源的一欄，現已改採 demo 的值
> （`FBK4_TX_POWER_REF` 預設 1）。
>
> 注意 gain 欄位方向相反：0 = 最大振幅（0 dB），3 = 最小（−1.8 dB）。所以 demo 的 level 4
> 不是單純比舊值大 —— 它加寬所有 band，但把 band 3 的 gain 從 0 移到 1。

### Node

| 用途 | width | gain |
|---|---|---|
| 音訊 | 3 3 3 3 | 4 4 3 2 |
| 音訊 fallback | 4 4 4 4 | 2 2 1 0 |
| 資料 | 6 6 6 6 | 2 2 1 0 |
| 資料 ACK | 6 6 6 7 | 3 3 2 2 |
| **ACK** | **5 5 5 5** | **1 1 1 1** |
| ACK pulse count | 1 | |

> node 的 ACK 功率是全 app 唯一**不隨 ladder 移動**的功率設定，
> 而且已經**等於 demo 的 `ack_power`**（width 5 / gain 1 四個 band），不需要對齊。

---

## 3. 音訊階梯

主通道：96 kHz / 40 樣本 / 2 聲道 / 24-bit｜`MAIN_CHANNEL_MAX_LATENCY_MS` = 40

| mode | 內容 | latency | acc | 樣本 | 酬載 | 每封包嘗試 |
|---|---|---|---|---|---|---|
| 0 | 96k 24-bit | 5 ms | 1.0x | 40 | 242 B | 1.6 |
| 1 | 48k 24-bit | 7 ms | 1.7x | 34 | 206 B | 2.7 |
| 2 | 48k 16-bit | 10 ms | 1.7x | 34 | 138 B | 2.7 |
| 3 | 48k ADPCM stereo | **40 ms** | **2.3x** | 46 | 54 B | 3.7 |
| 4 | 24k ADPCM stereo | **40 ms** | **4.0x** | 40 | 48 B | 6.4 |

- `MAIN_CHANNEL_MAX_ACC_MUL` 追隨 mode 4（4.0x），因為它是最大的
- SWC fallback 等級數 = 4（門檻 206 / 138 / 54 / 48，必須遞減）
- mode 3、4 的樣本數是**推導**的，不可手寫 —— 與 accumulator 脫節會讓該階整個靜音
- mode 4 的 acc 上限是結構性的：`acc4 < 2 × acc3`，等於 2 倍時兩階酬載相同、門檻不遞減 → init assert

### 記憶池

| | 值 | 說明 |
|---|---|---|
| SAC pool (DG / HS) | 100000 / 110000 | 寬估值。`print_stats` 會印 `Mem Pool: <實際>/<設定>`，尚未回報過實測 |
| SWC pool | 10500 兩端 | 未動過 |

---

## 4. 建置開關（CMake 已轉發，`-D` 可覆寫）

| 開關 | 預設 | 作用 |
|---|---|---|
| `NODE_ISI_MITIG` | `SWC_ISI_MITIG_2` | ISI 等級 |
| `MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT` | 2 | 主音訊 CCA 次數 |
| `FBK4_TX_POWER_REF` | **1** | 0=舊值（level 3 的複製）　1=demo 的 level 4（**預設**）　2=滿檔（lab） |
| `ACK_TX_POWER_MAX` | 0 | node ACK 功率拉滿（lab） |
| `RADIO_USE_SAVED_CALIB` | false | 是否用 NVM 裡的校準 |
| `STATS_VERBOSE` | 0 | 0=每秒一行　1=原廠完整統計區塊 |
| `I2S_MASTER_MODE` | 未定義=slave | 1 = master |
| `I2S_FMT_DEFAULT` | 未定義=RJF | 1=LJF　2=STD |
| `NO_CODEC` | 未定義=有 codec | 1 = 跳過 MAX98091 |
| `CERTIF_FORCE_MODE` | 未定義 | 強制認證模式 |
| `SINE_INJECT_DG` / `_HS` / `SINE_DEBUG_CAPTURE` | 未定義 | 測試訊號 |
| `LED_USER1_PD7` / `LED_RGB_BLUE_PA1` / `BYPASS_BOARD_REV_CHECK` | 未定義 | 板級覆寫 |

`FBK4_TX_POWER_REF=2` 與 `ACK_TX_POWER_MAX=1` 是**實驗室 arm，不可出貨**
（滿寬度滿振幅，未評估電磁相容）。兩者預設皆為關閉。

---

## 5. 合併前要知道的兩件事

**一、有未提交的改動。** `CMakeLists.txt`、`swc_cfg_coord.h`、`swc_cfg_node.h` 三個檔
帶著 `FBK4_TX_POWER_REF=2` 和 `ACK_TX_POWER_MAX` 兩支 lab arm 的分支。
**兩者預設都是關閉**，所以編出來的預設 binary 與 `ce44437` 相同，但檔案本身是 dirty 的。
合併其他功能之前先決定要不要提交，否則後續 diff 會混在一起。

**二、~~`NODE_ISI_MITIG` 註解與實際值不符~~ —— 已修正。**
註解曾寫「Back at 1 to measure...」而實際值是 2；現已改為記錄「2 是現行值、1 是繞路」。

---

## 6. 已知未解

- **遠端掉線距離**不如 SPARK demo。功率這條線（level 4 → 天花板 → node ACK）已走到底無效
- **mode 0 偶發小啵**：ISI 2 的空中時間 × 242 B。20-bit（242→202 B）是已識別的解法，尚未實作
- **mode 3↔4 邊界冷 FIR 爆音**：1:4 SRC 刻意沒有 discard 變體，機制仍在
- **`puretone_headset` 那條線**帶著 ISI 2 + 100 B mode 3 的組合，已知會當機，已在該處註記
