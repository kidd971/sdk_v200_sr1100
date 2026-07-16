# SPARK SWC — Dual-radio node parks (radios never serviced again) and cannot re-sync

**Reproduces on stock Quasar EVK: U5A5 coordinator + U5A5 node, SR1100, dual radio**
(`WPS_RADIO_COUNT == 2`, `tx_wakeup_mode = AUTO`, fast-sync off, SWC app build `v2.3.0`).

All behaviour below is with the BSP timer fix of §3 already applied.


---

## 1. Behaviour

**Repro:** node + coordinator paired and streaming → move the node out of RF range for **≥15 s**
(fallback climbs down, then `conn = LOST`) → bring it back.

**Result:** the node **never re-synchronizes**. Its radios are never serviced again. Only an MCU
reset recovers it. The coordinator stays fully healthy throughout.

What we observe on the node while it is parked:

```
hw:    r1_irq=290474 r2_irq=290472 r1_dma=633803 r2_dma=644339   <-- FROZEN across all dumps
sched: mrt=293074 ... 293684 ... 294295 ... 294905               <-- TIM4 update IRQ ADVANCING
tim4:  cen=1 arr=65533 cnt=42229 / 1136 / 24889 / 51112 uie=1    <-- running, pinned at max period
conn=LOST  swc=RUN  fault: cfsr=0 hfsr=0 pc=0 lr=0               <-- no CPU fault
```

**The scheduler is alive, TIM4 is alive, the CPU is fine — only the radios are dead.**
Radio IRQ pins idle (`irq1 = irq2 = 0`). `ARR` stays pinned at 65533; on a healthy node it toggles
to the retry period.

Additional observations:

- **`swc_disconnect()` + `swc_connect()` does not recover it.** `swc_disconnect()` hits
  `SWC_ERR_DISCONNECT_TIMEOUT` (its handshake waits on a frame-completion IRQ that never comes) and
  sets `swc = STOP`; the following `swc_connect()` does not return the core to `RUNNING` and the
  radios stay frozen. `swc_connect()` from a clean boot works.
- **Fast-sync is unavailable to us:** `swc_set_fast_sync(true, …)` returns
  `SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO` (`swc_api.c:546-549`).

## 2. Where we are stuck

We have no recovery path from the application side: fast-sync is blocked for dual radio,
`swc_disconnect()` + `swc_connect()` does not restart servicing, and only an MCU reset brings the
node back. We have characterised the failure as far as we can from the outside — the rest needs
your eyes on the stack.

## 3. A fix we already made — please review

While diagnosing the above we found and fixed a bug in the reference BSP. **We would like your
review: is this the right fix, or is the SWC expected to behave differently here?**

`quasar_timer_set_period()` (`bsp/quasar/quasar_timer_ext.c`) writes `ARR = period - 1` but never
touches `CNT`. TIM4 on STM32U5 is **32-bit**, so when the SWC lowers the period below the live count
(the dynamic retry/max re-arm does exactly this), the counter does not wrap — it runs all the way to
`0xFFFFFFFF` (~200 s at ~20 MHz) with **no update IRQ**, freezing the scheduler and both radios:

```
tim4: cen=1 arr=5464 cnt=29154652  uie=1   <- CNT >> ARR, climbing ~40M every 2 s
mrt frozen the whole time; frt (TIM8) keeps ticking; radios frozen
```

Our change:

```c
uint32_t auto_reload = (uint32_t)period - 1;
timer_instance->ARR = auto_reload;
if (timer_instance->CNT > auto_reload) {   /* 32-bit timer: don't let CNT run away */
    timer_instance->CNT = 0;
}
```

**Is this acceptable?** Or should the multi-radio timer helper guard against `CNT > ARR` itself —
or is the SWC expected to only ever *raise* the period? (The retry/max re-arm clearly lowers it.)

Note: all of the §1 behaviour was observed **with this fix already applied** — the §1 park is a
separate problem, not this timer bug.

---

# Appendix — a second, faster trigger on our STM32U535 field board

*(Different MCU from the EVK. Included only because it ends in **exactly the same park state** as
§1 — same frozen radios, same `ARR` pinned at 65533, same `mrt` still advancing, same inability to
recover. It looks like a second, much faster route into the same defect.)*

**No RF manipulation — ~11 s from boot, 100 % of the time.** On a **healthy, close-range** link the
node parks **at the exact sample the audio fallback up-shifts to `fb = 0` (96 kHz, top mode)**:

```
[LW 1 t=3383 ] OK lm=160 fb=2 rx_rej=1  r1_irq=8184    (advancing, healthy)
[LW 3 t=7384 ] OK lm=85  fb=1 rx_rej=47 r1_irq=24192   (advancing, healthy)
[LW 5 t=11384] OK lm=50  fb=0 rx_rej=56 r1_irq=39716   <-- fallback reaches 96 kHz
[LW 6 t=13384] OK lm=50  fb=0 rx_rej=56 r1_irq=39716   <-- FROZEN, forever
       tim4: arr=65533 pinned; cnt + mrt still advancing; no CPU fault
```

**Not RF:** link margin is healthy and corrupted-frame count ≈ 0 at the freeze — the node is being
*promoted* to the top mode because the link is **good**. A weak link never climbs to `fb = 0` and
never parks. Better RF makes it more likely, not less.

**It only happens when both ends are the same MCU:**

| Coordinator | Node | Parks at `fb = 0`? |
| --- | --- | --- |
| **U535** | **U535** | **Yes — ~11 s, every time** |
| U535 | U5A5 | No |
| U5A5 | U535 | No |
| U5A5 | U5A5 | No (but still parks on the §1 15 s repro) |

The **same U535 node** that parks in ~11 s against a U535 coordinator runs 96 kHz **indefinitely**
against a U5A5 coordinator — so it is not "U535 is too slow for 96 kHz"; only the *pairing* matters.
Pinning 96 kHz (`fallback_cfg.enabled = false`) makes the node never acquire sync at all.

**Already ruled out:** coordinator producer starvation (measured 2400 buffers/s, exactly nominal,
steady through the failure); UART RX IRQ storm (disabled the receiver entirely — no change); RF/link
quality (margin healthy, `rx_rej` ≈ 0); the §3 BSP timer bug (fixed; `ARR = 65533` with `CNT` in
range at the freeze).

**If you are willing to look:** is there a known timing constraint for dual-radio at **96 kHz**, and
is there any reason the *coordinator's* MCU would affect the *node's* radio servicing?
