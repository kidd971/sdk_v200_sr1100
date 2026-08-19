# AT user command 雙向通路決策規格 (DG ↔ HS over UWB)

> 對象:rc08 施工者,以及 ODM 端對接 AT 介面的 MCU 韌體。
> 目的:記錄「為什麼 user command 目前只有 HS→DG」、「DG→HS 要怎麼補」、
> 跌倒偵測為什麼**不**做專屬欄位而走 vendor 通道,
> ODM 自定指令(vendor pass-through)為什麼是 TLV 而不是再多幾個 AT 指令,
> 以及 AT 介面改名成 `LE_UWB_*` 與 PRD 的對齊(§11)。
> 對應 code:`app/common/at_cmd_core/at_cmd_core.c`、
> `app/example/puretone_headset/puretone_dongle.c`、`puretone_headset.c`。
> 行號以本分支(`rc08-at-cmd`,base = rc07 / SDK v2.3.1)為準。
> 相關:`uwb_disconnect_decision_spec.md`(連線類指令)、`soc_reset_wake_decision_spec.md`。

---

## 0. 一句話定位

user command 的雙向通路**兩個方向都設計過**,但只有 HS→DG 被接上,
而且**接的方式跟設計不一樣**。DG→HS 那半邊在 core 裡是完整的,只是從來沒有人啟用它。

跌倒偵測是第一個真正需要 DG→HS 的功能,所以它同時是「補通路」和「第一個使用者」。

vendor pass-through(§6.5)是第二個使用者,但它的意義不同:前面每一個功能都是
**我方定義語意**,而它是把定義權交出去——之後 ODM 要加指令不必等我方發版。

---

## 1. 現況:core 的 DG→HS 是完整的,只是沒人啟用

`at_cmd_core.c` 從一開始就是雙向設計。五個媒體控制 handler 全部長這樣:

```c
/* handler_play — at_cmd_core.c:594 */
if (s_cmd_tx_cb != NULL) {
    s_cmd_tx_cb(0x02 /* CMD_PLAY */, 0);   /* DG: forward to HS over UWB */
}
if (s_play_hw_cb != NULL) {
    s_play_hw_cb();                         /* HS: apply locally */
}
```

註解白紙黑字寫著 DG 要轉發,`at_cmd_core_register_cmd_tx_cb()` 也在
`at_cmd_core.c:127` 好好地存在。

**但沒有任何 app 呼叫過它。** `s_cmd_tx_cb` 宣告在 `at_cmd_core.c:49`,
初值 NULL,至今沒有任何 `.c` 檔案註冊過它。五個 `if (s_cmd_tx_cb != NULL)`
分支(`:541 :555 :569 :583 :597`)全部是死碼。

## 2. HS→DG 是繞過這個設計、另外接出來的

HS 沒有用 core 的轉發機制。它把**「HS 套用到硬體」的 callback 拿來當「往 UWB 轉發」用**:

```c
/* puretone_headset.c:503 — 註冊的是 hw_cb */
at_cmd_core_register_play_cb(at_play);

/* puretone_headset.c:3125 — 實作卻是往 UWB 送 */
static void at_play(void) { s_pending_cmd = 3; }
```

core 以為它在叫 HS「把 play 套用到本地硬體」,HS 實際做的是「打包丟給 DG」。

完整路徑:

| 步驟 | 位置 |
|---|---|
| HS 的 SOC 下 `AT+PLAY` | — |
| core 呼叫 `s_play_hw_cb` | `at_cmd_core.c:600` |
| HS 設 `s_pending_cmd = 3` | `puretone_headset.c:3125` |
| 10 ms 後打包送出,送完清 0 | `puretone_headset.c:2342-2343` |
| DG 解碼 `cmd_type == 3` | `puretone_dongle.c:976` |
| DG 吐 `+EVENT: PLAY` 給自己的 SOC | `at_cmd_core_notify_play_received()` |

反方向(`puretone_dongle.c:2232` 的 `data_callback()`)**只塞 `link_margin` 和
`button_state`,完全沒碰 `cmd_type`**;HS 的接收端(`puretone_headset.c:992`)
也**只讀那兩個欄位**。所以 DG→HS 這條線在實體上通、在軟體上斷。

## 3. 因此有兩套號碼表,而且互相衝突

| | VOL | PLAY | STOP | NEXT | PRE |
|---|---|---|---|---|---|
| core 的表(`at_cmd_core.c:541-598`) | 0x01 | 0x02 | 0x03 | 0x04 | 0x05 |
| **線上實際跑的**(app 自編) | — | 3 | 4 | 1 | 2 |

不是筆誤,是兩次獨立設計各編各的。core 那套從沒上過線,所以衝突至今沒有機會發生。

同源的錯位還有一個:`puretone_dongle.c:439` 註冊了
`at_cmd_core_register_vol_cb(at_set_vol)`,而該鉤子的文件(`at_cmd_core.h:302`)
寫的是 *"Not used on the DG side — DG forwards the command over UWB instead."*
DG 拿它去調自己的 back_channel 音量了。

## 4. 實際行為:DG 那五個媒體指令現在是假的

| 在哪打 | 結果 |
|---|---|
| DG 打 `AT+PLAY` / `STOP` / `NEXT_TRACK` / `PRE_TRACK` | `s_cmd_tx_cb` 是 NULL、hw_cb 也沒註冊 → **回 OK,什麼都沒發生** |
| DG 打 `AT+VOL=n` | 只調到 DG 自己的 back_channel 音量,HS 不受影響 |
| HS 打上述任一指令 | 正常運作(見 §2 路徑),DG 的 SOC 會收到對應 `+EVENT` |

「回 OK 但什麼都沒做」是這裡最危險的部分:對 ODM 而言,指令看起來是成功的。

---

## 5. 決策 1:號碼表凍結,**不要**讓 ODM 重 map

**決定:把線上現值(1=NEXT / 2=PRE / 3=PLAY / 4=STOP)定為正典,改 core 去配合它。**

