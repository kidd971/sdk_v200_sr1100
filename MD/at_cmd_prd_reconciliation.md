# AT 指令：PRD ↔ 程式碼 對照

> 比對日期 2026-08-24，程式碼基準 `v240-dev`（rc02 準備中）。
> 程式碼端：`app/common/at_cmd_core/at_cmd_core.c` 的 `at_server_register()` 與 `+EVENT` 字串。

---

## 0. ⚠️ 先確認你看的是哪一份 PRD

樹上有**三份同名檔案**，內容不同：

| 路徑 | 大小 | 時間 | 狀態 |
|---|---|---|---|
| **`D:\_SDK_v230_UART_at_command_git\sdk_v200_sr1100\document\…v1.3_draft.docx`** | 833295 | **2026-08-20** | ✅ **最新，這份才算數** |
| `…\document\…v1.3_draft.bak.docx` | 830476 | 2026-08-17 | 改版前的備份 |
| `D:\_Puretone_Gen2\…v1.3_draft.docx` | 830476 | 2026-08-17 | **過期**，內容等同上面那份備份 |

**這已經害人做過一次白工。** 2026-08-24 對照時先讀到 `_Puretone_Gen2` 那份，
據以把 `AT+LE_UWB_CONN_STATUS` 改成 `AT+LEUWB_CONN_STATUS`，
而最新的 PRD 早就寫成底線、跟原本的程式碼一致 —— 改動方向整個相反，已全數退回（未進版控）。

**檔名一樣、資料夾不一樣、時間差三天，看起來完全無法分辨。**
下次比對前先看 `document/` 那份的時間戳。

---

## 1. 結論：指令與事件已經對齊

Aug 20 那版 PRD 已經修掉先前所有落差：

* `AT+LE_UWB_*` **全部統一為底線**（之前有 `LE-UWB_` 和 `LEUWB_` 兩種寫法）
* `AT+BATTERY?` 跟程式碼一致（之前寫 `AT+BAT?`）
* **`AT+VENDOR_CMD=<id 1-223>[,"<hex>"[,"ACK"]]` 已補上**
* `AUDIO_CMD_OK` / `AUDIO_CMD_ERROR` 那段承諾**已改寫**成
  「Application-level command confirmation is not provided by the module」——
  那才是實際行為，而且避開了一個需要改線上格式才能實現的功能

**事件完全一致**：18 個對 19 個，程式碼的 `+EVENT: VENDOR_CMD_%s` 展開後正是
文件列的 `VENDOR_CMD_ACK` / `VENDOR_CMD_FAIL`。

---

## 2. 唯一剩下的落差：三個程式碼有、文件沒寫的指令

| 指令 | 回應 | 建議 |
|---|---|---|
| `AT+CONN_LM?` | `+CONN_LM: <n>dB` 或 `+CONN_LM: N/A` | **補進文件**。ODM 做 RF bring-up 會需要，而且 unidir 兩個角色的來源不同（見下） |
| `AT+VER` | `+VER: SPARK SDK SR1100 <ver>` | 可不寫。它跟 `AT+FW_VERSION?` 現在回同一個字串，重複記載反而多一個會走鐘的地方 |
| `AT+I2S_MUX` | `+I2S_MUX: <sel>` | 可不寫。板子內部的 codec 選擇，不是 ODM 介面 |

`AT+CONN_LM?` 在 unidir 上**兩端的意義不同**，補文件時要寫清楚：

* **HS（node）** 直接量自己收到的音訊連線 —— 它是接收端
* **DG（coord）** 沒有東西可以量，回的是 **node 最後一次回報的值**
* 兩者的 `0` 都代表**沒有量測**（未配對，或 node 還沒回報過），不是「0 dB」

---

## 3. 已經在程式碼修掉的

`+MODULE_INFO` 的 `FW=` 欄位原本寫死 `v2.3.0`，跟 `AT+FW_VERSION?` 回的版本不一致 ——
而且**看起來比較像正式答案的那個是過期的那個**。
已改用 `FW_VERSION_STRING`，兩者現在都回 `v2.4.0_rc01`（`e9bd9bf`）。
