# puretone_unidirectional v2.4.1_rc2 —— 測試說明

指令和事件列在最前面 —— 那是 ODM 整合時最常翻的兩節。
音訊、fallback、回連在第 4 節之後。

單向音訊：**DG（coordinator）送，HS（node）收**。24 kHz stereo、5 階 fallback、ISI 1。

檔名 `dg-` 是送端，`hs-` 是收端。**燒錯角色的症狀是「完全沒聲音」**，
跟其他故障長得一樣 —— 開機那行會告訴你燒的是哪一包。
哪塊板、驗到什麼程度，看同目錄的 `MANIFEST.txt`。

---

## 1. AT 指令

以下是**程式碼實際註冊的全部 22 個**。與 PRD 有出入時以這裡為準。

| 指令 | 回應 |
|---|---|
| `AT+PING` | `OK` |
| `AT+HELP` | `+HELP:` 後接指令清單 |
| `AT+VER` | `+VER: SPARK SDK SR1100 v2.4.1_rc2` |
| `AT+FW_VERSION?` | `+FW_VERSION: v2.4.1_rc2` |
| `AT+MODULE_INFO?` | `+MODULE_INFO: HW=<n>,FW=v2.4.1_rc2,Chip=<n>,SN=<16 hex>,Addr=0x<xx>` |
| `AT+MODULE_RESET` | `OK`，然後 MCU reset |
| `AT+LE_UWB_CONN_STATUS?` | `+LE_UWB_CONN_STATUS: <n> (<NAME>)`，`0=STANDBY 1=PAIRING 2=CONNECTED 3=CONNECTING` |
| `AT+LE_UWB_GET_ROLE?` | `+LE_UWB_GET_ROLE: <n> (<NAME>)`，`0=COORDINATOR 1=NODE` |
| `AT+LE_UWB_PAIR` | `OK`，立刻進配對 |
| `AT+LE_UWB_CONNECT` | `OK`，重建已儲存的鏈路（實作是 MCU reset）|
| `AT+LE_UWB_DISCONNECT` | `OK`，拆鏈路、**保留**配對紀錄 |
| `AT+LE_UWB_SHUTDOWN` | `OK`，拆鏈路並關掉 radio 電源；MCU 繼續回應 AT |
| `AT+VENDOR_CMD=<id>[,"<hex>"[,"ACK"]]` | `OK`；`id` 1–223，payload 最多 **8 bytes** |
| `AT+VOL=<0-100>` / `AT+VOL?` | `OK` / `+VOL: <n>`。**只有 HS 有輸出**，DG 不轉發 |
| `AT+PLAY` / `AT+STOP` / `AT+NEXT_TRACK` / `AT+PRE_TRACK` | `OK`，見 §7 的重要注意 |
| `AT+BATTERY?` | `+BATTERY: <0-100>` |
| `AT+CONN_LM?` | `+CONN_LM: <n>dB` 或 `+CONN_LM: N/A` |
| `AT+CONN_QUALITY?` | `+CONN_QUALITY: GOOD` / `WEAK` / `CRITICAL` / `N/A`，見 §2 |
| `AT+I2S_MUX` | `+I2S_MUX: <sel>`，板內 codec 選擇 |

**`AT+CONN_LM?` 兩端意義不同**（PRD 未收錄，依決定不收）：
HS 直接量自己收到的音訊連線；**DG 是發送端，回的是 HS 最後回報的值**。
兩者的 `0` 都代表「沒有量測」（未配對，或 HS 還沒回報過），**不是 0 dB**。

---

## 2. 事件（`+EVENT:`）

主動送出，不需要查詢。以下是**程式碼實際會送的全部**，格式照抄。

### 開機

```
+EVENT: BUILD: v2.4.1_rc2 role=<COORDINATOR|NODE> <date> <time>
+EVENT: LE_UWB_READY
```

### 配對

