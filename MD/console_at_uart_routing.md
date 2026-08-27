# console 與 AT 的走線 —— puretone_unidirectional / puretone_headset

> 用途：釐清「console 印到哪、AT 收在哪」。這件事散在兩個 backend、四個巨集、
> 三種板子變體裡，而且 **headset 與 unidir 對 LPUART1 的用途正好相反**，
> 光看其中一邊會得到錯誤結論——本文件就是被這樣誤判一次之後才寫的。
> 對應 commit：`36847a5`（u5a5 unidir console 改為預設走 USB CDC）
> 所有腳位皆取自 `bsp/*/quasar_def.h`，非憑記憶

---

## 1. 一覽表

| 應用 / 板子 | AT（需要**收**） | console / stats（只**發**） | 同一條線？ |
|---|---|---|---|
| **unidir** u5a5 | USART2 PA2=TX PA3=RX | **USB CDC** | ❌ 分離 |
| **unidir** u535 **LDO** | UART4 PC10/PC11（ST-Link VCP） | LPUART1 PA3 | ❌ 分離 |
| **unidir** u535 **SMPS** | LPUART1 PA3/PA2 | LPUART1（**併入 AT 的 queue**） | ✅ 共用 |
| **headset** u5a5 | USART2 PA2/PA3 | **USB CDC** | ❌ 分離 |
| **headset** u535（兩變體同） | LPUART1 PA3/PA2 | UART4 PC10/PC11（ST-Link VCP） | ❌ 分離 |

**最容易搞錯的一格**：u535 上 headset 拿 LPUART1 當 **AT**，unidir 拿它當 **console**。
兩者用途相反，所以「u535 走 LPUART」這句話在兩條產品線上指的不是同一件事。

---

## 2. 擴充埠不是同一個周邊

| | 周邊 | TX | RX | GPIO AF |
|---|---|---|---|---|
| u5a5 | USART2 | **PA2** | **PA3** | AF7 |
| u535 | LPUART1 | **PA3** | **PA2** | AF8 |

**方向是相反的**。u535 沒有 USART2，所以同樣兩根 pad 換成 LPUART1，TX/RX 也跟著對調。
這件事確認過一次又遺失過一次，現在 `puretone_unidirectional_backend.c` 有 `_Static_assert`
守著——寫錯不會建置失敗，只會表現為「一個永遠不回應的埠」。

ST-Link VCP 兩塊板一致：UART4，PC10=TX / PC11=RX，AF8。

---

## 3. 決定走線的三個巨集

都在 `backend/quasar_backend/puretone_unidirectional_backend/puretone_unidirectional_backend.c`
（headset 有自己的一份 `AT_CONSOLE_ON_STLINK`，**預設值不同**，見 §4）。

### `AT_CONSOLE_ON_STLINK` — AT 搬不搬到 ST-Link

```c
#if defined(U535_PWR_LDO)   → 1     // LDO：AT 走 UART4
#else                       → 0     // SMPS 與 u5a5：AT 走擴充埠
```

**為什麼 LDO 要搬**：那批板子的 PA2 焊墊不可靠，LPUART1 收不穩。AT 是這個應用裡
第一個需要**接收**的功能，所以移到板載除錯器那條一定通的線上。console 只發不收，
留在 LPUART1 沒有損失——兩個通道就此分開。

**SMPS** 則是 PA2/PA3 雙向都好，而且**沒有 UART4 排針**，所以 AT 佔用唯一那條。

### `CONSOLE_HAS_OWN_UART` — console 有沒有自己的線

```c
#if AT_CONSOLE_ON_STLINK || defined(U535_PWR_LDO)   → 1
#else                                               → 0    // 只有 SMPS 是 0
```

為 0 時 console 併進 AT 的 queue。**兩個寫入者餵同一個 FIFO，所以是整行整行交錯**——
完整的 `[DG] ...` 挨著完整的 `+EVENT: ...`。若在字元中間混掉，那才是故障。

### `CONSOLE_ON_CDC` — u5a5 的 console 走不走 USB

