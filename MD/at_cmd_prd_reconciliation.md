# AT 指令：PRD v1.3 draft ↔ 程式碼 對照

> 比對日期 2026-08-24，程式碼基準 `v240-dev`（rc02 準備中）。
> 文件端：`D:\_Puretone_Gen2\SPARK_Puretone_HS_Gen 2_PRD_v1.3_draft.docx`
> 程式碼端：`app/common/at_cmd_core/at_cmd_core.c` 的 `at_server_register()` 清單。
>
> **這份是給文件用的修改清單**，不是規格。有衝突時以能編出來、跑得動的程式碼為準，
> 除非下面特別標示「要改程式碼」。

---

## 0. 一句話

PRD 有 **14** 個指令、程式碼有 **21** 個。10 個對得上，
**6 個是拼法不同的同一個指令**，1 個是名稱衝突，**11 個程式碼有而 PRD 沒寫**——
其中包含 **`AT+VENDOR_CMD`，也就是 ODM 整合真正會用到的那一個**。

---

## 1. 拼法：PRD 自己就有三種寫法

同一個前綴，PRD 裡出現三種形式，而程式碼只認一種：**`LE_UWB_`**（底線）。

| PRD 寫的 | 程式碼實際接受的 | 出現處 |
|---|---|---|
| `AT+LEUWB_CONN_STATUS?` | `AT+LE_UWB_CONN_STATUS?` | 完全沒有分隔符 |
| `AT+LE-UWB_PAIR` | `AT+LE_UWB_PAIR` | 連字號 |
| `AT+LE-UWB_CONNECT` | `AT+LE_UWB_CONNECT` | 連字號 |
| `AT+LE-UWB_DISCONNECT` | `AT+LE_UWB_DISCONNECT` | 連字號 |
| `AT+LE-UWB_SHUTDOWN` | `AT+LE_UWB_SHUTDOWN` | 連字號 |
| `AT+LE-UWB_GET_ROLE?` | `AT+LE_UWB_GET_ROLE?` | 連字號 |

事件也一樣：

| PRD 寫的 | 程式碼實際送的 |
|---|---|
| `+EVENT: LE-UWB_READY` | `+EVENT: LE_UWB_READY` |
| `+EVENT: LE-UWB_PAIRED` | `+EVENT: LE_UWB_PAIRED` |
| `+EVENT: LE-UWB_CONNECTED` | `+EVENT: LE_UWB_CONNECTED` |
| `+EVENT: LE-UWB_CONNECT_FAIL` | `+EVENT: LE_UWB_CONNECT_FAIL` |
| `+EVENT: LE-UWB_DISCONNECTED` | `+EVENT: LE_UWB_DISCONNECTED` |

**這不是小事。** ODM 照文件打 `AT+LE-UWB_PAIR` 會得到 `ERROR`，
而 `LE-UWB` 在內文敘述裡當產品名稱寫是合理的 —— **只有指令字串不能這樣寫**。
建議文件把指令一律用等寬字體並統一成 `LE_UWB_`，內文名稱要叫 LE-UWB 沒關係。

---

## 2. 名稱衝突：電量

| PRD | 程式碼 |
|---|---|
| `AT+BAT?` → `+BAT: 85%` | `AT+BATTERY` → `+BATTERY: <0-100>` |

**這一項要決定改哪邊。** 程式碼的 `BATTERY` 已經在 rc08／headset 那條線上註冊並使用，
改名會動到已經在跑的東西；改文件則是一行的事。
**建議改文件**，除非 ODM 已經照 `AT+BAT?` 寫好了。

另外 PRD 的回應帶 `%` 符號（`+BAT: 85%`），程式碼只回數字（`+BATTERY: 85`）。
**這個要改文件** —— 回應裡放單位符號會讓解析變麻煩，而且沒有第二個單位可能。

---

## 3. 程式碼有、PRD 沒寫的指令

