# AT user command 雙向通路決策規格 (DG ↔ HS over UWB)

> 對象:rc08 施工者,以及 ODM 端對接 AT 介面的 MCU 韌體。
> 目的:記錄「為什麼 user command 目前只有 HS→DG」、「DG→HS 要怎麼補」、
> 以及跌倒偵測 `AT+FACE_UP` / `AT+FACE_DOWN` 為什麼**不**走 `cmd_type`。
> 對應 code:`app/common/at_cmd_core/at_cmd_core.c`、
> `app/example/puretone_headset/puretone_dongle.c`、`puretone_headset.c`。
> 行號以本分支(`rc08-at-cmd`,base = rc07 / SDK v2.3.1)為準。
> 相關:`uwb_disconnect_decision_spec.md`(連線類指令)、`soc_reset_wake_decision_spec.md`。

---

## 0. 一句話定位

user command 的雙向通路**兩個方向都設計過**,但只有 HS→DG 被接上,
而且**接的方式跟設計不一樣**。DG→HS 那半邊在 core 裡是完整的,只是從來沒有人啟用它。

跌倒偵測是第一個真正需要 DG→HS 的功能,所以它同時是「補通路」和「第一個使用者」。

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

## 6. 決策 2:`FACE_UP` / `FACE_DOWN` 走獨立狀態欄位,**不**走 `cmd_type`

`cmd_type` 是 edge-triggered 的一次性欄位:送出後立刻清 0
(`puretone_headset.c:2342-2343`),而這條 data 通路**沒有 ACK、沒有重送、沒有序號**。

把狀態塞進一次性欄位,失敗模式是:

* `FACE_DOWN` 掉一包 → 耳機端**永遠不知道有人跌倒**。
* `FACE_UP`(解除)掉一包 → 耳機端**永久卡在告警狀態**,
  因為下一次狀態轉換要等到下一次跌倒。

**單次遺失造成永久錯誤狀態,在告警功能上不可接受。**

而 `FACE_UP` / `FACE_DOWN` 命名的本來就是**狀態**(朝上/朝下),不是事件。
既然是狀態,就每包都送——這條線本來就 10 ms 一包(`DATA_TX_PERIOD_MS`):

```c
typedef struct user_data {
    bool    button_state;
    uint8_t link_margin;
    uint8_t cmd_type;      /* edge-triggered,維持現狀 */
    uint8_t battery_pct;
    uint8_t face_state;    /* 新增:每包重送當前朝向 */
} user_data_t;
```

```c
typedef enum {
    AT_FACE_UNKNOWN = 0,
    AT_FACE_UP      = 1,
    AT_FACE_DOWN    = 2,   /* 告警 */
} at_face_state_t;
```

這樣掉包**自我修復**——下一個 10 ms 就補上了。不需要 ACK、不需要重送邏輯、
不需要序號。空間夠:`MAX_DATA_PAYLOAD_SIZE` 是 16,struct 現在 4 bytes。

### 為什麼 `0` 必須是 UNKNOWN 而不是 UP

`user_data_t` 在兩端都是 `= {0}` 零初始化,而且沒有資料/未連線時讀到的就是 0。
若 `0 = UP`,一個空的或未收到的封包會被讀成「朝上 = 正常」——**把「不知道」
誤報成「安全」**。告警系統不能有這種預設。

### 對 ODM 的介面仍然是事件

HS 端只在 `face_state` **值改變時**才往 UART 吐 `+EVENT`,不是 100 Hz 洗 UART。
所以 ODM 看到的仍然是事件語意,AT 指令名稱維持 `AT+FACE_UP` / `AT+FACE_DOWN`。

DG 端的語意是「DG 的 SOC 告訴模組現在朝向是什麼」,模組存起來每包送——
與命名一致,不是一次性觸發。

---

## 7. rc08 施工順序

每一步可獨立驗證,且後一步依賴前一步。