```c
#ifdef QUASAR_U535   → 0     // u535 維持 UART
#else                → 1     // u5a5 走 USB CDC（36847a5 起）
```

`-DCONSOLE_ON_CDC=0/1` 可覆寫兩邊。

**改成預設開的原因**：u5a5 的 console 原本與 AT 擠在 USART2 擴充排針上，要讀就得外接
轉接器，而那顆為了供電早就插著的 USB 反而閒置。等於「用正常方式建出來的板子，
console 讀不到」，而那跟「板子什麼都沒印」是同一個現象。

**不需要開任何東西才能用**：`common_backend.c` 在每個 baremetal build 都無條件呼叫
`tinyusb_baremetal_setup()`，`tusb_init()` 與 `tud_task()` 都在裡面。錯的只有預設值。

**u535 不動**：它的擴充腳是 LPUART1 且雙向都通，那是硬體實測過的路由；CDC 在 u535
上沒人驗過。留著不一致，比為了對稱去動一條驗過的路由安全。

---

## 4. 兩條產品線走的是**不同函式**

這是最初誤判的根源。

| | 函式 | 實作位置 |
|---|---|---|
| **unidir** | `facade_print_string()` | `puretone_unidirectional_backend.c`，**三個**覆寫 |
| **headset** | `facade_stats_write()` | `puretone_headset_backend.c:483` |

headset **從不覆寫** `facade_print_string`——它落到 `common_backend.c:85` 那個
`__attribute__((weak))` 版本，內容就是寫 USB CDC。所以查 `facade_print_string` 會得到
「headset 走 CDC」的結論，**那只對 u5a5 成立**；u535 上 headset 的輸出走
`facade_stats_write`，落在 ST-Link VCP。

`unidir` 三個覆寫的守門條件：

```
#if defined(QUASAR_U535)                        → LPUART1（或併入 AT queue）
#if CONSOLE_ON_CDC                              → USB CDC（緩衝式，見 §5）
#if !defined(QUASAR_U535) && !CONSOLE_ON_CDC    → 擴充 UART（現為 u5a5 的退路）
```

`AT_CONSOLE_ON_STLINK` 的**預設值兩邊不同**：unidir 依 `U535_PWR_LDO` 決定，
headset 固定為 0。所以 headset 在 u535 兩種變體上都走擴充埠（LPUART1）。
headset 另有一條分支：`AT_CONSOLE_ON_STLINK && QUASAR_U535` 時 stats **直接靜音**，
把 VCP 讓給 AT，也避免 stats 突發干擾 RX 量測。

---

## 5. CDC console 會保留開機訊息

`common_backend.c` 的 weak 版在 `tud_cdc_connected()` 為 false 時**直接丟棄**字串。
開機 banner 在 reset 後幾毫秒就印出來，USB 列舉卻要幾百毫秒——所以最有價值的那段
（**說明板子上是哪顆 binary** 的那行）永遠到不了終端機，而一兩秒後的訊息卻好好地到了。
一份從半路開始的 log，看起來就像一塊從半路開始跑的板子。

unidir 的 CDC 版因此把列舉前的輸出存進 `s_boot_log[3072]`，開埠後第一次寫入時依序倒出。
**溢位時丟掉的是最新的**而不是最舊的——與一般 ring buffer 相反且刻意如此：早期那幾行
沒有別的方法取得，後面的訊息下一輪定期輸出還會再印。

判斷一顆 binary 是否走 CDC，看符號即可：

```
s_boot_log / s_boot_log_len / s_cdc_flushed   有 → CDC
stlink_ready                                  有 → u535 的 LPUART1 分支
兩者皆無                                       → 擴充 UART
```

---

## 6. 已出貨的 v240 套件

`SPARK_Audio_Puretone_Gen2_SDK_v240_9b84312` 早於 `36847a5`，所以**客戶手上那顆 u5a5
的 console 仍在擴充 UART**，要外接轉接器才讀得到。此改動要到下一包才會出去。

u535 完全未受影響——實測其 binary 與出貨版仍只差編進去的 `__DATE__`/`__TIME__`（8–10 bytes）。
