# puretone_unidirectional 待辦 —— 還沒搬的、已改未驗的、擱置的

> 建立於 2026-08-20。基準 `v240-dev`。
> 目標組態：unidir、5 階 fallback、24 kHz stereo、u535 + u5a5、雙 radio、ISI 1。
> 相關：[link_dropout_arms_ledger.md](link_dropout_arms_ledger.md)、
> [pairing_identity_preprovision_spec.md](pairing_identity_preprovision_spec.md)、
> [at_cmd_bidir_decision_spec.md](at_cmd_bidir_decision_spec.md)、
> [u535_ldo_rx_deficit.md](u535_ldo_rx_deficit.md)。

---

## 0. 版本切分（2026-08-21 定案）

| 版本 | 內容 | 狀態 |
|---|---|---|
| **v240 rc01** | unidir、5 階 fallback、24 kHz stereo、雙 radio、ISI 1、reconnect、階梯 hold | **AT 不在裡面** |
| **v240 rc02** | AT 層（§1.2）、預配對指令（§1.3） | 之後 |

**AT 進 rc02 而不是 rc01**，理由是 audio 不靠它：即使板子上有 SOC，
不做任何 AT 交握也能開機、配對、出聲、跑滿五階 —— reconnect 做完之後尤其如此。
把它們切開，rc01 就能先進 ODM 的 audio 測試，而不用等一個測試不需要的東西。

rc01 剩下的缺口在 §2 和下面的「rc01 收尾」。
其餘全部是「產品要但測試不需要」、「改了但沒上機驗」、或「知道但擱著」。

### rc01 收尾

| # | 項目 | 狀態 |
|---|---|---|
| a | fallback 階梯（凍結／沉澱／釘住） | **已驗**（2026-08-21） |
| b | 近端遮擋 + 最大脈寬 | **已驗**（2026-08-21，雙天線） |
| c | **u535 LDO 實機** | **已驗**（2026-08-21） |
| d | **u535 SMPS 實機** | 交給 ODM，這裡沒板子 |
| e | 板子組合矩陣 | 見下 |

### 板子與組態矩陣

| 板子／組態 | 狀態 | 備註 |
|---|---|---|
| u5a5，雙 radio，ISI 1，24k stereo | **已驗** | 近端不斷 |
| u535 **LDO** | **已驗**（2026-08-21） | |
| u535 LDO，雙 radio + ISI 1 | 通 | 近端這裡沒條件測 |
| u535 **SMPS** | **未驗** | **ODM 驗**，見下 |

#### SMPS 交給 ODM 之前做的事

SMPS 的 preset 之前是隱藏的（不是當前 campaign），而**隱藏的 preset 沒辦法
configure** —— `Cannot use hidden preset`。我們這邊還編得出來只是因為 build dir
還留著；**乾淨的 checkout 編不出 SMPS**。SMPS 現在是當前的了，所以 unidir 那三個
放回可見（headset 的 SMPS 仍不是當前的，維持隱藏）。

三包都已用現在的程式碼重編、0 error 0 warning，也確認乾淨 configure 可以過。

**ODM 要知道的差異**（§3.3）：

* console **只有 LPUART1（PA3）**，SMPS 沒有 UART4 那個 header；
* SMPS 的 **PA2 收送都正常**，跟 LDO 那塊不可靠的 pad 不一樣 ——
  所以 SMPS 上 console 是可以雙向的，之後 AT（rc02）走這裡最省事；
* **SMPS 從沒上過機**。LDO 過不代表 SMPS 過：兩者差的是電源架構，
  而這條線上每一個難纏的問題都跟 RF 有關。

---

## 1. 移植待辦（有順序，前兩項相依）

### 1.1 ~~reconnect~~ —— **已完成**（2026-08-21）

**已做完並實機驗過**（配對後單獨重開一邊會自己回來、出廠裝置仍能配對、
按鍵仍能 unpair、unpair 後下次開機是配對而不是回連）。
commit：`85e6e3c`（u5a5 保留頁位址修正）、`f555ef7` + `86d349d`（共用化，零行為改變）、
`1b7d5da`（開機回連）、`72ff128`（回連時快閃 LED）。

`reconnect_store` 現在住在 `app/common/reconnect_store/`，
flash backend 在 `common_backend/quasar_nv_backend.c`，兩個 app 共用。

以下保留原本的規劃記錄。

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
* **u535 的 console 分配 —— 兩塊板不一樣**（見 §3.3）：

  | 板子 | AT console | stats / LINK_WATCH |
  |---|---|---|
  | **LDO** | **UART4（PC10/PC11）**，因為手上這塊的 PA2 RX pad 不可靠，LPUART1 收不到 | LPUART1（PA3，只發） |
  | **SMPS** | **LPUART1（PA3/PA2）**，收送都正常，是它天生的位置 | UART4，或維持 LPUART1 共用 |

  也就是說 AT console 的落點是**板子相依**的，跟現在 console 的分工一樣用
  `U535_PWR_LDO` 判別。不要寫成一個固定的選擇。

