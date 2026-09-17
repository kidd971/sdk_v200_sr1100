# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

> `CLAUDE.md` is a near-identical twin of this file — only the title and the tool named in the line
> above differ. Edit both, or the next agent to read the other one works from a stale map.

## What this is

A working fork of the **SPARK Microsystems SDK** (v2.4.x line, SR1100 UWB transceiver) carrying one
product: **`puretone_unidirectional`** — a 96 kHz / 24-bit stereo audio link from a **DG
(coordinator, transmitter)** to an **HS (node, receiver)**, with a 5-rung quality fallback ladder
and a bidirectional 10 ms data connection for control, telemetry and ODM vendor pass-through.

The I2S/SAC input is 96 kHz, but **the shipping air ceiling is rung 1, 48 kHz / 24-bit**: rung 0 is
deactivated because 96 kHz parks a dual-radio node, a fault that is still open.
`MAIN_CHANNEL_ALLOW_96K` defaults to `0` (`config/sac_cfg.h`) and both ends must agree on it. Do not
quote "96 kHz over the air" from the top of this file without checking that flag.

The stock SDK examples (`audio_*`, `puretone_headset`, `hello_world`, `star_network`, …) are still
present and still build, but they are not the deliverable. `puretone_headset` in particular is the
*previous* product line: its presets are hidden, and it makes **opposite** hardware choices from
`puretone_unidirectional` on the same boards (see the console/UART trap below).

Target boards are both STM32U5: **u5a5** (`bsp/quasar`, STM32U5A5, has a MAX98091 codec) and
**u535** (`bsp/quasar-u535`, STM32U535, no codec, two power variants LDO/SMPS). A port to a
third-party MCU (Airoha AB1595) is in progress on a sibling worktree — see `MD/` below.

## Build, flash, debug

Everything runs through **pixi** (vendored at `.pixi/bin/pixi.exe`), which supplies CMake 3.31.8,
`gcc-arm-none-eabi` 10.3, Ninja, pyocd and dfu-util. Do not expect a system toolchain to work.

```bash
# Build one preset (configure + build). PRESET is the configure-preset name.
PRESET=unidir-u5a5-rjf-single-radio ./.pixi/bin/pixi.exe run _build_preset

# Build every visible preset (skips hidden ones, and any named datacom/unit-tests)
./.pixi/bin/pixi.exe run build-all

# Flash over DFU (prompts for an .elf if no path given; .elf path is rewritten to .bin)
./.pixi/bin/pixi.exe run flash build/<preset>/app/example/puretone_unidirectional/puretone_unidirectional_node.elf
```

- Build tree: `build/<presetName>/`, and each app's artifacts land under
  `build/<preset>/app/example/<app>/`. A `.bin` is produced next to each `.elf` by a POST_BUILD
  `objcopy`.
- 79 configure presets, 11 of them visible; the rest are hidden bases or the withdrawn headset line.
- Add a one-off flag by appending it to the configure step, e.g.
  `cmake --preset=<p> -G Ninja -DSTATS_VERBOSE=1 && cmake --build build/<p>`.
- Interactive debug is VS Code + cortex-debug + pyocd (`.vscode/launch.json` has per-ST-Link-serial
  configurations for running several boards at once). `pyocd.yml` is regenerated on every pixi
  activation with this checkout's absolute pack paths — it is gitignored, don't edit or commit it.

### There are no runnable unit tests in this package

The root `CMakeLists.txt` has a `BUILD_TESTS` branch that pulls in `tests/` and `dev_tool/`, but
neither directory ships here and no preset sets `BUILD_TESTS`. Verification is on-hardware:

- `app/tool/bsp_validator` — the `bsp-validator-u535-*` presets; SPI / CS / reset / IRQ / DMA checks
  per radio. These plus `app/tool/profiler` are the two milestones a new-MCU port aims at first.
- `script/vendor_cmd_test.py` — drives `AT+VENDOR_CMD` across a real DG/HS pair and counts losses,
  duplicates and reorderings. Run it **with audio playing**; its `--rate 1` case is the diagnostic
  one (the fault it guards is per-event, not load-dependent).
- `tools/at_rx_stress.py` — measures AT RX corruption rate, using the firmware's own `+DBG OK:` echo
  as the oracle. Run it twice (UWB idle, then connected + audio) and compare; the delta is the
  pass/fail metric for the RX path. Its header records the TX/RX pin swap between boards.