| # | 工作 | 為什麼是這個順序 |
|---|---|---|
| 1 | 號碼表統一成 enum(§5),消滅裸數字;順手處理 `dongle.c:439` 的 `vol_cb` 錯位 | DG→HS 一旦接通,兩套編碼就會**第一次同時上線**。沒先統一的話,`FACE` 會是第一個踩到的人,而症狀是「收到了但變成別的指令」——兩邊各自看都對,極難查 |
| 2 | 接通 DG→HS:DG 註冊 `at_cmd_core_register_cmd_tx_cb()`、`data_callback()`(`dongle.c:2232`)打包、HS 的 RX handler(`headset.c:992`)解碼 | 做完這步,DG 那五個現在等於假的媒體指令**第一次真的能用**。這本身就是 rc08 值得出的理由 |
| 3 | 加 `face_state` 欄位與 `AT+FACE_UP` / `AT+FACE_DOWN`(§6) | 此時只是在已經通的路上多一個欄位 |

**施工紀律:改動盡量壓進 `at_cmd_core.c`,app 檔案只留最薄的接線。**
理由見 §9——這決定了之後移植到 unidir 時有多少東西是白拿的。

## 8. 新增的 ODM 介面

| AT 指令 | 下在哪 | 模組端動作 |
|---|---|---|
| `AT+FACE_UP` | DG | 設 `face_state = 1`,之後每包送給 HS |
| `AT+FACE_DOWN` | DG | 設 `face_state = 2`(告警),之後每包送給 HS |
| `AT+FACE?` | 兩端 | 回 `+FACE: 0\|1\|2`(DG 回自己設的,HS 回收到的) |

| 事件 | 吐在哪 | 何時 |
|---|---|---|
| `+EVENT: FACE_DOWN` | HS | 收到的 `face_state` 由非 2 變成 2 |
| `+EVENT: FACE_UP` | HS | 收到的 `face_state` 由 2 變成 1 |

`AT+FACE?` 是否需要,以及 `UNKNOWN` 相關的轉換要不要吐事件,見 §9。

## 9. 未決事項

1. **`UNKNOWN` 的轉換要不要吐事件?**
   建議:**不吐**。只有 `UP ↔ DOWN` 之間的轉換才吐。
   `UNKNOWN → UP` 不是「告警解除」,把它吐成 `FACE_UP` 會讓 HS 的 SOC
   在每次連線建立時收到一個假的解除訊號。斷線後應回到 `UNKNOWN`,
   而不是假設任何一種朝向。

2. **HS 端要不要對 `FACE_DOWN` 做本地動作**(提示音、震動),
   還是純粹轉給 SOC 由它決定?本規格假設後者。

3. **`AT+FACE?` 是否需要。** 若 ODM 只吃事件不做輪詢,可以不做。

## 10. 與 w240 / unidirectional 的關係

rc08 走 rc07 / SDK v2.3.1 base,`puretone_headset` app。w240 走 SDK v2.4.0,
主力是 `puretone_unidirectional` app。兩者會並行一段時間,合併點在
w240 的 u535 bring-up 驗證完成之後。

合併成本已經確認過:

* `at_cmd_core.c` 與 `library/at_module` 在 v240-dev 上是 **0 個 commit**,
  所以 §7 步驟 1 的產出**完全乾淨地 merge**。
* app 側要動的 `user_data_t`、`data_callback()`、`conn_rx_data_success_callback()`
  三處,v240-dev 的大改動(dongle 539 行、headset 740 行)**一行都沒掃到**——
  它動的是 swc/sac init、fallback、log。

**但要記住:`puretone_unidirectional` 目前 `at_cmd_core` 呼叫數是 0**
(`puretone_headset` 是 66),backend 也沒有 expansion UART facade。
所以 unidir 的 AT 是**從零移植**,不是「加兩個指令」。那是一筆獨立的工作,
排在 w240 收斂之後——屆時 §7 的 core 產出是白拿的,app 那三處接線要重打。

這也是 §7「改動盡量壓進 core」那條紀律的全部理由。
