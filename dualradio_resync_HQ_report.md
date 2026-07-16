# SPARK SWC — Dual-radio node fails to re-sync after prolonged link loss

## 1. Summary

On a **dual-radio** SWC link (SR1100, `WPS_RADIO_COUNT == 2`), when the **node** loses the
link for a sustained period (~15 s out of RF range) and is then brought back, the node's
radios stop being serviced and the node does **not** re-synchronize with the coordinator —
or only recovers after a very long time and only when placed immediately next to the
coordinator. The **coordinator (sync master) stays fully healthy** throughout.

We instrumented the node heavily and isolated **two independent problems**:

- **Problem A (multi-radio timer counter runaway).** Root cause of the *hard* wedge. This
  turned out to be in the BSP timer helper (`quasar_timer_set_period`), which we have
  **fixed locally**; flagged here because it also exists in the SPARK reference BSP
  (`bsp/quasar`).
- **Problem B (no autonomous dual-radio re-sync).** After Problem A is fixed, the node still
  fails to re-sync on its own: the scheduler timer runs but the radios are never woken, and
  dual-radio has **no aggressive re-sync path** (fast-sync is hard-blocked with two radios).
  **This is the item we need HQ guidance on.**

## 2. Environment

- SWC application build string: `v2.3.0`, role = HS (node).
- Transceiver: **SR1100**. Multi-transceiver: `DUAL_TRANSCEIVER` (`WPS_RADIO_COUNT == 2`).
- Reproduced on **two MCUs**: STM32U535 (field board) and STM32U5A5 (Quasar EVK) — same behaviour.
- Roles: Coordinator (dongle / "DG") + Node (headset / "HS"). Only the **node** is affected.
- Multi-radio scheduler timer = **TIM4** on both quasar BSPs
  (`quasar_timer_multi_radio_set_callback` → `quasar_it_set_timer4_callback`). On STM32U5,
  **TIM4 is a 32-bit timer** (confirmed empirically — see §3).
- `tx_wakeup_mode` = AUTO (default, `WPS_DEFAULT_MULTI_TX_WAKEUP_MODE`); fast-sync = disabled
  (default, and unavailable for dual-radio — see §4).

## 3. Problem A — TIM4 counter runaway when the period is lowered below the live count

### Mechanism
`quasar_timer_set_period()` (BSP, `bsp/quasar*/quasar_timer_ext.c`) writes `ARR = period - 1`
but **never touches `CNT`**:

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
(`MULTI_RADIO_RETRY_TIMER_PERIOD_US`, ARR ≈ 5464), the max period (`set_max_period`,
ARR = 0xFFFD = 65533), and `timer_frequency_ratio * sleep_time`
(`wps_phy.h`, `wps_phy_prepare_frame`). When the SWC **lowers** the period (e.g. 65533 → 5464)
while the live `CNT` is already **above** the new `ARR`, a **32-bit** counter does not wrap
immediately — it counts all the way to `0xFFFFFFFF` (~20 MHz tick → ~200 s) before the next
update event. During that runaway **no update IRQ fires**, so the scheduler heartbeat stops,
and because the dual-radio radios are `SLEEP_IDLE` (woken only by this MCU timer, see §4) both
radios freeze. Only a reset recovers.

### Evidence (node crash-dump; `mrt` = TIM4 update-IRQ count)
```
tim4: cen=1 arr=5464 cnt=29154652  uie=1   <- CNT >> ARR, and climbing:
tim4: cen=1 arr=5464 cnt=69154129  uie=1       ~+40,000,000 every 2 s (≈20 MHz),
tim4: cen=1 arr=5464 cnt=109154357 uie=1       heading for the 32-bit wrap.
mrt frozen at 361376 the whole time; frt (TIM8) keeps ticking; radios frozen.
```

### Our local fix (in BSP, `quasar_timer_set_period`, both `bsp/quasar` and `bsp/quasar-u535`)
```c
uint32_t auto_reload = (uint32_t)period - 1;
timer_instance->ARR = auto_reload;
if (timer_instance->CNT > auto_reload) {   // 32-bit timer: don't let CNT run away
    timer_instance->CNT = 0;
}
```
This removes the hard wedge (heartbeat stays alive, no runaway). **Question for HQ:** the same
`set_period` is in the SPARK reference BSP (`bsp/quasar/quasar_timer_ext.c`); should SWC's
multi-radio timer helper guard against `CNT > ARR` on 32-bit timers, or is the SWC expected to
only ever raise the period? (The dynamic retry/max re-arm clearly lowers it.)

