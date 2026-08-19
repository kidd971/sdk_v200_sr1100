# 配對身分與預配對規格 —— 有哪些 ID、怎麼產生、要寫什麼

> 對象：產線治具設計者、ODM 端對接 AT 介面的 MCU 韌體、以及之後把 AT 層搬到
> `puretone_unidirectional` 的人。
> 目的：把「配對到底在交換什麼」一次講完 —— 每一個 ID 多長、由誰產生、
> 存不存進 flash、以及**有線預配對要寫的是哪幾個 byte**。
> 對應 code：`module/spark_pairing/`、`app/example/puretone_headset/reconnect_store.c`、
> `backend/quasar_backend/puretone_headset_backend/puretone_headset_nv_backend.c`。
> 基準：`v240-dev`（SDK v2.4.0-rc2）。
> 相關：[boot_auto_reconnect_design.md](boot_auto_reconnect_design.md)（開機自動回連）。

---

## 0. 一句話

**一對裝置的全部身分只有 4 個 byte**，而且它們是兩顆 radio 序號的**純函數** ——
所以預配對完全可以離線算出來，不需要跑空中配對，也不需要任何中央配號機制。

---

## 1. 到底有幾個 ID

| # | 名稱 | 長度 | 誰產生 | 存進 flash？ | 上空中？ |
|---|---|---|---|---|---|
| 1 | `unique_id`（radio serial number） | **64 bit** | 晶片出廠燒錄，`swc_node_get_radio_serial_number()` 讀出 | ❌ | 配對時 node → coord 送一次 |
| 2 | `app_code` | **64 bit** | 編譯期常數 `PAIRING_APP_CODE = 0x0000000000000999` | ❌（在 code 裡） | 配對時 coord → node 送一次 |
| 3 | `generated_address` | 23 bit | `unique_id` 的 CRC 雜湊，中間產物 | ❌ | ❌ |
| 4 | `pan_id` | **15 bit** | 從 #3 切出來（coord 的） | ✅ | ✅ |
| 5 | `coordinator_address` | **8 bit** | 從 #3 切出來（coord 的） | ✅ | ✅ |
| 6 | `node_address` | **8 bit** | 從 #3 切出來（node 的），撞號時遞增 | ✅ | ✅ |