```
+EVENT: LE_UWB_PAIRING       進入配對
+EVENT: LE_UWB_PAIRED        配對成功
+EVENT: LE_UWB_PAIR_FAIL     逾時 / app code 錯 / 中止，三者共用這一個
+EVENT: LE_UWB_UNPAIRED      配對紀錄已擦除
```

**`LE_UWB_UNPAIRED` 只會由按鍵觸發**，因為只有按鍵會擦掉紀錄（見 §5）。
下完 AT 指令不要等這個事件。

### 連線

```
+EVENT: LE_UWB_CONNECTED
+EVENT: LE_UWB_DISCONNECTED
+EVENT: LE_UWB_CONNECT_FAIL
+EVENT: LE_UWB_STANDBY
```

### 鏈路品質

```
+EVENT: LE_UWB_QUALITY:GOOD
+EVENT: LE_UWB_QUALITY:WEAK
+EVENT: LE_UWB_QUALITY:CRITICAL
```

**判準是 fallback 階數，不是 link margin**（v2.4.1_rc2 起；舊版用 5/10 dB 門檻，
在 0.5 m 也會報 WEAK，因為它比較的是原始碼值不是 dB）。對應關係：

| 階 | 音訊格式 | 事件 | SoC 該做什麼 |
|---|---|---|---|
| 0–1 | 96k/48k 24-bit | `GOOD` | 維持 UWB |
| 2 | 48k 16-bit | `WEAK` | 藍芽**必須已經可以出聲** |
| 3–4 | ADPCM | `CRITICAL` | 切藍芽 |

**HS 另有保底**：只要輸出真的斷了一次（consumer underflow，每次 30 ms 靜音），
不管在第幾階都直接送 `CRITICAL`。DG 沒有這一條 —— 斷音發生在 HS 的喇叭上，
DG 量不到，所以 **DG 永遠不會送 `CRITICAL`**，它的事件只代表階梯餘裕。

**時序**：只在狀態改變時送，不重送。變壞立刻送；變好要**連續 5 秒**而且**一次只回一階**
（`CRITICAL`→`WEAK`→`GOOD` 最少 10 秒）。CONNECTED 後 2 秒內不評估也不送，
那段時間階數還是上一次斷線前的值。

**重要**：從 `WEAK` 到 `CRITICAL` 最快只有約 200 ms（階梯每 10 Hz 取樣才降一階）。
藍芽不能等收到 `WEAK` 才開始暖機，來不及；必須常態保持隨時可出聲。
理由與實測見 `MD/uwb_quality_indicator_decision_spec.md`。

### Vendor pass-through

```
+EVENT: VENDOR_CMD:<id>,"<hex>"     收到對面的 vendor 指令
+EVENT: VENDOR_CMD_ACK:<id>         對面確認收到（只有下 "ACK" 才會有）
+EVENT: VENDOR_CMD_FAIL:<id>        重送用完仍未被確認
```

### 媒體鍵（由對面轉來）

```
+EVENT: PLAY
+EVENT: STOP
+EVENT: NEXT_TRACK
+EVENT: PRE_TRACK
+EVENT: VOL=<n>
```

---

## 3. 序列埠

每塊板有 **AT** 和**除錯 console** 兩條，落點依板子而定。全部 **115200 8N1**。

| 板子 | AT 指令 | 除錯 console |
|---|---|---|
| **u535 LDO** | **UART4 — PC10 (TX) / PC11 (RX)**，即 ST-Link VCP，插 USB 就有 | LPUART1 — PA3，只發 |
| **u535 SMPS** | **LPUART1 — PA3 (TX) / PA2 (RX)** | **同一條**，見下 |
| **u5a5** | **USART2 — PA2 (TX) / PA3 (RX)**，**expansion 排針**，要外接轉接板 | **USB CDC** — 插著供電的那條線就有 |

u5a5 的 ST-Link VCP 是另一個 UART，**上面什麼都沒有** —— 那是這塊板最先會走錯的地方。