ODM 已經在對接 HS→DG 這條線。雖然請他們重 map 只是幾分鐘的工,但這筆交換划不來:

* **要買的**:讓一份沒有任何執行路徑會跑到的表看起來一致。它從未上線。
* **要付的**:一個版本錯配窗口。ODM 的 MCU 韌體與模組韌體必須同步更新,
  而我方不控制對方發版節奏。窗口期間新舊混用的失敗模式**不是「不動作」,
  而是安靜地執行錯的指令**——叫 PLAY 它 STOP。這種問題在現場看起來像硬體故障,
  第一個被懷疑的絕不會是編號。

當「有文件但沒跑過」與「沒文件但在生產環境跑」衝突時,**在跑的那個才是事實**。
把事實改成配合文件是反過來了。

落地方式——寫成 enum,兩邊共用,裸數字全部消滅:

```c
/* at_cmd_core.h */
typedef enum {
    AT_CMD_NONE       = 0,
    AT_CMD_NEXT_TRACK = 1,   /* HS -> DG,已上線,不可更動 */
    AT_CMD_PRE_TRACK  = 2,   /* HS -> DG,已上線,不可更動 */
    AT_CMD_PLAY       = 3,   /* HS -> DG,已上線,不可更動 */
    AT_CMD_STOP       = 4,   /* HS -> DG,已上線,不可更動 */
    AT_CMD_VOL        = 5,   /* 保留:尚未上線,值可自由選 */
} at_cmd_code_t;
```

`1`–`4` 之所以標「不可更動」,理由必須留在註解裡,否則下一個看到
「core 明明是 0x01–0x05」的人會很想把它改回去。

## 6. 決策 2:`FACE_UP` / `FACE_DOWN` **不做專屬欄位**,走 vendor 通道

> 本節在 2026-08-16 改過方向。原本的決策是給 `face_state` 一個獨立欄位、每包重送。
> 保留原理由於 §6.1,因為它現在變成 vendor 通道的已知限制,不是被推翻的錯誤。

**決定:跌倒偵測不進 `user_data_t` 的專屬欄位,而是當成 ODM 自定指令走 §6.5 的
vendor pass-through。**

理由是**通則優先於個案**:SOC 端未來還會有第二個、第三個「跌倒偵測」——
每一個都做一個專屬欄位,等於每一個新功能都要模組韌體改版 + HS/DG 雙邊同燒。
既然 vendor 通道存在的全部意義就是「ODM 加指令不必等我方發版」,
那第一個新功能就應該走它,否則等於一邊蓋了通道一邊繞過它。

跌倒偵測**沒有**任何非走專屬欄位不可的技術理由。它需要的可靠性
(見 §6.1)是 vendor 通道**也**做得到的,只是做的地方從模組移到 ODM。

### 6.1 但要誠實記下代價:vendor 是事件語意,不是狀態語意

原方案的核心論點仍然成立,而且現在變成 vendor 通道的一條使用注意事項:

* `FACE_DOWN` 掉包 → 對端**永遠不知道有人跌倒**。
* `FACE_UP`(解除)掉包 → 對端**永久卡在告警狀態**,
  因為下一次狀態轉換要等到下一次跌倒。

vendor 通道用「連送 `AT_VENDOR_TX_REPEAT` 包」把單次失敗率壓到 p³,
但那是**降低機率**,不是**自我修復**。專屬欄位每包重送才是後者。

**這個差別必須寫進給 ODM 的文件。** 解法不在模組端,在 ODM 端,而且很簡單:
把朝向當成狀態,**定期重送**(例如每 1 秒送一次目前朝向,而不是只在翻面時送一次)。
這樣就恢復了自我修復的性質,而且是 ODM 自己決定頻率——
比模組替他們決定「每 10 ms」更合理,因為告警的時間尺度是秒不是毫秒。

### 6.2 `0` 必須是「不知道」這條規則沒有消失

原方案裡「`0` 必須是 UNKNOWN 而不是 UP」的理由——
零初始化的封包會被讀成「朝上 = 正常」,**把「不知道」誤報成「安全」**——
在 vendor 通道上原封不動地成立,只是搬了家:

* wire 層:`vendor_id = 0` 保留為「本包沒有 vendor 指令」(§6.5),
  所以 ODM **不可以**把 0 當成一個合法的指令編號。
* ODM 層:ODM 自己的 payload 編碼也要遵守同一條規則。
  如果他們用一個 byte 表示朝向,`0` 應該是 UNKNOWN,不能是 UP。
  這條我方無法強制,只能寫在介面文件裡。

## 6.5 決策 3:vendor pass-through — ODM 自定指令走 TLV,模組不解讀內容

### 為什麼不是「再加幾個 AT 指令」

`PLAY` / `STOP` / `FACE_DOWN` 這些都是**我方定義、我方實作**的語意。每多一個,
ODM 就要等我方發版,而且 HS/DG 兩顆一起燒。ODM 想加一個「切換降噪模式」,
成本是一次模組韌體改版。

透傳把這件事反過來:模組只保證「把這串 bytes 從一端搬到另一端」,
內容由 ODM 的兩顆 SOC 自己約定。之後 ODM 要加幾個指令都不必動模組韌體。

代價是模組不再看得懂線上跑什麼,現場除錯只能看到 opcode 和長度。
這筆交換值得——因為模組本來就不該替 ODM 定義應用層語意。

### 前置:加欄位之前必須先修的兩件事

**(a) `user_data_t` 有兩份手抄定義。**
`puretone_headset.c:202` 與 `puretone_dongle.c:166` 是各自獨立的 struct,
靠人工保持一致。只改一邊 → **編得過、跑起來欄位錯位**,症狀是參數變成亂數。
vendor 欄位要動它,所以先抽到共用 header(`puretone_link_data.h`)。

**(b) `wireless_read_data()` 在 payload 過大時洩漏 RX buffer。**

```c
/* dongle.c:2578 / headset.c 同形 */
if (payload_size > size) {
    return 0;              /* ← 沒有呼叫 swc_connection_receive_complete() */
}
```