- `unidir-u5a5-rjf-DG-sine` injects a 1 kHz tone so a signal path can be judged without a source. It
  is the only `*-sine` preset left visible; the u5a5 HS and both u535 sine presets are hidden, not
  deleted — unhide the one you need (or add `-DSINE_INJECT_DG=1` / `-DSINE_INJECT_HS=1` to a normal
  preset's configure line) rather than assuming the tone injection was removed.

## Preset and binary selection — the most common way to waste a bench session

A `puretone_unidirectional` preset normally builds **two** executables,
`puretone_unidirectional_coordinator.elf` (DG) and `puretone_unidirectional_node.elf` (HS). A
preset's description says which of the two it is *for*; the other one it produces is a by-product.

- On **u5a5** one preset serves both roles (`unidir-u5a5-rjf-single-radio` → flash the coordinator
  binary to the DG and the node binary to the HS).
- On **u535** the roles need different `I2S_MASTER_MODE` (DG = I2S slave, HS = I2S master), so DG
  and HS have **separate presets** (`unidir-u535-ldo-DG` vs `unidir-u535-ldo-HS-single-radio`).
  Taking both binaries from one u535 preset produces a silent link.
- The DG is **single-radio only** — the app's `CMakeLists.txt` skips the coordinator target entirely
  when `MULTI_TRANSCEIVER=DUAL_TRANSCEIVER`, so a `*-dual-radio` preset builds *one* executable.
  Build the DG half from the matching single-radio preset.
- LDO and SMPS are different u535 boards with different console wiring — `U535_PWR_LDO` is not a
  cosmetic flag. Note that the hidden `quasar-u535` base preset defaults it to `1`, so an SMPS
  preset must set it to `0` explicitly.

Flashing the wrong role, board or power variant does not fail; it presents as "no audio", which is
also what a dozen real faults look like. The boot banner is the disambiguator — it prints role,
`BOARD_NAME`, `RADIO_TAG`, version and `__DATE__`/`__TIME__` before anything can go wrong
(`puretone_unidirectional_coord.c:420`).

## Architecture

Five layers, top to bottom. The seam that matters is **facade ↔ backend**.

| Layer | Where | What |
|---|---|---|
| Application | `app/example/<app>/*.c` | `main()`, state machine, pipeline wiring. Portable C, no register access. |
| Facade (header only) | `app/example/<app>/facade/*.h` | The contract the app is allowed to ask the platform for. |
| Backend | `backend/quasar_backend/<app>_backend/` | The implementation of that contract for these boards. |
| BSP | `bsp/quasar`, `bsp/quasar-u535` | `quasar_*` peripheral drivers, `quasar_def.h` pin map. |
| Cores | `core/audio` (SAC), `core/wireless` (SWC) | Vendor audio and wireless stacks. |

The facade is a **compile-time** dependency only — no vtables, no runtime polymorphism. The app
calls `facade_*()`; CMake links exactly one backend that defines them. Porting to a new board means
writing a backend, not touching the app. Shared, app-independent code lives in `app/common/`
(`at_cmd_core`, `fw_version`, `link_data`, `reconnect_store`); reusable leaf code in `library/`
(`adpcm`, `at_module`, `queue`, `resampling`, …).

Which app gets built is the `APP` cache variable, and it is switched on in **two parallel lists**
that must stay in sync: `app/example/CMakeLists.txt` picks the app, `backend/quasar_backend/
CMakeLists.txt` picks its backend (`HARDWARE` selects `quasar_backend` in the first place). Adding
an app means editing both, in both the USB-audio branch and the normal branch of each.

`core/wireless` ships as **prebuilt `.a` files** (`core/wireless/prebuilt/`, selected by
transceiver / single-vs-dual / M4-vs-M33 / QSPI / stats-per-band). `RELEASE_MODE` defaults ON and
uses them; SPARK-internal source builds would set it OFF, which is not possible from this package.

### The unidirectional application

`puretone_unidirectional_coord.c` and `_node.c` are the two halves, ~2.5k lines each. Both follow
the same shape: `facade_board_init()` → `at_cmd_core_init()` (first, because on some boards the AT
UART *is* the console) → boot banner → PendSV handler and button callbacks bound → timers init →
`try_boot_reconnect()` → a `while(1)` of `facade_button_handling()` / `at_cmd_core_process()` /
stats / `wfi`.

Note what is *not* in that list: the radio and audio stacks. `app_init()` — SWC core, `swc_connect()`,
SAC core, `sac_pipeline_start()` — runs only once a peer identity exists, so it is reached from inside
`try_boot_reconnect()` or from a successful pairing, never from `main()` directly. A certification
build (`CERTIF_FORCE_MODE`) is the exception: it calls `app_init()` immediately with hard-coded
addresses and spins, bypassing pairing, flash and boot reconnect entirely — which is why a
certification binary cannot be used to test pre-provisioning.

`try_boot_reconnect()` returns three ways, and the third is the interesting one: `BOOT_RECONNECT_OK`
(stored link re-established), `BOOT_RECONNECT_PAIR` (no usable record → `enter_pairing_mode()`), and
`BOOT_RECONNECT_IDLE` — a record existed but the peer was not up within `RECONNECT_TIMEOUT_MS`
(10 s). **IDLE deliberately does not re-pair, on either side**: both leave the wireless core running
and wait, because a timed-out peer is transmitting a schedule rather than sitting in pairing mode, so
a device that answered a timeout by pairing could never meet it. Whichever box is switched on second
joins the first. The persisted address is 4 bytes in a reserved flash page behind
`app/common/reconnect_store/` (magic `'RCON'` + version + CRC32; a blank page reads 0xFF and
naturally fails as "no record", and a mass-erase reflash wipes it). Pairing itself is
`module/spark_pairing/`, with the app code and 10 s timeout in `config/pairing_cfg.h` — note that the
app code differs from the headset line's, so a unidir device and a headset device **cannot** pair,
and the only symptom is a pairing timeout.

Pairing is **blocking**: `pairing_process()` spins for up to 10 s, and its application callback is
the only thing servicing buttons and AT for that whole window. It reuses the SWC memory pool, so the
wireless core must be stopped first.

Two logical SWC connections run concurrently (three objects on the DG — audio TX, data TX, data RX):

- **Audio** — unidirectional DG→HS, carrying the SAC pipeline output.
- **Data** — bidirectional, every `DATA_TX_PERIOD_MS` (10 ms), carrying `user_data_t` from
  `app/common/link_data/puretone_link_data.h`. This doubles as the HS's beacon/sync source, which is
  why the period is capped at 10 ms.

**Init order is load-bearing, twice over.** The TX connections share the coordinator's 20 of 21
timeslots, and whichever is initialised first takes `AUDIO_CONNECTION_PRIORITY` — which is why
`app_swc_core_init()` carries two mirrored branches, the `FACADE_CERTIF_DATA` one building the *data*
connection first and handing it the audio priority (`puretone_unidirectional_coord.c:601`).
Reordering those calls silently re-prioritises the link. Separately, **SWC must be initialised before
SAC**: `app_audio_core_init()` stores raw `swc_connection_t *` into the SAC producer/consumer and
fallback instances, so `app_init()`'s order (`swc_core_init` → `swc_connect` → `audio_core_init` →
`sac_pipeline_start`) is not stylistic.

