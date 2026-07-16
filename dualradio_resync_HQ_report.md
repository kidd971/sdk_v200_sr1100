# SPARK SWC — Dual-radio node parks (radios never serviced again) and cannot re-sync

**Reproduces on stock Quasar EVK hardware: U5A5 coordinator + U5A5 node, SR1100, dual radio.**

Short version (1 page): `dualradio_resync_HQ_report_brief.md`.

---

## 1. Summary

On a **dual-radio** SWC link (SR1100, `WPS_RADIO_COUNT == 2`, `tx_wakeup_mode = AUTO`), after the
node loses the link for a sustained period (~15 s out of RF range) and is brought back, the node's
**radios stop being serviced and never recover**. The multi-radio scheduler timer (TIM4) keeps
ticking and the CPU is fine, but every radio IRQ/DMA counter is frozen and the node never
re-synchronizes. **Only an MCU reset recovers it.** The coordinator stays fully healthy throughout
(instrumented and measured).

The node cannot get itself out: fast-sync — the aggressive RX-hunt path — is **hard-blocked for
dual radio**, and `swc_disconnect()` + `swc_connect()` does not restart servicing either.

This report is what we have been able to characterise from the outside: the behaviour, what we
measured, and the raw logs. There is no recovery path available to us at the API level (§6).

Separately, while diagnosing this we found and fixed a bug in the reference BSP's
`quasar_timer_set_period()` (§5). **We would like your review of that fix.** All behaviour in
§3-§4 is with that fix already applied.

An additional, much faster trigger into what appears to be the **same park state** was found on our
STM32U535 field board (~11 s from boot, no RF manipulation, only when both ends are the same MCU).
It is in **Appendix A** — it is not EVK hardware, but it lands in the identical failure state, so a
fix for §3 would likely cover it too.

## 2. Environment

- SWC application build string: `v2.3.0`, role = HS (node).
- Transceiver: **SR1100**. Multi-transceiver: `DUAL_TRANSCEIVER` (`WPS_RADIO_COUNT == 2`).
- MCUs in play: **STM32U5A5** (Quasar EVK) and **STM32U535** (our field board), both on the quasar
  BSP. **The problem in §3-§4 reproduces on the stock EVK (U5A5 + U5A5).**
- Multi-radio scheduler timer = **TIM4** on both quasar BSPs
  (`quasar_timer_multi_radio_set_callback` → `quasar_it_set_timer4_callback`). On STM32U5,
  **TIM4 is a 32-bit timer** (confirmed empirically — see §5).
- `tx_wakeup_mode` = AUTO (default, `WPS_DEFAULT_MULTI_TX_WAKEUP_MODE`); fast-sync = disabled
  (default, and unavailable for dual-radio — see §4.2).
- Audio: 96 kHz/24-bit base mode (`fb = 0`) with a fallback ladder down to 48 kHz variants
  (`fb = 1` 48k/24b, `fb = 2` 48k/16b, `fb = 3` 48k/ADPCM). Fallback is decided and signalled by the
  coordinator; the node follows.
- Roles: Coordinator (dongle / "DG") + Node (headset / "HS"). Only the **node** is affected.

## 3. Behaviour (Quasar EVK, U5A5 + U5A5)

### 3.1 Reproduction

1. Dual-radio node (HS) + coordinator (DG), paired and streaming.
2. Move the node ~out of RF range for **≥15 s** (fallback climbs down the ladder, then
   `conn = LOST`).
3. Bring the node back.
4. **The node does not re-sync.** Radio HW counters stay frozen. Only a full MCU reset recovers it
   (or, occasionally, moving the node immediately adjacent to the coordinator — after a very long
   wait).

### 3.2 What we observe while parked

```
hw:    r1_irq=290474 r2_irq=290472 r1_dma=633803 r2_dma=644339   <-- FROZEN across all dumps
sched: mrt=293074 ... 293684 ... 294295 ... 294905               <-- TIM4 update IRQ ADVANCING (~305 Hz)
tim4:  cen=1 arr=65533 cnt=42229 / 1136 / 24889 / 51112 uie=1    <-- running, pinned at max period
conn=LOST  swc=RUN  fault: cfsr=0 hfsr=0 pc=0 lr=0               <-- no CPU fault
```