## 4. Problem B — No autonomous dual-radio re-sync after prolonged loss (NEEDS HQ)

After Problem A is fixed, the node no longer hard-wedges, **but still does not re-sync on its
own**. Observed state during the failure:

- `mrt` (TIM4 heartbeat) **keeps ticking at the max-period rate** (~305 Hz, ARR = 65533).
- Radio HW counters (r1/r2 IRQ and RX-DMA) are **frozen**; radio IRQ pins idle (`irq1=irq2=0`).
- `conn = LOST`, `swc = RUN`. It recovers only when moved adjacent to the coordinator, and only
  after a long delay.

### Where the radios stop being serviced
`wps_phy_multi_process_radio_timer()` (`core/wireless/protocol_stack/multi_radio/wps_phy.c:253-290`):
the TIM4 callback wakes the radios (`phy_wakeup_multi()` + `sr_access_close()`) only in the
"main-frame" branch (line 258-259). The **else branch (lines 286-289) only re-arms the timer at
max period and never wakes the radios**:

```c
if (wps_phy->xlayer_main == NULL || (wps_phy_multi_get_tx_wakeup_mode() == MULTI_TX_WAKEUP_MODE_MANUAL) ||
    (wps_phy->xlayer_main->frame.destination_address == wps_phy->local_address)) {
    /* ... phy_wakeup_multi() + sr_access_close() ... */
} else {
    swc_hal_timer_multi_radio_timer_set_max_period();   // radios NOT woken
}
```

The else branch is taken when the prepared timeslot is a **TX-auto** slot
(`destination != local_address`, AUTO wakeup) — the design delegates waking to the radio's
hardware AUTOWAKE bit (`prepare_radio_tx`, `wps_phy_common.c` sets `AUTOWAKE`). `xlayer_main`
only advances to the next (RX) slot inside `process_next_timeslot()`
(`wps_mac.c`), which runs **only** from a frame-outcome IRQ
(`PHY_SIGNAL_FRAME_SENT_ACK/NACK/RECEIVED/MISSED`). If the schedule is parked on a TX-auto slot
whose completion IRQ never returns, `xlayer_main` never advances, and TIM4 idles at max period
forever — the radios never reach an RX slot to hear the coordinator.

### Fast-sync is not available as a workaround (dual-radio)
`swc_set_fast_sync(true, …)` returns `SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO`
(`core/wireless/api/swc/sr1100/swc_api.c:546-549`). The unsynced aggressive-RX-hunt code
(`wps_mac.c`, `prepare_rx_main`, gated on `fast_sync_enabled`) is therefore dead for two
radios. So a dual-radio node has **no aggressive sync-channel RX hunting on loss** — this
appears to be the root of "no autonomous recovery".

We found **no** explicit latched "park / give-up" state after N losses; the schedule keeps
advancing in principle. The freeze is emergent: TX-auto idle + no re-hunt.

### The park is steady-state-only: a *strong/clean* link triggers it, a *weak* link masks it

The most counter-intuitive — and most diagnostic — finding: the wedge happens when the node is
**close to the coordinator with a clean link**, and does **not** happen when the node (or
coordinator) is moved far away into a weak link. This is the opposite of an RF-margin failure,
and it pins the mechanism on the AUTO-wakeup steady state rather than on signal strength.

The freeze requires **both** of:
1. the node has **fully synced and settled into the steady-state AUTO-wakeup park** (TIM4 idling
   at max period, radios woken only by the hardware AUTOWAKE bit + frame-completion IRQs — no
   active retry/config cycling); **and**
2. the coordinator emits **dead air** on the main timeslot the node's auto-reply is waiting on
   (no RF energy at all — e.g. an **empty TX queue / no frame to send**). With no carrier to
   trigger AUTOWAKE, the auto-TX completion IRQ never fires, `xlayer_main` never advances, and
   TIM4 idles at max period forever (§4, `wps_phy.c:286-289`).