The **fallback ladder** (`config/sac_cfg.h`) is the heart of the audio behaviour: 5 rungs —
96k/24-bit → 48k/24-bit → 48k/16-bit → 48k ADPCM stereo → 24k ADPCM stereo — each with its own
sample count, latency target, payload size and SWC threshold. Several constants there are *derived*
macros and are commented "never hand-written" — the accumulator ratio, rung divisor and sample count
are coupled, and a value out of step with its neighbours is not a glitch but every packet on that
rung dropped. Payload sizes must descend monotonically or `swc_connection_set_fallback_cfg()` asserts
at init. The radio side lives in `config/sr1100/swc_cfg{,_coord,_node}.h`.

### Execution contexts

Little of the real work happens in the `while(1)`. SWC callbacks are deferred to **PendSV**
(`facade_set_context_switch_handler()` is bound before pairing is reachable); the audio RX/TX success
callbacks are the hot path and call `sac_pipeline_produce()` from there. **TIM17** runs
`sac_pipeline_process()`/`consume()` and is *event-driven, not periodic* — its period is left at
1000 ms and every real invocation comes from an explicit `quasar_timer_generate_event()`. **TIM16**
fires every 10 ms and calls `swc_connection_send()` **from ISR context**. The main loop holds only
button polling, `at_cmd_core_process()`, stats printing, and the two blocking operations — pairing
and boot reconnect — which is why those two must pump buttons and AT themselves.

