# Radio stall wedge —— 待修，先記錄

> 狀態：**已知、未修、刻意不修**。2026-08-19 又觀測到一次，這份記錄的目的是把
> 這次新得到的資訊固定下來，以免下次從頭查。
> 相關：`dualradio_resync_HQ_report.md`（含 `_brief`）、`dualradio_HQ_update_digest.md`、
> `dualradio_fb0_park_HQ_followup.md` —— 現象本身已報給 SPARK HQ。

---

## 0. 一句話

放著跑一段時間（本次 t ≈ 6.8×10⁶ ms，約 113 分鐘）之後 radio 停住，
`swc_disconnect()` + `swc_connect()` 的自動復原**打不開**，
而使用者去按配對鍵會 trap 成紅燈。

## 1. 觀測到的樣子

```
[HS v2.4.0-rc2 t=6802888] Connected    fb=1 coord_lm=90 rx=0/s miss=0/s rej=0/s swc=STOP

+AUTO-RECOVER: radio stall -> swc reconnect
[HS v2.4.0-rc2 t=6803888] Connected    fb=1 coord_lm=90 rx=0/s miss=0/s rej=0/s swc=STOP
   ...（每 6 秒重複一次，狀態完全不變）...

SWC TRAP puretone_headset.c:2680 code=-27
```

最後那一行是按下配對鍵造成的。

## 2. 這次新知道的三件事

### 2.1 錯誤碼是 `SWC_ERR_INTERNAL`，不是 `SWC_ERR_DISCONNECT_TIMEOUT`

`code=-27` = **`SWC_ERR_INTERNAL`**。`SWC_ERR_DISCONNECT_TIMEOUT` 是 **-29**。

`puretone_headset.c` 裡好幾處註解寫著「tolerate SWC_ERR_DISCONNECT_TIMEOUT」
（`stall_auto_recover()`、`app_teardown()`、以及 DG 那邊對應的地方）。
那些地方**行為沒問題**，因為它們是無條件忽略錯誤；但**註解指錯了錯誤碼**。
從凍住的狀態呼叫 `swc_disconnect()`，回來的是 INTERNAL。

改註解的時候順手改，但不要為了改註解單獨動這個檔案。

### 2.2 band-aid 證實無效 —— 復原矩陣最軟的那一格是 NO

`LATCH_TEST_HOOKS` 的註解把問題定義成一個矩陣：SR1100 的 latch 是
「普通 reboot 就會清」、「要明確把 radio 斷電才會清」、還是「要真的拔板子電源」。

這次的 log 把**最軟的那一格填上了**：

| 復原手段 | 結果 |
|---|---|
| `swc_disconnect()` + `swc_connect()`（`stall_auto_recover()` 做的事） | ❌ **無效**，連開四次以上，`swc=STOP` / `rx=0/s` 完全不動 |
| `dbg_radio_por()`（拉兩顆 radio 的 shutdown 腳 + MCU reset） | 未測 ← **下一格** |
| 拔板子電源 | 未測 |

所以 `stall_auto_recover()` 這個 band-aid **對這一種卡法沒有作用**。
它註解裡自己就寫了「proper fix (dual-radio aggressive re-sync) belongs in SPARK's
SWC core — report to HQ」，這次的 log 是那句話的證據。

### 2.3 狀態行在卡住時會說謊

`Connected`、`coord_lm=90` 都是**凍住的舊值**，跟 `rx=0/s` 同時出現。
看 log 判斷「有沒有卡住」要看 `swc=STOP` 和 `rx=0/s`，**不要看前面那兩欄**。

## 3. 卡住時要按哪顆鍵

**按「音量−」（`dbg_radio_por`），不要按配對鍵。**

配對鍵會走 `unpair_device()` → `swc_disconnect()` → `ASSERT_SWC_STATUS` → trap。
這件事 [puretone_headset.c:110](app/example/puretone_headset/puretone_headset.c#L110)
的註解早就寫了，`LATCH_TEST_HOOKS` 把兩顆音量鍵改成「卡住時可以按、不會 trap」的
復原測試，就是為了這個。

trap 點有兩個，都在配對鍵那條路上：
[2507](app/example/puretone_headset/puretone_headset.c#L2507)（`enter_pairing_mode`）、
[2680](app/example/puretone_headset/puretone_headset.c#L2680)（`unpair_device`）。

## 4. 那兩個 assert 是刻意留著的 —— 不要拿掉

曾經提議把它們改成不 assert（DG 的 `app_teardown()` 就是那樣做的），**決定不改**。

理由：**assert 響，代表這裡有一個還沒修的問題。** 拿掉之後裝置會從「紅燈死機」
變成「安靜地半殘」——看起來像修好了，實際上只是把同一個卡住藏起來，
而且藏得剛好足以帶著出貨。在真正的修法出現之前，吵是對的行為。

等 §5 的根因修掉之後，這兩個 assert 該不該留是另一個問題，屆時再談。

## 5. 還沒回答的問題

1. **這份 log 是哪一個 preset 的 binary？** `stall_auto_recover()` 只有在
   `facade_get_radio_hw_counters()` 回 true 時才會動，而它的註解說 single-radio
   板子沒有那些計數器。既然 AUTO-RECOVER 有印出來，推測是**雙 radio** build——
   但沒有確認。
2. **是不是 rc08 merge（`6904422`）之後才有的？** 傾向不是：`STALL_AUTO_RECOVER`
   與 `LATCH_TEST_HOOKS` 都早於那次 merge，而且是為了同一個現象而生的。
   但沒有實際比對過 merge 前後。

這兩題在真的要修的時候要先答，因為第 2 題如果答案是「merge 之後才有」，
優先級完全不同。

## 6. 修的時候從哪裡開始

1. 先答 §5 的兩題
2. 卡住時按「音量−」，把復原矩陣的第二格填掉 —— 這決定 latch 是 radio 層還是 MCU 層
3. 根因在 SPARK 的 SWC core（dual-radio re-sync），四份 HQ 文件已經在那邊
