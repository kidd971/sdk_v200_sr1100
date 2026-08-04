# Follow-up — v2.3.1 closes the re-sync trigger; the `fb = 0` trigger and the park itself remain

Follow-up to `dualradio_resync_HQ_report.md` (dual-radio node parks, radios never serviced again).
Please read that report first — this document only reports what changed.

**Short version:** v2.3.1 fixes the §3.1 re-sync trigger, confirmed on our hardware. The Appendix-A
`fb = 0` trigger survives it unchanged, and we can now show why: the v2.3.1 bound is gated on the
*unsynced* state, and the `fb = 0` park happens while **synced and streaming**. The wedge the two
triggers share — TIM4 left at max period with nothing to re-arm it — has not been addressed.

---

## 1. What v2.3.1 fixed — confirmed

We ported the v2.3.0 → v2.3.1 wireless-core delta (4 files, clean vendor update, no local edits over
it) and retested.

| Repro | Before v2.3.1 | After v2.3.1 |
| --- | --- | --- |
| §3.1 — node out of range ≥15 s, then back | Never re-syncs; MCU reset only | **Fixed** — re-syncs normally |

Thank you — that was a fast turnaround and it closed a real failure.

---

## 2. What still fails

**Config:** U535 coordinator + U535 node, dual radio, `SCHEDULE` slot width **250 µs** (both ends),
fallback open to mode 0, close range (`link_margin` ≈ 200), v2.3.1 applied.

**Result:** the node still parks on the up-shift to `fb = 0`, with the **identical signature** to
Appendix A:

```
r1_irq / r2_irq   frozen
tim4: arr=65533   pinned at max period
mrt / cnt         still advancing
fault             none (cfsr=0 hfsr=0)
+AUTO-RECOVER (swc restart)  ->  swc=STOP, radios stay frozen; only MCU reset recovers
```

No change from the pre-v2.3.1 behaviour.

---

## 3. Why v2.3.1 cannot affect this path

This is structural, not a margin question. In `wps_mac_xlayer_get_xlayer_for_rx()`:

```c
bool unsync = (!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync)) &&
              (wps_mac->node_role == NETWORK_NODE);
...
if (unsync && connection->cfg.tx_sync_frame_on_syncing) {
    wps_mac->rx_node->xlayer.frame.payload_memory_size = 0;   /* header-only bound */
}
```

Both halves of the fix — the RX payload bound above and the RX timeout bound via
`link_tdma_get_rx_timeout_base_value()` — apply **only while `unsync == true`**.

The `fb = 0` park occurs on a **synced, streaming** node at the moment the audio fallback promotes
to 96 kHz. `unsync` is false throughout, so neither bound is ever in effect on this path.

So the two triggers are independent after all. Our earlier note (§A.7) suggested a single fix at the
re-arm path would close both; that was the right target, but v2.3.1 fixed a *trigger* instead — it
removes one way to overrun the timeslot, not the consequence of overrunning it.

---

## 4. New evidence — the `fb = 0` park is a timing-margin race, not a state-transition bug

This is the main new material since the original report.

We swept the `SCHEDULE` slot width (9 slots, applied to both ends, fallback-open build, close range
with `link_margin` ≈ 200 held constant):

| Slot width | Result |
| --- | --- |
| **250 µs** | Parks after **~11 s** at `fb = 0` |
| **260 µs** | Survives **minutes** — ~20 min total run, `rx_ok` ≈ 2 000 000 before parking |
| **280 µs** | Never reaches `fb = 0` — throughput cannot sustain 96 kHz, stays at `fb = 2` |

Same board, same binary, same link. Only the slot width changes, and the time-to-park moves by
orders of magnitude while the wedge signature stays identical.

**Two conclusions:**

1. **The park is not the `fb = 0` transition itself.** At 260 µs the node runs *at* `fb = 0` for
   minutes and millions of frames before wedging. It is a per-opportunity race that fires while
   running at 96 kHz, not a one-shot fault at the mode switch.

2. **Slot tuning is not a workaround.** Narrow enough to sustain 96 kHz still parks (only later);
   wide enough to be safe cannot reach 96 kHz at all. Throughput budget: 96 kHz needs ~2400 pkt/s;
   main slots/s = 7 / (9 × slot) → 250 µs = 3111, 280 µs = 2778, 320 µs = 2431. There is no slot
   width that is both fast enough and safe.