**The scheduler is alive, TIM4 is alive, the CPU is fine — only the radios are dead.** Radio IRQ
pins idle (`irq1 = irq2 = 0`). `ARR` stays pinned at 65533; on a healthy node it toggles to the
retry period (see §4.3).

### 3.3 Recovery paths that do not work

- **Fast-sync is unavailable to us.** `swc_set_fast_sync(true, …)` returns
  `SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO` (`core/wireless/api/swc/sr1100/swc_api.c:546-549`). The
  unsynced aggressive-RX-hunt code (`wps_mac.c`, `prepare_rx_main`, gated on `fast_sync_enabled`) is
  therefore dead code with two radios — a dual-radio node has **no aggressive sync-channel RX
  hunting on loss**.
- **`swc_disconnect()` + `swc_connect()` does not recover it.** As an app-level mitigation we added
  a watchdog: if the HW counters stay frozen for 6 s while paired, call `swc_disconnect()`
  (tolerating `SWC_ERR_DISCONNECT_TIMEOUT`) then `swc_connect()`. Result: **no recovery.**

  ```
  +AUTO-RECOVER: radio stall -> swc reconnect
  swc=STOP conn=LOST ... hw: r1_irq=282054 (frozen) ...   <- after disconnect+connect
  swc=STOP ... (stays STOP, radios stay frozen, no reconnect)
  ```

  `swc_disconnect()` does stop the core (`swc = STOP`) — from the frozen state its handshake depends
  on a frame-completion IRQ that never comes, so it hits `SWC_ERR_DISCONNECT_TIMEOUT`; `is_started`
  is still forced false — but the subsequent `swc_connect()` does **not** return the core to
  `RUNNING` and the radios stay frozen. `swc_connect()` from a clean boot works.

We found **no** explicit latched "park / give-up" state after N losses; the schedule keeps advancing
in principle. The freeze looks emergent: TX-auto idle + no re-hunt.

## 4. Where we traced it to

*(Included for completeness — you know this code far better than we do. §4.3 is the part we think
is most useful diagnostically.)*

### 4.1 Where the radios stop being serviced

`wps_phy_multi_process_radio_timer()` (`core/wireless/protocol_stack/multi_radio/wps_phy.c:253-290`):
the TIM4 callback wakes the radios (`phy_wakeup_multi()` + `sr_access_close()`) only in the
"main-frame" branch (lines 258-259). The **else branch (lines 286-289) only re-arms the timer at max
period and never wakes the radios**:

```c
if (wps_phy->xlayer_main == NULL || (wps_phy_multi_get_tx_wakeup_mode() == MULTI_TX_WAKEUP_MODE_MANUAL) ||
    (wps_phy->xlayer_main->frame.destination_address == wps_phy->local_address)) {
    /* ... phy_wakeup_multi() + sr_access_close() ... */
} else {
    swc_hal_timer_multi_radio_timer_set_max_period();   // radios NOT woken
}
```

The else branch is taken when the prepared timeslot is a **TX-auto** slot
(`destination != local_address`, AUTO wakeup) — the design delegates waking to the radio's hardware
AUTOWAKE bit (`prepare_radio_tx`, `wps_phy_common.c` sets `AUTOWAKE`). `xlayer_main` only advances to
the next (RX) slot inside `process_next_timeslot()` (`wps_mac.c`), which runs **only** from a
frame-outcome IRQ (`PHY_SIGNAL_FRAME_SENT_ACK/NACK/RECEIVED/MISSED`). If the schedule is parked on a
TX-auto slot whose completion IRQ never returns, `xlayer_main` never advances, and TIM4 idles at max
period forever — the radios never reach an RX slot to hear the coordinator.

### 4.2 Dual-radio radios are `SLEEP_IDLE`

With `WPS_RADIO_COUNT != 1` the radios are forced `SLEEP_IDLE`
(`core/wireless/link/sr1100/link_tdma_sync.c`) — i.e. woken only by the MCU timer / AUTOWAKE path
above. That is why a stall in §4.1 freezes both radios completely rather than degrading.

### 4.3 A *good* link exposes the park; a *weak* link masks it — TIM4 `ARR` is the tell

