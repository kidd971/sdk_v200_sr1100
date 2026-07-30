# UWB Disconnect / Connect 決策規格 (BT SoC ↔ UWB MCU)

> 對象:BT SoC(master)韌體 與 STM32U5+UWB module(slave)韌體 兩邊對齊。
> 目的:定義「什麼條件下 SoC 該下 disconnect / connect / shutdown」,以及每個指令在 module 端**實際**做什麼。
> 對應 code:`app/common/at_cmd_core/at_cmd_core.c`、`app/example/puretone_headset/puretone_headset.c`、
> `puretone_dongle.c`;背景見 `boot_auto_reconnect_design.md`。

---

## 0. 一句話定位

在這個「BT SoC 當 master、UWB module 當受控音訊 slave」的架構下,**UWB module 對耳機是第二條音訊路,不是主控**。
因此「UWB disconnect」的真實語意 **不是斷 link,而是把整個 MCU+UWB 島關掉省電**。
純粹「斷 link 但 MCU 繼續醒著」在本產品沒有價值,已刻意不實作。

---

## 1. 指令實際行為(module 端,以 code 為準)

| AT 指令 | module 端實際動作 | 是否返回 | 電源狀態 | 重連方式 |
|---|---|---|---|---|
| `AT+UWB_DISCONNECT` | `at_start_disconnect()` → 設狀態 STANDBY → `facade_enter_standby()` | **不返回** | 整島斷電(radio+MCU standby) | 只能靠 reset |
| `AT+UWB_SHUTDOWN` | 目前**等同 DISCONNECT**(`at_start_shutdown()` 直接呼叫 `at_start_disconnect()`) | **不返回** | 同上 | 只能靠 reset |
| `AT+UWB_CONNECT` | connect callback → `facade_system_reset()` → 冷開機進 boot auto-reconnect | 重開 | 開機 | 自動重連上次配對 addr(timeout 10s;**HS 逾時不再進配對**,見 §3/§6) |
| `AT+MODULE_RESET` | `facade_system_reset()` | 重開 | 開機 | 同上(等於 CONNECT 的效果) |
| `AT+UWB_PAIR` | pair callback(進配對流程) | 是 | 開機 | 建立**新**配對 |
| `AT+UWB_CONN_STATUS?` | 回 `+UWB_CONN_STATUS: N` | 是 | — | 查詢用 |
| `AT+CONN_LM?` | 回 `+CONN_LM: NdB` link margin | 是 | — | 查詢用 |

**狀態碼**(`AT+UWB_CONN_STATUS?` 回傳):
`0=Standby` / `1=Pairing` / `2=Connected` / `3=Connecting`(下 CONNECT 後的嘗試視窗)。
（`1=Pairing` 現在**只**由 `AT+UWB_PAIR`、pairing 按鍵、或首次開機無配對記錄進入;reconnect 逾時**不再**自動轉 Pairing。）

**非同步事件**(module 主動吐給 SoC):
`+EVENT: UWB_CONNECTED` / `+EVENT: UWB_DISCONNECTED` / `+EVENT: UWB_CONNECT_FAIL`。

---

## 2. 決策表:SoC 該在什麼條件下下什麼指令

| # | 觸發條件(SoC 端判斷) | SoC 應下指令 | 結果 | 主要理由 |
|---|---|---|---|---|
| 1 | UWB 是當前音訊路且正在播 | (不動作) | 維持 CONNECTED | — |
| 2 | 切到 BT / BT 成為音訊源 | `AT+UWB_DISCONNECT` | 整島斷電 | **省電**:UWB 閒著純耗電 |
| 3 | 兩條路都無音訊 idle 超過 T 秒 | `AT+UWB_DISCONNECT` | 整島斷電 | **省電** |
| 4 | 使用者要(切回)UWB 音訊 | `AT+UWB_CONNECT` | reset→auto-reconnect | 重建上次 link,~3s |
| 5 | 首次配對 / 綁新的源 | `AT+UWB_PAIR` | 進配對 | 建立新 addr |
| 6 | 換源 / 忘記舊配對 | 清 NV + `AT+UWB_PAIR`(見 §5) | 新配對 | 舊 addr 需先清 |
| 7 | 疑似 link wedge(見 §4) | `AT+UWB_DISCONNECT` 再 `AT+UWB_CONNECT` | 電源循環復原 | 溫柔 disconnect 救不回 wedge |
| 8 | 耳機整機關機 / MCU rail 即將斷 | `AT+UWB_SHUTDOWN` | 整島斷電 | 明確表達「要關機了」 |