payload 沒被釋放。**舊版 FW 收到新版 FW 的大包 → 每 10 ms 洩漏一個 RX buffer
→ queue 滿 → 整條 data 連線永久死掉**,link_margin 跟著停,fallback 會亂跳。

這條在現行 code 裡踩不到(兩端 struct 一樣大),但本規格接下來每一步都在加欄位,
版本錯配從「不可能」變成「出貨期間的常態」。**先修,再加欄位。**

**修法是截斷,不是丟棄**:`user_data_t` 只准 append,所以短的那份看得懂的前綴
在長的那份裡位置完全一樣。兩個方向都退化成「兩邊都認得的欄位」,
這正是混版時想要的行為。

反方向(新版收舊包)本來就沒事:`received_user_data = {0}` 有零初始化,
少的欄位讀到 0。這也是 §6.2 那條「`0` 必須是『不知道』」對所有新欄位都成立的原因。

### 線上格式

```c
#define AT_VENDOR_PAYLOAD_MAX 8

typedef struct user_data {
    bool     button_state;
    uint8_t  link_margin;
    uint8_t  cmd_type;                    /* 1-4 既有媒體鍵,不可更動 */
    uint8_t  battery_pct;
    uint8_t  vendor_ack;                  /* 我方最後收到對方的哪個 seq */
    /* ---- 以下只在「本包有 vendor 指令」時才送 ---- */
    uint8_t  vendor_id;                   /* 0 = 本包無 vendor 指令 */
    uint8_t  vendor_seq;                  /* 1-255,收端據此去重;0 保留 */
    uint8_t  vendor_len;                  /* 0..AT_VENDOR_PAYLOAD_MAX */
    uint8_t  vendor_data[AT_VENDOR_PAYLOAD_MAX];
} user_data_t;                            /* 16 bytes = MAX_DATA_PAYLOAD_SIZE */
```

### 但**不是每包都送 14 bytes** —— 這件事比它看起來重要

vendor 是極低機率事件。若無條件送滿整個 struct,等於為了載空氣把
**每 10 ms、雙向**的封包從 4 bytes permanently 拉到 14。

這條線的 payload 長度就是空中時間,而 data 連線**與 back-channel audio 共用 timeslot**
——正是 fb=0 park 追查時把邊際問題歸因到的那個預算。所以送出長度是算出來的
(`user_data_tx_size()`),不是 `sizeof`:

| 情況 | 送出 |
|---|---|
| 無 vendor 指令(≈ 全部封包) | **5 bytes** |
| 有指令、無 payload | 8 bytes |
| 有指令 + N bytes payload | 8+N,最多 16 |

**穩態成本是 +1 byte,不是 +10。** 那 1 byte 是 `vendor_ack`,它是 level 必須每包送,
所以它排在 vendor 區塊**前面**;整個 vendor 區塊排在最後,才切得掉。

收端不需要任何長度旗標:它零初始化、`wireless_read_data()` 只填收到的部分,
所以被截掉的封包 `vendor_id` 讀到 0,本來就是「沒有指令」。
**「每個欄位的 0 代表不存在」這條規則(§6.2),就是變長傳輸免費的原因。**

代價是 append-only 規則多一條推論:**欄位要按「多常被送」排序,常送的在前**。
加在 vendor 區塊後面的新欄位**永遠不會被送出去**,因為長度算到 payload 就停了。

**全部欄位都是 byte,所以沒有 padding,`sizeof` 兩端必然一致。**
以後新增欄位務必維持 `uint8_t`——塞一個 `uint16_t` 進來會引入對齊 padding,
而 struct 是直接 `memcpy` 上線的,padding 的內容是未定義的。

### 為什麼是 TLV,不是固定的 `cmd_para1` / `cmd_para2`

固定兩個參數的形狀更省(2 bytes vs 8 bytes),但它把「以後夠不夠用」壓在
**wire format** 上。ODM 哪天需要第三個參數,代價就是再一次 struct 改版 +
HS/DG 雙邊同燒 + 一個版本錯配窗口——就是 §5 花整節在避免的那種東西。

12 bytes 本來就閒置在那裡沒人用。**用空間換掉一次未來的 wire format 改版,買。**

### 為什麼 `AT_VENDOR_PAYLOAD_MAX` 是 8

8 是**不動 `MAX_DATA_PAYLOAD_SIZE` 前提下能給的最大值**:5(常送欄位)+ 3(vendor header)
+ 8 = 16。ODM 說 6 大致夠用但多一點更好,而多給不必付代價,所以給滿。

**它是上限不是保留區。** `user_data_tx_size()` 送的是 `offsetof(vendor_data) + vendor_len`,
所以 payload 只放 1 byte 就只送 9 bytes,跟這個常數設多少無關。
**把上限拉高,在真的有人用到之前是完全免費的。**

大方給是因為兩個上限的時間性質相反:

| | 事後能不能改 |
|---|---|
| `AT_VENDOR_PAYLOAD_MAX` | **不能**。舊版收到較長 payload 會 clamp,那只防越界不防誤解——ODM 拿到砍半的指令照舊解析。必須在 ODM 對接前定案,跟 §5 的號碼表同性質 |
| `MAX_DATA_PAYLOAD_SIZE` | **可以**。舊版收到較長封包直接截斷、忽略看不懂的部分,不會誤解 |

所以**先把免費的空間花在前者,後者留著當以後的逃生口**,不是反過來。

真的需要更長的 payload 時還有第二條路:`vendor_seq` 已經在那裡,可以往上疊分段重組。

#### 代價:餘裕歸零

16 = `MAX_DATA_PAYLOAD_SIZE`,一個 byte 都不剩。所以未來若真的需要一個**常送欄位**
(照規則要 append 在 `vendor_data` 之後),就必須同時調大 `MAX_DATA_PAYLOAD_SIZE`。