Consequence to respect: anything shared between a callback and the loop needs `volatile`, and
`facade_button_handling()` dispatches **synchronously**, so a button callback can re-enter code the
loop is still inside. That re-entrancy already caused one post-mortem (a press NULLed connection
handles a poll was still dereferencing), which is why boot reconnect defers presses via a flag.

### Liveness and dropout — there is no reconnect state machine

A mid-session dropout **tears nothing down**. Nothing unpairs, the wireless core keeps its schedule,
both SAC pipelines keep running, and the link resumes when packets arrive again. Loss is only
*reported*. Do not go looking for the recovery path; it is the absence of one.

Liveness is measured by the app, not asked of the SWC, and the reason is written out at
`puretone_unidirectional_coord.c:2058`: `link_update_connect_status()` hands a coordinator
`synced == true` unconditionally, and a TX connection keeps transmitting into its own timeslots
whether or not anyone listens. Instead, the peer's 10 ms **data** packet is the heartbeat — data, not
audio, because it arrives unconditionally even on rungs where audio slots run idle — and
`link_is_up()` is "stamped within `NODE_RX_TIMEOUT_MS` (200 ms)". `DEVICE_PAIRED` is therefore *not*
"connected"; it is set before the link is ever proven.

Three different notions of "down" coexist and are meant to: 20 ms inside the Wireless Core, 200 ms of
data silence for the app, and a further 400 ms of down-edge debounce before the AT layer emits
`+EVENT: LE_UWB_DISCONNECTED`.

On the DG only, `fallback_hold_handler()` keeps an outage from corrupting the ladder: with the peer
absent the queue backs up and reads as a bad link, so the ladder is **frozen**; after the peer
returns it is held a further `LADDER_SETTLE_MS` (2 s), because otherwise the warm-up queue walks the
ladder 1→4 in one step; silence past `NODE_RESTART_SILENCE_MS` (3 s) releases any pin, since a
restarted peer is a different link. It compiles out entirely under `FALLBACK_FORCE_MODE`.

### Flags that must match on both ends

Flashing one end only is a real hazard, because these are read by both roles and a mismatch is
silent: `MAIN_CHANNEL_ALLOW_96K`, `NODE_ISI_MITIG` (changes the preamble — a mismatched pair simply
does not link, and it looks exactly like the dual-radio failure), the fallback mode indices, and the
`user_data_t` layout.

### Two wire contracts, both append-only

1. `user_data_t` (`app/common/link_data/puretone_link_data.h`) is memcpy'd onto the air, un-packed,
   and shared with the ODM's SOC through vendor pass-through. Its header states four rules (append
   only, after `vendor_data`; every field `uint8_t`/`bool`; zero must mean "absent"; must fit
   `MAX_DATA_PAYLOAD_SIZE`). Mixed-version pairs degrade by common prefix, so a reordering silently
   *misinterprets* rather than failing.
2. `at_cmd_code_t` (`app/common/at_cmd_core/at_cmd_core.h:120`) — values 1–4 (NEXT/PRE/PLAY/STOP) are
   on the wire and an ODM host is already integrated against them. Renumbering does not break the
   link; it makes PLAY arrive as STOP. Append only; `AT_CMD_VOL=5` is the one value still unspent.

### The AT command subsystem

Three layers: `library/at_module` (transport + line parser, hardware-agnostic, function-pointer
UART) ← `app/common/at_cmd_core` (all 22 commands, the link state machine, the *only* emitter of
`+EVENT:` lines) ← the app, which registers role-specific callbacks. Hardware comes in through
`at_cmd_core_facade.h` (7 functions).

Registration is a flat 128-slot array with case-insensitive linear lookup (`at_module.c:43`); a
handler is `bool (*)(const char *args, char *resp, uint16_t resp_size)` where `args` is `""`, `"?"`,
`"=val"` or `"=?"`. All 22 are registered in one place, `at_cmd_core_init()`.

