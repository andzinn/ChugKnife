---
phase: 2
level: 2
researched_at: 2026-10-12
---

# Phase 2 Research — TS808 Gain Staging

## Questions Investigated
1. Why does Drive 0 + Level max produce a wall of distortion?
2. What does the real TS808 do at Drive 0?
3. What is the real tone-stage network?

## Root cause (audit of v1.0 TubeScreamer.cpp)
1. **Made-up makeup gain.** Output was `vout * 2.2` and then `* (0.4 + level*3.5)`.
   At Level max that is ×8.6 (+18.7 dB) on top of the circuit's own output.
   The real Level control is a 100 k audio-taper **potentiometer divider**: it can only attenuate.
   With a ~0.8 V circuit output mapped as 1 V = 1.0 FS, the plugin was sending peaks around
   +20 dBFS into the amp. That is the "wall of noise" — the noise floor was raised by the same amount.
2. **No volt calibration.** Digital 1.0 was treated as ~0.45 V. A Hi-Z interface input clips around
   +12 dBu ≈ 4.6 V peak, so a guitar peaking at 0.5 V reads ≈ -19 dBFS. Using 1.0 FS = 4 V puts the
   diodes at the right operating point for a real DI.
3. Wrong diode emission coefficient (1.25; SPICE 1N4148 is N = 1.752, Is = 2.52 nA).
4. Level smoother advanced once per block; Drive smoother reset at host rate but run at 2× rate.
5. Tone stage was a generic high shelf, not the TS network.

## What the real pedal does at Drive 0 (honest expectation)
Clipping stage gain above ~720 Hz is 1 + (51 k)/(4.7 k) ≈ 11.9 even at Drive 0, so hard picking
does lightly clip — the TS808 is never a perfectly clean boost. What makes it work as a "boost" is:
bass passes at unity (Vout = Vin + Vd, and Cz blocks low frequencies from the gain path), mids are
lifted, and the output is bounded at roughly Vin + 0.6 V. So the correct target is:
**bounded output, modest mid boost, mild peak compression — not distortion.**

## Circuit (ElectroSmash/Geofex TS808 values, Yeh DAFx-07 structure)
- Clipping stage (non-inverting op-amp): Rz = 4.7 k + Cz = 47 nF to ground; feedback
  Rf = 51 k + Drive·500 k ‖ Cf = 51 pF ‖ antiparallel 1N4148. Vout = Vin + Vd.
- Tone stage: R7 = 1 k into node P, C5 = 220 nF P→GND (723 Hz LP). 20 k tone pot from P to the
  op-amp inverting node N; wiper → R8 = 220 Ω → C6 = 220 nF → GND. R9 = 1 k from output to N.
  LF: unity. Pot toward N: HF gain → 1 + 1 k/220 ≈ +14.8 dB above ~3.3 kHz. Toward P: HF shunted.
- Level: 100 k A-taper divider (≤ 0 dB).

## Decisions Made
| Decision | Choice | Rationale |
|---|---|---|
| Calibration | 1.0 FS = 4 V in and out | Matches Hi-Z interface headroom; same scale both ways keeps unity reference |
| Diode solver | Newton on g(v)=Geq·v+2Is·sinh(v/nVt)-I, started from the analytic upper bound min(I/Geq, nVt·asinh(I/2Is)) | g is odd, monotonic, convex for v>0 → NR from an upper bound converges monotonically, no damping hacks |
| Tone stage | 3-node MNA, trapezoidal capacitor companions, matrix inverse cached per tone change | Exact network, no hand-derived transfer function errors |
| Oversampling | 4× polyphase IIR around the clipping stage only | 2× was weak for diode harmonics |
| Level | dB taper: 0 → mute, 1 → 0 dB, 0.5 → -20 dB | A-taper behaviour, never boosts |
