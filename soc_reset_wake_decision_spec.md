# SoC Reset / Wake 決策規格 (BT SoC 端)

> 對象:**耳機端 BT SoC(master)韌體**。定義「什麼條件下 SoC 該拉 NRST 喚醒 / reset UWB module,
> 什麼時候不要」。與 module 端行為對齊,module 端規格見 `uwb_disconnect_decision_spec.md`。
> 背景見 `boot_auto_reconnect_design.md`。對應 module commits:HS reconnect 逾時進 Standby = `69a8d36`。

0 定位	NRST 只用來喚醒/復原,不用來睡;範圍=耳機端 SoC↔UWB MCU,dongle 不在內
1 狀態模型	SoC 眼中 module 五態:ASLEEP/WAKING/SEARCHING/CONNECTED/SUSPECT_HUNG
2 狀態來源	AT 事件 + 查詢 + SoC 記帳 + 靜默訊號 +(建議)STATUS GPIO
3 何時拉 NRST	決策表 5 條(要用而睡著、醒著未連、重試、喚醒失敗、卡死)
4 何時不要	已連/醒著、正在 waking/searching、我叫它睡的、自己重開又活
5 重試迴圈	核心:HS 睡了不會自醒 → SoC 週期 NRST 重試;含一輪時間帳 + backoff 狀態機 + 「別在 boot 視窗內 NRST」
6 意圖	預設 reconnect;要配對先 NRST 叫醒再送 AT+UWB_PAIR
7 卡死	IWDG 主、SoC ping→NRST 第二層(僅該醒著時)
8 時序圖	冷喚醒 / 開機時間差重試 / 卡死復原 三個
9 時間常數	RECONNECT_TIMEOUT_MS=10s、CONNECT_FAIL≈5s、一輪≈開機+10s
10 待確認	STATUS GPIO、backoff 數字、觸發源、終止條件、NRST 脈寬、未來 WKUP 腳

---

## 0. 一句話定位與範圍

- 架構:**BT SoC = master、STM32U5+UWB = 受控的第二音訊路 slave**。按鍵接在 **BT SoC**,不接 STM32。
- SoC 對 UWB module 有兩條控制路:
  1. **AT UART**(雙向指令/事件)—— **module 在 Standby 時 UART 是死的**。
  2. **NRST 線**(硬體 reset)—— v2 板:`AB1577M GPIO6_RST → R5 1K → STM32 NRST`,**任何電源態(含 Standby)都能把 module 拉回來**。
- **核心原則:NRST 只用來「喚醒 / 復原」,永遠不用來「讓它睡」。** 睡是 SoC 用 `AT+UWB_DISCONNECT` 叫它睡,或 module 自己逾時進 Standby。
- **範圍**:只管耳機端。Dongle(coordinator)是另一台、USB 供電、core 常開自我管理,不在本規格內。

---

## 1. SoC 對 UWB module 的狀態模型

SoC 心裡維護一個 module 狀態機:

| SoC 眼中的狀態 | 意義 | 進入條件 |
|---|---|---|
| `ASLEEP` | module 在 Standby,UART 死,只有 NRST 能叫醒 | 送過 DISCONNECT、或 module reconnect 逾時自己睡、或收到 CONNECT_FAIL 後轉靜默 |
| `WAKING` | 剛拉 NRST,等 module 開機 | 拉 NRST 後 |
| `SEARCHING` | module 醒著、正在 reconnect | 收到開機 banner / `+EVENT: UWB_READY`,尚未 CONNECTED |
| `CONNECTED` | link 真的起來、在串流 | 收到 `+EVENT: UWB_CONNECTED` |
| `SUSPECT_HUNG` | 本該醒著卻沒心跳 | 見 §7 |

---

## 2. SoC 怎麼知道 module 現在什麼狀態

三個來源交叉判斷:

