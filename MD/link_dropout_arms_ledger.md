# 斷音調查總帳 —— 已排除的項目與剩下的候選

> 對象：`app/example/puretone_unidirectional`｜Quasar U5A5 + SR1100，single radio｜SDK v2.4.0-rc2
> 對照組：SPARK 官方 audio demo `SPARK_AUDIO_DEMO_eng-v1.0.0-ext_codec_support-r1`（同一塊 EVK）
> 前一世代的同一問題見 [fallback_mono_rung_rationale.md](fallback_mono_rung_rationale.md)
> 日期：2026-08-18（§5 於同日重寫）｜基準 commit：`1c3a672`

---

## 0. 一句話

**近端已解，遠端未解。**
近端的答案是 ISI 2 + 重傳餘裕 + 40 ms buffer 三個一起上（tag `v240-unidir-fbk5-ok`）。
遠端量到掉線當下 `lm = 0`，確認是真的鏈路預算耗盡；但把「輸出功率」這條線從頭走到底
（level 4 → 天花板 → node ACK 拉滿）都沒動到距離。
**發射設定現已與 demo 逐值對齊，距離仍然輸，而公開 API 裡已經沒有可加的旋鈕** —— 見 §5。

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

## 2. 有效的（現行配置）

> **2026-08-20 重大變更：unidir 的 ISI 已降為 level 1，近端改由雙天線分集守。**
> 這一節以下的三項（accumulator / buffer / mode 3 縮身）不受影響，仍然有效。
> 詳見 §2.1。

| 項目 | 值 | 為什麼有效 |
|---|---|---|
| **ISI mitigation** | **unidir：`SWC_ISI_MITIG_1`**<br>headset：`SWC_ISI_MITIG_2`<br>（兩端必須一致） | 見 §2.1 —— level 2 與雙 radio 不相容 |
| **Accumulator（重傳餘裕）** | mode 3 = 23/10（3.7 次）<br>mode 4 = 40/10（6.4 次） | Accumulator 在 resampler **之前**，是唯一決定「一包能被送幾次」的旋鈕。slot 是按**包**花的不是按 byte |
| **底部兩階 buffer** | 40 ms（30 ms 不夠，近端斷音會回來） | 遮擋持續數百 ms；queue 排空之後重送再多次也沒用，封包已經過了播放時限 |
| **mode 3 縮回 54 B** | acc 4.6× → 2.3× | 100 B 的 mode 3 + ISI 2 的長 preamble 塞不進 250 µs slot → 長距離持續遮擋會當機。重傳餘裕改由 mode 4 扛 |

**這四項缺一不可**：ISI 提高「單次送達的機率」，accumulator 提供「足夠的次數讓機率兌現」，
buffer 提供「撐過突發遮擋的時間」。三個各補一個獨立的資源，任何一個單獨上都無效。

### 2.1 ISI 2 → 1：因為 level 2 與雙 radio 不相容（2026-08-20）

**量到的事實：雙 radio 的 node 在 `SWC_ISI_MITIG_2` 下完全不出聲** —— 只有 fallback mode 3
偶爾起得來 —— 而同一份 build 換成 level 1 或 level 0 就正常。coord 一律是單 radio。
在 u5a5 上用 headset app 量到，換到 u535 與 unidir 上行為一致。

**不是 SDK 的問題。** 乾淨的 vendor 樹（v2.4.0-rc2）雙 radio 跑得好好的，而把我們的
`swc_cfg.h` 跟它逐行比對，**唯一的差異就是 `NODE_ISI_MITIG` 這一行** —— vendor 根本沒設
`isi_mitig`，等於 level 0。

**機制就是 §3 第 7 條講 level 3 的那個，早了一級發生。** ISI mitigation 把 preamble 拉長，
而 preamble 屬於**連線**不屬於 fallback mode，必須容納 ladder 上最大的 payload 進 250 µs
slot；雙 radio 的排程更緊，所以**在兩顆 radio 上，level 2 的行為就是 level 1 顆 radio 上
level 3 的行為**。只有 mode 3 活下來是決定性的旁證 —— 它是 payload 最小的那一階。

