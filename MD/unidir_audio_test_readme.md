# puretone_unidirectional v2.4.0 rc02 —— 測試說明

單向音訊：**DG（coordinator）送，HS（node）收**。24 kHz stereo、5 階 fallback、ISI 1。
rc02 比 rc01 多了 **AT 指令層**。

**這一包裡的 binary 是哪塊板、哪個角色、驗到什麼程度，看同目錄的 `MANIFEST.txt`。**

檔名 `dg-` 是送端，`hs-` 是收端。**燒錯角色的症狀是「完全沒聲音」**，
跟其他故障長得一樣 —— 開機那行會告訴你燒的是哪一包，先看那行再懷疑別的。

---

## ⚠️ rc01 和 rc02 不能混在同一對板子上

線上格式從 2 bytes 變成 5 bytes（帶 vendor 指令時最多 14）。這個改動本身是安全的，
**但 rc01 的接收路徑有一個 bug**：收到比自己長的封包時會漏掉一個接收緩衝區，
每 10 ms 一個，佇列塞滿之後 data connection 永久死掉。

那個修正在 rc02，**到不了已經交出去的 rc01 binary**。

**兩端都要燒 rc02。** 混用的症狀是「配對成功、link margin 不再更新、階梯亂跳」，
看起來很像 RF 問題。

---

## 1. 兩條序列埠

每塊板有 **AT 指令**和**除錯 console** 兩條，落點依板子而定。全部 **115200 8N1**。

| 板子 | AT 指令 | 除錯 console |
|---|---|---|
| **u535 LDO** | **UART4 — PC10 (TX) / PC11 (RX)**，即 ST-Link VCP，插 USB 就有 | LPUART1 — PA3，只發 |
| **u535 SMPS** | **LPUART1 — PA3 (TX) / PA2 (RX)** | **同一條**，見下 |
| **u5a5** | **USART2 — PA2 (TX) / PA3 (RX)**，expansion 排針，要外接轉接板 | **同一條**，見下 |

**SMPS 和 u5a5 上兩者共用一條線。** 那不是疏漏 —— 那兩塊板各自只有一個可用的埠，
而把統計靜音會讓板子完全沒有診斷輸出。兩者走同一個傳送佇列，
所以**交錯只會發生在整行之間**：

```
[DG] v240_rc02 12340 fb=1 48kHz 24-bit  tx=600/s idle=0/s ...
+EVENT: LE_UWB_CONNECTED
[DG] v240_rc02 13340 fb=1 ...
```

不該出現 `[DG] v240_+EVENT: LE_` 這種字元中間被切斷的情況。**看到就回報。**

開機第一行：

```
[BOOT] puretone_unidirectional coordinator u535 r1 v2.4.0_rc02 Aug 24 2026 11:12:09
```

---

## 2. AT 指令

完整規格看 PRD。這裡只列測試會用到的，以及**文件沒寫但存在**的。

```
AT+PING                      連通性
AT+FW_VERSION?               韌體版本
AT+MODULE_INFO?              硬體／序號／位址
AT+LE_UWB_CONN_STATUS?       0=STANDBY 1=PAIRING 2=CONNECTED 3=CONNECTING
AT+LE_UWB_PAIR               開始配對
AT+LE_UWB_CONNECT            重建已儲存的鏈路（實作方式是 reset）
AT+LE_UWB_DISCONNECT         拆鏈路，保留配對紀錄
AT+LE_UWB_SHUTDOWN           拆鏈路並關掉 radio 電源
AT+LE_UWB_GET_ROLE?          0=coordinator 1=node
AT+VENDOR_CMD=<id 1-223>[,"<hex>"[,"ACK"]]    vendor pass-through
AT+VOL=[0-100] / AT+VOL?     只有 HS 有輸出，DG 不轉發
AT+BATTERY?
```

**PRD 沒寫但可用的**：`AT+CONN_LM?`（link margin）。
HS 直接量自己收到的音訊連線；**DG 是發送端，回的是 HS 最後回報的值**。
兩者的 `0` 都代表「沒有量測」（未配對，或 HS 還沒回報過），**不是 0 dB**。