1. **AT 事件**(module 主動吐):`+EVENT: UWB_READY`(開機就緒)/ `UWB_CONNECTED` / `UWB_DISCONNECTED` / `UWB_CONNECT_FAIL` / `UWB_QUALITY:WEAK|GOOD` / `UWB_UNPAIRED` / `UWB_PAIRING` / `UWB_PAIRED` / `UWB_PAIR_FAIL`。
   - ⚠️ **`UWB_DISCONNECTED` 跟 `UWB_UNPAIRED` 不一樣,SoC 的處置相反**:前者是「對端不在,但 flash 還記得它」→ 重試迴圈(NRST / `AT+UWB_CONNECT`)有意義;後者是「配對記錄已被抹掉」→ 再怎麼重試都連不上,只有 `AT+UWB_PAIR` 有用。收到 `UWB_UNPAIRED` 就該**停掉重試迴圈**並顯示未配對。
   - `UWB_UNPAIRED` **只在記錄真的被抹掉時**才發,也就是按鍵單押解除配對那條路;`AT+UWB_PAIR` 不會發(它保留舊記錄,見 §6)。順序是 `UWB_UNPAIRED` 在前、`UWB_DISCONNECTED` 在後(後者由既有的 link 輪詢發出)。只認得舊事件的 host 不受影響。
2. **AT 查詢**:`AT+UWB_CONN_STATUS?` → `0=Standby / 1=Pairing / 2=Connected / 3=Connecting`;`AT+CONN_LM?` → link margin。
   - ⚠️ `2=Connected` **不等於 link 健康**(scheduler 有 silent wedge 盲區),要交叉「有沒有真的在收音訊」判斷(見 `uwb_disconnect_decision_spec.md` §4)。
3. **SoC 自己的記帳**:我有沒有叫它睡、我剛剛有沒有拉 NRST。用來區分「預期中的睡」vs「異常掛掉」。
4. **靜默訊號**:module 進 Standby 後 **AT 全無回應、`+CRASH_DUMP:` periodic 也停** = 睡著了(或掛了,靠記帳區分)。
5. **(建議補)STATUS/READY GPIO**:module→SoC 一條線,醒著拉高、睡著拉低。因為 Standby 時 UART 死,單靠 UART 分不出「睡 vs 掛」,一條 GPIO 最乾淨。目前**尚未確認有這條線**(§10)。

---

## 3. 何時拉 NRST(喚醒 / reset)—— 決策表

| # | 觸發條件(SoC 判斷) | 前提 module 狀態 | 動作 | 理由 |
|---|---|---|---|---|
| 1 | 使用者要(切到)UWB 音訊 | `ASLEEP` | **拉 NRST** → `WAKING` | Standby 只能 NRST 叫醒;醒來會自動 reconnect |
| 2 | 要 UWB 音訊,但 module 醒著且未連 | `SEARCHING`(awake) | 送 `AT+UWB_CONNECT`(軟 reset)或拉 NRST | 醒著時 UART 可用,軟 reset 即可 |
| 3 | **reconnect 重試**:收到 `UWB_CONNECT_FAIL` 後 module 轉靜默(睡了),但仍想連上 | `ASLEEP` | **依 backoff 拉 NRST**(見 §5) | module 睡了只能 NRST 再試一輪 |
| 4 | 喚醒沒成功:拉了 NRST 但逾時內沒 banner/AT | `WAKING` 卡住 | **重拉 NRST 1~2 次**,仍死 → 判硬體故障停手報錯 | 別無限 reset |
| 5 | 卡死復原:本該醒著卻沒心跳、且非我叫它睡 | `SUSPECT_HUNG` | **拉 NRST**(IWDG 之外的第二層,見 §7) | 溫柔手段救不回 wedge |

---

## 4. 何時「不要」拉 NRST(這些坑更重要)

- **已 `CONNECTED` / 醒著**:此時按鍵是音量、播放、pairing 長按等 → **走 AT 指令**,不是 reset。對正在串流的 link 發 reset = 硬砍掉。
- **正在 `WAKING` / `SEARCHING`**:它已經在開機/重連了,**讓它跑完**(一輪 ~10s),別在視窗內再補一發 → 避免 reset 風暴。
- **是我自己叫它睡的、且現在不需要它**:預期中的 Standby,不 reset。
- **module 自己重開了又活回來**(IWDG / crash-dump 自救):只要**重新同步狀態模型**,別再補一發。

---

## 5. reconnect 重試迴圈(開機時間差場景)—— 本規格重點

