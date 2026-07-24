# Boot Auto-Reconnect — Design Note

**Branch:** `feat-boot-auto-reconnect` (base: `release-unify-boards`)
**Scope:** puretone headset (HS / node) **and** dongle (DG / coordinator)
**Status:** implemented 2026-07-22 (all 6 work items). Builds clean on u535 and
u5a5 (both puretone_headset.elf + puretone_dongle.elf). See "Implementation" at
the end for what was built and where it deviated from this note.

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
- DG status color for reconnect = **green** (per the `#else` branch).

### 6.1 Reserve **per-chip, not per-role** (flash geometry confirmed 2026-07-22)

Both roles (HS/DG) build for **both** chips — `puretone-headset-quasar-u535-*`
and `puretone-headset-quasar-u5a5-*` presets exist, and one preset builds
`puretone_headset.elf` **and** `puretone_dongle.elf` from the same board. So the
reserved region is a property of the **chip**, not the role. Reserve the last
page in **each linker script** and expose `_user_data_base`; the app references
that symbol, so one code path lands on the right address for whichever chip it
was compiled for. Role never touches the address.

| | HS board (u535) | DG board (u5a5) |
|---|---|---|
| Flash total | 512 KB | **4096 KB** |
| Bank layout | dual-bank, bank = 256 KB | dual-bank, bank = **2 MB** (`0x200000`) |
| Page size | **8 KB (`0x2000`)** | **8 KB (`0x2000`)** — same |
| Reserved last page | `0x0807E000`–`0x0807FFFF` | **`0x081FE000`–`0x081FFFFF`** |

- **Size is identical (8 KB), address is not** — the two flashes differ 8×, so
  each chip's "last page" sits at a different absolute address. Forcing a common
  address would carve a hole in the middle of u5a5's 4 MB code region; not worth
  it. `_user_data_base` per linker keeps the C code chip-agnostic.
- `FLASH_PAGE_SIZE == 0x2000` on both chips (verified in
  `cmsis_device_u5/.../stm32u535xx.h` and `stm32u5a5xx.h`).
- **Driver needs no change**: `quasar_memory.c` exists in **both**
  `bsp/quasar-u535/` and `bsp/quasar/`, and computes bank/page from the address
  via `FLASH_PAGE_SIZE`/`FLASH_BANK_SIZE` HAL macros (incl. bank-2).

---

## 7. Work breakdown (resume here)

1. **NV record module** (`reconnect_store.[ch]` or similar): magic+version+CRC on
   top of `quasar_memory_*`; `save(addr)` / `load(addr)->bool` / `clear()`.
   Handle 16-byte padding, erase-before-write, cache invalidate.
2. **Linker**: reserve last 8 KB page **per chip** (§6.1); expose
   `_user_data_base`.
   - `bsp/quasar-u535/GCC/STM32U535xx_FLASH.ld`: `512K → 504K`;
     `_user_data_base = 0x0807E000;`
   - `bsp/quasar/GCC/STM32U5A5AJHXQ_FLASH.ld`: `4096K → 4088K`;
     `_user_data_base = 0x081FE000;`
   - ⚠️ `CLANG_*_FLASH.ld` variants exist alongside both. pixi is arm-gcc
     (POST_BUILD uses `arm-none-eabi-objcopy`), so GCC scripts suffice now; sync
     the clang scripts too if the toolchain ever switches, or `_user_data_base`
     is undefined.
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
- Timeout: **3 s → pairing** (`RECONNECT_TIMEOUT_MS`; tuned down from 5 s on
  2026-07-22 after HW confirmed reconnect works — 5 s was an unnecessarily long
  wait before falling back to pairing). Elsewhere in this note "5 s" reflects the
  original design value.
- LED: **fast blink × 5** on the board status color (blue HS / green DG).
- Timeout keeps the flash record: **yes** (recommended default; revisit if undesired).
- **Fast-blink period: 100 ms** (settled 2026-07-22).
- **DG flash geometry: u5a5 = 4 MB / dual-bank / 8 KB page → reserve `0x081FE000`**
  (settled 2026-07-22, §6.1). Reserve is per-chip, not per-role.

