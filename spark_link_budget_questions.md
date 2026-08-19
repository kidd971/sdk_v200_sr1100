# Two API questions on link budget — SR1100 / SDK v2.4.0-rc2

## Setup

SR1100, SDK v2.4.0-rc2, single radio at both ends, unidirectional audio with a
five-rung fallback ladder, 21 × 250 µs timeslots (5.25 ms schedule).

## What we are seeing

At a fixed distance of **5 m with a human body blocking line of sight**, the link
drops. The test is the same every time — same distance, same blocking — so our
measurements are comparable with each other.

Two counters separate the two failure modes for us:

- `cca_fail` (coordinator) — **0 at every distance we have measured**, so packets
  are being transmitted, not held back by CCA.
- `rx_rej` (node) — climbs during the drop, i.e. packets arrive and fail to decode.

## What we have already ruled out

We are not asking you to re-suggest these — all were measured on hardware:

| Tried | Result |
|---|---|
| CCA try count 2 → 14 | No change (`cca_fail` is 0, so this is a no-op) |
| ISI mitigation level 1 / 2 / 3 | Level 2 is what we run. Level 1 is not enough close-in; level 3 breaks the top rung. Helps multipath, does nothing for distance |
| Fallback level-4 TX power → your demo's values | No change |
| Fallback level-4 TX power → maximum (width 7/7/7/7, gain 0/0/0/0) | No change |
| Node ACK TX power → maximum (width 7, gain 0) | No change |
| `RADIO_USE_SAVED_CALIB` false → true (as in your demo) | No change |
| Bottom-rung buffer 30 → 40 ms | Fixes close-in dropouts, does nothing for distance |
| Retransmission budget (accumulator) increased | Same |
| Ladder free-running vs locked to the bottom rung | Identical drop point, so this is not a ladder-threshold problem |

We have also confirmed these already match your demo exactly: fallback TX power
levels 1–3, `SR1100_PULSE_COUNT` (1 on both sides), chip rate (20.48 MHz),
FEC, modulation, concurrency mode, channels, and node ACK power.

So on our side every "give it more energy" knob reachable from the application
has been tried and none of them moved the drop point.

## Question 1 — `chip_repet` is missing from `swc_connection_cfg_t`

Chip repetition is the one setting that multiplies energy per bit, and in
v2.4.0-rc2 we cannot reach it at all:

| | v2.4.0-rc2 |
|---|---|
| `swc_chip_repetition_t` type | present (`swc_def.h`) |
| `swc_connection_cfg_t.chip_repet` field | **absent** |
| Any public function that sets it | **none found** |
| Precompiled library symbol `wps_set_chip_repet` | exported, but not declared in any header |

The SDK our reference demo was built against has the field in
`swc_connection_cfg_t`.

**Is the removal intentional, or is this a header that did not get updated?
If it is intentional, what is the supported way to set chip repetition in
v2.4.0?**

(For completeness: we checked, and the demo leaves it at zero-init, i.e.
`SWC_CHIP_REPET_1`, so this is not the explanation for any difference between
the two builds. We are asking because it is the only remaining energy knob and
we currently have no way to evaluate it.)

## Question 2 — will `swc_connection_fallback_cfg_t.presets` ship in v2.4.0?

Per-rung PHY presets are not available to us in v2.4.0-rc2, so a single ISI
mitigation level has to serve the whole ladder. That forces a compromise: the
top rung wants a short preamble, the bottom rung wants multipath resistance, and
level 2 is the value that is wrong for both by the smallest amount.

**Will `presets` be in the v2.4.0 release?** We are not expecting it to solve the
range problem — we would like to stop trading the top rung against the bottom one.

## One smaller thing

Could you tell us **under what conditions your published range figure is
measured** — free space or with blocking, what orientation, and whether the
fallback ladder is free-running or locked? Ours is 5 m with a human body in the
path, which is a different physical situation from free-space range, and we would
like to be sure we are comparing the same measurement before we read anything
into the difference.