**要解的問題**:耳機先開、dongle 一分鐘後才開,也要能連上。
**module 端已做的**:HS 每次開機跑一輪 reconnect(~10s),連不上就**閃一次回連 LED → 進 Standby 睡著**。它**不會自己醒**。
**所以 SoC 必須週期性 NRST 把它叫醒重試,直到 CONNECTED。**

一輪的時間帳(SoC 要對齊):
```
拉 NRST ─▶ module 開機(~數百 ms)─▶ +EVENT: UWB_READY
        ─▶ reconnect poll 10s(RECONNECT_TIMEOUT_MS)
              ├─ 連上 ─▶ +EVENT: UWB_CONNECTED     ← 結束,停止重試
              └─ 沒連上 ─▶ +EVENT: UWB_CONNECT_FAIL(~5s 先送)─▶ 10s 到 ─▶ 進 Standby(靜默)
```

**SoC 重試狀態機(建議)**:
```
state = ASLEEP
loop while 使用者仍想要 UWB 且未 CONNECTED:
    拉 NRST;  state = WAKING;  記 t0
    等 UWB_READY(逾時 T_boot ≈ 2s 沒來 → 回 §3#4 重拉,累計失敗上限後報錯)
    state = SEARCHING
    等 UWB_CONNECTED 或「module 靜默」(≈ 收到 CONNECT_FAIL 後 + module 不再回應):
        UWB_CONNECTED → state = CONNECTED;  break（成功）
        靜默(這一輪失敗,module 已自己睡)→ state = ASLEEP
    backoff 等待 T_retry(不要馬上再 NRST)
```

**重試間隔 `T_retry`(backoff)建議**:
- 剛掉線/剛開機時對方多半很快就來 → 前幾輪短(如 3s)。
- 長期缺席 → 拉長(如 10s → 30s 封頂),省電池、避免一直 NRST。
- 一輪 module 本身要 ~10s,所以 SoC 一輪節奏 ≈ `10s(module 嘗試)+ T_retry(睡等)`。

> ⚠️ **不要在 module 還醒著的 boot 視窗內 NRST**:module 開機 + 10s poll 期間收到又一發 reset 只會打斷正在跑的那輪。等到「這一輪已失敗、module 已靜默」再進 backoff。

---

## 6. 喚醒後的意圖:reconnect vs pairing

- **預設**:NRST/reset 後 module 一律跑 **boot auto-reconnect**(用 flash 舊配對)。這是重試迴圈要的。
- **要配新對象(pairing)**:module 必須醒著且 UART 活著才收得到 `AT+UWB_PAIR`。所以 SoC:
  1. 若 module 在 Standby → 先拉 NRST 叫醒 → 等 `UWB_READY` → 再送 `AT+UWB_PAIR`;
  2. 或(若有 strap)reset 期間拉一支 strap GPIO 讓 module 開機時判定進 pairing。目前 module 端無此 strap,走 (1)。
- **切回 BT / 不用 UWB**:送 `AT+UWB_DISCONNECT` 讓 module 自己進 Standby(不是 NRST)。
- **`AT+UWB_PAIR` 是單一動作**:一條指令就會「拆掉現有連線 + 進入配對窗」,不用送兩次。序列固定是:

  ```
  SoC : AT+UWB_PAIR
  MCU : OK
  MCU : +EVENT: UWB_PAIRING      ← 配對窗開始,10s;SoC 在這裡點配對燈效
  MCU : +EVENT: UWB_PAIRED       ← 成功,新配對已寫入 flash
        或 +EVENT: UWB_PAIR_FAIL ← 逾時/中止,舊配對原封不動還在
  ```

  配對窗期間 `AT+UWB_CONN_STATUS?` 回 `1=Pairing`,重複送 `AT+UWB_PAIR` 是 no-op(**不會**把窗重新計時)。
- **重配對是「取代」不是「先清掉再賭」**:`AT+UWB_PAIR` **不會**預先抹掉舊記錄,舊記錄一直留到新配對成功才被覆寫。所以配對失敗不會損失既有配對——這很重要,因為配對需要兩端同時進窗,配失敗是**常態**而非例外。
  - 收到 `UWB_PAIR_FAIL` 後 SoC 有兩條路:再試一次 `AT+UWB_PAIR`(記得叫使用者同時操作另一端),或送 `AT+UWB_CONNECT` 放棄重配、回到原本的對象(module 會 reset 進 boot auto-reconnect,重新讀回保留的記錄)。
  - 這跟 §5 重試迴圈的政策一致:連不上不等於要忘記。**唯一會真的抹掉記錄的是按鍵單押解除配對**,那時才會收到 `UWB_UNPAIRED`。