The most counter-intuitive — and most diagnostic — observation: the park happens when the node is
**close to the coordinator with a clean link**, and does **not** happen when either end is moved far
away into a weak link. This is the opposite of an RF-margin failure.

A weak link never settles: it stays in the **retry/config-incomplete path**
(`wps_phy.c:262-277`), which re-arms TIM4 at the *retry* period and **re-calls `phy_wakeup_multi()`
every cycle** — continuously re-waking the radios. A bad link is therefore self-protecting; a good
link is what kills it.

**Register-level signature — TIM4 `ARR`:**

- **Parked (close, clean link):** `arr` is **pinned at 65533** (max period) in *every* dump — the
  node never re-enters the retry/wakeup path. Radios frozen. (§8a)
- **Healthy (far, weak link):** `arr` **toggles between the retry period
  (~4997/4919/5074 ≈ `MULTI_RADIO_RETRY_TIMER_PERIOD_US`) and 65533** — the node is continuously
  cycling through config/retry, re-waking the radios each time. `r1_irq/r2_irq` keep advancing; it
  never parks. (§8c)

Practical implication: this bug is **masked by a poor link and exposed by a good one**, so it
surfaces exactly in close-range / clean-RF QA, and any change that merely *improves* RF margin makes
it **more** likely, not less.

## 5. A fix we already made — please review

While diagnosing the above we found and fixed a bug in the reference BSP. **We would like your
review: is this the right fix, or is the SWC expected to behave differently here?**

`quasar_timer_set_period()` (`bsp/quasar/quasar_timer_ext.c`) writes `ARR = period - 1` but **never
touches `CNT`**:

```c
void quasar_timer_set_period(quasar_timer_selection_t timer_selection, uint16_t period)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);
    if (timer_instance != NULL) {
        timer_instance->ARR = (uint32_t)period - 1;   // CNT untouched
    }
}
```

The SWC re-arms the multi-radio timer with a range of periods — the retry period
(`MULTI_RADIO_RETRY_TIMER_PERIOD_US`, ARR ≈ 5464), the max period (`set_max_period`, ARR = 0xFFFD =
65533), and `timer_frequency_ratio * sleep_time` (`wps_phy.h`, `wps_phy_prepare_frame`). When the SWC
**lowers** the period (e.g. 65533 → 5464) while the live `CNT` is already **above** the new `ARR`, a
**32-bit** counter does not wrap immediately — it counts all the way to `0xFFFFFFFF` (~20 MHz tick →
~200 s) before the next update event. During that runaway **no update IRQ fires**, so the scheduler
heartbeat stops, and because the dual-radio radios are `SLEEP_IDLE` (§4.2) both radios freeze. Only
a reset recovers.

Evidence (node crash-dump; `mrt` = TIM4 update-IRQ count):

```
tim4: cen=1 arr=5464 cnt=29154652  uie=1   <- CNT >> ARR, and climbing:
tim4: cen=1 arr=5464 cnt=69154129  uie=1       ~+40,000,000 every 2 s (≈20 MHz),
tim4: cen=1 arr=5464 cnt=109154357 uie=1       heading for the 32-bit wrap.
mrt frozen at 361376 the whole time; frt (TIM8) keeps ticking; radios frozen.
```

Our change (applied in both `bsp/quasar` and `bsp/quasar-u535`):

```c
uint32_t auto_reload = (uint32_t)period - 1;
timer_instance->ARR = auto_reload;
if (timer_instance->CNT > auto_reload) {   // 32-bit timer: don't let CNT run away
    timer_instance->CNT = 0;
}
```

This removes the runaway (heartbeat stays alive). **Is this acceptable?** Or should the multi-radio
timer helper guard against `CNT > ARR` itself — or is the SWC expected to only ever *raise* the
period? (The dynamic retry/max re-arm clearly lowers it.)

Note: all of the §3-§4 behaviour was observed **with this fix already applied** — the §3 park is a
separate problem, not this timer bug.

## 6. Where we are stuck

We have no recovery path from the application side: fast-sync is blocked for dual radio,
`swc_disconnect()` + `swc_connect()` does not restart servicing (§3.3), and only an MCU reset brings
the node back. We have characterised the failure as far as we can from the outside — behaviour
(§3), where we traced it to (§4), raw logs (§8) — the rest needs your eyes on the stack.

