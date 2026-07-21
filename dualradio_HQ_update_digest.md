# Dual-radio node park — new findings this week

*Short version for HQ. Full detail (register-level logs, traced code paths) in
`dualradio_resync_HQ_report.md` §3 / §4.1 / Appendix A.7 — available on request.*

Two results from testing on the U535 pair since the last report:

**1. The `fb = 0` (96 kHz) audio up-shift is confirmed as one trigger — and capping below it removes that trigger.**
We deactivated audio mode 0 on the coordinator (auto fallback still on, so it degrades to 48 kHz
variants but can never up-shift to `fb = 0`). The ~11 s spontaneous park is **gone** — on a good link
the node runs steadily, TIM4 `ARR` toggles healthily (65533 ↔ ~5436), radio IRQ counters keep
advancing.

**2. But the node still parks — via range-cycle → re-sync — with the *identical* wedge.**
Cycling the same node in and out of RF range, one re-sync wedges into exactly the EVK park state:

```
 t=136033  LOST  tim4 arr=5387  r1_irq=524302   (healthy re-hunt: ARR at retry period, radios advancing)
 t=138031  LOST  tim4 arr=65533 r1_irq=531470   (ARR jumps to max period)
 t=140031  LOST  tim4 arr=65533 r1_irq=531470   <-- FROZEN (ARR pinned; mrt still advancing; radios dead)
+AUTO-RECOVER: radio stall -> swc reconnect  ->  swc=STOP, radios stay frozen (does not recover)
```

Same signature as the EVK park: `r1/r2_irq` frozen, `ARR` pinned at 65533, `mrt`/`CNT` still
advancing, no CPU fault, `swc` restart cannot recover.

**What this means:**

- **One wedge, two triggers.** The `fb = 0` up-shift and the re-sync-after-loss are two entry points
  into the *same* defect — the multi-radio timer (TIM4) left at max period and never re-armed. A fix
  at the re-arm path should close both.
- **The U535 board now reproduces the EVK re-sync park on demand and fast** — just range-cycle a
  U535↔U535 pair; no ≥15 s wait, no U5A5 EVK needed. Cheapest rig to instrument the re-arm path.
- The park is **not** an audio-mode transition: our per-transition probe logged no `fb` change
  anywhere near the drop (fb stayed pinned at the bottom mode on the weak link). It is purely the
  re-sync path.

Happy to send the full report with register-level logs and the code paths we traced.
