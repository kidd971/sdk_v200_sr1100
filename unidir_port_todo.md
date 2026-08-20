# puretone_unidirectional 待辦 —— 還沒搬的、已改未驗的、擱置的

> 建立於 2026-08-20。基準 `v240-dev`。
> 目標組態：unidir、5 階 fallback、24 kHz stereo、u535 + u5a5、雙 radio、ISI 1。
> 相關：[link_dropout_arms_ledger.md](link_dropout_arms_ledger.md)、
> [pairing_identity_preprovision_spec.md](pairing_identity_preprovision_spec.md)、
> [at_cmd_bidir_decision_spec.md](at_cmd_bidir_decision_spec.md)、
> [u535_ldo_rx_deficit.md](u535_ldo_rx_deficit.md)。

---

## 0. 一句話

**現在就可以發給 ODM 測 audio** —— unidir 沒有 TODO/FIXME、沒有欠著的分支。
下面全部是「產品要但測試不需要」、「改了但沒上機驗」、或「知道但擱著」。

---

## 1. 移植待辦（有順序，前兩項相依）

### 1.1 reconnect —— 先做

unidir 對 `reconnect_store` / `facade_nv_*` / `try_boot_reconnect()` 的引用數是 **0**。
現況是 `main()` 裡一句無條件的 `enter_pairing_mode()`，**每次開機都重新配對**，
單獨重開一邊就斷線。

**為什麼排第一**：`reconnect_store.c` 只依賴 `string.h` 和 facade，
對 `at_cmd_core` 的引用是 0 —— 它不需要 AT。反過來不成立：§1.3 的預配對指令
要寫的就是這筆記錄，沒有它那些指令沒有東西可寫。**相依性是單向的。**

**而且它已經不只是產品需求，它是量測工具。** §3.1 的距離掃描做不了，
就是因為沒有回連 —— 鏈路一斷就得兩邊重開重新配對，每個距離點量到的都是
不同的一次連線。要回答 u535 那 30% 是天線還是設定，得先有這個。

要搬的（pairing spec §6.2）：

| | 備註 |
|---|---|
| `reconnect_store.c` / `.h` | 純邏輯 |
| NV backend | 對應 `puretone_headset_nv_backend.c` |
| **linker script 的保留頁** | **最容易漏，漏了會安靜地寫壞東西** |
| app 端 boot 狀態機 | 換掉那句無條件的 `enter_pairing_mode()` |

建議切兩個 commit：先「搬檔案 + 保留頁 + backend」（不改行為），
再「boot 狀態機」（改行為），這樣要回退乾淨。

### 1.2 AT 層

§10 寫「從零」已經過時 —— **三個接線點都在**（`user_data_t`、`tx/rx_data_conn`、
RX callback），`MAX_DATA_PAYLOAD_SIZE` 也已經是 16，跟 headset 一致。

缺的是：

* **backend facade 6 個函式**（`facade_expansion_uart_init/write/flush/read_byte`、
  `facade_system_reset`、`facade_uwb_shutdown`）。`facade_get_tick_ms` 已有。
* **app 端呼叫**：`at_cmd_core_init()` + role + ~10 個 `register_*`、
  主迴圈加 `at_cmd_core_process()`、打包時 `user_data_pack_vendor()`、
  收包時 `user_data_deliver_vendor()` 加 `cmd_type` switch。
* **`user_data_t` 擴充**：現在只有 2 bytes（`button_state` + `link_margin`），
  而且 **node 和 coord 各有一份手抄**（node:102、coord:122）——
  正是 §6.5(a) 那個「改一邊、編得過、跑起來欄位全錯位」的陷阱。
  headset 已用共用 header 解掉。建議把 `puretone_link_data.h` 搬到 `app/common/`
  兩邊共用，因為 vendor 通道的線上格式是**要交給 ODM 的契約**，
  兩個產品不該有兩份定義。
* **u535 的 console 分配**：AT 需要 RX，而 PA2 那隻 RX pad 是壞的，
  所以 AT console 走 UART4（PC10/PC11），stats/LINK_WATCH 退到 LPUART1（PA3，只發不收）。

**注意這會改變上空中的格式**（`user_data_t` 2 → 最多 16 bytes，常見 5 bytes），
兩端必須一起重燒。unidir 目前沒有出貨在外面的版本，所以沒有版本錯配風險——
但如果 ODM 手上已經有 binary，時間點要挑。

### 1.3 預配對 AT 指令

`AT+LE_UWB_SET_PAIR` / `GET_PAIR?` / `SERIAL?`（spec §5.4）。依賴 §1.1 和 §1.2。
產線用的，不擋 ODM 整合。

### 1.4 SINE generator —— 獨立，隨時可做

unidir 的 SINE 引用數是 **0**。headset 有：