Two smaller items alongside it:

- **Please review our BSP change in §5** (`quasar_timer_set_period`, `CNT > ARR` on 32-bit timers).
  It is in the reference BSP, so it would be better fixed upstream than carried as a local patch.
- **Appendix A** documents a second, much faster route into the same park state on our U535 field
  board (~11 s from boot, no RF manipulation). Different MCU, but it reaches the identical failure
  state — it may be the easier one to debug against.

## 7. Key code references

| Item | Location |
| --- | --- |
| Multi-radio timer callback / TX-auto idle branch | `core/wireless/protocol_stack/multi_radio/wps_phy.c:253-290` (else: 286-289) |
| Retry/config-incomplete path (re-wakes radios) | `core/wireless/protocol_stack/multi_radio/wps_phy.c:262-277` |
| Period re-arm from radio IRQ path | `core/wireless/protocol_stack/multi_radio/wps_phy.h` (`wps_phy_prepare_frame`) |
| `xlayer_main` advance only on frame-outcome IRQ | `core/wireless/protocol_stack/wps_mac.c` (`process_next_timeslot`, `wps_mac_phy_callback`) |
| Dual-radio radios forced `SLEEP_IDLE` | `core/wireless/link/sr1100/link_tdma_sync.c` (`WPS_RADIO_COUNT != 1`) |
| Fast-sync blocked for dual-radio | `core/wireless/api/swc/sr1100/swc_api.c:546-549` |
| BSP timer set_period (§5) | `bsp/quasar/quasar_timer_ext.c` / `bsp/quasar-u535/quasar_timer_ext.c` (`quasar_timer_set_period`) |
| Coordinator fallback cfg (Appendix A) | `swc_connection_set_fallback_cfg` / `fallback_cfg` in our `puretone_dongle.c` |

## 8. Representative node logs

### 8a. The park — TIM4 alive at max period, radios never serviced
```
hw:    r1_irq=290474 r2_irq=290472 r1_dma=633803 r2_dma=644339   (frozen across all dumps)
sched: mrt=293074 ... 293684 ... 294295 ... 294905               (mrt ADVANCING ~305 Hz)
tim4:  cen=1 arr=65533 cnt=42229 / 1136 / 24889 / 51112 uie=1    (timer running, CNT in range)
conn=LOST swc=RUN — recovers only when adjacent to coordinator, very slowly
```

### 8b. `swc_disconnect()` + `swc_connect()` does not recover
```
+AUTO-RECOVER: radio stall -> swc reconnect
swc=STOP conn=LOST ... hw: r1_irq=282054 (frozen) ...
swc=STOP ... (stays STOP; radios frozen; no reconnect until full MCU reset)
```

### 8c. Weak link is self-protecting (either end moved *far*, never parks)
```
+EVENT: UWB_QUALITY:WEAK
hw:    r1_irq=4548492 ... 4556495 ... 4564501 ... 4572484   (ADVANCING — radios serviced)
sched: mrt=4548571 ... 4556584 ... 4564581 ... 4572560      (ADVANCING)
tim4:  cen=1 arr=4997 cnt=4244 / arr=4919 cnt=4565 / arr=5074 cnt=4611 / arr=65533 cnt=2757
       ^ ARR TOGGLES retry-period(~5000) <-> 65533 = node continuously re-hunting, re-waking radios
lm=10/5/0/5 (flapping near 0)  rx_rej=11675->12545 (climbing, CRC-fail)  swc=RUN conn=OK — never parks
+EVENT: UWB_QUALITY:GOOD

Contrast 8a (node close, clean link, PARKED): arr PINNED at 65533 every dump, r1_irq/r2_irq FROZEN.
```

### 8d. The §5 BSP timer bug — TIM4 counter runaway (before our fix)
```
tim4: cen=1 arr=5464 cnt=29154652  uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
tim4: cen=1 arr=5464 cnt=69154129  uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
tim4: cen=1 arr=5464 cnt=109154357 uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
(hw: r1_irq/r2_irq/r1_dma/r2_dma all frozen; frt/TIM8 keeps ticking)
```

---