A **weak** link never satisfies (1): the node cannot hold sync (`lm` flapping near 0,
`UWB_QUALITY:WEAK`, `rx_rej` climbing from CRC-failed frames), so it stays in the
**retry/config-incomplete path** (`wps_phy.c:262-277`), which re-arms TIM4 at the *retry* period
and **re-calls `phy_wakeup_multi()` every cycle** — continuously re-waking the radios. Ironically,
a bad link is self-protecting: the node is always "awake and hunting," so a dead-air burst cannot
park it. A **strong/clean** link lets the node settle into (1), and the next coordinator dead-air
burst lands (2) and kills it. Even the weak-link CRC-failed frames help — they are RF energy that
keeps the AUTOWAKE / completion-IRQ chain alive; the close-range starved case is *pure* dead air.

**Register-level smoking gun — TIM4 `ARR`:**
- **Wedged (node close, clean link):** `arr` is **pinned at 65533** (max period) in *every*
  dump — the node never re-enters the retry/wakeup path. Radios frozen.
- **Healthy (node/coordinator far, weak link):** `arr` **toggles between the retry period
  (~4997/4919/5074 ≈ `MULTI_RADIO_RETRY_TIMER_PERIOD_US`) and 65533** — the node is continuously
  cycling through config/retry, re-waking the radios each time. `r1_irq/r2_irq` keep advancing;
  it never wedges. (See §9d.)

Practical implication: this bug is **masked by a poor link and exposed by a good one**, so it
surfaces exactly in close-range / clean-RF QA, and any change that merely *improves* RF margin
makes it **more** likely, not less. In our system the dead air of (2) comes from a coordinator
audio-producer under-run (an app/board clock-domain issue on our side); but the underlying SWC
question is general: **should a fully-synced dual-radio node be able to park unrecoverably when
the coordinator simply stops transmitting on a slot?**

## 5. Problem C — `swc_disconnect()` + `swc_connect()` does not recover a wedged node

As an app-level mitigation we added a watchdog: on the node, if the HW counters stay frozen for
6 s while paired, call `swc_disconnect()` (tolerating `SWC_ERR_DISCONNECT_TIMEOUT`) then
`swc_connect()`. Result: **it does not recover.**

```
+AUTO-RECOVER: radio stall -> swc reconnect
swc=STOP conn=LOST ... hw: r1_irq=282054 (frozen) ...   <- after disconnect+connect
swc=STOP ... (stays STOP, radios stay frozen, no reconnect)
```

`swc_disconnect()` does stop the core (`swc = STOP`), but the subsequent `swc_connect()` does
**not** return the core to `RUNNING` from this state, and the radios stay frozen.
(Note: from the frozen state `swc_disconnect()`'s handshake depends on a frame-completion IRQ
that never comes, so it hits `SWC_ERR_DISCONNECT_TIMEOUT`; `is_started` is still forced false,
but `swc_connect()` afterward does not restart servicing.) So the **only** recovery we have is a
full MCU reset (`swc_connect` from a clean boot works). **Question for HQ:** what is the
supported way for an application to force a dual-radio node back into active sync-hunting after
a prolonged loss, without a full reset?

## 6. Reproduction

1. Dual-radio node (HS) + coordinator (DG), paired and streaming.
2. Move the node ~out of RF range for ≥15 s (fallback climbs to the lowest mode, then
   `conn = LOST`).
3. Bring the node back. Node does not re-sync (radio HW counters stay frozen; see logs).
4. Only a full reset (or moving the node immediately adjacent to the coordinator, after a long
   wait) recovers it.

## 7. Key code references

| Item | Location |
| --- | --- |
| Multi-radio timer callback / TX-auto idle branch | `core/wireless/protocol_stack/multi_radio/wps_phy.c:253-290` (else: 286-289) |
| Period re-arm from radio IRQ path | `core/wireless/protocol_stack/multi_radio/wps_phy.h` (`wps_phy_prepare_frame`) |
| `xlayer_main` advance only on frame-outcome IRQ | `core/wireless/protocol_stack/wps_mac.c` (`process_next_timeslot`, `wps_mac_phy_callback`) |
| Dual-radio radios forced `SLEEP_IDLE` | `core/wireless/link/sr1100/link_tdma_sync.c` (`WPS_RADIO_COUNT != 1`) |
| Fast-sync blocked for dual-radio | `core/wireless/api/swc/sr1100/swc_api.c:546-549` |
| BSP timer set_period (Problem A) | `bsp/quasar/quasar_timer_ext.c` / `bsp/quasar-u535/quasar_timer_ext.c` (`quasar_timer_set_period`) |