**但這不是犧牲，取捨消失了。** 當初選 level 2 是因為 level 1 擋不住近端多重路徑 ——
**那是在單 radio 上量的**。雙天線看到的多重路徑不同，所以雙 radio 是用**分集**去對付
同一個問題，跟拉長符號是兩條不同的路。實測：**u5a5、雙 radio、ISI 1、24 kHz stereo，
近端遮擋不斷音**。

| | 近端遮擋 | 雙 radio |
|---|---|---|
| 單 radio + ISI 2（舊） | ✅ | ❌ 完全不通 |
| **雙 radio + ISI 1（現行）** | ✅ | ✅ |

而且分集對距離也有幫助，那是 ISI 給不了的（§5.3：ISI 不增加能量）。

**headset 維持 level 2**，因為那條線出貨走單 radio，沒有東西跟它衝突。
兩個 app 的預設不一樣是刻意的，兩邊的註解都寫了不要把它們對齊。
但要注意：**headset 自己的 dual radio preset 在預設值下是壞的**，要跑得加
`-DNODE_ISI_MITIG=SWC_ISI_MITIG_1`。

**還沒驗的兩件（交給 ODM）**：u535 的近端遮擋（這裡沒有條件測），
以及 level 4 輸出脈寬拉到 7 的影響（2026-08-20 才改，見 §3 第 4 條下方）。

---

## 3. 已測無效 —— 本世代（puretone_unidirectional，5.25 ms 排程）

| # | 嘗試 | 結果 | 為什麼無效 |
|---|---|---|---|
| 1 | **CCA try count 2 → 14**<br>`MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT` | ❌ 無改善 | `rx_rej` 爬而 `cca_fail` 不動 —— 是解碼失敗不是通道占用。14 次約 66 µs 吃掉 250 µs slot 的四分之一，還不是免費的。已改回 2（commit `28d1a67`） |
| 2 | **CCA try count（遠端）** | ❌ 不可能有效 | 遠端 `cca_fail` 也是 0，這格在任何距離都是 no-op |
| 3 | **Fallback level 4 輸出功率 = SPARK demo 值**<br>`FBK4_TX_POWER_REF=1`，width 6/6/6/6 gain 1/1/1/0 | ❌ 掉線距離完全沒動 | 量了**兩次**（一次配 ISI 1、一次配 slim3）。加功率不能把反射「解塗抹」—— 反射跟著等比放大。**雖然無效，仍已改為預設值**：這是整張功率表最後一個沒有對照來源的欄位，對齊參考比留著自創值有價值 |
| 4 | **Fallback level 4 輸出功率拉到天花板**<br>`FBK4_TX_POWER_REF=2`，width 7/7/7/7 gain 0/0/0/0 | ❌ **完全無影響** | 刻意跳過「再加一階」直接測上限：三階 width 都沒動到的東西，多一階不會翻盤。實測證實了這個預期。`bin/v240-unidir-slim3-l4max` |
| 5 | **Node ACK 功率拉到天花板**<br>`ACK_TX_POWER_MAX=1`，width 7 gain 0 | ❌ **完全無影響** | ACK 是全 app 唯一「不隨 ladder 移動」的功率，形狀最符合「鎖 mode 4 與自由跑距離一樣」——形狀對，結果還是沒動。`bin/v240-unidir-slim3-allmax` |
| 6 | **ISI level 1**（SPARK 出貨值） | ❌ 近端斷音回來 | 對 multipath 不夠 |
| 7 | **ISI level 3** | ❌ 無遮擋就爆音 | preamble 屬於**連線**不屬於 fallback mode，必須容納 ladder 上**最大**的 payload 進 250 µs slot。注意失效形式是頂階音質壞掉，不是 init 紅燈 |
| 8 | **底部階 buffer 30 ms** | ❌ 近端斷音回來 | 40 ms 才夠 |
| 9 | **Ladder 反應速度**（自由跑 vs 鎖 mode 4） | ❌ 兩者掉線距離相同 | 不是降階不夠快。這一條同時排除了整條 ladder 的門檻調整 |
| 10 | **radio saved calibration**<br>`RADIO_USE_SAVED_CALIB=true`（demo 傳 true，SDK example 傳 false） | ❌ **完全無影響** | 曾是「消去法剩下的首選」，實測沒有差別。`bin/v240-unidir-calib-isi1` |

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