**Both roles register the same 22 commands**; what differs is which app callbacks are wired behind
them, so several commands answer `OK` and do nothing on one side (`AT+VOL` on the DG, `AT+PLAY` on
the HS). That is by design, not a bug to "fix" — check `at_cmd_core_register_*_cb()` call sites in
`..._coord.c:394` and `..._node.c:340` before concluding a command is broken.

To add a command, everything happens in `at_cmd_core.c` — prototype, handler, `at_server_register()`
— plus **the hand-maintained `cmds[]` list in `handler_help()`**, which is a second copy of the
command list that nothing generates; forgetting it is invisible until someone runs `AT+HELP`. No
CMake edit is needed. If the command needs app state, do not act in the handler (it runs before `OK`
reaches the wire) — set a request flag and act from `at_cmd_core_process()`, as PAIR / RESET /
CONNECT / DISCONNECT / SHUTDOWN do. If it must cross the link, prefer `AT+VENDOR_CMD` pass-through
over spending a new `at_cmd_code_t` value.

Traps worth knowing before debugging a host integration: every line beginning `AT` is unconditionally
echoed as `+DBG OK: [...]` before dispatch, so the first line back is never the answer; handlers that
put `"OK"` in `resp` produce **two** `OK` lines, because `at_module` appends its own; an unknown
command dumps help and returns *neither* `OK` nor `ERROR`, hanging a strict host until timeout; and
`AT+VOL` with a bad argument prints `ERROR` then `OK`.

**`unpair_device()` usually does not unpair.** `AT+LE_UWB_DISCONNECT`, `AT+LE_UWB_PAIR` from a paired
state, and an aborted boot reconnect all pass `false` — link down, flash record kept. Only the button
held while paired passes `true` and erases the record. `AT+LE_UWB_CONNECT` is implemented as a
`facade_system_reset()`, deliberately reusing boot auto-reconnect rather than re-implementing it.

**Two role enums with opposite polarity coexist.** `AT_DEVICE_ROLE_NODE=0, COORDINATOR=1`
(`at_cmd_core.h:95`) versus `PAIRING_DEVICE_ROLE_COORDINATOR=0, NODE=1` (`config/pairing_cfg.h`).
Both are used correctly through their named constants; the hazard is reading a bare `0`/`1`. Note
that `MD/unidir_audio_test_readme.md` documents `AT+LE_UWB_GET_ROLE?` with the polarity inverted —
the firmware's own `AT+HELP` text is right and the readme is wrong.

### Console and AT routing — verify, never recall

Where the console prints and where AT is received differs per app, per board and per power variant,
and **`puretone_headset` and `puretone_unidirectional` use LPUART1 for opposite purposes on u535**.
The table lives in `MD/console_at_uart_routing.md` with pins taken from `quasar_def.h`. The
expansion pads are USART2 on u5a5 (PA2=TX) and LPUART1 on u535 (PA3=TX) — *the direction reverses* —
which is now guarded by `_Static_assert`s in
`backend/quasar_backend/puretone_unidirectional_backend/puretone_unidirectional_backend.c`, because
getting it wrong compiles fine and presents as a port that never answers. On u535-SMPS the AT
channel and the stats console share one queue, so `+EVENT:` lines interleave with statistics.

## Build flags are the experiment mechanism

The root `CMakeLists.txt` is a long list of optional `-D` forwards, and this is deliberate: most
tuning knobs have `#ifndef` defaults in headers, so a comparison arm is a flag on the configure line
rather than an edit somebody has to remember to undo. Examples: `FALLBACK_FORCE_MODE` (pin one
rung to attribute a latency figure), `SWC_SINGLE_RADIO_ID`, `NODE_ISI_MITIG`, `MULTI_RADIO_FORCE`,
`STATS_VERBOSE`, `DEBUG_IO_TXEN`, `CERTIF_FORCE_MODE`, `CONSOLE_ON_CDC`, `FW_VERSION_RELEASE`.

**A knob is only real once the root `CMakeLists.txt` forwards it.** Because the header `#ifndef`
default still produces a correct-looking build, an un-forwarded `-D` is accepted and silently does
nothing — this has happened repeatedly (see commits `bc1533f`, `cd1ae4e`). When adding a knob, add
the `add_compile_definitions()` forward at the same time, and prefer a hard `FATAL_ERROR` over
quietly ignoring a flag that this app cannot honour (as `CONSOLE_ON_CDC` does for non-unidir apps).