那是可以接受的,因為上表第二列:調大 frame 上限是向後相容的。而且照本規格的設計意圖,
新功能本來就該走 vendor 通道而不是加欄位——`FACE` 就是第一個實例(§6)。

### 可靠性:兩種模式,由 host 逐指令選

vendor 指令跟媒體鍵有個關鍵差別:**媒體鍵掉了使用者會再按一次,
ODM 的自定指令沒有這個人肉重試。** 但不是每個指令都值得付 ACK 的代價,
所以做成兩種模式,第三個參數選:

| 模式 | 行為 | 適用 |
|---|---|---|
| best effort(預設,或 `"NOACK"`) | 連送 `AT_VENDOR_TX_REPEAT`(=3)包就丟掉,**沒人被告知結果** | host 本來就會定期重送的狀態類指令;人會重試的動作 |
| acknowledged(`"ACK"`) | **每包重傳**直到對方回 ack 或逾時(`AT_VENDOR_ACK_TIMEOUT_PACKETS`=100 包 ≈ 1 秒),兩種結果都吐事件 | 一次性、掉了會出事的指令 |

best effort 的算術:單包遺失率 p → 失敗率 p³,沒有狀態機、沒有逾時,
代價有界且已知(每個指令佔 vendor 欄位約 30 ms)。

**兩者的差別不是「比較可靠」,是「會不會告訴你」。** best effort 送 3 包只是把
失敗機率變小;acknowledged 是真的回報發生了什麼。一個叫「不能掉」但其實不保證
送達的旗標,比沒有旗標更糟——它讓 host 以為安全了。所以旗標的語意寫死成
「要不要回報」,不是「要不要可靠」。

### ACK 怎麼走線

`vendor_ack` 是**每包都送的 level**,不是一次性事件:它帶的是「我最後收到你的哪個 seq」,
連沒帶指令的包也照送。理由跟 §6.1 完全一樣——**只送一次的 ACK,跟它要確認的那個
指令一樣容易掉**。做成 level 之後,ACK 自己不需要再被確認一次。

`vendor_seq` 因此改成 **1–255,跳過 0**。沒收到過任何東西的一端 `vendor_ack` 是 0,
若 0 是合法 seq,那個 0 會被讀成「已確認第 0 號指令」。與 `vendor_id = 0`、
§6.2 是同一條規則。

`vendor_seq` 只用來去重與比對 ACK,**刻意不做 gap 偵測**。best effort 模式下
知道掉了也做不了什麼;acknowledged 模式下**發送端的逾時才是權威**,
收端再從 gap 猜一次只會給出第二個互相矛盾的答案。

### 一個併發陷阱:ACK 不能在收到的地方處理

ACK 在 **RX callback context** 抵達,但 TX queue 的 head 由 **data timer context** 推進。
若讓 RX 直接把已確認的項目退掉,head 就有兩個 writer,SPSC ring 不再成立,得上鎖。

所以 RX 只寫一個 byte(`s_vendor_peer_ack`),由 TX 路徑自己去看。
代價是一個封包的延遲(10 ms),換回整個免鎖的論證。

### `vendor_id = 0` 保留

`user_data_t` 兩端都零初始化,沒資料/未連線時讀到的就是 0。
若 `0` 是一個合法的 ODM opcode,一個空包會被讀成「ODM 送了 0 號指令」。
與 §6.2 同一個理由:**`0` 永遠是「沒有」,不是一個值。**

### id 號碼空間:ODM `1`–`223`,模組保留 `224`–`255`

| 範圍 | 誰的 |
|---|---|
| `0` | 保留為「本包沒有 vendor 指令」,永遠不是一個值 |
| `1`–`223` | **ODM 自己分配**,我方不解讀 |
| `224`–`255`(32 個) | **模組保留**,目前一個都沒用 |

保留 32 個是因為**現在是唯一免費的時機**。ODM 從自己的文件裡分號碼,一旦發出去,
要拿回任何一個就得協調兩份我方不控制的 host 韌體改版。把 255 個全給出去、
以後再回頭要,這筆交易沒有一個好的版本。

**而且保留只有在「今天的韌體收到就已經不轉發」時才成立。** 兩邊都做:

* `AT+VENDOR_CMD` 收到 `224`+ 回 `+CME ERROR: RESERVED_ID` —— 擋住 ODM 開始用。
* `at_cmd_core_notify_vendor_received()` 收到 `224`+ **直接丟棄,不吐事件** ——
  擋住已出貨的機器把「未來韌體的模組內部流量」當成 vendor 事件轉給 host。

第二條才是關鍵。只有發送端遵守的保留區,在**兩端跑不同版本**時就失效了 ——
而那正是保留 id 唯一會出現的場合。

`RESERVED_ID` 與 `BAD_ID` 分開回:前者是「指令合法但你在分配不屬於你的號碼」,
後者是「你的指令本身壞了」,host 的反應不同。

### 送出端要一個小 queue

10 ms 一包、一包一個 vendor 指令,而且一個指令要佔 3 包。ODM 連續下指令的話,
沒有 queue 就是後者覆蓋前者——跟 `cmd_type` 現在的失敗模式完全一樣。

深度 4(`AT_VENDOR_TX_PENDING_MAX`)的 ring buffer:queue 滿代表 ODM 在約 120 ms 內
連下 4 個指令,此時**回 `+CME ERROR: BUSY` 讓它自己退避,比安靜丟掉好**。
理由跟 §4 一樣——「回 OK 但什麼都沒做」是最危險的失敗模式。

實作是 single-producer / single-consumer ring,不上鎖:producer(AT context)寫完
slot 才推進 tail,consumer(10 ms timer/中斷)用完才推進 head,並且**永遠留一格空**
以區分滿與空。用共用計數器會需要兩邊都做 read-modify-write,那才需要鎖。

### ODM 介面

| 方向 | 形式 |
|---|---|
| 下指令 | `AT+VENDOR_CMD=<id>[,"<hex payload>"[,"ACK"]]` |
| 收事件 | `+EVENT: VENDOR_CMD:<id>[,"<hex payload>"]` |
| ACK 結果 | `+EVENT: VENDOR_CMD_ACK:<id>` / `+EVENT: VENDOR_CMD_FAIL:<id>` |