**Consistent with your own root-cause statement.** The v2.3.1 commit describes the failure as a
reception that *"overrun[s] the timeslot timing and parking the scheduler."* That is exactly what
the sweep measures: at `fb = 0` the payload is at its maximum
(`link_scheduler_get_current_timeslot_main_max_payload_size`), so the RX window is longest and the
timeslot budget is thinnest. v2.3.1 removed one source of over-long RX (corrupted size field while
syncing). We are reporting a second one that needs no corrupted frame — normal 96 kHz operation at a
tight slot budget reaches the same edge.

**Hypothesis for the same-MCU-only pattern (§A.2):** with both ends on the same MCU the relative
clock drift is ≈ 0, so the phase relationship between the timeslot boundary and the RX window sticks
at whatever offset it lands on — including a bad one. With mixed MCUs the ppm drift slides the phase
continuously, so a bad offset is never held long enough to fire. This is inference from the sweep,
not something we can measure directly.

---

## 5. Correction to §A.2 of the original report

The original table recorded U5A5 ↔ U5A5 as **"No"** for the spontaneous `fb = 0` park. That was
measured at the **default slot width** and over a limited run.

Given §4, we no longer read that as immunity — only as a wider margin and a lower hit rate at that
slot width. We are re-running U5A5 ↔ U5A5 at 250 µs with exposure counted in main slots rather than
wall-clock, so the two platforms are comparable (U535 parks within ~34 000 main slots at 250 µs).
We will send the number when we have it.

We flag this because we do not want the defect scoped to "the customer's U535 board." The sweep
shows the margin is narrow on both platforms; U535 simply sits on the wrong side of it.

---

## 6. The open ask

The original report's §6 ask is unchanged and is now the more important one:

> **A timeslot overrun parks the scheduler permanently, and nothing recovers it but an MCU reset.**

`wps_phy_multi_process_radio_timer()` (`protocol_stack/multi_radio/wps_phy.c:253`) sets the
multi-radio timer to max period unconditionally on both exit paths (lines 269 and 288, both commented
*"Sync timer on frame start"*). The only path that re-arms a short period is the
`signal_main != PHY_SIGNAL_CONFIG_COMPLETE` retry at line 263. After the max-period write, recovery
depends entirely on the next frame-start event arriving. If it does not, `ARR` stays at 65533 and
nothing in the stack notices — no watchdog, no re-arm, no error.

Application-side recovery does not exist: `swc_disconnect()` + `swc_connect()` leaves the core in
STOP with the radios frozen (§3.3), and `swc_set_fast_sync()` returns
`SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO`.

**We would like two things:**

1. **`fb = 0` / 96 kHz dual radio** — either the timeslot budget at mode 0 gets margin, or 96 kHz on
   dual radio is documented as unsupported. Either answer lets us close it with the customer; the
   current state does not.

2. **The park itself** — a re-arm or watchdog on the multi-radio timer, so that *any* timeslot
   overrun degrades into a re-sync instead of a permanent dead link. v2.3.1 removed one trigger; we
   have shown a second that survives it. As long as the consequence is unrecoverable, a third
   trigger is a matter of time.

Ask 2 is the one we would prioritise. It is independent of the 96 kHz question and it is what turns
this class of bug from "link dies until reset" into "link recovers."

---

## Appendix — reproduction config

| Item | Value |
| --- | --- |
| Pair | U535 coordinator (DG) + U535 node (HS), both same MCU |
| Radio | SR1100, dual radio (`WPS_RADIO_COUNT == 2`), `tx_wakeup_mode = AUTO`, fast-sync off |
| SDK | v2.3.0 + v2.3.1 wireless-core delta applied |
| BSP | includes our `quasar_timer_set_period()` `CNT > ARR` guard (original report §5) |
| `SCHEDULE` | 9 slots @ 250 µs, identical on both ends — both must be reflashed together |
| Audio | 96 kHz 24-bit main channel, fallback **open** to mode 0 (no ceiling) |
| Link | close range, `link_margin` ≈ 200 held steady |
| Time to park | ~11 s from reaching `fb = 0` |

**A weak link masks the test.** With a poor link the fallback stays pinned at `fb = 2` and never
climbs to mode 0, so nothing parks. The link must be strong enough to reach 96 kHz for the repro to
be valid.

Available on request: `.elf` / `.map` / commit hash for the exact build, full periodic dump logs
across the sweep, and the coordinator-side producer instrumentation from §A.5.