**注意這會改變上空中的格式**（`user_data_t` 2 → 最多 16 bytes，常見 5 bytes），
兩端必須一起重燒。unidir 目前沒有出貨在外面的版本，所以沒有版本錯配風險——
但如果 ODM 手上已經有 binary，時間點要挑。

### 1.3 預配對 AT 指令

`AT+LE_UWB_SET_PAIR` / `GET_PAIR?` / `SERIAL?`（spec §5.4）。依賴 §1.1 和 §1.2。
產線用的，不擋 ODM 整合。

### 1.4 + 1.5 ~~階梯的凍結與釘住~~ —— **已完成**（2026-08-21）

原本規劃成兩條，實作時證明是**同一條**，所以合併在這裡。

**最後做出來的規則，跟原規劃不一樣**，差異就是這一節存在的理由：

| 規則 | 條件 | 為什麼 |
|---|---|---|
| **FREEZE** | `link_is_up()` 為假 | 對面不在時降階，是對著沒有人的鏈路下判斷 |
| **SETTLE** | 對面回來後 `LADDER_SETTLE_MS`（2 s） | **原規劃沒有這條**，見下 |
| **PIN** | 看到**往下踏進底階那一步**（prev < 4 且 cur == 4） | **不是**「現在在底階」，見下 |
| 全部讓位 | `fallback_state != FALLBACK_AUTO` | 有人按了按鍵選階，不能被蓋掉 |

開關是 `FALLBACK_PIN_AT_BOTTOM`（`sac_cfg.h`，預設 1），只管 PIN；
FREEZE 和 SETTLE 是無條件的，因為它們修的是既有的錯誤行為，不是新功能。

#### 為什麼 PIN 必須是「那一步」而不是「在那裡」

原規劃寫的是「目前模式等於 4 就 `set_manual_mode(true)`」。**實作出來會一直卡在 4。**

對面重開時，佇列在 `link_is_up()` 察覺之前（200 ms）就已經把階梯推到底了，
所以凍結接手時它人已經在 4。等對面回來解凍，「現在在底階」**當場成立** ——
在一條完好的鏈路上永久釘死。這正是這兩條規則本來要避免的那個結果。

改成偵測 transition 之後，**凍結期間也照樣記錄 mode**：
階梯若是在被 hold 的時候降下去的，解除時那個位置就是基準線，
看不到「往下那一步」，latch 不了。

#### 為什麼多出 SETTLE

**DG 單獨重開也會釘住**，而那時候對面全程都在 —— 凍結從未生效。
原因是同一個佇列論證再往前一步：鏈路剛建立時，producer 已經在填、
connection 還沒起來，佇列深是**暖機**不是證據，`is_link_bad()` 讀成鏈路壞，
1→4 一口氣走完。

所以「對面回來之後」也要 hold 一段時間，不只是「對面不在的時候」。
這點不直觀 —— 對面明明可達，凍結已經解除。

順帶解決了原 §1.5 記的觀察（HS 重開後從 mode 1 直衝 mode 4），
當時寫成「原設計如此」放過了；一旦降階會 latch，它就不能再放著。

#### PIN 不會活過對面重開

`NODE_RESTART_SILENCE_MS`（3 s）。重開和遮擋在任何單一瞬間都一樣
（都是「什麼都沒收到」），分得開的是**持續時間**：重開是秒，遮擋是數百毫秒
（`link_dropout_arms_ledger.md`）。超過 3 秒 → 已經不是同一條鏈路 → 放掉 pin。
遮擋是同一條鏈路 → **保留 pin**，而遮擋下的來回震盪正是 PIN 存在的理由。

#### 副作用，量測和 ODM 說明要跟著講

釘住之後鏈路變好也不會回到 48 kHz，
所以「聽起來一直是 24k」**不再是故障徵兆，而是預期行為**。

commit：`748fdf1`（transition）、`bbeb33c`（settle）。

**還沒驗的**：PIN 的**正向路徑** —— 真的因為遮擋從 3 掉到 4、印出
`[FB] stepped down to the bottom rung; pinned`、之後不再往上爬。
負向路徑（DG／HS 重開**不該**釘住）已驗過。

### 1.6 SINE generator —— 獨立，隨時可做

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
而它跟 unidir 現在的兩件事**都**衝突，搬過來時要一起處理：

* 96 kHz 被 cap 住（§3.2），mode 0 根本不在啟用清單裡；
* §1.4+1.5 的 FREEZE／SETTLE／PIN 會用 `sac_fallback_set_manual_mode()`
  把階梯壓在別的地方 —— 這**不再是假設**，已經實作了。

三者搶的是同一個 manual mode 旗標，所以不是「注意一下」，是**開 SINE 就得
把階梯的 hold 整個讓開**，大概要一個把 `fallback_hold_handler()` 停用的開關。

---

## 2. 已改未驗 —— ODM 這輪會驗到