**參數是固定位置的,不是可變長度清單。** payload 參數只有**一個**,而且是
一整塊不透明 bytes。`AT+VENDOR_CMD=1,"02","03"` 會回 `SYNTAX`。

要 ACK 但沒 payload 是 `AT+VENDOR_CMD=1,"","ACK"`(空引號),不能寫 `=1,,"ACK"`。
第三個參數只收 `"ACK"` 或 `"NOACK"`(預設),**不收數字 0/1** ——
同一個參數有兩種寫法,等於把歧義又放回來。

範例用跌倒偵測(§6),因為它是第一個真實使用者。假設 ODM 把 `id = 1` 定義為
「朝向」、payload 是 1 個 byte:

```
AT+VENDOR_CMD=1,"02"        → 0x02 = 朝下(告警),best effort
AT+VENDOR_CMD=1,"01"        → 0x01 = 朝上
AT+VENDOR_CMD=1,"00"        → 0x00 = 不知道(§6.2:0 保留給「不知道」)
AT+VENDOR_CMD=1,"02","ACK"  → 同上但要求確認
                              → +EVENT: VENDOR_CMD_ACK:1  或  VENDOR_CMD_FAIL:1
AT+VENDOR_CMD=7             → id 7,無 payload

+EVENT: VENDOR_CMD:1,"02"   ← 對端收到
```

`1` 和 `02` 的意義**全部由 ODM 定義**,模組不解讀。上面只是示範,不是規格。

錯誤回覆(都是 `+CME ERROR: <reason>`,可分辨才有意義):

| reason | 意思 |
|---|---|
| `SYNTAX` | 格式錯:缺 `=`、非 hex 字元、hex 位數是奇數 |
| `BAD_ID` | id 不在 1–255(含下了 `0`) |
| `RESERVED_ID` | id 在 224–255,那是模組保留區 |
| `TOO_LONG` | payload 超過 `AT_VENDOR_PAYLOAD_MAX` |
| `BUSY` | 送出 queue 滿,退避後重下 |

`BUSY` 與 `SYNTAX` 必須分得開:前者是「等一下再來」,後者是「你的指令本身錯了」。
host 對這兩者的反應完全不同,合併成一個 `ERROR` 等於逼它猜。

### 為什麼 payload 要加引號

**引號是必要的,而且不是裝飾。** AT 慣例是:**不加引號的數字參數 = 十進位,
加引號的參數 = 字串**(`AT+CPBW=1,"12345",129,"John"`)。沒有引號的話,
讀到 `AT+VENDOR_CMD=17,12` 的人無從知道 17 是十進位而 12 是十六進位——
兩個相鄰的數字用不同進位,卻沒有任何視覺提示。

加引號等於借用慣例自己內建的區分機制,而不是另外發明一條要人從文件背下來的規則。

### 為什麼 payload 是無分隔的連續 hex

不是 raw bytes:AT 通道是行導向的 ASCII 協定,raw bytes 裡出現 `\r` / `\n` / `\0`
會把 parser 打爛。hex 是唯一不必替 AT 通道另外定義 escape 規則的選擇,
代價只是長度 ×2(8 bytes → 16 字元)。

不加分隔符(`01:FF`、`01 FF`),雖然那樣人比較好讀:

* **這是 AT 的既有慣例。** 3GPP 的 `AT+CMGS`(PDU mode)、`AT+CRSM`、`AT+CSIM`
  傳二進位都是連續 hex。ODM 若已有 AT stack,parser 多半已經預期這個形狀。
* **兩端產生與解析都最簡單。** 產生是 `sprintf("%02X", b)` 跑迴圈;
  加了分隔符就要對「第一個 byte 要不要加」做條件判斷,解析端也要跳過並驗證分隔符
  的位置正確——每個指令都多這點摩擦。
* 長度上限只有 16 字元,可讀性的痛不會累積到失控。

**代價要記著:bring-up 時看 log 要自己數 byte 邊界。** 這是刻意付的,
文件裡的範例應該幫忙分好組(如上面每行只放 1 個 byte),而不是改 wire format。

`id` 用十進位、payload 用十六進位看起來不一致,但這是刻意的:`id` 是 ODM 在自己
文件裡編號用的,十進位比較好講;payload 是 bytes,十六進位才對得上 byte 邊界。

### 對稱:兩端都能下、兩端都會收

`AT+VENDOR_CMD` 兩端都收、`+EVENT: VENDOR_CMD:` 兩端都吐。**透傳沒有方向性**,
模組不知道也不需要知道哪一端是指令發起者——這是它跟其他所有 AT 指令的差別。

程式上的體現:`at_cmd_core` 的 vendor 部分沒有任何 `if (role == ...)`,
兩個 app 各自呼叫同一組 `at_cmd_core_vendor_tx_pop()` / `at_cmd_core_notify_vendor_received()`。

---

## 7. rc08 施工順序

每一步可獨立驗證,且後一步依賴前一步。**四步都已完成**,兩個 u535 preset
(`...-slave-std-nocodec-ldo`、`...-master-std-nocodec-ldo-hs`)都 build 過,實機未驗。