**落地保存的只有 #4 #5 #6**，就是 `pairing_assigned_address_t`
（[pairing_def.h:30](module/spark_pairing/api/pairing_def.h#L30)）：

```c
typedef struct pairing_assigned_address {
    uint16_t pan_id;               /* 2 bytes，實際只用 15 bit */
    uint8_t  coordinator_address;  /* 1 byte */
    uint8_t  node_address;         /* 1 byte */
} pairing_assigned_address_t;      /* = 4 bytes */
```

`unique_id` **不落地、也不參與連線**。它只在配對搜尋階段用來產生位址，產生完就不再需要
—— 這一點是預配對可行性的關鍵，見 §5。

### 1.1 另外還有一組「配對階段專用」的臨時位址

配對程序自己也要能通訊，所以它先用一組固定值把臨時連線建起來
（[pairing_wireless_cfg.h](module/spark_pairing/wireless/pairing_wireless_cfg_sr1100/pairing_wireless_cfg.h)）：

```c
#define PAIRING_PAN_ID        SWC_RESERVED_PAN_ID   /* 0x0000 */
#define PAIRING_COORD_ADDRESS 0x01
#define PAIRING_NODE_ADDRESS  0x02
```

**PAN 0x0000 是保留給配對用的**，正常連線永遠不會用到它（見 §2.4 的 reserved 檢查）。
所以「兩台都在配對模式」時它們必定在同一個 PAN 上，這是設計，不是巧合。

### 1.2 認證模式的例外

`certification_mode != FACADE_CERTIF_NONE` 時，位址被硬寫成固定值，完全跳過配對
（[puretone_dongle.c:569](app/example/puretone_headset/puretone_dongle.c#L569)）：

```c
app_pairing->pan_id = 0xABC;
remote_address = 0x2;
local_address  = 0x1;
```

認證用的板子因此永遠互通，也因此**認證 binary 不能拿來做預配對驗證**。

---

## 2. 位址怎麼產生

### 2.1 入口

Coordinator 進入配對時（[pairing_state_coordinator.c:124](module/spark_pairing/state/pairing_state_coordinator.c#L124)）：

```c
unique_id         = pairing_wireless_get_radio_serial_number();
generated_address = pairing_address_generate_serialized_address(unique_id);
pan_id              = EXTRACT_PAN_ID(generated_address);         /* (x >> 8) & 0x7FFF */
coordinator_address = EXTRACT_DEVICE_ADDRESS(generated_address); /*  x       & 0xFF   */
```

Node 的位址由 **coordinator 算**，用的是 node 在 identification 訊息裡送過來的 `unique_id`
（[pairing_state_coordinator.c:222](module/spark_pairing/state/pairing_state_coordinator.c#L222)）：

```c
generated_address = pairing_address_generate_serialized_address(identification.unique_id);
node_address      = EXTRACT_DEVICE_ADDRESS(generated_address);
node_address      = pairing_address_get_available_node_id(node_address);  /* 撞號則遞增 */
```

所以：**pan_id 和 coordinator_address 來自 DG 的序號，node_address 來自 HS 的序號。**

### 2.2 23 bit 是怎麼切的

```
generated_address (23 bit)
  bit 22..16  SFD      (7 bit) ─┐
  bit 15..8   network  (8 bit) ─┴─→ pan_id (15 bit)
  bit  7..0   address  (8 bit) ───→ coordinator_address / node_address
```

### 2.3 雜湊本體

[pairing_address.c:84](module/spark_pairing/communication/pairing_address.c#L84)：

```c
#define GENERATE_SERIALIZED_LEN              4
#define GENERATE_SERIALIZED_CRC_POLY         0x1021
#define GENERATE_SERIALIZED_CRC_CCITT_RELOAD 0xFFFFFFFF

uint32_t pairing_address_generate_serialized_address(uint64_t seed)
{
    uint8_t byte_array[8] = {0};
    uint32_t result = GENERATE_SERIALIZED_CRC_CCITT_RELOAD;
    uint32_t crc;

    do {
        memcpy(byte_array, &seed, sizeof(uint64_t));

        for (uint8_t i = 0; i < GENERATE_SERIALIZED_LEN; i++) {   /* 只吃 4 個 byte */
            crc = result;
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x80000) {                              /* ← 注意：bit 19 */
                    crc = (crc << 1) ^ GENERATE_SERIALIZED_CRC_POLY;
                } else {
                    crc <<= 1;
                }
            }
            result = crc ^ byte_array[i];
        }

        result = result & 0x7FFFFF;   /* 只留 23 bit */
        seed += 1;                    /* 撞到保留位址就換一顆 seed 重算 */

    } while (pairing_address_is_address_reserved(result));

    return result;
}
```

三個必須照抄、不能照描述重寫的細節：

1. **只吃 `unique_id` 的低 4 個 byte**（`GENERATE_SERIALIZED_LEN = 4`），高 32 bit 不參與。
2. **`if (crc & 0x80000)` 判的是 bit 19，不是 MSB**，儘管註解寫著 "Most significant bit"。
   這是 SPARK 的實作原樣，是它就是它 —— 治具要**逐行移植這個迴圈**，不要拿標準
   CRC-CCITT 函式庫代替，結果會不一樣。
3. **retry 迴圈**：`seed += 1` 在迴圈內，且 `memcpy` 也在迴圈內，所以每次重試是拿
   `unique_id + 1`、`unique_id + 2` … 重新雜湊，不是把結果加一。

### 2.4 保留位址檢查

[pairing_address.c:115](module/spark_pairing/communication/pairing_address.c#L115)，任何一項成立就重算：

| 欄位 | 不可為 |
|---|---|
| SFD (byte 2) | `0x00` |
| network (byte 1) | `0x00`、`0xFF` |
| address (byte 0) | `0x00`、`0xFF` |

`network == 0x00 && SFD == 0x00` 會讓 `pan_id` 變成 `0x0000`，那是配對專用 PAN；
`address` 的 `0x00` / `0xFF` 是 SWC 的廣播／保留位址。

### 2.5 node 撞號遞增

[pairing_address.c:134](module/spark_pairing/communication/pairing_address.c#L134)：算出的
`node_address` 若已存在於 discovery list（兩機網路裡就是「等於 coordinator_address」），
就 `+1`，到 `0xFE` 後回捲到 `0x01`，直到找到沒被佔用的。

兩機網路踩到的機率是 1/254，但**治具必須實作它**，否則那 1/254 的機台算出來的
`node_address` 會跟韌體配對出來的不一致。

---

## 3. 配對交換了什麼（三個往返）

跑在 PAN `0x0000` / 位址 `0x01`↔`0x02` 的臨時連線上，payload 上限 16 bytes
（[pairing_message.h](module/spark_pairing/communication/pairing_message.h)）：

| 階段 | 方向 | 內容 | 用途 |
|---|---|---|---|
| Authentication | coord → node | `app_code` (u64) | 產品碼比對，擋掉別家／別型號的裝置 |
| Identification | node → coord | `device_role` (u8) + `unique_id` (u64) | node 交出序號讓 coord 幫它算位址 |
| Addressing | coord → node | `pan_id` (u16) + `coordinator_id` (u8) + `node_id` (u8) | **就是那 4 個 byte，發下去** |

Node 收到 addressing 訊息就是照抄
（[pairing_state_node.c:260](module/spark_pairing/state/pairing_state_node.c#L260)）：

```c
pairing_address_set_pan_id(pairing_addressing_message.pan_id);
pairing_address_set_coordinator_address(pairing_addressing_message.coordinator_id);
pairing_address_set_node_address(pairing_addressing_message.node_id);
```

**兩端最後持有的 `pairing_assigned_address_t` 完全相同**，沒有任何一邊多存什麼。

---

## 4. 落地：flash 上長什麼樣

[reconnect_store.c](app/example/puretone_headset/reconnect_store.c) 的記錄剛好 16 bytes
（= 一個 flash quad-word，一次對齊寫入、無尾端 padding）：

| offset | 長度 | 欄位 | 值 |
|---|---|---|---|
| 0 | 4 | `magic` | `0x4E4F4352`（'RCON' little-endian） |
| 4 | 2 | `version` | `1` |
| 6 | 2 | `reserved` | `0`（寫 0） |
| 8 | 2 | `addr.pan_id` | ← |
| 10 | 1 | `addr.coordinator_address` | ← |
| 11 | 1 | `addr.node_address` | ← |
| 12 | 4 | `crc32` | 前 12 bytes 的 CRC32 |

CRC32 是標準的：poly `0xEDB88320`（reflected）、init／final `0xFFFFFFFF`。
逐 bit 計算，無查表 —— 12 bytes 而已，速度無所謂。

空白 flash 讀出來是 `0xFFFFFFFF`，magic 檢查自然不過，所以**出廠未寫過的機器天然
回報「沒有記錄」**，不需要額外的 provisioned 旗標。

### 4.1 記錄放在哪

保留 flash 最後一個 8 KB page，linker script 把 FLASH 區段縮短讓 code 永遠不會放進去：

| 板子 | `_user_data_base` | FLASH 區段 |
|---|---|---|
| U5A5 | `0x081FE000` | 4096K → **4088K** |
| U535 | `0x0807E000` | 512K → **504K** |

存取走 `facade_nv_read` / `facade_nv_write` / `facade_nv_erase`
（[puretone_headset_nv_backend.c](backend/quasar_backend/puretone_headset_backend/puretone_headset_nv_backend.c)），
它處理三件 flash 的細節：寫前先 erase、quad-word（16 byte）寫入粒度、寫完 invalidate I-cache。

---

## 5. 預配對（有線寫入）

### 5.1 為什麼可行

開機自動回連的路徑
（[puretone_dongle.c:2456](app/example/puretone_headset/puretone_dongle.c#L2456) `try_boot_reconnect()`）
已經在做「不跑配對、只靠 flash 裡的 4 bytes 把連線建起來」：

```
reconnect_store_load(&pairing_assigned_address)        ← 4 bytes 從 flash
  → pairing_discovery_list[COORD].node_address = coordinator_address
    pairing_discovery_list[NODE ].node_address = node_address
  → app_swc_core_init(&pairing_assigned_address)
        swc_cfg_t.pan_id = pan_id
        swc_node_cfg_t.coordinator_address / local_address / remote_address
  → 連上
```

`unique_id` 一次都沒被用到。所以**預配對和開機自動回連是同一條路徑**，
唯一差別是那 4 bytes 是誰寫進 flash 的：配對成功寫的，還是產線治具寫的。

### 5.2 治具要做的事

```
1. 讀 DG 的 radio serial number            → unique_id_DG
2. 讀 HS 的 radio serial number            → unique_id_HS
3. h_DG = generate_serialized_address(unique_id_DG)      ← §2.3 逐行移植
   pan_id              = (h_DG >> 8) & 0x7FFF
   coordinator_address =  h_DG       & 0xFF
4. h_HS = generate_serialized_address(unique_id_HS)
   node_address        =  h_HS       & 0xFF
   if (node_address == coordinator_address) → §2.5 遞增
5. 把同一組 {pan_id, coordinator_address, node_address}
   分別寫進 DG 和 HS 各自的 flash
```

結果跟跑一次空中配對**完全相同**。唯一性由序號天然保證，不需要任何配號機制。

### 5.3 序號怎麼讀

韌體內是 `swc_node_get_radio_serial_number()`。治具端目前**沒有取得它的管道** ——
現有 AT 指令沒有一條會吐出 radio serial。這是 §6 的工作項之一。

（另一條路：**讓模組自己算**。治具只下「請把位址設成這組」的指令，序號和雜湊都留在韌體內，
治具只負責把 DG 算出來的結果轉貼給 HS。這樣 §2.3 就不必在治具端重寫一次，
少一個會不一致的地方 —— **建議走這條**。）

### 5.4 目前還沒有的：AT 指令

`at_cmd_core` 現在**沒有**任何一條指令能讀寫這個記錄。已有的 `AT+UWB_PAIR`
（rc08 上是 `AT+LE_UWB_PAIR`）是「跑一次空中配對程序」，不是「寫入位址」。

需要新增的，大致是：

```
AT+LE_UWB_SET_PAIR=<pan_id>,<coord_addr>,<node_addr>
    → 填 pairing_assigned_address_t → reconnect_store_save() → OK
AT+LE_UWB_GET_PAIR?
    → reconnect_store_load() → 回三個數（產線複檢）
AT+LE_UWB_SERIAL?
    → swc_node_get_radio_serial_number()（走 §5.2 治具自算路線才需要）
```

這些走的是**本地 AT 指令**，不上空中，所以不受 vendor pass-through 的 8-byte payload
上限拘束（`AT_LINE_MAX_LEN = 256`）。連線那半邊**一行都不必寫** —— 下次開機
`try_boot_reconnect()` 自己會撿起來。

---

## 6. 待辦與注意事項

1. **AT 指令尚未實作**（§5.4）。排在 rc08 的 AT vendor pass-through 合進 `v240-dev` 之後，
   避免同一份 `at_cmd_core.c` 兩邊各改一次。
2. **`reconnect_store` 目前只在 `puretone_headset`**，`puretone_unidirectional` 沒有。
   AT 層搬到 unidir 時它要一起搬（`reconnect_store.c` + `facade_nv_*` backend + linker
   script 的保留頁；後兩者是共用的，只差 app 端）。
3. **治具若自算雜湊，§2.3 的三個細節必須逐行移植**，特別是 `crc & 0x80000`。
   建議寫一支對拍測試：同一顆序號，韌體配對出來的位址 vs 治具算出來的，必須一致。
4. **認證 binary 不能用來驗預配對**（§1.2 會蓋掉位址）。
5. **`app_code` 目前是 `0x999`**，全產品線共用一個值。要區分型號的話它是唯一的旋鈕，
   但改動它等於讓新舊韌體配不起來 —— 屬於出貨前要定死的東西。
6. **`pan_id` 只有 15 bit 而且來自雜湊**，理論上兩對裝置可能撞 PAN。撞了會怎樣沒測過：
   同 PAN 不同 address 在 SWC 上是否互不干擾，需要向 SPARK 確認。實務上機率極低，
   但預配對是產線大量寫入，跟現場一對一配對的統計條件不同，值得問一次。