| 指令 | 回應 | 重要性 |
|---|---|---|
| **`AT+VENDOR_CMD=<id>,"<hex>"`** | `+EVENT: VENDOR_CMD:<id>` 等 | **最高，見 §4** |
| `AT+VER` | `+VER: SPARK SDK SR1100 <ver>` | 中 |
| `AT+CONN_LM?` | `+CONN_LM: <n>dB` 或 `+CONN_LM: N/A` | 中，量測用 |
| `AT+I2S_MUX` | `+I2S_MUX: <sel>` | 低，板子相關 |

事件同樣沒寫的：

`LE_UWB_PAIRING`、`LE_UWB_PAIR_FAIL`、`LE_UWB_UNPAIRED`、`LE_UWB_STANDBY`、
`LE_UWB_QUALITY:GOOD` / `WEAK`、`BUILD: <ver> <role> <date> <time>`。

其中 **`LE_UWB_PAIRING` / `PAIR_FAIL` / `UNPAIRED` 是配對流程的完整狀態機**，
PRD 目前只寫了 `PAIRED`（成功那一個），
所以照文件寫的主機**不知道配對失敗長什麼樣**，只會看到超時。

---

## 4. `AT+VENDOR_CMD` —— PRD 完全沒有，但這是 ODM 整合的主線

vendor pass-through 是給 ODM 自己的 SOC 用的通道：
模組不解讀內容，只負責把它送到對面並回報。**PRD 裡一個字都沒有。**

需要補的內容：

* 指令格式、`id` 的範圍與保留區間（vendor id 224–255 保留給模組）
* payload 長度上限（`AT_VENDOR_PAYLOAD_MAX`）
* **ACK 模式**與 `+EVENT: VENDOR_CMD_ACK:<seq>` / `VENDOR_CMD_FAIL:<seq>`
* 去重規則（seq）與「送一次會不會到」的保證強度
* 線上格式的欄位在 `app/common/link_data/puretone_link_data.h`，
  那份 header 是**這條通道的契約**，文件應該指向它而不是重寫一遍

---

## 5. PRD 有、程式碼沒有：`AUDIO_CMD_OK` / `AUDIO_CMD_ERROR`

PRD 寫了每個媒體鍵都會回：

```
+EVENT: AUDIO_CMD_OK:[cmd]
+EVENT: AUDIO_CMD_ERROR:[cmd],[error_code]
```

**程式碼沒有實作這一對。** 現況是媒體鍵送出去就沒有下文 ——
`AT+PLAY` 回 `OK` 只代表「指令收到並排入下一個封包」，
不代表對面收到，也不代表對面做了。

這是**真正的功能落差**，不是拼字問題。要決定：

* **要做** → 需要對面回報執行結果，也就是資料封包要多一個回覆欄位。
  注意 `cmd_type` 是 edge 觸發、無重送、無序號（見 `puretone_link_data.h`），
  所以「沒有 OK」和「命令掉了」分不出來 —— 要做就得連可靠性一起做。
* **不做** → 文件要拿掉，否則 ODM 會等一個永遠不來的事件。

**rc02 建議：文件先拿掉，記成之後的項目。** 它需要的是線上格式改動，
而 rc02 已經改過一次格式（2 → 5 bytes），不該在同一版再改第二次。

---

## 6. 已經在程式碼修掉的

`+MODULE_INFO` 的 `FW=` 欄位原本寫死 `v2.3.0`，
跟 `AT+FW_VERSION?` 回的版本不一致 —— 而且**看起來比較像正式答案的那個是過期的那個**。
已改為使用 `FW_VERSION_STRING`，現在兩者都回 `v2.4.0_rc01`。

---

## 7. 對得上、不用動的

`AT+PING`、`AT+HELP`、`AT+FW_VERSION?`、`AT+MODULE_INFO?`、`AT+MODULE_RESET`、
`AT+VOL=<0-100>`、`AT+VOL?`、`AT+PLAY`、`AT+STOP`、`AT+NEXT_TRACK`、`AT+PRE_TRACK`。

`+EVENT: PLAY` / `STOP` / `NEXT_TRACK` / `PRE_TRACK` / `VOL=<n>` 也一致。