事件（`+EVENT:`）跟 PRD 一致，包含 `LE_UWB_PAIRING` / `PAIRED` / `PAIR_FAIL` /
`UNPAIRED` / `READY` / `CONNECTED` / `DISCONNECTED` / `CONNECT_FAIL` /
`QUALITY:GOOD|WEAK` / `VENDOR_CMD*` / `BUILD:`。

**`+EVENT: LE_UWB_UNPAIRED` 只會由按鍵觸發**，因為只有按鍵會擦掉紀錄（見 §3）。
不要等 AT 指令產生它。

---

## 3. 配對與回連

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

### 要重新配對 —— rc02 有三條路

**配對紀錄存在 flash，重開機不會清掉。這是刻意的**，也是自動回連能運作的原因。
所以「重開機再配對一次」**沒有用** —— 它會直接回連到原本那台。

| 方式 | 效果 |
|---|---|
| **按配對鍵**（配對狀態下按一下） | **擦掉紀錄**，下次開機回到配對模式。發 `+EVENT: LE_UWB_UNPAIRED` |
| **`AT+LE_UWB_PAIR`** ← rc02 新增 | 立刻進配對。舊紀錄保留到新配對成功才被覆寫。**沒有配對鍵的板子用這個** |
| **flash 全擦除**（chip erase） | 回到出廠狀態 |

**全擦除要用 chip erase，不能只擦 application 區**：紀錄放在 flash 最後一頁，
不在程式碼那一段裡，只重燒 firmware 不會動到它。

**換對板子測之前務必先擦掉。** 兩台各自記得舊夥伴的板子湊在一起，
症狀是「配對成功但沒聲音」或「一直在回連」，而那看起來很像 RF 問題。

---

## 4. 一行 log 怎麼讀

```
[HS] v240_rc02 55493 fb=4 24kHz ADPCM   rx=600/s rej=0/s miss=3106/s fill=16% lm=170
[DG] v240_rc02 55493 fb=4 24kHz ADPCM   tx=600/s idle=1187/s cca_fail=0/s tx_drop=0/s
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

## 5. 測 audio 時最需要注意的三件事

### 5.1 停在 24 kHz 是預期行為，不是故障

階梯一旦**真的因為鏈路變差**從 fb=3 掉到 fb=4，就會**釘在那裡不再往上爬**。會印：

```
[FB] stepped down to the bottom rung; pinned
```

這是刻意的：已經掉到底的鏈路通常會爬上去、失敗、再掉下來，
而那個來回震盪比一直待在 24 kHz 更難聽。

**「聽起來一直是 24 kHz」不要當成 bug 回報**，除非它是在鏈路明明很好的時候發生的。

### 5.2 重開機不會把階梯拖下去

**任一邊單獨重開機，另一邊的階數應該不動。**
如果看到「HS 重開之後 DG 就掉到 fb=4」，那是 bug，請回報。

### 5.3 遮擋測試要看的是「有沒有斷音」

不是看 fb 掉到幾階 —— **掉階正是它該做的事**。
掉階但聲音連續 = 正常運作；聲音斷掉才是問題。

回報時請一起附上**斷音當下前後幾行 log**，`fb=`、`rx=`/`tx=`、`lm=` 都要。

---

## 6. 已知限制

| 項目 | 狀況 |
|---|---|
| **96 kHz** | **關閉**。這條線上 96 kHz 會斷音，原因未明。不要嘗試打開 |
| **`AUDIO_CMD_OK` / `AUDIO_CMD_ERROR`** | **未實作**。`AT+PLAY` 等回 `OK` 只代表「已排入下一個封包」，不代表對面收到或執行了。PRD 已改為明確說明模組不提供這層確認 |
| **解除配對** | 沒有 AT 指令會擦掉紀錄。見 §3 |
| **u5a5 console** | 跟 AT 共用 USART2。u5a5 沒有 USB CDC 輸出 |

各板子還有什麼驗過、什麼沒驗過，看 `MANIFEST.txt`。

---

## 7. 回報什麼

1. 板子和**開機那行**
2. 單 radio 還是雙 radio
3. 症狀：**斷音** / 沒聲音 / 配對不上 / 回連不上 / AT 沒回應
4. 斷音前後幾行 log，以及當下的 AT 往來
5. 距離、有沒有遮擋（人體、金屬）
6. 這對板子測之前**有沒有做過 flash 全擦除**