| # | 工作 | 為什麼是這個順序 | 狀態 |
|---|---|---|---|
| 0 | 修 `wireless_read_data()` 的 payload 洩漏(§6.5),兩個 app 都要 | 步驟 3 會改變 struct 大小。不先修,出貨期間任何一次 HS/DG 版本錯配都不是「新欄位讀不到」,而是**整條 data 連線永久死掉**。唯一一個「不修就會讓後面每一步都變危險」的項目 | 已完成,改成截斷 + 一定 `receive_complete()` |
| 1 | `user_data_t` 抽到共用 header;號碼表統一成 enum(§5),消滅裸數字;處理 `dongle.c:439` 的 `vol_cb` 錯位 | 兩件事都是「讓後面兩步只需要改一個地方」。號碼表:DG→HS 一旦接通,兩套編碼就會**第一次同時上線**,症狀是「收到了但變成別的指令」——兩邊各自看都對,極難查 | 已完成,新增 `puretone_link_data.h` + `at_cmd_code_t` |
| 2 | 接通 DG→HS:DG 註冊 `at_cmd_core_register_cmd_tx_cb()`、`data_callback()` 打包、HS 的 RX handler 解碼 | 做完這步,DG 那四個現在等於假的媒體指令**第一次真的能用**。這本身就是 rc08 值得出的理由。步驟 3 是這條通路上的乘客,不是它的替代品 | 已完成,含解掉 echo 迴圈(見下) |
| 3 | 加 vendor pass-through:`vendor_*` 欄位、TX ring buffer、`AT+VENDOR_CMD` 與 `+EVENT: VENDOR_CMD:`(§6.5) | 唯一需要 TX queue 和 seq 去重的,複雜度最高;而且一旦交付 ODM 格式就凍結,前面每一步都還是我方內部可改的 | 已完成 |

### 步驟 2 施工時發現的 echo 迴圈

HS 原本把 `at_cmd_core_register_play_cb()` 這組**硬體** callback 拿來當「轉發給 DG」用
(§2)。而 `at_cmd_core_notify_play_received()` 呼叫的正是同一組 callback——
所以 DG→HS 一接通,DG 送來的 PLAY 會觸發 HS 的 `at_play()`,把 PLAY 再排回去給 DG,
**兩端以 10 ms 為週期無限來回同一個媒體鍵**。

解法是讓 HS 改註冊 `at_cmd_core_register_cmd_tx_cb()`(即 core 原本就設計好的轉發鉤子),
硬體 callback 保持 NULL。線上行為完全不變,但收到指令時只會吐 `+EVENT` 給本地 SOC。
**這是「§7 步驟 1 先統一、再接通」那條紀律真正救到的東西**——
如果先接通再整理,這個迴圈會在實機上以「媒體鍵按一次就鎖死」的形式出現。

`AT+VOL` 兩端都保持「調自己的音量」,兩邊的 `at_cmd_tx()` 都明確濾掉 `AT_CMD_VOL`。
理由見 §5 末段與 `at_cmd_core_register_vol_cb()` 的註解:DG 一直是這個行為,
ODM 已經對接了,所以錯的是註解不是行為。

**施工紀律:改動盡量壓進 `at_cmd_core.c`,app 檔案只留最薄的接線。**
理由見 §10——這決定了之後移植到 unidir 時有多少東西是白拿的。
實際落點:core 拿到 enum、vendor queue、`AT+VENDOR_CMD` 與事件;app 各自只多了
「打包時呼叫 `user_data_pack_vendor()`、收包時呼叫 `user_data_deliver_vendor()`」兩行。

## 8. 新增的 ODM 介面

| AT 指令 | 下在哪 | 模組端動作 |
|---|---|---|
| `AT+VENDOR_CMD=<id 1-223>[,"<hex>"[,"ACK"]]` | 兩端 | 排入 vendor TX queue。預設連送 3 包;加 `"ACK"` 則每包重傳到被確認或逾時。錯誤碼見 §6.5 |
| `AT+LE_UWB_*`(原 `AT+UWB_*`) | 兩端 | **改名**,見 §11 |
| `AT+PLAY` / `STOP` / `NEXT_TRACK` / `PRE_TRACK` | 兩端 | **DG 端從「回 OK 但什麼都沒做」變成真的送出**,HS 端行為不變 |

| 事件 | 吐在哪 | 何時 |
|---|---|---|
| `+EVENT: VENDOR_CMD:<id>[,"<hex>"]` | 兩端 | 收到 `vendor_id != 0` 且 `(vendor_id, vendor_seq)` 與上一次不同 |
| `+EVENT: VENDOR_CMD_ACK:<id>` | 兩端 | 帶 `"ACK"` 的指令被對端確認 |
| `+EVENT: VENDOR_CMD_FAIL:<id>` | 兩端 | 帶 `"ACK"` 的指令約 1 秒內未被確認 |
| `+EVENT: LE_UWB_*`(原 `UWB_*`) | 兩端 | **改名**,見 §11 |
| `+EVENT: PLAY` / `STOP` / `NEXT_TRACK` / `PRE_TRACK` | 兩端 | **HS 端是新增的**(以前只有 DG 會吐) |

跌倒偵測不在這張表裡:它由 ODM 自己配一個 `vendor_id`(§6),我方不定義。

### 給 ODM 的兩條使用注意事項

1. **狀態類的指令要定期重送**,不要只在變化時送一次。vendor 通道是事件語意,
   重送 3 包只把遺失機率壓到 p³,不會自我修復。詳見 §6.1。
2. **payload 裡的 `0` 要保留給「不知道」**,不要當成一個合法值(§6.2)。

## 9. 未決事項

1. ~~vendor 指令要不要 ACK。~~ **已決定並實作**:兩種模式,由第三個參數逐指令選(§6.5)。
   `vendor_ack` 欄位每包回帶最後收到的 seq,發送端逾時後吐 `VENDOR_CMD_FAIL`。
   第三個參數是**引號關鍵字** `"ACK"` / `"NOACK"`,不是數字旗標:一個裸的結尾 `1`
   讀者無從得知是什麼,跟不加引號的 payload 是同一個毛病往後挪一個參數。
   代價是 payload 從 8 bytes 降到 6。

2. **重送次數 3 是猜的。** 依據是「單包遺失率 p 的三次方」,但這條 data 連線在
   fallback 深度時的實際 p 沒有量過。若現場出現 vendor 指令偶爾不到,
   先量 p 再決定調重送次數還是加 ACK,不要直接加大到 5、10——
   那只是把沒量測的問題往後推。