| 開關 | 位置 |
|---|---|
| `SINE_INJECT_DG` | `puretone_dongle.c`（2 處） |
| `SINE_INJECT_HS` | `puretone_headset.c`（4 處） |
| `SINE_DEBUG_CAPTURE` | `sac_cfg.h`（預設註解掉） |

另有 5 個隱藏的 `*-sineinj` preset。

**不依賴上面任何一項**，順序上可以插在任何地方。什麼時候該做：
**要做客觀音質量測（THD、頻譜）而不是用耳朵聽的時候**。
現在靠聽力判斷「有沒有聲音、會不會斷」是夠的；
一旦要回答「24 kHz ADPCM 這階的音質可不可以接受」，就需要一個已知的訊號源。

注意 `SINE_INJECT_DG` 會**把模式釘在 0（96 kHz）**做純音測試，
而 unidir 目前 96 kHz 是被 cap 住的（§3.2）—— 搬過來時這兩者互斥，要處理。

---

## 2. 已改未驗 —— ODM 這輪會驗到

| # | 項目 | 狀態 |
|---|---|---|
| 2.1 | **u535 近端遮擋**（雙 radio + ISI 1） | u5a5 上通過；u535 這裡沒條件測 |
| 2.2 | **level 4 脈寬 7/7/7/7** | 2026-08-20 才改，**從沒上過機** |
| 2.3 | **ACK 脈寬 7/7/7/7**（HS→DG） | 同上 |

**如果近端變差，第一個要關的是 `-DTX_PULSE_WIDTH_MAX=0`** ——
它是唯一會把更多能量送進近端反射的新改動，而且是新加的，不是原本就在的。

---

## 3. 已知、擱置

### 3.1 u535 到達率只有 30% —— **卡在 §1.1 reconnect**

見 `u535_ldo_rx_deficit.md`。**天線／傳播** vs **接收鏈設定** 還沒分開，
而分開它們的方法是距離掃描（3 m → 10 cm，看到達率是隨距離爬升還是平的）。

**做不了，因為 unidir 沒有 reconnect。** 掃描過程中鏈路會斷 ——
那本來就是要量的東西 —— 而沒有自動回連，斷了就得把兩邊一起重開重新配對，
每一個距離點都要來一次。這不只是麻煩：每次重新配對都換一組位址、重跑一次
排程對齊，**等於每個點量的都是不同的一次連線**，那條曲線就沒有意義了。

**所以這件事把 reconnect 從「產品要」升級成「量測工具」。** §1.1 做完才能做這個。

### 3.2 96 kHz 仍然會斷 —— cap 留著（2026-08-20 實測）

`MAIN_CHANNEL_ALLOW_96K=0`，**維持**。

當初搬這個 cap 過來是為了 96k 的 dual-radio TIM4 park，而那個診斷後來證明
**不是** dual 沒聲音的原因（是 ISI 2）。當時留下的問題是「ISI 1 之下 96 kHz
還會不會斷」。

**已實測：還是斷的。** 所以兩件事都成立而且互相獨立：

* dual radio 不出聲 → **ISI 2**，已修（改用 level 1）
* **96 kHz 本身在這條線上就是壞的** → 跟 ISI 無關，cap 是對的

也就是說 §4.1 那筆文件債的內容要修正一半：`radio_stall_wedge_open_issue.md`
把 TIM4 park 記成未解之謎是錯的（它有 workaround），但把 96k 當成一個真正的
問題**是對的**。

### 3.3 console 同時送兩個 UART

UART4（PC10）和 LPUART1（PA3）都送。這是 bring-up 的權宜做法，
知道哪個 header 通了之後應該砍掉一個——送兩次每行多花約 2 ms。

---

## 4. 文件債

| # | 檔案 | 問題 |
|---|---|---|
| 4.1 | `radio_stall_wedge_open_issue.md` | 把 TIM4 `arr=0xFFFD` 記成未解之謎，其實是已知的 96k dual park（`d531b16` 描述過）。而且當時推測那就是 dual 沒聲音的原因——**不是**，是 ISI 2 |
| 4.2 | `u535_ldo_rx_deficit.md` | 裡面 fb=0 的 30% 量的是一個**現在已被 cap 掉**的模式。fb=2/3 的 29%/30% 才是還算數的 |

---

## 5. 已經確認**不是**待辦的

* **`feat-fallback-24k` 分支**（領先 16 個 commit）——只動 `puretone_headset`，
  0 個檔案碰到 unidir。是 v231 時代的 headset 實驗，結論已在 v240-dev 上
  重新實作進 unidir（`b5d4cf6`、`cf05ccf`、`4a3a7b2`、`93d03f9`）。**被取代，不是欠著。**
* **`feat-fallback-mono` 分支**——領先 0 個 commit，已全數合入。
* **unidir 原始碼裡的 TODO / FIXME / XXX**——**0 個**。