| # | 項目 | 狀態 |
|---|---|---|
| 2.1 | **u535 近端遮擋**（雙 radio + ISI 1） | u5a5 上通過；u535 這裡沒條件測 |
| 2.2 | **level 4 脈寬 7/7/7/7** | **近端已通過**（2026-08-21，雙天線） |
| 2.3 | **ACK 脈寬 7/7/7/7**（HS→DG） | 同上 |

2.2／2.3 的經過值得留著，因為它示範了一個容易誤讀的結果：

近端曾經斷音，脈寬是當時**唯一會把更多能量送進近端反射的新改動**，
所以被當成第一嫌疑犯關掉（`375f18e`）。**接著近端就好了 —— 但原因是裝上第二支天線。**
那次乾淨的測試是在**脈寬關閉**的狀態下跑的，所以它對脈寬本身什麼都沒說。

脈寬改回最大（`7c45f52`）之後**重跑近端，通過**。
到這裡才算驗過，而在那之前，「近端好了」和「脈寬沒問題」是兩件不同的事。

gain 兩者都**沒動**，維持原值。

---

## 3. 已知、擱置

### 3.1 u535 到達率只有 30% —— **§1.1 做完，已解鎖**

見 `u535_ldo_rx_deficit.md`。**天線／傳播** vs **接收鏈設定** 還沒分開，
而分開它們的方法是距離掃描（3 m → 10 cm，看到達率是隨距離爬升還是平的）。

**曾經做不了**，因為掃描過程中鏈路會斷 —— 那本來就是要量的東西 ——
而沒有自動回連，斷了就得兩邊一起重開重新配對，每個距離點都要來一次；
每次重配對換一組位址、重跑排程對齊，**等於每個點量的都是不同的一次連線**。

**§1.1 已完成，這個限制消失了。** 現在鏈路斷掉會自己接回來，
可以搬著板子掃距離。做的時候照下面三點。

### 3.4 u5a5 完全沒有 console 輸出 —— minor，之後處理

unidir 的 `facade_print_string` 只在 `#if defined(QUASAR_U535)` 裡被覆寫成 UART。
u5a5 落到 `common_backend.c` 的 weak 版本：

```c
if (tud_cdc_connected()) { tud_cdc_write_str(string); ... }
```

而**整個 tree 裡沒有任何地方呼叫 `tusb_init()` / `tud_init()` / `tud_task()`**
（只有 `dev_board_io_test_backend` 有 `tud_task`），unidir 又是 `USB_AUDIO_ENABLED=0`。
所以 `tud_cdc_connected()` 永遠是 false，**u5a5 上一個字都不會印** ——
不只開機橫幅，是全部。

影響：u5a5 只能靠聽聲音和看 LED 測。已寫進 `unidir_audio_test_readme.md` §6。

修法大概是給 u5a5 一個 UART console（跟 u535 一樣走 facade 覆寫），
或是把 USB CDC 真的初始化起來。前者簡單得多。

### 3.5 開機橫幅接電幾毫秒就送出，terminal 開得晚會錯過

在 `main()` 最前面印，這是刻意的 —— 沒有它的話，「serial port 什麼都沒出來」
有兩個分不開的意思：console 壞了，或 console 好好的但程式在印任何東西之前就死了。

代價是先上電、後開 terminal 的人看不到它。統計行每行都帶 `v240_rc01`，
所以版本不會丟，丟的是 **build 時間和板子／radio 標籤** ——
而那正是分辨「同一個 commit 編給不同板子的兩包」的欄位。

改法（已寫好但沒套用）：把橫幅抽成 `print_banner()`，
在第一行統計之前再印一次。兩次不重複，因為它們**在相反的情況下失效**。

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

### 3.3 console 走哪個 UART —— 依板子變體而定（2026-08-20 定案）

不再是「暫時都送、之後砍一個」，是**刻意的分工**：

| 板子 | console |
|---|---|
| **LDO** | **UART4（PC10）+ LPUART1（PA3）都送**。兩個 header 都有焊、bench 上兩個都會用到，**會用很久**，所以都留。代價是每行多一次阻塞傳送、約 2 ms，這是接受的 |
| **SMPS** | **只有 LPUART1（PA3）**，那是這個變體有的 header |

判別用 `U535_PWR_LDO` —— CMakeLists 只在 preset 選 LDO 時才定義它，
所以**它不存在就代表是 SMPS**。兩邊都 build 過確認分支正確。

console **只發不收**是因為它是單向的診斷通道，不是因為收不到 ——
而**接收能力兩塊板不一樣**，這對之後要接 AT 很關鍵：

| 板子 | PA2（RX） | 對 AT 的意義 |
|---|---|---|
| **LDO** | 手上這塊是**不可靠的 pad** | LPUART1 不能收，**要 RX 的東西得走 UART4（PC10/PC11）** |
| **SMPS** | **收送都正常** | LPUART1 可以當完整的雙向 console |

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
