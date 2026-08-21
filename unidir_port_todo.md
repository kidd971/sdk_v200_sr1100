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

### 1.4 fallback 掉到 24k 就釘住，不再往上爬 —— 獨立，暫時性

**暫時的做法，要有 define 開關。** 現在階梯是雙向的：條件變好就往上恢復。
要改成單向 —— 一旦降到 mode 4（24 kHz stereo），就固定在那裡不再回升。

**跟 reconnect、AT 都無關**，ladder 完全活在 coord 裡（`sac_fallback_instance`、
`change_fallback_state()`），隨時可以做。

實作大概是：在 coord 的週期性檢查裡看目前模式，一旦等於 4 就
`sac_fallback_set_manual_mode(true)` 把自動移動整個停掉 —— 那比逐一
deactivate mode 0–3 乾淨，因為 §3.2 的 96k cap 已經在用
`sac_fallback_mode_set_active_state()`，兩者疊在一起會很難讀。

開關預設**開**（因為這是現在要的行為），但一定要留得掉 —— 它是暫時的，
名字和註解都要說清楚它為什麼存在、以及什麼條件成立時該拿掉。
不然三個月後沒有人敢動它。

**⚠️ 先讀 §1.5。** 對面重開機會讓 coord 的佇列積起來、把階梯一路推到 mode 4 ——
如果這時候「釘住」，產品會在第一次 peer 重開之後永遠停在 24k。
**這兩件事必須一起設計**，不能只做這一條。

**要記得的副作用**：釘住之後，鏈路變好也不會回到 48 kHz，
所以「聽起來一直是 24k」不再是故障徵兆，而是預期行為 ——
量測和 ODM 的測試說明都要跟著講。

### 1.5 對面不在時，階梯會一路降到底 —— 原設計如此，可優化，先不改

**觀察**：HS 單獨重開機，coord 沒動，HS 一回來就看到 `fb=4`。
從 mode 1 起跳、但馬上跳到 4。

**這不是移植帶進來的，是 SAC fallback 原本的設計。** 查證過的鏈條：

1. **node 是純跟隨者。** `sac_fallback.c` 裡 RX 端直接把收到的標頭寫進自己的模式：
   ```c
   if (inst->is_tx_device) { update_state(inst); header->fallback = ...; }
   else                    { sac_fallback_set_current_mode(inst, header->fallback, ...); }
   ```
   所以 HS 顯示 4 = **coord 正在送 4**。node 這邊沒有決策，
   §3.2 加在 node 上的 cap 只管到「收到第一包之前」的起始值。

2. **coord 的降階條件是佇列，不是無線品質**：
   ```c
   is_link_bad() = is_link_queue_size_high() || is_link_cca_bad()
   is_link_queue_size_high() = consumer 佇列平均長度 > 該階門檻
   ```
   對面消失 → TX 佇列積起來 → 判定 link bad → 降階 → 佇列還是滿 → 再降 → 幾步到底。

**它正確地偵測到一個真實狀況，但把「對面不在」當成「鏈路很差」處理**，
而這兩件事該有不同的反應 —— 對著沒有人的地方降到 24k，一樣沒有人。

**可以怎麼優化。** coord 現在**有能力分辨**了：§1.1 的收包看門狗 `link_is_up()`
回答的正是「對面在不在」。所以可以在對面不在時**把階梯凍住（manual mode）
而不是讓它降**，對面回來再解凍。成本很低，因為判斷式已經在那裡了。

### ⚠️ 跟 §1.4 的交互作用 —— 這兩件事不能分開做

§1.4 要的是「掉到 24k 就釘住，不再往上爬」。如果**先做 §1.4 而不處理這一條**，
結果會是：

> **任何一次對面重開機 → 佇列積起來 → 降到 mode 4 → 被永久釘住。**

也就是說產品會在第一次 peer 重開之後，**永遠停在 24k**，而且鏈路再好也回不來。
那不是 §1.4 想要的行為，但它會是 §1.4 直接得到的行為。

所以兩者必須一起設計。合理的組合大概是：
**對面不在 → 凍住階梯（不降）；對面在且降到 4 → 釘住。**
這樣「釘住」只會發生在真的因為鏈路差而降到底的情況，
而不是因為有人按了重開機。

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
而 unidir 目前 96 kHz 是被 cap 住的（§3.2）—— 搬過來時這兩者互斥，要處理。
而且 §1.4 若把階梯釘在 mode 4，跟「釘在 mode 0 做純音」也是互斥的，一併考慮。

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

### 3.1 u535 到達率只有 30% —— **§1.1 做完，已解鎖**

見 `u535_ldo_rx_deficit.md`。**天線／傳播** vs **接收鏈設定** 還沒分開，
而分開它們的方法是距離掃描（3 m → 10 cm，看到達率是隨距離爬升還是平的）。

**曾經做不了**，因為掃描過程中鏈路會斷 —— 那本來就是要量的東西 ——
而沒有自動回連，斷了就得兩邊一起重開重新配對，每個距離點都要來一次；
每次重配對換一組位址、重跑排程對齊，**等於每個點量的都是不同的一次連線**。

**§1.1 已完成，這個限制消失了。** 現在鏈路斷掉會自己接回來，
可以搬著板子掃距離。做的時候照下面三點。

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
