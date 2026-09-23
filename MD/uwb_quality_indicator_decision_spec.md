# UWB 連線品質指示 決策規格 (BT SoC ↔ UWB MCU)

> 日期 2026-09-17。程式碼基準 `rel/v2.4.1_rc2`。
> 對象:BT SoC(master)韌體 與 STM32U5+UWB module(slave)韌體 兩邊對齊。
> 目的:定義 module 用什麼量測、什麼狀態機回報連線品質,以及 SoC 該怎麼解讀。
> 對應 code:`app/common/at_cmd_core/at_cmd_core.c`、`puretone_unidirectional_node.c`、
> `puretone_unidirectional_coord.c`、`config/sac_cfg.h`。
> PRD:`D:\_Puretone_Gen2\SPARK_Puretone_HS_Gen 2_PRD_v1.3_draft2.docx`(833295 bytes,2026-08-20,
> 即 `at_cmd_prd_reconciliation.md` 認證的那一份)。品質事件在 §STM32 to BLE SoC 事件表;
> 本文件 §8 列出要改的段落。
> 背景見 `uwb_disconnect_decision_spec.md`、`link_dropout_arms_ledger.md`、`u535_ldo_rx_deficit.md`;
> **階梯的反應速度與底部餘裕見 `fallback_mono_rung_rationale.md`,那份的實測直接限制了本文件
> §2.1 與 §3.1 能承諾多少提前時間。**

> **2026-09-22 修正:門檻改成階 3 `WEAK` / 階 4 `CRITICAL`,推翻 §3.2 的「階 2 / 階 3」。**
>
> | 階 | 等級 |
> |---|---|
> | 0–2 | `GOOD` |
> | 3 | `WEAK` |
> | 4(最底階) | `CRITICAL` |
>
> HS 斷音保底不變(任何階斷音都是 `CRITICAL`)。程式碼只動 `AT_UWB_QUALITY_WEAK_RUNG` /
> `AT_UWB_QUALITY_CRITICAL_RUNG` 兩個常數(`at_cmd_core.h`)。
>
> **為什麼推翻 §3.2**:§3.2 的論證是「室內任何降階都代表遮擋,等到階 4 等於等那一步跨完」。
> 它漏掉的是 `CRITICAL` 對 SoC 的意思——**切藍芽的指令**。階 3 是 48k ADPCM,聲音連續、
> 值得留在 UWB;在階 3 就叫 SoC 切走,是在不必走的時候把它趕走。`CRITICAL` 應該只代表
> 「階梯已經沒有下一階可以退」。代價是提前時間:`WEAK`→`CRITICAL` 變成一個 10 Hz 取樣
> (約 100 ms),`GOOD`→`CRITICAL` 約 300 ms。§3.2 對 SoC 架構的結論因此更加成立:
> 藍芽必須常態保持隨時可出聲。
>
> **重連後的 `CRITICAL` 不是韌體問題,是 SoC 切回規則的問題。** 重連時階梯停在斷線那段
> 佇列堆積推到的底階,所以第一則品質事件是 `CRITICAL`。客戶回報的「重連後又切回藍芽」是
> SoC 在 `LE_UWB_CONNECTED` 就切回 UWB、2 秒後收到這則 `CRITICAL` 又切走。SoC 規則定為
> **快切慢回**:收到 `CRITICAL` 切藍芽;在藍芽上只有收到 `WEAK`/`GOOD` 才切回 UWB;
> `LE_UWB_CONNECTED` 不是切回的訊號。照這個規則,重連後的 `CRITICAL` 抵達時 SoC 已在藍芽上,
> 留著即可,不需要韌體分辨「停在底階」和「掉進底階」。
>
> 未採用:重連時把階梯拉回最高階(遮擋後重連會直接跳 48k/24 報 `GOOD`,而 DG 分不出
> 「對面重開」和「被擋很久」);新增 `BAD` 等級並把「掉進底階」latch 成事件(在上面的 SoC
> 規則下,它能多做的只剩語意,卻要 ODM 多處理一個字串)。後者的 patch 存在
> Claude memory 的 `quality-4level-bad-latch.patch`。

---

## 0. 一句話定位

`+EVENT: LE_UWB_QUALITY` 存在的目的是**讓 SoC 在使用者聽到斷音之前切回藍芽**。