Version strings come from one place, `app/common/fw_version/fw_version.h`; the banner, `AT+VER`,
`AT+FW_VERSION?` and `+MODULE_INFO` all read it.

## Documentation in `MD/`

Design decisions, bench measurements and open issues live in `MD/`, written in Traditional Chinese.
They are load-bearing — several code comments cite them by filename as the evidence for a constant
(grep `MD/` across `app/` to see which). Consult them before changing radio, fallback or pairing
behaviour.

They are also **records of a decision at a point in time, not a description of today's code**, and
several have drifted — some describe the `puretone_headset` line, some describe an approach this app
later replaced. Where a doc and the code disagree, the code is the fact; fix the doc rather than
"restoring" the behaviour it describes.

- `unidir_audio_test_readme.md` — the ODM-facing test procedure; the authoritative list of the 22
  registered AT commands and every `+EVENT:` line, and it outranks the PRD where they disagree. It
  has nonetheless drifted from the code in places, so confirm details against `at_cmd_core.c`.
- `console_at_uart_routing.md` — console/AT pin routing for both apps and all board variants.
- `at_cmd_bidir_decision_spec.md`, `at_cmd_prd_reconciliation.md` — AT surface and where code and
  PRD diverge.
- `boot_auto_reconnect_design.md`, `pairing_identity_preprovision_spec.md`,
  `uwb_disconnect_decision_spec.md`, `soc_reset_wake_decision_spec.md` — link lifecycle.
- `uwb_quality_indicator_decision_spec.md` — why `+EVENT: LE_UWB_QUALITY` is driven by the fallback
  rung and not by link margin, and the measured range numbers (32.9 m clear, 3.5 m blocked) that
  make blocking the only failure mode inside the specified room. Implemented in `c057a45`.
- `link_dropout_arms_ledger.md`, `fallback_mono_rung_rationale.md`, `link_rf_settings_baseline.md`,
  `spark_link_budget_questions.md` — the measurements the ladder constants are set from.
- `u535_ldo_rx_deficit.md`, `radio_stall_wedge_open_issue.md`, `dualradio_*.md` — open hardware and
  dual-radio issues.
- `unidir_port_todo.md` — what is unported, changed-but-unverified, or deliberately shelved.
- `spark_sdk_overview.md` → `bes_mcu_port_analysis.md` — read in that order; the SDK's shape as
  background, then the six-step plan for porting it onto a third-party MCU (Airoha AB1595).

Release packages are assembled under `bin/<version>_<variant>/` — role-named binaries
(`v241rc2_u535ldo_std_HS_node.bin`) plus a `MANIFEST.md` naming the git commit and what was
verified. Keep the folder suffix equal to `FW_VERSION_RELEASE`. `bin/` is gitignored, so these are
local artifacts; the MANIFEST's commit is what actually identifies the source a package came from.

## Conventions

- **Comments explain why, not what.** The prevailing style in this tree is a long comment recording
  the reasoning, the measurement, or the failure that a constant or guard exists to prevent —
  frequently citing a commit or an `MD/` document (`sac_cfg.h` is the reference example). Match it;
  do not strip it. New files carry the SPARK doxygen file header
  (`@file`/`@brief`/`@copyright`/`@license`/`@author`).
- **Commit subjects** are lowercase, `[tag] what changed and why it matters` — e.g.
  `[unidir] refuse -DCONSOLE_ON_CDC=1 on the u535 instead of letting it collide`. Common tags:
  `[unidir]`, `[docs]`, `[cfg]`, `[at]`, `[bsp]`, `[build]`, `[release]`, `[ver]`, `[script]`,
  `[validator]`. The subject states the effect, not the diff.
- 120-column rulers, trailing whitespace trimmed, final newline inserted (`.vscode/settings.json`).
- `middleware/tinyusb/` and `third-party/` are vendored upstream — including the `CLAUDE.md` and
  `AGENTS.md` at `middleware/tinyusb/src/`, which describe TinyUSB's build system, not this one.
  Leave them alone.
- This checkout is **one of five git worktrees** sharing a repository (`git worktree list`). The
  stash stack is shared; use a WIP commit or a uniquely named `git stash push -m` rather than bare
  `git stash`.