- **兩端都要各自進配對**:HS 與 DG 之間唯一的通道就是 UWB 本身,沒連線時一端無法通知另一端。所以重配對必須兩邊分別觸發(HS 按鍵/AT、DG 按鍵/AT),並落在同一個配對窗內——跟藍牙配對一樣的心智模型。單邊送 `AT+UWB_PAIR` 只會等到 `UWB_PAIR_FAIL`。

---

## 7. 卡死復原:NRST 是第二層,IWDG 是主

| 需求 | 正解 | 由誰 |
|---|---|---|
| 睡著 → 要用 → 叫醒 | NRST(§3#1) | SoC |
| module 卡死(醒著但 wedge)→ 復原 | **IWDG 逾時 auto-reset(主)** | module 自己 |
| 上面第二保險 | SoC 在「module 本該醒著」時定期 ping(AT / STATUS),逾時沒回 → **拉 NRST** | SoC(**僅 module 本該醒著時**才有意義;睡著本來就不回,別誤判成卡死) |

---

## 8. 典型時序

**A. 冷喚醒到連上(dongle 已在)**
```
User: 按鍵要用 UWB
SoC : (module=ASLEEP) 拉 NRST
MCU : +EVENT: UWB_READY
MCU : +EVENT: UWB_CONNECTED     ← ~10s 內
SoC : state=CONNECTED,音訊走 UWB
```

**B. 開機時間差(dongle 還沒開)— 重試迴圈**
```
SoC : 拉 NRST
MCU : UWB_READY → (10s) → UWB_CONNECT_FAIL → 進 Standby(靜默)
SoC : 等 T_retry(backoff)後再拉 NRST
MCU : UWB_READY → (10s) → UWB_CONNECT_FAIL → Standby ...
      ...(dongle 這時開機了)...
SoC : 拉 NRST
MCU : UWB_READY → UWB_CONNECTED   ← 連上,SoC 停止重試
```

**C. 卡死復原**
```
SoC : (認為 module 醒著) 定期 AT+UWB_CONN_STATUS?
MCU : (無回應 / 音訊斷但 status 還回 2)  ← 疑似 wedge
SoC : 拉 NRST → 重跑 reconnect
```

---

## 9. Module 端時間常數(SoC 要對齊)

| 常數 | 值 | 意義 |
|---|---|---|
| `RECONNECT_TIMEOUT_MS` | 10000 | 每輪 reconnect poll 長度;逾時 HS 進 Standby |
| `AT_UWB_CONNECT_TIMEOUT_MS` | 5000 | `+EVENT: UWB_CONNECT_FAIL` 送出時機(比整輪早,別當成「這輪結束」) |
| module 開機到 `UWB_READY` | ~數百 ms(實測為準) | `WAKING` 逾時 `T_boot` 抓 ~2s |
| 一輪總長 | ≈ 開機 + 10s | SoC backoff 節奏的基準 |

---

## 10. 待確認 open items

- [ ] **STATUS/READY GPIO**:module→SoC 有沒有這條線?沒有的話「睡 vs 掛」只能靠「AT 無回應 + 我有沒有叫它睡」推斷(可用但不如 GPIO 直接)。建議補一條。
- [ ] **backoff 具體數字**:`T_retry` 初值 / 上限 / 成長曲線,依實際使用情境定。
- [ ] **「使用者要 UWB」由什麼觸發**:哪個按鍵 / 哪個音源切換事件 → 進重試迴圈?
- [ ] **重試迴圈的終止條件**:除了 CONNECTED,要不要「試 N 次仍失敗 → 拉長到很慢 / 停止 / 通知使用者」?
- [ ] **NRST 極性 / 脈寬**:GPIO6_RST 驅動 NRST 的低電平脈寬需符合 STM32 NRST 規格。
- [ ] **WKUP 腳(未來)**:若日後把按鍵接到 module 的 WKUP 腳(u535 pairing 鍵=PA4=WKUP2),可不經 SoC 直接喚醒;現階段走 NRST。