**u5a5 的 console 自 `36847a5` 起走 USB CDC**，不再與 AT 共用 USART2。在那之前預設是
共用的，所以**比這個 commit 早的 binary（含已出貨的 v240 套件）console 仍在 expansion
排針上**，插 USB 是看不到東西的。`-DCONSOLE_ON_CDC=0` 可以編回舊行為。
走線的完整對照見 [console_at_uart_routing.md](console_at_uart_routing.md)。

**SMPS 上 AT 和統計共用一條線。** 那不是疏漏 —— 那塊板只有一個可用的埠，
而把統計靜音會讓板子完全沒有診斷輸出。兩者走**同一個傳送佇列**，
所以**交錯只會發生在整行之間**：

```
[DG] v241_rc2 12340 fb=1 48kHz 24-bit  tx=600/s idle=0/s ...
+EVENT: LE_UWB_CONNECTED
[DG] v241_rc2 13340 fb=1 ...
```

不該出現 `[DG] v241_rc2 1234 fb=+EVENT: LE_` 這種字元中間被切斷的情況。**看到就回報。**

開機第一行（送到 console；SMPS 上就是 AT 那一條，u5a5 上是 USB CDC）：

```
[BOOT] puretone_unidirectional coordinator u535 r1 v2.4.1_rc2 <date> <time>
```

**這行是接電之後幾毫秒就送出的**，terminal 開得晚就會錯過。想看它就先開 terminal 再上電。

---

## ⚠️ rc01 不能和之後的版本混在同一對板子上

線上格式從 2 bytes 變成 5 bytes（帶 vendor 指令時最多 14）。這個改動本身是安全的，
**但 rc01 的接收路徑有一個 bug**：收到比自己長的封包時會漏掉一個接收緩衝區，
每 10 ms 一個，佇列塞滿之後 data connection 永久死掉。
那個修正從 rc02 起才有，**到不了已經交出去的 rc01 binary**。

**兩端要燒同一版。** 混用的症狀是「配對成功、link margin 不再更新、階梯亂跳」，
看起來很像 RF 問題。

---

## 4. 配對與回連

| 情況 | LED |
|---|---|
| 進入配對 | 慢閃 ×2（250 ms） |
| 配對成功 | RGB 恆亮（u535 藍、其他綠） |
| 開機回連中 | **快閃 ×5**（100 ms） |
| 回連逾時 | 單次慢閃（300 ms） |
| audio 走起來 | LED_USER_4 跟著對面的按鍵狀態 |

**第一次要配對**：兩邊都按配對鍵，或兩邊都下 `AT+LE_UWB_PAIR`。
之後**開機會自動回連**，配對結果存在 flash 裡。

回連逾時**不會**自動重新配對。DG 是時間基準，它的無線核心會繼續跑，
HS 什麼時候開機都能同步上 —— 所以「HS 還沒開」不該讓 DG 把已經好的配對丟掉。

---

## 5. 要重新配對 —— 三條路

**配對紀錄存在 flash，重開機不會清掉。這是刻意的**，也是自動回連能運作的原因。
所以「重開機再配對一次」**沒有用** —— 它會直接回連到原本那台。

| 方式 | 效果 |
|---|---|
| **按配對鍵**（配對狀態下按一下） | **擦掉紀錄**，下次開機回到配對模式。發 `+EVENT: LE_UWB_UNPAIRED` |
| **`AT+LE_UWB_PAIR`** | 立刻進配對。舊紀錄保留到新配對成功才被覆寫。**沒有配對鍵的板子用這個** |
| **flash 全擦除**（chip erase） | 回到出廠狀態 |

**全擦除要用 chip erase，不能只擦 application 區**：紀錄放在 flash 最後一頁，
不在程式碼那一段裡，只重燒 firmware 不會動到它。

**換對板子測之前務必先擦掉。** 兩台各自記得舊夥伴的板子湊在一起，
症狀是「配對成功但沒聲音」或「一直在回連」，而那看起來很像 RF 問題。

---