## 8. Questions for HQ

1. **Dual-radio re-sync (Problem B):** what is the intended mechanism for a *dual-radio* node
   to re-acquire sync after a prolonged loss, given fast-sync is disallowed with two radios? Is
   the observed "radios never woken while parked on a TX-auto timeslot" a known limitation /
   bug, or are we mis-configuring something (schedule, sleep levels, tx_wakeup_mode)?
   In particular (see §4, "steady-state-only"): should a **fully-synced** dual-radio node in the
   AUTO-wakeup steady state be able to park **unrecoverably** when the coordinator simply stops
   transmitting on a slot (empty TX queue / dead air)? We can reproduce this at will on a
   **clean, close-range** link, while a weak link never wedges — the register signature is TIM4
   `ARR` pinned at 65533 (wedged) vs toggling to the retry period (healthy/self-recovering).
2. **App-level recovery (Problem C):** what is the supported API sequence to force a wedged
   dual-radio node back into sync-hunting without an MCU reset? (`swc_disconnect()` +
   `swc_connect()` does not recover it in our testing.)
3. **BSP timer helper (Problem A):** should the SWC multi-radio timer helper / reference BSP
   `set_period` guard against `CNT > ARR` on 32-bit timers, since the dynamic re-arm lowers the
   period below the live count?

## 9. Representative node logs

### 9a. Problem A — TIM4 counter runaway (before the BSP fix)
```
tim4: cen=1 arr=5464 cnt=29154652  uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
tim4: cen=1 arr=5464 cnt=69154129  uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
tim4: cen=1 arr=5464 cnt=109154357 uie=1 (cr1=0x1 dier=0x1)   mrt frozen 361376
(hw: r1_irq/r2_irq/r1_dma/r2_dma all frozen; frt/TIM8 keeps ticking)
```

### 9b. Problem B — TIM4 alive at max period, radios never serviced (after the BSP fix)
```
hw:    r1_irq=290474 r2_irq=290472 r1_dma=633803 r2_dma=644339   (frozen across all dumps)
sched: mrt=293074 ... 293684 ... 294295 ... 294905               (mrt ADVANCING ~305 Hz)
tim4:  cen=1 arr=65533 cnt=42229 / 1136 / 24889 / 51112 uie=1    (timer running, CNT in range)
conn=LOST swc=RUN — recovers only when adjacent to coordinator, very slowly
```

### 9c. Problem C — swc_disconnect()+swc_connect() does not recover
```
+AUTO-RECOVER: radio stall -> swc reconnect
swc=STOP conn=LOST ... hw: r1_irq=282054 (frozen) ...
swc=STOP ... (stays STOP; radios frozen; no reconnect until full MCU reset)
```

### 9d. Problem B — weak link is self-protecting (node/coordinator moved *far*, never wedges)
```
+EVENT: UWB_QUALITY:WEAK
hw:    r1_irq=4548492 ... 4556495 ... 4564501 ... 4572484   (ADVANCING — radios serviced)
sched: mrt=4548571 ... 4556584 ... 4564581 ... 4572560      (ADVANCING)
tim4:  cen=1 arr=4997 cnt=4244 / arr=4919 cnt=4565 / arr=5074 cnt=4611 / arr=65533 cnt=2757
       ^ ARR TOGGLES retry-period(~5000) <-> 65533 = node continuously re-hunting, re-waking radios
lm=10/5/0/5 (flapping near 0)  rx_rej=11675->12545 (climbing, CRC-fail)  swc=RUN conn=OK — never wedges
+EVENT: UWB_QUALITY:GOOD

Contrast 9b (node close, clean link, WEDGED): arr PINNED at 65533 every dump, r1_irq/r2_irq FROZEN.
```