# Appendix A — STM32U535 field board: a second, faster trigger into the same park

*(Different MCU from the EVK, so we understand if this is out of scope. We include it because it
ends in **exactly the same state** as §3 — same frozen radios, same `ARR` pinned at 65533, same
`mrt` still advancing, same inability to recover — but reaches it in ~11 s with no RF manipulation
at all. It looks like a second, much faster route into the same defect.)*

## A.1 Behaviour

**Repro — no RF manipulation, ~11 s from boot, 100 % of the time:**

1. Dual-radio coordinator **and** node both on the **same MCU** (U535 + U535), paired, close range,
   clean RF, streaming audio.
2. Just let it play. The fallback climbs the ladder (`fb` 2 → 1 → 0) as the link proves good.
3. **The instant `fb` reaches 0 (96 kHz, the top/base mode), the node's radio IRQ/DMA counters
   freeze and never resume.** Only an MCU reset recovers.
4. Swap **either** end to a U5A5 and the same test runs 96 kHz indefinitely.

**It is not RF.** At the freeze the link margin is healthy and the corrupted-frame count is
essentially zero — the node is not losing the link, it is being *promoted* to the top mode **because
the link is good**. A weak link never climbs to `fb = 0` and therefore never parks. Better RF makes
it more likely, not less. (Same asymmetry as §4.3.)

## A.2 It only happens when both ends are the same MCU

| Coordinator (DG) | Node (HS) | Spontaneous park at `fb = 0`? |
| --- | --- | --- |
| **U535** | **U535** | **Yes — ~11 s, every time** |
| U535 | U5A5 | No |
| U5A5 | U535 | No |
| U5A5 | U5A5 | No (but still parks on the §3.1 forced 15 s-loss repro) |

The **same U535 node** that parks in ~11 s against a U535 coordinator runs 96 kHz **indefinitely**
against a U5A5 coordinator. So this is **not** "U535 is too slow for 96 kHz" — the node hardware is
demonstrably capable. Only the *pairing* matters.

**Forcing 96 kHz prevents sync entirely.** Setting `fallback_cfg.enabled = false` on the coordinator
to pin the base 96 kHz mode with no mode changes results in the node **never acquiring sync at all**
(`rx_ok = 0` indefinitely, `conn = LOST`), while its radios stay alive and hunting (`r1_irq`
advancing, TIM4 `ARR` cycling retry↔max). So 96 kHz appears reachable only by climbing the fallback
ladder — and reaching it is fatal.

## A.3 What we ruled out

| Hypothesis | How it was ruled out |
| --- | --- |
| Coordinator audio producer starved (dead air) | Instrumented the coordinator: `prod = 2400/s` (= 96000/40, exactly nominal) rock-steady through the failure (§A.5). `tx_noframe` ≈ 52 % is simply `fb = 1` (48 kHz) occupying half of the 96 kHz-provisioned slots — the same ratio appears in healthy runs. |
| UART RX IRQ storm stealing CPU from the SWC data timer | Fully disabled the LPUART1 receiver (cleared `RE` + `RXNEIE`). **No change** — still parks at `fb = 0`. |
| U535 too slow for dual-radio 96 kHz | Same U535 node runs 96 kHz fine against a U5A5 coordinator (§A.2). |
| RF / link degradation | `link_margin` healthy and `rx_rej` ≈ 0 at the freeze; weak links never park. |
| The §5 BSP timer bug (`CNT > ARR` runaway) | Fixed and verified; `ARR = 65533` with `CNT` in range at the freeze, no runaway. |

## A.4 Log — park fires exactly at the fallback up-shift to `fb = 0`

U535 coordinator + U535 node, close range, clean RF, no RF manipulation. Node LINK_WATCH, 2 s
cadence. `fb` climbs 2 → 1 → 0; the radios freeze at the exact sample `fb` reaches 0 and never
resume. `lm` is healthy and `rx_rej` is essentially flat throughout — this is **not** RF loss.

