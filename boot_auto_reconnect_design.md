# Boot Auto-Reconnect — Design Note

**Branch:** `feat-boot-auto-reconnect` (base: `release-unify-boards`)
**Scope:** puretone headset (HS / node) **and** dongle (DG / coordinator)
**Status:** design agreed, not yet implemented. Resume from "Work Breakdown" below.

---

## 1. Goal

On power-up, a device that was previously paired should **re-establish its old
link automatically**, without the user re-running the pairing procedure. If the
peer is not reachable, fall back to normal pairing after a **5-second timeout**.

Today ([puretone_headset.c:494-495](app/example/puretone_headset/puretone_headset.c#L494))
the node unconditionally calls `enter_pairing_mode()` on every boot — there is
no persistence, so a reboot always forces a fresh pairing.

---

## 2. Key insight — the reconnect path already exists

A completed pairing produces **only 4 bytes** of per-pair state
([pairing_def.h](core/wireless/pairing/api/pairing_def.h)):

```c
typedef struct {
    uint16_t pan_id;
    uint8_t  coordinator_address;
    uint8_t  node_address;
} pairing_assigned_address_t;
```

Everything else (channels, timeslots, modulation, app code) is compile-time
constant. And [`at_start_connect()`](app/example/puretone_headset/puretone_headset.c#L2550)
already turns those 4 bytes straight into a live link:

```c
static void at_start_connect(void) {
    if (device_pairing_state == DEVICE_PAIRED) return;
    if (pairing_assigned_address.node_address == 0) return;   // only gate
    app_init();                          // builds SWC from stored addrs + swc_connect()
    device_pairing_state = DEVICE_PAIRED;
}
```

So the missing piece is **not** connection logic — it is that
`pairing_assigned_address` lives in RAM and is zeroed on every boot. Persist it
to flash and read it back at boot, and reconnect works.

---

## 3. Flash storage

### 3.1 Driver — already present, unused

[`quasar_memory.c`](bsp/quasar-u535/quasar_memory.c) already provides
`quasar_memory_read / write / erase / invalidate_cache` over the STM32U5 HAL.
**Nothing in the app calls it yet.** Gotchas to wrap:

- Programming granularity is **quad-word (16 bytes)**; the write helper reads a
  full 16 bytes even for the tail → **pad the record to a multiple of 16 bytes**.
- STM32U5 ECC: each quad-word can be programmed **once** after erase → save =
  `erase page` then `write`.
- Call `quasar_memory_invalidate_cache()` (ICACHE) after writing.
- `quasar_memory_write` truncates `buffer_size` to `uint16_t` — fine for a small
  record, but keep the record well under 64 KB.

### 3.2 Reserved region — the gap the user flagged

The u535 linker ([STM32U535xx_FLASH.ld](bsp/quasar-u535/GCC/STM32U535xx_FLASH.ld#L45-L50))
gives the whole 512 KB to `FLASH` with **no reserved page**.

- U535 = 512 KB, dual-bank, **8 KB page**. FLASH currently ~30 % used (~160 KB).
- Reserve the **last page**: `0x0807E000`–`0x0807FFFF` (8 KB).
  - Shorten `FLASH` LENGTH 512K → 504K.
  - Add symbol `_user_data_base = 0x0807E000;` for the app to reference.
- `quasar_memory_erase()` already computes bank/page from the address and
  handles bank-2, so the last page works with no driver change.
- **DG board may differ** (dongle is not U535 — see §6). Reserve the equivalent
  last page in the dongle's linker script too.

### 3.3 Record format

```c
typedef struct {
    uint32_t magic;                       // e.g. 'RCON' — presence check
    uint16_t version;                     // format version
    uint16_t _pad;
    pairing_assigned_address_t addr;      // the 4 persisted bytes
    uint32_t crc32;                       // integrity
    // pad to a multiple of 16 bytes
} reconnect_record_t;
```

Blank flash reads as `0xFF…` → `magic` mismatch → treated as "no record", so a
never-written device naturally falls through to pairing.

---

## 4. Boot / reconnect state machine (HS and DG symmetric)

```
boot
 ├─ valid record in flash?
 │    ├─ yes → facade_notify_reconnecting()          // blue(HS)/green(DG) fast blink x5
 │    │        load addr into pairing_assigned_address
 │    │        app_init() + swc_connect()
 │    │        poll at_get_link_status() up to 5 s
 │    │          ├─ link up  → facade_notify_pairing_successful()   // solid
 │    │          └─ timeout  → swc_disconnect() → enter_pairing_mode()  // slow blink x2
 │    └─ no  → enter_pairing_mode()                  // first boot, unchanged
```

- **Link detection:** reuse [`at_get_link_status()`](app/example/puretone_headset/puretone_headset.c#L413)
  (returns `bool`); no new detection code.
- **5 s timeout:** poll on `facade_get_tick_ms()`, same idiom as the existing
  periodic timers in `main()`.
- **On timeout: KEEP the flash record** (do not erase). The peer being off is not
  a reason to forget the pair; the record stays valid for the next boot or a
  manual AT `connect`. We only enter pairing-mode UI as a fallback.

### Save / clear triggers

| Event | Action |
|---|---|
| `PAIRING_EVENT_SUCCESS` in [`enter_pairing_mode()`](app/example/puretone_headset/puretone_headset.c#L2337) | write record to flash (after `app_init()`) |
| [`unpair_device()`](app/example/puretone_headset/puretone_headset.c#L2358) | **erase** record (else next boot reconnects to the device the user just unpaired) |

Certification-mode path ([app_swc_core_init](app/example/puretone_headset/puretone_headset.c#L557))
is unaffected — it hard-codes its own addresses.

---

## 5. LED indication (single blue status channel on HS)

On the u535 headset, RGB **green (PB4) and red (PH11) are borrowed as TX/RX
activity LEDs** ([quasar_def.h:144-163](bsp/quasar-u535/quasar_def.h#L144)); only
**blue (PD7)** remains as a status color. That is why
`facade_notify_pairing_successful()` is `#ifdef QUASAR_U535 → BLUE`.

Existing blue semantics, all on the one channel, distinguished by cadence:

| State | Function | Blue behaviour |
|---|---|---|
| Enter pairing | `facade_notify_enter_pairing` | slow blink 250 ms × 2 |
| Paired / connected | `facade_notify_pairing_successful` | **solid on** |
| Not paired (timeout/abort) | `facade_notify_not_paired` | one 250 ms flash |
| Fatal error | error handler | RGB **red** solid |

**New "reconnecting" state → fast blink × 5.** Fits the gap cleanly:

| State | New blue pattern |
|---|---|
| **Reconnecting** (new) | **fast blink ~100 ms × 5** (~1 s) |
| Reconnect success | solid (reuse `pairing_successful`) |
| Reconnect failed → pairing | slow blink 250 ms × 2 (reuse `enter_pairing`) |

Fast (100 ms) vs pairing's slow (250 ms), plus 5 vs 2 blinks, is distinguishable.

- Add `facade_notify_reconnecting()` (weak, blocking `quasar_rgb` blink like the
  existing notify fns — only fires at transitions, never in the audio loop).
- Follow the same `#ifdef QUASAR_U535` split: **HS → blue**, **DG → green**
  (DG's status color, from the `pairing_successful` `#else` branch). The facade
  hides the per-board color from the app.

---

## 6. DG (dongle / coordinator) side

Feature must be symmetric or the pair never re-links: if the HS remembers its
addresses but the DG re-pairs / re-assigns on boot, the two disagree.

- Same design applies to [puretone_dongle.c](app/example/puretone_headset/puretone_dongle.c)
  (notify calls at lines 2156 / 2182 / 2193 / 2243 mirror the HS).
- Confirm the DG board's flash page size / bank layout and reserve its own
  user-data page in the dongle linker script.
- DG status color for reconnect = **green** (per the `#else` branch).

---

## 7. Work breakdown (resume here)

1. **NV record module** (`reconnect_store.[ch]` or similar): magic+version+CRC on
   top of `quasar_memory_*`; `save(addr)` / `load(addr)->bool` / `clear()`.
   Handle 16-byte padding, erase-before-write, cache invalidate.
2. **Linker**: reserve last 8 KB page on HS (u535) **and** DG; expose
   `_user_data_base`.
3. **Save/clear hooks**: write on `PAIRING_EVENT_SUCCESS`, erase in
   `unpair_device()` — both HS and DG.
4. **Boot decision**: replace unconditional `enter_pairing_mode()` in `main()`
   with the §4 state machine (load → reconnect w/ 5 s poll → fallback), both apps.
5. **`facade_notify_reconnecting()`**: fast blue(HS)/green(DG) blink × 5, weak,
   in the backend facade.
6. **Build/verify** both presets in the mainline worktree
   (`D:/_SDK_v230_UART_at_command_git/sdk_v200_sr1100`, pixi toolchain).

### Effort / risk
- Small: ~1 NV module (~150 lines) + linker (3 lines/board) + a few edits each in
  main/pairing/unpair + one facade fn.
- Risk is in flash write timing (quad-word / ECC / cache) and the 5 s fallback —
  **not** in the link itself. No SWC-core changes.

### Open items settled
- Both HS + DG: **yes**.
- Timeout: **5 s → pairing**.
- LED: **fast blink × 5** on the board status color (blue HS / green DG).
- Timeout keeps the flash record: **yes** (recommended default; revisit if undesired).

### Still to confirm on resume
- Fast-blink period: 100 ms assumed (vs 60 ms).
- DG board flash geometry (page size / bank split) before reserving its page.