3. ~~`AT_VENDOR_PAYLOAD_MAX` 是否夠。~~ **已定為 8**(§6.5),
   ODM 回覆「6 基本夠用,多一點更好」,而多給到 16-byte 封包的上限不必付代價。
   注意它**沒有平滑放寬路徑**:舊版收到較長 payload 會 clamp,只防越界不防誤解。

3a. ~~vendor id 號碼空間全給了 ODM。~~ **已保留 `224`–`255`**(§6.5),
   收發兩端都擋。

3b. **`AT_VENDOR_ACK_TIMEOUT_PACKETS = 100`(約 1 秒)是猜的。** 依據是「撐過短暫
   RF 掉包,但要在人失去耐性之前給答案」,不是量出來的。若現場出現「明明連著卻
   VENDOR_CMD_FAIL」,先量斷線的時間分布再調。

3c. ~~`AUDIO_CMD_OK` / `AUDIO_CMD_ERROR`~~ **已決定不做,PRD 那張表已刪除**(2026-08-17)。

   三個理由:
   1. **模組回答不了它問的問題。** `AUDIO_CMD_OK:PLAY` 宣稱「執行成功」,
      但模組只知道封包到了。播放器有沒有真的暫停只有 DG 的 SOC 知道——
      而它往主機送 HID/AVRCP 本來就是 fire-and-forget,多半也不知道。
      做出來的 `OK` 實際語意是「handler 有回傳」,幾乎不含資訊卻長得像保證。
   2. **傳輸層的那個問題已經有通用答案。** 「到了沒」正是 `"ACK"` 在做的事。
      再蓋一套媒體專用的,就是同一個問題有兩個強度不同的答案,
      而媒體那套會是比較弱的——它自己是個會掉的事件,沒有自己的 ACK。
   3. **ODM 現在就能自己做,零模組改版。** 收到 `+EVENT: PLAY` → 執行 →
      回 `AT+VENDOR_CMD=<自訂id>,"<結果>"`。「成功」的定義該由懂它的人來定。

   **但它指到一個真的缺口,分開記著:** 媒體鍵走 `cmd_type`,是 edge-triggered、
   不重送、無 ACK(§6.5 開頭)。link 剛好斷的瞬間下 `AT+PLAY`,它會安靜消失。
   解法不是 `AUDIO_CMD_OK`,而是 host 自己看 `AT+LE_UWB_CONN_STATUS?`——
   把媒體鍵搬到 vendor 通道走不了,`cmd_type` 1–4 已凍結(§5)。
   這條已寫進 PRD 取代該表的那段說明。

4. **要不要 `AT+VENDOR_CMD?`(查最後收到的 vendor 指令)。** 目前沒做。
   ODM 若只吃 `+EVENT` 不做輪詢就不需要;要做的話語意要先想清楚——
   「最後收到的」在一個會掉包的通道上是個容易誤用的概念。

5. ~~實機驗證還沒做。~~ **已在 u535 實機上驗過,全部通過**(2026-08-19,
   `...-slave-std-nocodec-ldo` 一對板子)。最小驗證組(兩個方向、單發不重複、
   ACK、關機逾時 FAIL、`AT+PLAY` 不回彈)、`script/vendor_cmd_test.py` 的計數
   壓測與 soak、保留 id 與 payload 上限的邊界、以及關機/reset 前的 UART flush,
   都做過。

   **這一輪抓到兩個 bug,都不在 vendor 通道本身,但都是它把問題逼出來的**——
   記在這裡是因為兩個都會被下一個人重新踩到:

   ### 5a. `+EVENT` 不能在 wireless RX callback 裡用阻塞 UART 寫出去

   症狀是**一秒一條 vendor 指令就足以讓音樂破音**,而且改速率沒有差別、
   ACK 與 best-effort 也沒有差別。後者其實是線索不是巧合:去重讓兩種模式
   都剛好吐一個 `+EVENT`,所以付的是同一筆錢——**成本是每事件的,不是負載的**。

   `facade_expansion_uart_write()` 當時是 `quasar_uart_transmit_blocking()`,
   一行 vendor 事件在 115200 下要 spin 約 **2.3 ms**。而 `conn_rx_data_success_callback()`
   是由 PendSV 派送的,優先權 **12**(`QUASAR_DEF_PRIO_PENDSV_IRQ`),而 audio
   process timer 是 **13 / 14**、SWC data timer 是 **15** ——**全部在它下面**。

   改成 IRQ 驅動(`quasar_uart_transmit_string_irq()`,推進 4096 byte 的軟體
   FIFO 就返回)。代價是「寫了」不再等於「送出去了」,所以三個「印完就不回來」
   的點必須先 flush:`facade_system_reset()` 前、`facade_uwb_shutdown()` 前、
   以及 `at_cmd_core_notify_standby()` 結尾——那個函式的註解本來就白紙黑字
   寫著它依賴阻塞行為。

   **規則:AT 通道的任何輸出都不准在 callback context 裡等待。**

   ### 5b. coordinator 不能用 SWC connect status 判斷「對面在不在」

   拔掉 HS,DG 永遠停在 `2 (CONNECTED)`,不吐 `LE_UWB_DISCONNECTED`;
   因此 HS 回來時也沒有轉變可報,`LE_UWB_CONNECTED` 一樣不會出現。
   HS 抓 DG 消失卻完全正常——**失效是單向的**。

   根因在 `update_connect_status_main()`:

   ```c
   synced = (node_role == NETWORK_NODE) ? link_tdma_sync_is_slave_synced(..) : true;
   ```

   **coordinator 永遠拿到 `synced == true`。** node 是靠掉 sync 立刻判定斷線的;
   coordinator 沒有 sync 可以掉,只剩「累積 frame outcome」那條路,而那條路在
   這個情境下沒有觸發。中途試過只問 `rx_data_conn`(不問 `tx_data_conn`,因為
   coordinator 是 timebase master,對面在不在它都照發,`wps_mac.c` 甚至會把
   沒有 ACK 的 TX 連線直接釘成 CONNECTED)——**還是不夠**。

   最後改成**收包看門狗**:HS 的 `data_callback()` 每 10 ms 無條件送一包,
   所以收到包本身就是心跳。DG 的 RX callback 蓋時間戳,`link_is_up()` 只回答
   「最近 200 ms 內有沒有聽到」。AT 層的 400 ms 下降緣去抖疊在上面,
   總共約 600 ms 報出斷線;上升緣不去抖。

   同一個 bug 還有另一半:`try_boot_reconnect()` 用的是同一個 `link_is_up()`,
   所以 DG 的開機自動回連可以在 **HS 根本沒開機**的情況下宣告成功。看門狗那個
   「開機後還沒聽到過」的旗標就是為了堵這個。

   **規則:要給 host 的連線事件,量「有沒有聽到對面」,不要問 wireless core
   的連線狀態——尤其在 coordinator 這一側。**