> 註:#2、#3、#8 在 module 端**目前行為相同**(都進 Standby)。用不同指令名只是為了讓 SoC 端 log 與意圖清楚,
> 未來若要讓 SHUTDOWN 真的做更深的動作(見 §6)也不必改 SoC。

---

## 3. 典型時序

**省電關島 → 重連**
```
SoC: AT+UWB_DISCONNECT
MCU: OK
MCU: +EVENT: UWB_DISCONNECTED        (狀態轉 Standby 後)
     ...(整島斷電,SoC 端此時 CONN_STATUS? 會逾時或 module 無回應)...
SoC: AT+UWB_CONNECT
MCU: OK
MCU: (reset → boot auto-reconnect)
MCU: +EVENT: UWB_CONNECTED           (連上,~10s 內)
  或 +EVENT: UWB_CONNECT_FAIL        (逾時。HS:停在 idle/Standby,不進配對,由 SoC 重下 AT+UWB_CONNECT 重試;DG:目前仍 fall through 進配對)
```

> **HS reconnect 逾時重試(SoC 端注意)**:CONNECT_FAIL 事件約在 5s(`AT_UWB_CONNECT_TIMEOUT_MS`)先送出,但 boot poll 到 10s(`RECONNECT_TIMEOUT_MS`)才真正結束。在 boot 視窗內重下 `AT+UWB_CONNECT` 會被 module 端當 no-op 忽略(防 reset 風暴),所以 **SoC 的重試間隔應 ≥ ~10-12s**。
> 此行為目前**只在 HS(node)實作**;DG(coordinator)仍是舊行為(逾時進配對),之後再處理。

---

## 4. Wedge 盲區(SoC 必讀)

`AT+UWB_CONN_STATUS?` 回 `2=Connected` **不等於 link 真的健康**。
已知 scheduler 有 silent wedge(TIM4 re-arm 時序 race),wedge 時狀態機凍結會**持續回報 Connected**。
SoC 不可只信 CONN_STATUS,要交叉檢查:

- **有沒有真的在收音訊**(SoC 端最可靠的訊號);
- `AT+CONN_LM?` link margin 是否合理;
- 疑似 wedge 就走決策表 #7:`DISCONNECT` → `CONNECT` 電源循環,不要期待原地恢復。

**Standby vs 當機的區分**(給 SoC 判斷 module 到底是睡了還是掛了):
真的進 Standby,`+CRASH_DUMP:` 週期輸出會**停**;若還在吐 crash dump 或有異常 log,是當機不是睡著。
(HS 在 `LATCH_TEST_HOOKS` 下 `USER_3` 是 soft reset,外觀像「睡了又醒」,別誤判。)

---

## 5. 換源 / 忘記配對

boot auto-reconnect 會把上次配對的 4-byte addr persist 在 flash 保留頁,reset 後自動重連。
要綁**不同的**源,必須先清掉這筆記錄,否則會一直重連舊的:

- NV 清除:`reconnect_store` 的 erase(底層 `facade_nv_erase()`)。
- 目前尚無專用「forget」AT 指令 —— **待確認**是否要新增 `AT+UWB_FORGET`,或由 `AT+UWB_PAIR` 內部先清再配。
  (見 §7 open item。)

---

## 6. 現況與缺口(open items)

1. **DISCONNECT 與 SHUTDOWN 語意塌掉**:兩者在 headset 端行為相同,SHUTDOWN 在 core 額外的
   `facade_uwb_shutdown()`(拉 radio shutdown pin)是**死碼**(standby 不返回,到不了那行)。
   功能上 radio 由 standby 關掉沒問題,但若日後要讓 SHUTDOWN = 更深的斷電(整島掉電、非 standby),
   需要在 standby 之前先拉 pin,或重排順序。
2. **無 forget 指令**:換源需人工清 NV(§5)。
3. **Standby 喚醒源**:目前只有 NRST(SoC 線 / reset 鍵)。u535 產品板 pairing 按鍵是 PA4=WKUP2,
   未來可 arm 成 wake source,不影響本規格。
4. **Wedge 偵測要 SoC 配合**:module 端無法百分百自知 wedge(§4),需 SoC 用音訊有無做 watchdog。

---

## 7. 待兩邊確認

- [ ] idle 省電門檻 T 秒由誰計時、多長?(建議 SoC 端計,因為只有 SoC 知道兩條路的音訊狀態)
- [ ] 是否需要 `AT+UWB_FORGET`(換源清 NV)?
- [ ] SoC 的 wedge watchdog 條件(多久沒音訊 + margin 低於多少 → 觸發 #7)?
- [ ] SHUTDOWN 是否要做成比 DISCONNECT 更深的斷電?還是永久當 alias?