---

## 8. Implementation (done 2026-07-22)

Files touched:

| Item | Where |
|---|---|
| Linker reserve (§6.1) | `bsp/quasar-u535/GCC/STM32U535xx_FLASH.ld` (504K, `_user_data_base=0x0807E000`); `bsp/quasar/GCC/STM32U5A5AJHXQ_FLASH.ld` (4088K, `_user_data_base=0x081FE000`). Verified with `nm`: symbol lands exactly on each address. CLANG_*.ld left untouched (arm-gcc toolchain). |
| Raw NV facade | `facade_nv_read/write/erase` declared in `puretone_headset_facade.h`, implemented in **new** `backend/quasar_backend/puretone_headset_backend/puretone_headset_nv_backend.c` (wraps quasar_memory + `_user_data_base`; erase-before-write, cache invalidate, 16-byte multiple guard). |
| Record module | **new** `app/example/puretone_headset/reconnect_store.[ch]` — 16-byte record (magic `RCON` + version + pad + `pairing_assigned_address_t` + CRC32), bitwise CRC32, `load/save/clear`. Compiled into both elfs. |
| LED | `facade_notify_reconnecting()` (100 ms x5, blue u535 / green else) in `puretone_headset_backend.c`. |
| Boot SM + hooks | `try_boot_reconnect()` + save-on-success + clear-on-unpair in both `puretone_headset.c` and `puretone_dongle.c`. |

Deviations from the design above (all deliberate):

1. **Link detection is NOT `at_get_link_status()`.** That function only returns
   `device_pairing_state == DEVICE_PAIRED`, which `at_start_connect()` sets the
   instant the local core comes up — it never reflects whether the *peer* is
   reachable, so it would make the 5 s poll succeed immediately. Instead the poll
   uses `swc_connection_get_connect_status()` on the same connection each app's
   `link_watch()` already treats as the live OK/LOST signal: **node → `rx_audio_conn`,
   coordinator → `tx_audio_conn`**.
2. **Layered instead of one module on raw quasar_memory.** The app never includes
   bsp headers, and `_user_data_base` is a per-board linker symbol. So the record
   format lives app-side (`reconnect_store`, board-agnostic, byte-in/out) and the
   flash access lives in the backend (`facade_nv_*`, which owns quasar_memory and
   the linker symbol). Clean split along the existing facade boundary.
3. **Coordinator rebuilds its discovery list from the 4-byte record.** The DG's
   `app_swc_core_init()` reads local/remote node addresses from
   `pairing_discovery_list[]`, not from `pairing_assigned_address`. On reconnect we
   set `discovery_list[COORDINATOR].node_address = addr.coordinator_address` and
   `discovery_list[NODE].node_address = addr.node_address` (these are equal by
   construction), so the same 4-byte record serves both roles.

Open risk to check on hardware (not a build issue):
- Whether `swc_connection_get_connect_status(tx_audio_conn)` on the **coordinator**
  goes true only after the node ACKs (peer really present) or as soon as the
  master's own core connects. If the latter, the DG's reconnect "succeeds"
  immediately even with no node — harmless (link_watch then shows LOST and the
  user re-pairs) but means the 5 s peer-presence gate is effectively node-side
  only. If undesired, switch the DG poll to RX-count-based presence
  (`rx_audio_conn` `packet_successfully_received_count > 0`).

---

## 9. Persistence scope: survives power-cycle, NOT reflash (decided 2026-07-22)

The record lives in the reserved flash page, so it **survives a power-cycle** —
which is the entire point of the feature (end users only ever power the device
off/on). It does **not** survive a **reflash**: a normal full-chip program
(ST-Link / CubeProgrammer default = mass-erase) wipes the reserved page along
with everything else, so a reflashed device boots with a blank record and falls
through to pairing.