判準是斷音,但**觸發點必須早於斷音**。這兩件事合起來決定了能用什麼量測:
凡是從「已經掉了封包」推導出來的訊號都太晚(§2),所以這支指示只能建立在
**餘裕類/趨勢類**訊號上——也就是「還沒壞,但撐不住多久了」。
距離與 dB 不是判準,它們只是造成斷音的原因之一;`AT+CONN_LM?` 是工程查詢,不是決策依據。

---

## 1. 現況為什麼不可用

實測症狀:**距離 0.5 m 以內仍回報 WEAK**。三個原因疊在一起,不是把門檻調鬆就能解決。

| # | 問題 | 位置 |
|---|---|---|
| 1 | 量測來源是 **audio 連線**的 fallback link margin。沒有音訊在傳時這個值讀 0,`0 < 5` 直接 WEAK,與距離、與斷音都無關 | [node.c:2128](../app/example/puretone_unidirectional/puretone_unidirectional_node.c#L2128) |
| 2 | 那是 uint8_t **原始碼值不是 dB**:近距實測 ≈ 200(`dualradio_fb0_park_HQ_followup.md:29`),臨界時在 0/5/10 之間跳(`dualradio_resync_HQ_report.md:267`)。門檻 5/10 卻被註解成 dB,比階梯自己的 30~60 低一個數量級 | [at_cmd_core.h:86](../app/common/at_cmd_core/at_cmd_core.h#L86) |
| 3 | 每秒單點取樣、跨門檻就吐,沒有連續次數要求。臨界區會 WEAK/GOOD 成對亂跳 | [at_cmd_core.c:639](../app/common/at_cmd_core/at_cmd_core.c#L639) |

DG 端讀的是同一個值(HS 用 data 封包回報,[coord.c:924](../app/example/puretone_unidirectional/puretone_unidirectional_coord.c#L924)),兩端一起錯。

### 1.1 前置驗證(要先量,15 分鐘)

0.5 m、已連線,分別在**有播音訊**與**完全不播**兩種狀態下打 `AT+CONN_LM?`。
若不播時是 0 或個位數、有播時是一兩百,即確認 §1 的 #1 是主因。

---

## 2. 硬約束:斷音沒有前置時間

這是整份規格的物理上限,先講,因為它把大部分看起來合理的方案都排除掉了。

HS 的緩衝深度是**每階的 latency**([sac_cfg.h:23](../app/example/puretone_unidirectional/config/sac_cfg.h#L23) 起):

| 階 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|
| 緩衝 | **7 ms** | 10 ms | 40 ms | 40 ms |

(階 0 是 5 ms,已停用。上限 `MAIN_CHANNEL_MAX_LATENCY_MS` = 40。)

sac_cfg.h 自己的註解說明了這個數字的意義:

> Buffer depth is how long an outage the rung can ride out, and it is a different resource from
> retransmission: once the consumer queue drains, no number of retries helps, because the retried
> packet has missed its playback deadline.

**所以在出貨階(階 1),從「封包停止到達」到「斷音」只有 7 ms。** 推論:

- `consumer_buffer_underflow_count`(斷音計數)前置時間 = **0**,它就是斷音本身。
- `producer_packets_corrupted_count`(破音/沒收到,[sac_api.c:329](../core/audio/api/sac_api.c#L329)、
  [:484](../core/audio/api/sac_api.c#L484))前置時間 = **當前階的緩衝深度,7~40 ms**。
- audio 連線的 rej / miss 率:同上,而且要一整秒的窗才有統計意義,實際上更晚。
- data 連線 PER:量的是 link 存活,跟斷音沒有因果關係,更不用說提前。

**沒有任何基於「已經掉包」的訊號能給 SoC 有用的準備時間。** 要提前,只能用餘裕類訊號。
也不能靠加深緩衝來換時間:PRD 規定 E2E latency ≤ 5 ms,而階 3/4 已經是 40 ms——
緩衝深度與延遲規格直接互斥,底部兩階的 40 ms 已經是拿延遲換來的(那段註解說明了為什麼
只有底兩階加寬)。

### 2.1 兩種劣化要分開,只有一種能提前

| 型態 | 例子 | 能否提前 | 該不該切 BT |
|---|---|---|---|
| **趨勢型** | 走遠、持續干擾、遮蔽物擺著不動 | **可以**,秒級 | 該切 |
| **瞬時型** | 手或身體掃過,持續數百 ms | **不可能**,任何訊號都來不及 | **不該切**,它會自己回來 |

底兩階的 40 ms 緩衝與 `sac_mute_on_underflow` 的 30 ms 靜音,就是為瞬時型準備的——
用一次短暫的靜音換「不要把使用者從一個馬上就會恢復的音源切走」。

**而且在這個產品上,趨勢型幾乎不會發生。** 見 §2.2。

**瞬時型連階梯自己都來不及,這是實測過的。** `fallback_mono_rung_rationale.md` §3:

> 遮擋是突發性衰減。降階機制是**反應式**的(要等 buffer 堆積或 CCA 惡化才動作),
> 等它反應完,音訊已經斷了。

而且「把降階門檻調早」這條路也試過了,無效:放寬 mode 2 的門檻讓它更早退到 mode 3 之後,
fb 確實更常待在 3,**但斷音跟著搬到 fb=3**。所以瞬時遮蔽不是靠更早的預警解決的,
是靠底部的重傳餘裕(同份文件 §5)。這也是為什麼本文件不承諾對瞬時型有任何提前時間。

### 2.2 實測距離:失效是階梯函數,不是斜坡

**v240,室外,一步以 70 cm 計:**

| 情境 | 步數 | 距離 |
|---|---|---|
| 不遮擋 | 47 步 | **32.9 m** |
| 遮擋 | 5 步 | **3.5 m** |

比值 9.4 倍 ≈ **19.5 dB 人體衰減**。對照 PRD 的兩條規格:

- **LOS > 5 m**:不遮擋有 6.6 倍餘裕(約 16 dB 用不到)。
- **5×5 m² 室內良好覆蓋**:房間對角線 7.07 m,而遮擋時只有 3.5 m,
  **不到對角線的一半**。

這兩行合起來決定了這支指示能做什麼:

1. **距離不是失效模式。** 在規格內的房間裡走到任何角落都還在 33 m 的餘裕內,
   link margin 會一路貼在飽和值附近(近距實測 ≈ 200,§1)。所以任何「看著某個值慢慢變差」
   的方案在這個產品上都沒有動態範圍可用——那個值在能用的範圍內幾乎是常數,
   然後一步跨到死。這是 §1 的三個問題之外,更深的第四個原因。
2. **遮擋才是失效模式,而且它在規格內的房間裡就會發生。** 3.5 m 是一般房間裡
   隨便一個轉身、一個人走過去就會跨過的距離。
3. **所以失效是階梯函數。** 19.5 dB 在一次轉身(數百 ms)內出現,不是走遠那種
   分鐘級的斜坡。能觀察的不是「值在下降」,而是**那一階本身,而且要越早越好**。

這把 §3 的觸發點往上推了一階:等階 4 等於等那一步跨完。見 §3.2。

如果 SoC 要求「連瞬時遮蔽也不能有任何斷音」,那在這個延遲規格下做不到,
不是門檻能解決的。這一條要跟 ODM 講清楚,不要讓它變成之後的 open issue。

---

## 3. 決策:以 FB 階數為主觸發,斷音計數為保底

```
主觸發   = FB 階數                    ← 提前數百 ms(§2.2/§3.2),決定 WEAK / CRITICAL
保底觸發 = 斷音(underflow)            ← 前置時間 0,一旦動了就直接 CRITICAL
```

| 階 | 內容 | 事件 | SoC 該做什麼 |
|---|---|---|---|
| 0 | 96k/24-bit | — | (`MAIN_CHANNEL_ALLOW_96K=0`,永遠不會出現;host 不要把 0 當最佳值) |
| 1 | 48k/24-bit | `GOOD` | 維持 UWB |
| 2 | 48k/16-bit | `WEAK` | **預備**:藍芽必須已經可以出聲(§3.2) |
| 3 | 48k ADPCM | `CRITICAL` | **切藍芽** |
| 4 | 24k ADPCM | `CRITICAL` | 維持(已經切走了) |

這兩個門檻要做成常數(`QUALITY_WEAK_RUNG` / `QUALITY_CRITICAL_RUNG`),不要寫死在
判斷式裡——它們是 §6 的 T_bt 決定的,而 T_bt 要 ODM 給。上表是**預設值**,理由見 §3.2。

外加一條保底:**任何 `consumer_buffer_underflow_count` 增加 → 立即 `CRITICAL`**,
不管當時在第幾階。理由是它代表使用者已經聽到了,而在階 1/2 上斷音意味著劣化快到
階梯來不及反應(瞬時遮蔽,或 `u535_ldo_rx_deficit.md` 那類到達率問題)。

**這條保底同時是預警品質的量尺**:它每次觸發,都表示主觸發沒有提前成功。
把「保底觸發次數 / 總切換次數」當成這個功能的驗收指標(§7)。

### 3.1 FB 階數能提前多少——分情境,不要一概而論

階梯降階的條件是 `is_link_queue_size_high() || is_link_cca_bad()`
([sac_fallback.c:996](../core/audio/processing/sac_fallback.c#L996))。
第一項看的是 **DG 的 TX queue 平均深度**——DG 送不出去、封包開始堆積,
這件事發生在 HS 少收到任何一個封包**之前**。同一個訊號也以 `tx_queue_level_high` 這個
header bit 在每個封包裡傳到 HS([sac_api.h:126](../core/audio/sac_api.h#L126)),
CDC 就是靠它做時鐘補償的。

所以階數不是「已經壞了」的紀錄,它是 DG 在說「我快送不動了」。
而且它的門檻是 bench 量出來的(每階 `link_margin_threshold` 30~60 + hysteresis 20,
[coord.c:1256](../app/example/puretone_unidirectional/puretone_unidirectional_coord.c#L1256) 起),
升階要 margin 連續好 2~5 秒且一次一階([sac_fallback.c:202](../core/audio/processing/sac_fallback.c#L202)),
天生重度去抖。

直接讀 `tx_queue_level_high` 原始 bit 沒有比階數更好:那是階梯的輸入,
階梯已經用 10 Hz 的窗與量過的門檻把它整理過了。

**但「DG 的 queue 開始積」與「HS 斷音」之間有多少時間,取決於劣化有多快**:

| 情境 | 階數的提前時間 | 依據 |
|---|---|---|
| 趨勢型(走遠、持續干擾) | 秒級,夠用 | 劣化比階梯的 10 Hz 取樣慢好幾個數量級 |
| 瞬時型(遮蔽) | **≈ 0,來不及** | `fallback_mono_rung_rationale.md` §3 實測(見 §2.1) |

看起來這支指示的承諾只對趨勢型成立——**但 §2.2 的量測說,這個產品上趨勢型幾乎不存在**
(不遮擋 33 m,房間裡走不到邊緣)。所以實務上要面對的永遠是中間那一格:
遮擋造成的階躍,而階梯對它能給的是**數百 ms**,不是秒級。§3.2 的門檻就是照這個數字定的。

§7 的量測仍要把兩種情境分開統計,否則瞬時型的 0 提前時間會把趨勢型的數字拉爛,
看起來像整個方案不可行;但驗收要以遮擋情境為主,因為那是真的會發生的那一種。

### 3.2 為什麼預設是階 2 / 階 3,而不是階 3 / 階 4

> **已被 2026-09-22 修正取代**(見文件開頭):現行是階 3 `WEAK` / 階 4 `CRITICAL`。以下保留原論證。

§2.2 的量測:不遮擋 33 m、遮擋 3.5 m。在 5×5 m 的房間裡,**沒有任何正常使用會把 link
推到餘裕邊緣**——除非被遮擋,而遮擋是 19.5 dB 的一步。

推論:**室內任何一次降階,本身就已經是「有東西擋住了」的證據**,不是「距離漸漸變遠」。
階梯是一階一階走的,但推著它走的那件事是階躍的,所以:

- 等到階 4 才喊 `CRITICAL`,等於等那一步跨完——階 3→4 之間沒有第二個 19.5 dB 可以等。
- 階 1→2 的降階在這個餘裕下就已經異常,拿它當 `WEAK` 是合理的,誤報成本也只是
  「藍芽被暖機了但沒用到」。

階梯降階最快是每個 10 Hz 取樣一階,所以階 2 → 階 4 最短約 200 ms。這就是這個產品上
**提前時間的物理上限:數百 ms**。要更多的話只能往階梯的「輸入」挖,不要等它的「輸出」:
`core/audio` 是原始碼不是 prebuilt(`core/audio/CMakeLists.txt` 直接編 `sac_fallback.c`),
而 `_internal.consumer_queue_metrics` 就在公開 struct 裡、10 Hz 更新——看 queue 爬向門檻,
比等它跨過門檻降階更早。這條路不需要任何外部 source,但它是第二階段,
等 §7 量出提前時間確實不夠再做。

**對 SoC 架構的結論(這條要讓 ODM 看到):**
既然提前時間上限是數百 ms,**藍芽不能等收到 `WEAK` 才開始暖機**,那來不及。
藍芽必須常態保持在「隨時可出聲」的狀態,`WEAK` 只是提示、`CRITICAL` 是動作。
如果 ODM 的架構做不到常態暖機,那 UWB 側再怎麼改都買不到他們要的時間,
這件事要在規格階段講清楚,不要變成韌體的 open issue。

### 3.3 盲點與它為什麼不重要

不播音訊時 TX queue 不會積,階梯不動。**但沒有音訊就沒有斷音,SoC 也沒有要切的串流**,
所以這不是缺口。這是把判準定在斷音而不是距離之後少掉的一整類麻煩。

### 3.4 DG 端的語意

斷音發生在 HS 的喇叭上,DG 量不到,它只有階數。

- **第一版**:DG 的 `QUALITY` 只由階數決定(也就是沒有保底那一條),語意是「餘裕狀態」。
  DG 側的 SoC 本來也不是決定要不要切藍芽的那一邊。
- 若之後 DG 側 SoC 真的需要:在 `user_data_t` **尾端** append 一個 `uint8_t`
  (`puretone_link_data.h` 的四條規則:只能加在 `vendor_data` 之後、必須是 uint8_t、
  0 代表「沒有資料」、要塞得進 `MAX_DATA_PAYLOAD_SIZE`)。這是 wire 變更,**兩端都要重燒**,
  第一版不做。

因此「ODM host 接哪一端」是必須先確認的事:保底那一條只有 HS 做得到。

---

## 4. 狀態機

### 4.1 去抖動

**階數用 callback 驅動,不要輪詢。** `sac_fallback_instance_t` 有一個現成的
`fallback_state_change_callback(uint8_t mode_index)`([sac_fallback.h:98](../core/audio/processing/sac_fallback.h#L98)),
在三個換階路徑上都會觸發(`set_current_mode` 的鏡像路徑、`recover_to_previous_mode`、
`trigger_next_mode`),而**兩個 app 目前都沒有用它**。掛上去就省掉最多 1 秒的輪詢延遲——
在一個以提前時間為目的的功能上,那 1 秒是白送的。

注意它跑在 audio process context(HS 是封包到達的處理路徑),所以**只能設 flag**,
由 `at_cmd_core_process()` 發事件——與這個樹上 AT handler 一律「設 request flag、
在 process 裡動作」的規則一致。

斷音那一路仍然每秒評估(沿用 `AT_UWB_LINK_QUALITY_CHECK_INTERVAL_MS`,1000 ms;
它是保底,而且計數率本來就需要一秒的窗)。兩路都**只在狀態改變時吐事件**。
現況是跨門檻就吐,那不是狀態機。

- 變壞:**立即**,不等連續次數。階數本身已經是去抖過的訊號(§3.1),再加一層等待
  就是把好不容易爭到的前置時間還回去。保底的 underflow 同理,立即。
- 變好:~~連續 **5 秒**成立才改變,且一次只回一階~~ → **2026-09-23 改成立即,且直接報當下等級**。
- 從保底進入的 `CRITICAL`:~~額外要求 5 秒內沒有新的 underflow~~ → 下一次評估(最多 1 秒)
  沒有新的 underflow 就離開。

**為什麼拿掉變好的 5 秒**(2026-09-23):實機上「階梯 4→3 沒有送 `WEAK`,要到階 2 才送」。
原因是這個窗比階梯自己在階 3 停留的時間還長:階 3→2 只要 link margin ≥ 60 連續 2 秒
(CCA 那一項在換階時被 `reset_link_stats()` 設成最大值,等於不設限),而事件要等 5 秒才動一級,
於是 `WEAK` 送出時階梯已經在階 2——**報告跟事實對不上**,而且停在階 3 的鏈路看起來像漏送。

去抖本身不該做在這裡。階梯往上爬的門檻已經是「連續數秒的好鏈路」(階 4→3 要 3 秒),
往下掉才是 100 ms 一階,所以「快掉慢升」本來就內建在階梯裡。這個窗真正在遮的是底階
4↔3 的震盪,那是鏈路問題,遮住它只會讓其他所有回報都變晚。不想跟著震盪的 host 自己把門檻
拉高就好:**切回 UWB 只認 `GOOD`**,等於要求階梯爬到階 2,中間隔了兩階、五秒的 link margin。

### 4.2 CRITICAL 的恢復,以及 `FALLBACK_PIN_AT_BOTTOM`

**決策:最後一階要可恢復,`FALLBACK_PIN_AT_BOTTOM=0`。**

**2026-09-17 修正:改動 default,不再由出貨 preset 帶。**
原本的折衷是「default 維持 1,出貨 preset 上加 `-DFALLBACK_PIN_AT_BOTTOM=0`」。
那等於把一個正確性條件寄放在 build 指令上——忘了加不會有任何徵兆,只會得到一顆
`CRITICAL` 永遠不解除的韌體,而這正是本節要消滅的失效模式。既然下面的論證得出的結論是
「pin 對這個產品是錯的」,就該寫在 default 裡,而不是每次 build 記得帶。

這個開關已經存在([sac_cfg.h:247](../app/example/puretone_unidirectional/config/sac_cfg.h#L247),
現預設 0,已在 [CMakeLists.txt:174](../CMakeLists.txt#L174) forward 過去),不需要新做一個。

但它存在的理由正是恢復問題本身。sac_cfg.h 的註解:

> TEMPORARY. Wanted now because a link that has already fallen to 24 kHz tends to climb, fail and
> fall again, and the oscillation is worse to listen to than the bottom rung is.

- pin=1(舊 default):踩進最後一階就鎖住,只有 peer 靜默 `NODE_RESTART_SILENCE_MS`(3 s)才釋放
  ([coord.c:1578](../app/example/puretone_unidirectional/puretone_unidirectional_coord.c#L1578))。
  訊號變好也不會升回 → **`CRITICAL` 永遠不會解除**,UWB 再也回不來。
- pin=0(現況):底部 4↔3 震盪回來。

震盪在新的架構下**不再是聽感問題**:進階 4 就吐 `CRITICAL`、SoC 就切走了,
之後的震盪發生在一條沒有人在聽的 link 上。它只剩一個影響——`QUALITY` 事件會在
`CRITICAL`/`WEAK` 之間抖,而那由 §4.1 的「變好要 5 秒」擋掉。

所以去抖動必須做在事件層,不能靠 pin。pin 只是把震盪藏起來,代價是永遠不恢復。

震盪本身仍然是個未修的問題,只是換了歸屬:它不再由 pin 遮住,而是由 §4.1 的
`AT_UWB_QUALITY_RECOVER_MS` 擋在事件層。根因照舊要修(u535 上最可能是
`u535_ldo_rx_deficit.md` 的到達率不足);把 `FALLBACK_PIN_AT_BOTTOM` 設回 1
現在只剩一個用途——聽感 A/B 比較,不是出貨組態。

### 4.3 與斷線的關係

品質不能單獨解讀,必須跟連線狀態併看:

- 沒有 peer 時階梯被 freeze,回來後再 hold `LADDER_SETTLE_MS`(2 s)才放
  ([coord.c:1551](../app/example/puretone_unidirectional/puretone_unidirectional_coord.c#L1551)
  的註解說明為何不能省)。**這段時間的階數是舊值。**
- 因此:`DISCONNECTED` 之後品質狀態清為未知、不吐事件;`CONNECTED` 之後等 2 秒
  (對齊 `LADDER_SETTLE_MS`)再重新評估並吐第一句。現況只在 disconnect 時清一個 flag
  ([at_cmd_core.c:634](../app/common/at_cmd_core/at_cmd_core.c#L634)),三態版要一起改。
- 完全沒有音訊在播時維持上一個狀態、不吐事件,`AT+CONN_QUALITY?` 回上一個已知值。
  不要因為沒有音訊就吐 `GOOD`——那是在斷言一件沒有量到的事。
- 三種「斷」的時間尺度不變:Wireless Core 20 ms / app `link_is_up()` 200 ms /
  AT 層 down-edge debounce 400 ms。品質事件不參與其中任何一個。

---

## 5. AT 介面

### 5.1 事件:維持冒號格式,新增第三態

```
+EVENT: LE_UWB_QUALITY:GOOD
+EVENT: LE_UWB_QUALITY:WEAK
+EVENT: LE_UWB_QUALITY:CRITICAL      ← 新增
```

**冒號不改。** ODM 已經整合冒號版(`unidir_audio_test_readme.md:80`、PRD 事件表都是冒號),
換成下底線等於破壞既有解析,而它沒有換取任何東西。

FB 階數與斷音次數**都不出現在 AT 介面上**,只作為內部判斷依據。這樣以後階梯改幾階、
量測換來源,host 的解析都不用動。

### 5.2 新增 `AT+CONN_QUALITY?`

```
AT+CONN_QUALITY?  →  +CONN_QUALITY: GOOD | WEAK | CRITICAL | N/A
```

`N/A` 用於未連線或 §4.3 的 settle 視窗內。與事件同源,兩者不會不一致
(`uwb_disconnect_decision_spec.md` 對 `CONN_STATUS?` 的同一個要求)。

### 5.3 `AT+CONN_LM?` 保留

命令與回應格式不動(`+CONN_LM: NdB`),但**量測改成真 dB**:
`rx_data_conn` 的 `stats.link_margin_avg / 10`(單位是 tenth-dB,
[swc_api.h:155](../core/wireless/swc_api.h#L155);這才是 `at_cmd_core.h` 註解本來要的東西)。
理由:現在那句 `dB` 是假的,而它是唯一的工程診斷入口。改用 data 連線之後 idle 也有數字。

它目前在 `handler_help()` 的清單裡是被註解掉的 `(internal)`
([at_cmd_core.c:1108](../app/common/at_cmd_core/at_cmd_core.c#L1108)),PRD 命令表也沒有它。
維持 internal、不對外文件化。`AT+CONN_QUALITY?` 反之要進 help 與 PRD。

### 5.4 不需要動的東西

- **`at_cmd_code_t` 不動**:純本地查詢,不跨 link。
- **`user_data_t` 第一版不動**(§3.4)。其中的 `link_margin` 欄位要留著:
  `sac_fallback_set_rx_link_margin()` 靠它餵階梯,而且它是 append-only 契約的一部分,
  不能移除也不能改序。
- 兩個 append-only wire contract 第一版都不會被碰到,這是這個做法最大的好處。

---

## 6. 需要 ODM 提供的一個數字

**T_bt:SoC 從收到 `CRITICAL` 到藍芽真的出聲,需要多久?**

這個數字決定整個設計是否成立,而 module 端無法自己知道:

- 若 T_bt ≲ 數百 ms(藍芽已連線、只是沒在播):階 4 → `CRITICAL` 的前置時間足夠,照 §3 做。
- 若 T_bt 是數秒(藍芽要重新連線):階 4 太晚,必須把 `CRITICAL` 提前到**階 3**,
  並接受更高的誤切率——階 3 上有很多條 link 是可以一直聽下去的。
- 兩者都不夠的話,問題不在 module:任何 UWB 側訊號都買不到比階梯更多的時間(§2)。

`WEAK`(階 3)存在的目的就是讓 T_bt 可以被藏起來:SoC 在 `WEAK` 就把藍芽暖機到
「隨時可出聲」,`CRITICAL` 只負責最後的切換。這是把 T_bt 縮到最小的唯一辦法。

---

## 7. 驗收指標

不是「門檻調到多少」,而是兩個可量的數:

| 指標 | 定義 | 目標 |
|---|---|---|
| **提前時間** | `CRITICAL` 發出 → 第一次 underflow 的時間 | ≥ T_bt(§6) |
| **預警失敗率** | 由保底(underflow)進入 `CRITICAL` 的次數 / 總進入次數 | 越低越好;高代表主觸發太晚 |
| **誤切率** | 進了 `CRITICAL` 但其後 30 s 內完全沒有 underflow 的次數比例 | 由 ODM 定可接受值 |

量法:走遠 / 遮蔽兩種情境各做 10 次,每 100 ms 記一行,同時記階數、
`consumer_buffer_underflow_count`、`producer_packets_corrupted_count`、`CONN_LM?`。
兩種情境要分開統計(§2.1),瞬時遮蔽的「誤切」不算失敗,那是刻意不切。

PRD 的設計目標是 LOS > 5 m、5×5 m² 室內良好覆蓋,所以 5 m LOS 靜態應該穩定在 `GOOD`。
若不是,那是 RF 問題不是門檻問題,不要用調門檻蓋掉(見 `u535_ldo_rx_deficit.md`)。

---

## 8. 實作清單

程式碼:

- [ ] `at_cmd_core.c`:三態狀態機取代現在的 two-flag;新增註冊 API(回階數與斷音計數的
      getter,形式對稱於現有的 margin cb);`handler_conn_quality()`;
      `at_server_register("CONN_QUALITY", ...)`。
- [ ] **`handler_help()` 裡那份手維護的 `cmds[]`** —— 漏掉不會有編譯錯誤,只有 `AT+HELP` 看不到。
- [ ] `at_cmd_core.h`:門檻常數改名(現在叫 `..._THRESHOLD_DB`,單位是錯的)。
- [ ] node.c:註冊 getter,回 `sac_fallback_get_current_mode(&main_channel_fallback_instance)`
      與每秒差分的 `sac_pipeline_get_consumer_buffer_underflow_count(main_channel_accumulator_pipeline)`。
      **要讀 accumulator pipeline**——`sac_mute_on_underflow` 掛在它上面
      ([node.c:1361](../app/example/puretone_unidirectional/puretone_unidirectional_node.c#L1361)),
      codec 的 consumer 也在它上面;讀錯 pipeline 會拿到永遠是 0 的計數。
      注意那段處理級在 `#if !USB_AUDIO_ENABLED` 內。
- [ ] coord.c:註冊 getter,只回階數(§3.4)。
- [ ] 兩端都掛 `fallback_state_change_callback`,只設 flag(§4.1)。這一項不是最佳化,
      是提前時間的一部分。
- [ ] `AT+CONN_LM?` 的量測換成 `rx_data_conn` 的 `stats.link_margin_avg / 10`。
- [ ] handler 不要把 `"OK"` 放進 `resp`(`at_module` 會自己補,會變兩行 OK)。
- [x] `FALLBACK_PIN_AT_BOTTOM` default 改成 0(§4.2)。原訂做法是出貨 preset 加旗標,
      改成動 default,出貨 preset 因此不需要任何額外旗標。

文件:

- [ ] PRD 事件表:`LE_UWB_QUALITY:WEAK | GOOD` → 加 `CRITICAL`,觸發條件整段重寫
      (現在寫的「WEAK below 5 dB, GOOD above 10 dB, polled once a second」三句都要改),
      寫明 DG / HS 的語意差異,以及 §2.1 的瞬時遮蔽不切。
- [ ] PRD 命令表 Status Inquiry 區加 `AT+CONN_QUALITY?`。
- [ ] `unidir_audio_test_readme.md`:事件清單加 `CRITICAL`,命令數 21 → 22。
- [ ] `at_cmd_prd_reconciliation.md`:記錄這次是 code 與 PRD 同時改,不是單邊對齊 PRD。

---

## 9. 工作量

| 項目 | 估計 |
|---|---|
| §1.1 前置驗證(兩次 `CONN_LM?`) | 15 min |
| 程式碼 + 編譯 + 兩塊板燒進去看到三態事件 | 半天 |
| §7 提前時間與誤切率量測(兩情境 × 10 次) | 半天 |
| 回歸:`vendor_cmd_test.py --rate 1`、`at_rx_stress.py` 各跑一次 | 1 h |

「半天」對得上**程式碼到第一次看到事件**。§7 是另外半天,而且它不是校正門檻
(階數的門檻已經是既有值),是**驗證前置時間夠不夠**。若不夠,結論是把 `CRITICAL`
提前到階 3(§6),那是改一個常數,不是重做。

---

## 10. 未採用的選項

| 選項 | 為什麼不用 |
|---|---|
| 只用 audio 連線 lm(現況) | §1。量的是原因不是結果,而且沒音訊時讀 0 |
| **用 underflow 當主觸發** | **前置時間 0,它就是斷音本身(§2)。只能當保底** |
| 用 `producer_packets_corrupted_count` 當主觸發 | 前置時間 = 當前階緩衝,階 1 只有 7 ms(§2) |
| 用 audio 連線 rej / miss 率 | 同上,而且要一秒的窗才有統計意義,更晚 |
| data 連線 PER | 量 link 存活與距離,跟斷音沒有因果關係(§2);它的位置在 `DISCONNECTED` |
| 加深緩衝來換前置時間 | 與 PRD 的 E2E latency ≤ 5 ms 直接衝突;底兩階的 40 ms 已經是拿延遲換的(§2) |
| 把降階門檻調早以求提前 | **實測無效**:fb 更常待在低階,但斷音跟著搬到那一階(`fallback_mono_rung_rationale.md` §3) |
| 直接讀 `tx_queue_level_high` header bit | 那是階梯的輸入,階梯已經用 10 Hz 窗與量過的門檻整理過(§3.1) |
| CCA fail count | 量通道壅塞,已經是階梯的另一個輸入 |
| RSSI / RNSI 分開看 | 近場飽和,干擾時也會升 |
| `swc_qos_indicators` 的 `isi_indicator` | 只在 `WPS_ENABLE_PHY_STATS_PER_BANDS` 的 prebuilt 才有,不是出貨版 |
| ToF / ranging 真距離 | 這個 SWC 沒有這個 API;而且距離本來就不是判準 |