## 10. 與 w240 / unidirectional 的關係

rc08 走 rc07 / SDK v2.3.1 base,`puretone_headset` app。w240 走 SDK v2.4.0,
主力是 `puretone_unidirectional` app。兩者會並行一段時間,合併點在
w240 的 u535 bring-up 驗證完成之後。

合併成本已經確認過:

* `at_cmd_core.c` 與 `library/at_module` 在 v240-dev 上是 **0 個 commit**,
  所以 §7 的 core 產出(`at_cmd_code_t`、vendor queue、`AT+VENDOR_CMD`)**完全乾淨地 merge**。
* app 側要動的 `user_data_t`、`data_callback()`、`conn_rx_data_success_callback()`
  三處,v240-dev 的大改動(dongle 539 行、headset 740 行)**一行都沒掃到**——
  它動的是 swc/sac init、fallback、log。
* 新增的 `puretone_link_data.h` 是全新檔案,不會有衝突。

**但要記住:`puretone_unidirectional` 目前 `at_cmd_core` 呼叫數是 0**
(`puretone_headset` 是 66),backend 也沒有 expansion UART facade。
所以 unidir 的 AT 是**從零移植**,不是「加兩個指令」。那是一筆獨立的工作,
排在 w240 收斂之後——屆時 §7 的 core 產出是白拿的,app 那三處接線要重打。

vendor 通道在這件事上的價值特別明顯:unidir 那邊要接的**只有兩行**
(`user_data_pack_vendor()` / `user_data_deliver_vendor()`),
而且之後 ODM 再加幾個指令,unidir 都不必再動。

這也是 §7「改動盡量壓進 core」那條紀律的全部理由。

---

## 11. AT 介面改名:`UWB_*` → `LE_UWB_*`,以及與 PRD 的對齊

PRD(`document/SPARK_Puretone_HS_Gen 2_PRD_v1.3_draft.docx`)與韌體長期不一致,
2026-08-17 一次對齊完畢。**沒有保留舊名別名**——一次到位。

### 改韌體去配合 PRD 的部分

| 舊(韌體) | 新 |
|---|---|
| `AT+UWB_CONN_STATUS?` / `PAIR` / `CONNECT` / `DISCONNECT` / `SHUTDOWN` / `GET_ROLE?` | `AT+LE_UWB_*` |
| `+EVENT: UWB_*`(10 個) | `+EVENT: LE_UWB_*` |

PRD 用的是連字號 `LE-UWB_`,而且自己也混用(`AT+LEUWB_CONN_STATUS?` 沒連字號、
`AT+LE-UWB_PAIR` 有)。**統一成底線 `LE_UWB_`**:連字號在 AT 指令名裡不合慣例
(慣例是英數 + 底線),而且既然兩邊都要改,就一次改成對的。

### 改 PRD 去配合韌體的部分

| PRD 舊 | 新 |
|---|---|
| `AT+BAT?` → `+BAT: 85%` | `AT+BATTERY?` → `+BATTERY: 85` |

`%` 一併拿掉:回應值應該是純數字,單位寫在文件裡,不要讓 host 去剝字尾。

### 兩個回應格式順手統一

```
+LE_UWB_CONN_STATUS: <n> (<NAME>)     n=0..3, NAME=STANDBY|PAIRING|CONNECTED|CONNECTING
+LE_UWB_GET_ROLE: <n> (<NAME>)        n=0..1, NAME=NODE/HS|COORDINATOR/DG
```

PRD 原本的 `+GET_ROLE: 0` 是唯一一個回應不帶前綴的,看起來是漏改;
`+LE-UWB_CONN_STATUS: 1` 則只有數字沒有名稱。

數字在固定位置,host 讀 `: ` 之後的整數即可,括號可以完全忽略。
名稱是給人看的——而且**名稱與數字來自同一個 `switch`**,不是查表也不是註解,
所以不會發生「數字改了名字沒改」。角色同時印 `NODE/HS` 是刻意的:
這顆韌體以外的一切(電路圖、preset、release 包、PRD)講 HS/DG,只有 wireless
stack 講 node/coordinator,而這兩組正是最容易講反的。

### PRD 補上的東西

韌體有、PRD 漏掉的 6 個事件:`BUILD:`、`LE_UWB_PAIRING`、`LE_UWB_PAIR_FAIL`、
`LE_UWB_UNPAIRED`、`LE_UWB_STANDBY`、`LE_UWB_QUALITY:WEAK|GOOD`,
以及 `AT+VENDOR_CMD` 與它的三個事件。

`BUILD:` 特別重要:客戶板不一定接得到 ST-Link,那條線是唯一能確認
「燒對 binary 沒有」以及「模組是不是一直在重開」的證據。

### 這次改名沒有版本錯配風險

與 §5 的號碼表不同,這些名字**從來沒有一致過**——PRD 寫 `LE-UWB_`,韌體寫 `UWB_`。
照 PRD 寫的 host 現在就是壞的(指令會掉到 fallback 吐 HELP),
所以不存在「正在正常運作、改了會壞」的整合。§5 那條原則在這裡不適用,
因為沒有任何一邊是「在跑的事實」。