```
[LW 1 t=3383 ] OK lm=160 fb=2 rx_ok=2740  rx_miss=3284  miss/s=1640 rx_rej=1  r1_irq=8184  r2_irq=8182
[LW 2 t=5384 ] OK lm=120 fb=2 rx_ok=5564  rx_miss=6476  miss/s=1595 rx_rej=39 r1_irq=16187 r2_irq=16185
[LW 3 t=7384 ] OK lm=85  fb=1 rx_ok=8388  rx_miss=9666  miss/s=1595 rx_rej=47 r1_irq=24192 r2_irq=24190
[LW 4 t=9384 ] OK lm=90  fb=1 rx_ok=11211 rx_miss=12856 miss/s=1595 rx_rej=51 r1_irq=32191 r2_irq=32189
[LW 5 t=11384] OK lm=50  fb=0 rx_ok=13935 rx_miss=15789 miss/s=1466 rx_rej=56 r1_irq=39716 r2_irq=39716  <-- fb reaches 0 (96 kHz)
[LW 6 t=13384] OK lm=50  fb=0 rx_ok=13935 rx_miss=15789 miss/s=0    rx_rej=56 r1_irq=39716 r2_irq=39716  <-- FROZEN
[LW 7 t=15384] OK lm=50  fb=0 rx_ok=13935 rx_miss=15789 miss/s=0    rx_rej=56 r1_irq=39716 r2_irq=39716
[LW 8 t=17384] OK lm=50  fb=0 rx_ok=13935 rx_miss=15789 miss/s=0    rx_rej=56 r1_irq=39716 r2_irq=39716
+AUTO-RECOVER: radio stall -> swc reconnect      (does not recover — same as §3.3)

Crash dump at the freeze:
 swc=RUN conn=OK fb=0 lm=50 cca_fail=0 tx_drop=0 rx_ok=13935 rx_miss=15789 rx_rej=56
 hw:    r1_irq=39716 r2_irq=39716 r1_dma=137060 r2_dma=125571   (all FROZEN)
 sched: mrt=39999 ... 40609 ... 41220                           (ADVANCING — scheduler alive)
 tim4:  cen=1 arr=65533 cnt=44708/6509/27383 uie=1              (TIM4 running, pinned at max period)
 fault: cfsr=0 hfsr=0 pc=0 lr=0                                 (no CPU fault)
```

Earlier independent capture of the same transition at a different margin:

```
[LW 21 t=43334] OK lm=85  fb=1 miss/s=1598 rx_rej=923 r1_irq=168187   (advancing)
[LW 22 t=45334] OK lm=180 fb=1 miss/s=1599 rx_rej=934 r1_irq=176189   (advancing)
[LW 23 t=47334] OK lm=145 fb=0 miss/s=1011 rx_rej=934 r1_irq=181597   <-- fb reaches 0
[LW 24 t=49334] OK lm=145 fb=0 miss/s=0    rx_rej=934 r1_irq=181597   <-- FROZEN (lm healthy, rx_rej flat)
```

## A.5 Log — the coordinator is measurably healthy through the failure

Coordinator LINK_WATCH (`prod` = audio buffers produced/s, nominal 96000/40 = 2400). Steady at
2400/s throughout, `tx_drop = 0`. The ~52 % `tx_noframe` is `fb = 1` (48 kHz) occupying half of the
96 kHz-provisioned timeslots — normal fallback slot occupancy, **not** producer starvation.

```
[LW 27 t=19851] OK fb=1 node_lm=10  tx_slot=40497 tx_noframe=20964 tx_drop=0 prod=2400/s swc=RUN
[LW 30 t=21351] OK fb=1 node_lm=15  tx_slot=45009 tx_noframe=23327 tx_drop=0 prod=2400/s swc=RUN
[LW 35 t=23851] OK fb=1 node_lm=110 tx_slot=52538 tx_noframe=27301 tx_drop=0 prod=2400/s swc=RUN
[LW 43 t=27851] OK fb=1 node_lm=85  tx_slot=64556 tx_noframe=33513 tx_drop=0 prod=2400/s swc=RUN
```

## A.6 Question (if you are willing to look)

Is there a known timing constraint or minimum margin for dual-radio at **96 kHz** (`fb = 0`)? And is
there any reason the *coordinator's* MCU would affect the *node's* radio servicing? We suspect a
coordinator↔node timing relationship that is static when both time bases match and drifts when they
differ, leaving the node in a dead spot — but we cannot identify it.