### 三次功率／校正嘗試一起看：發射端這條路已經走到底

第 3、4、5、10 條全部無影響，而它們涵蓋的是**能從 app 這一側動的每一種「多給一點」**：
fallback level 4 的 width/gain 加到對齊 demo、再加到天花板、ACK 功率加到天花板、
以及讓 radio 用存下來的校正開機。四個方向、全部沒有動到掉線點。

這不是四個各自失敗的嘗試，是**同一個結論的四次確認**：這個失效不是能量不足能解的。
再往「加功率」的方向找下去沒有東西了——§5.5 之所以說參數空間是空的，
證據就是這一組。

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

## 5. 遠端距離：查證結果與剩餘候選

### 5.1 遠端失效已確認是真的鏈路預算耗盡

遠端掉線的當下量到 **`lm = 0`**。不是假斷線、不是同步問題，是訊號弱到解不開。
所以「加鏈路預算」是正確的方向 —— 問題是**已經沒有旋鈕可加了**，見下。

### 5.2 發射端已與 demo 完全對齊（`1c3a672`）

| 項目 | 狀態 |
|---|---|
| Fallback 輸出功率 level 1–3 | 本來就逐值相同 |
| Fallback 輸出功率 level 4 | **已改採 demo 的值**（width 6/6/6/6 gain 1/1/1/0），`FBK4_TX_POWER_REF` 預設 1 |
| Node ACK 功率 | 查證後**本來就相同**（width 5 / gain 1），無需更動 |
| `SR1100_PULSE_COUNT` | **兩邊都是 1**（demo 定義在 `wireless_common.c:16`，全套件唯一一處） |
| chip rate 基準 | 兩邊都是 20.48 MHz |
| FEC / 調變 / 並行模式 / 通道 | 相同 |

**同一塊板、同一支天線、發射設定逐值相同，距離仍然輸。**
所以差異只可能在接收靈敏度或 PHY 內部。

### 5.3 三個曾被視為候選的設定，逐一排除

| 候選 | 判定 | 理由 |
|---|---|---|
| **PHY rate / ISI** | ❌ 方向相反 | ISI mitigation 是在符號間插停頓，**不增加能量**。純距離靠 Eb/N0，ISI 只是花空中時間換抗多重路徑。而且本 app 已在五檔中最慢的一檔（10.24 MHz 有效），往上走是拿距離換頻寬 |
| **`SR1100_PULSE_COUNT`** | ❌ 無差異 | 曾列為「+3 dB，比整個 gain 欄位範圍還大」的首選。實查**兩邊都是 1** |
| **排程**（5.25 ms vs 2.25 ms） | ❌ 物理不對 | 週期短只讓重傳機會更密集。**距離不夠時每一次重傳都同樣太弱**，多試不會讓封包突然解得開。這跟近端遮擋是不同的物理 |

### 5.4 `chip_repetition` —— 唯一真正的能量旋鈕，但兩邊都沒用

這是本次唯一能直接乘上每位元能量、因而直接換距離的設定（`SWC_CHIP_REPET_1..4`）。

| | 本 SDK v2.4.0-rc2 | demo 的 SDK |
|---|---|---|
| 型別 `swc_chip_repetition_t` | 有（`swc_def.h:106`） | 有 |
| `swc_connection_cfg_t.chip_repet` 欄位 | **沒有** | **有**（`swc_api.h:230`） |
| 公開 API 接受它 | **沒有任何函式** | 透過連線設定 |
| 預編譯庫符號 | 有 `wps_set_chip_repet`（無標頭宣告） | — |
| **demo 實際設定的值** | — | **沒設，zero-init = `SWC_CHIP_REPET_1`** |

**結論：欄位差異不是距離差異的來源。** 我們的 SDK 確實少了這個入口，
但 demo 從頭到尾沒有用它，兩邊實際都跑在 chip repetition = 1。

> 附帶更正：先前推測「demo 的 preset 內含 `chip_repetition`，所以底階能多拿能量」是錯的。
> preset 0 與 preset 1 的差別**只有 `isi_mitig`**，`chip_repetition` 屬於連線層、兩個 preset 共用。
> preset 機制給的是「逐階切 ISI」（頂階短 preamble、底階抗多重路徑），
> 那是**容量與近端**的好處，不是距離的。