**Decision: this is fine — do NOT make reflash preserve the record.** Reasoning:

- **Reflash is a developer / factory action, never an end-user one.** The one
  scenario the feature exists for (user power-cycle) already works.
- **Factory flow is "flash first, pair later."** At flash time there is nothing
  to preserve — the unit is not yet paired.
- **The only case where reflash-preservation would matter is field OTA update
  without re-pairing — and there is no OTA path here** (updates = attach a
  debugger and reflash, itself an engineering action). Revisit only if a real
  OTA/bootloader path is added; that flow would be page-preserving by design and
  is a separate mechanism from today's mass-erase.
- **Cost vs. benefit is lopsided.** Preserving across reflash needs
  page-preserving programming (per-sector erase excluding the last page, or
  disabling mass-erase / specifying a range in the flash tool) — tool-dependent
  and fragile. During development a stale record after reflash is actively
  *harmful* (misleading state); a clean boot-to-pairing is what you want. The
  only thing preserving saves is one button-press re-pair after each flash.

**Test procedure implied by this:** flash the firmware once on **both** ends,
pair once (writes the record), then **power-cycle only — do not reflash between
runs** — to exercise auto-reconnect. Reflashing mid-test clears the record and
(correctly) drops back to pairing.

Format evolution is safe regardless: the record carries `version` + CRC32, so a
new firmware reading an old-format record sees a version/CRC mismatch, treats it
as "no record," and falls through to pairing.

---

## 10. Pairing button during the reconnect window (fixed 2026-07-24)

**Bug.** `try_boot_reconnect()` sets `device_pairing_state = DEVICE_PAIRED`
*before* the poll loop, and the loop calls `facade_button_handling()` /
`at_cmd_core_process()`. `facade_button_handling()` dispatches synchronously, so
a pairing-button press inside the window ran `pairing_button_callback()` ->
`DEVICE_PAIRED` -> `unpair_device()`, which NULLs `rx_audio_conn` /
`tx_audio_conn` **while the loop is still running**. The next pass then called
`swc_connection_get_connect_status(NULL, ...)` — and that API dereferences
`conn->wps_conn_handle` with no NULL check (`swc_api.c`, unlike every other
`swc_connection_*` entry point) -> HardFault. Second, weaker path: even without
the NULL deref, `unpair_device()` runs `swc_disconnect()` on a link that never
synced, which can return `SWC_ERR_DISCONNECT_TIMEOUT` and trap in
`ASSERT_SWC_STATUS` -> `swc_error_handler()`'s `while(1)`. `AT+PAIR` /
`AT+DISCONNECT` reached the same teardown through `at_start_pairing()` /
`at_start_disconnect()`.

**Fix — defer, don't disable.** Dead-disabling the button for 3 s is wrong UX:
a press during reconnect almost always means "stop waiting for the old peer, let
me pair a new one." So the handlers now only *raise a flag*:

- `s_boot_reconnect_active` / `s_boot_reconnect_abort` (both `volatile`, HS+DG).
- `pairing_button_callback()`, `at_start_pairing()`, `at_start_disconnect()`
  return early with `s_boot_reconnect_abort = true` while active — no SWC or
  flash access from inside the loop.
- The loop breaks on the flag (and, belt-and-braces, on the connection handle
  going NULL), clears `s_boot_reconnect_active`, then does the teardown itself
  through the already-proven timeout path `at_start_disconnect()`, and returns
  false so `main()` enters pairing mode.

Net behaviour: **press pairing during boot reconnect -> reconnect aborts
immediately and the device enters pairing mode.** The flash record is *not*
erased by the abort (and on DG the discovery list is not wiped) — it is
overwritten when the new pairing succeeds, matching the "record kept on
timeout" rule in §4.

**Not changed:** the missing `conn == NULL` guard inside
`swc_connection_get_connect_status()` is a SPARK core defect. Left untouched to
avoid diverging from vendor code; the app-side NULL check covers us. Add it to
the HQ report list.