## 6. 一行 log 怎麼讀

```
[HS] v241_rc2 55493 fb=4 24kHz ADPCM   rx=600/s rej=0/s miss=3106/s fill=16% lm=170
[DG] v241_rc2 55493 fb=4 24kHz ADPCM   tx=600/s idle=1187/s cca_fail=0/s tx_drop=0/s
```

`55493` 是開機毫秒數。`fb=` 是目前階數：

| fb | 模式 |
|---|---|
| 1 | 48 kHz 24-bit ← **開機從這裡開始** |
| 2 | 48 kHz 16-bit |
| 3 | 48 kHz ADPCM |
| 4 | 24 kHz ADPCM stereo ← 最底階 |

要看的欄位：**HS 的 `rx=`** 和 **DG 的 `tx=`**。兩個接近就是鏈路正常。

`miss=` 在 fb=3 或 4 會偏高，因為 DG 有很多空的 timeslot 也被算成 miss
（DG 的 `idle=` 就是那些）。**只在 `idle=0` 的階數（fb 1 和 2）拿 miss 來算到達率**，
其他階只能拿來比較相對值。

雙 radio 的 HS 會在同一行尾端多帶 `tim4:` 的排程器狀態，單 radio 沒有。

---

## 7. 測 audio 時最需要注意的三件事

### 7.1 停在 24 kHz 是預期行為，不是故障

階梯一旦**真的因為鏈路變差**從 fb=3 掉到 fb=4，就會**釘在那裡不再往上爬**。會印：

```
[FB] stepped down to the bottom rung; pinned
```

這是刻意的：已經掉到底的鏈路通常會爬上去、失敗、再掉下來，
而那個來回震盪比一直待在 24 kHz 更難聽。

**「聽起來一直是 24 kHz」不要當成 bug 回報**，除非它是在鏈路明明很好的時候發生的。

### 7.2 重開機不會把階梯拖下去

**任一邊單獨重開機，另一邊的階數應該不動。**
如果看到「HS 重開之後 DG 就掉到 fb=4」，那是 bug，請回報。

### 7.3 遮擋測試要看的是「有沒有斷音」

不是看 fb 掉到幾階 —— **掉階正是它該做的事**。
掉階但聲音連續 = 正常運作；聲音斷掉才是問題。

回報時請一起附上**斷音當下前後幾行 log**，`fb=`、`rx=`/`tx=`、`lm=` 都要。

---

## 8. 已知限制

| 項目 | 狀況 |
|---|---|
| **96 kHz** | **關閉**。這條線上 96 kHz 會斷音，原因未明。不要嘗試打開 |
| **媒體鍵沒有執行確認** | `AT+PLAY` / `STOP` / `NEXT_TRACK` / `PRE_TRACK` 回 `OK` **只代表「已排入下一個封包」**，不代表對面收到或執行了。它們是 edge 觸發、無序號、無重送，所以沒有東西可以確認。PRD 已明確說明模組不提供這層確認。**要可靠的東西請走 `AT+VENDOR_CMD` 的 ACK 模式** |
| **解除配對** | 沒有 AT 指令會擦掉紀錄。見 §5 |
| **u5a5 的 console** | **預設走 USB CDC**（`36847a5` 起），插著供電的 USB 就看得到。更早的 binary 是跟 AT 共用 USART2 expansion 排針。`-DCONSOLE_ON_CDC=0` 可編回去。見 [console_at_uart_routing.md](console_at_uart_routing.md) |

各板子還有什麼驗過、什麼沒驗過，看 `MANIFEST.txt`。

---

## 9. 回報什麼

1. 板子和**開機那行**
2. 單 radio 還是雙 radio
3. 症狀：**斷音** / 沒聲音 / 配對不上 / 回連不上 / AT 沒回應
4. 斷音前後幾行 log，以及當下的 AT 往來
5. 距離、有沒有遮擋（人體、金屬）
6. 這對板子測之前**有沒有做過 flash 全擦除**