### 5.5 目前的處境

**在 v2.4.0-rc2 的公開 API 範圍內，已經沒有可以增加鏈路預算的旋鈕。**
功率表已滿並與 demo 對齊、pulse count 相同、chip repetition 兩邊都是 1 且我們構不到、
`swc_set_phy_mode()` 只能在有效速率階梯上移動而方向與需求相反。

剩下無法從 app 這一側解決或驗證的：

1. **排程**（2.25 ms / 7+2 槽 vs 5.25 ms / 20+1 槽）—— 論證上不該影響距離，但未實測
2. **預編譯 wireless core 的版本差異** —— 兩邊 SDK 版本不同，PHY 與解調實作無法檢視
3. **量測方法本身** —— 我方的條件已經固定下來並記在這裡：**距離固定 5 公尺、
   以人體遮擋視線**，每次都一樣，所以我方數據之間是可比的。**尚未確認的是 demo 那一邊
   用什麼條件量**（路線、朝向、有無人體遮擋、自由跑 vs 鎖階）。在把差異歸因於韌體之前
   值得先確定這點——特別是「人體遮擋」與「純自由空間距離」是兩種物理，
   如果兩邊量的不是同一件事，整個比較就不成立

### 5.6 建議向 SPARK 確認的兩件事

1. `swc_connection_cfg_t` 的 `chip_repet` 欄位在 v2.4.0 被移除是刻意的嗎？
   型別仍在 `swc_def.h`、庫裡仍匯出 `wps_set_chip_repet`，但沒有標頭宣告 ——
   看起來像漏掉的介面而非刻意移除
2. `swc_connection_fallback_cfg_t.presets`（逐階 PHY preset）v2.4.0 正式版會補上嗎？
   這不是距離問題的解，但它是「頂階要短 preamble、底階要抗多重路徑」在本 SDK 上
   表達不出來的原因 —— 目前只能全階共用一個 ISI 等級

### 5.7 其他沒測過的

| 項目 | 現值 | 備註 |
|---|---|---|
| `dynamic_phy_mode_enabled` | `false`（未設定） | `swc_set_phy_mode()` 的前提。但速率階梯方向與需求相反 |
| 排程 | 21 槽 × 250 µs | 見 §5.5 第 1 點 |

## 6. 建議順序

1. ~~先補 §3 的三個空格~~ **已補**（l4max / allmax / calib，三者皆完全無影響）。
2. **確認 demo 那一邊的量測方法**（§5.5 第 3 點）。我方是固定 5 m + 人體遮擋；
   demo 那邊用什麼條件還不知道。這比再試一個設定便宜。
3. **向 SPARK 提 §5.6 的兩個問題**。`chip_repet` 欄位與 fallback `presets` 都是
   本 SDK 構不到、而 demo 的 SDK 有的東西。這是目前唯一還有槓桿的方向。
   要寄出去的那份寫在 [spark_link_budget_questions.md](spark_link_budget_questions.md)。
4. **不要讓出貨卡在這裡。** 現行配置近端已解、遠端當機已解，距離的缺口有明確的
   技術理由。擋出貨的是 AT / u535 / auto-reconnect 等移植，那條線該繼續走。

## 附註：一處註解與實際值不符 —— 已修正

[swc_cfg.h](app/example/puretone_unidirectional/config/sr1100/swc_cfg.h) 的 `NODE_ISI_MITIG`
註解曾寫「Back at 1 to measure it against the current ladder」，但實際值是 `SWC_ISI_MITIG_2`
（`624e046` 改成 1 之後 `e46a386` 又收回 2，註解沒跟上）。已改寫成記錄「2 是現行值、
降到 1 是追當機時的繞路，而真正的解是把 mode 3 縮回 54 B」。

同時：`FBK4_TX_POWER_REF` 預設已從 0 改為 1，level 4 輸出功率改採 SPARK demo 的值。
node 的 ACK 功率經查**本來就等於 demo**（width 5 / gain 1），無需更動。
完整的設定快照見 [link_rf_settings_baseline.md](link_rf_settings_baseline.md)。
