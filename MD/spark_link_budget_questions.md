# EVK Audio Demo v1.4.0 out-ranges our build — which combination gets you there?

**To:** Gab (SPARK)

**Reference:** `SPARK_AUDIO_DEMO_node_v1.4.0.bin` + `..._analog_coordinator_v1.4.0.bin`
**Ours:** `puretone_unidirectional` **v2.4.0 rc01**, SDK v2.4.0-rc2
**Test:** same two Quasar EVK boards, **single radio at both ends**, same room, 5 m with a
human body blocking line of sight
**Result:** **v1.4.0 is better at both ends — close-in blocking and far drop point.**

At our far drop point: `cca_fail = 0` (packets do go out), `rx_rej` climbing (they arrive
and fail to decode), `lm = 0`.

---

## What we changed and measured

Helped close-in, **none of it moved the far drop point**:

1. ISI mitigation level 2 (level 1 is not enough against close-in multipath)
2. Retransmission budget: mode 3 = 3.7x, mode 4 = 6.4x
3. Buffer on modes 3 and 4 = 40 ms
4. Mode 3 payload 100 B -> 54 B (accumulator 46/10 -> 23/10)

On ISI: one level has to serve the whole ladder here. In SDK v2.4.0-rc2
`swc_connection_fallback_cfg_t` carries only `enabled`, `fallback_mode_count`, `thresholds`
and `cca_try_count` — there is no per-rung PHY preset — and `isi_mitig` is set on the
connection. So we cannot give the ADPCM rungs a different ISI level from the top rung: the
preamble has to fit the largest payload on the ladder into a 250 us slot, which is why
level 3 breaks the top rung for us. **Is per-rung ISI available in your SDK, and does
v1.4.0 use it?**

No effect at all, each measured on hardware:

5. CCA try count 2 -> 14 (no-op, `cca_fail` is 0)
6. Fallback level-4 TX power raised to width 6/6/6/6, gain 1/1/1/0 — measured twice
7. Fallback level-4 TX power to the ceiling (width 7/7/7/7, gain 0/0/0/0)
8. Node ACK TX power to the ceiling (width 7, gain 0)
9. `RADIO_USE_SAVED_CALIB` false -> true
10. ISI level 3 (top rung breaks) / buffer 30 ms (close-in dropouts return)
11. Ladder free-running vs locked to the bottom rung — identical drop point

## Our configuration (v2.4.0 rc01)

**Fallback ladder**

| mode | audio | payload | retransmissions | buffer |
|---|---|---|---|---|
| 0 | 96 kHz 24-bit | 242 B | 1.0x | 5 ms |
| 1 | 48 kHz 24-bit | 206 B | 1.0x | 7 ms |
| 2 | 48 kHz 16-bit | 138 B | 1.0x | 10 ms |
| 3 | 48 kHz ADPCM stereo | 54 B | 3.7x | 40 ms |
| 4 | 24 kHz ADPCM stereo | 48 B | 6.4x | 40 ms |

**Radio / SWC**

| | |
|---|---|
| Schedule | 21 slots x 250 us = 5.25 ms (coordinator 20 / node 1) |
| ISI mitigation | level 2, both ends |
| Chip rate | 20.48 MHz |
| Pulse count | `SR1100_PULSE_COUNT` = 1 |
| FEC / modulation | `SWC_FEC_1_2_5_0` / IOOK |
| Concurrency | high performance |
| Channels | `{164, 174, 184, 194}`, sequence `{0, 1, 2, 3}` |
| CCA | main channel try 2 / retry 96; fallback try counts 7 / 13 / 14 / 14 |
| Node ACK power | width 5, gain 1 |
| Fallback level-4 TX power | width 7/7/7/7, gain 1/1/1/0 (levels 1-3 step up normally) |
| Saved calibration | `RADIO_USE_SAVED_CALIB` = false |

## Our question

**Which combination gets v1.4.0 its range?** For example: timeslot layout + bottom-rung
payload + retransmission ratio — or is the wireless core itself doing the work? If you can
name the three or four items that matter, and their values, we will change exactly those
and re-measure. We would rather implement your recipe than keep testing one knob at a time.
