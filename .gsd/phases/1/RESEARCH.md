---
phase: 1
level: 1
researched_at: 2026-10-12
---

# Phase 1 Research — Routing + Internal 4CM + Gate

## Questions Investigated
1. How to do 4CM without extra buses?
2. Is the current gate detector correct for low-tuned guitars?
3. Does the processing chain introduce delay between DI (detector) and the gated audio?

## Findings

### 1. Internal 4CM
Copy the input to a pre-allocated `diBuffer` at the top of `processBlock`, run the chain in-place
on the main buffer, then call `gate.process(main, diBuffer)`. Detector and VCA are decoupled, so
TS808 hiss / Chugger never reach the detector. Pre-allocate in `prepareToPlay` (current code calls
`setSize` in `processBlock` — allowed with `avoidReallocating`, but simpler to size once).

Trade-off (documented to user): with 2-in/2-out the gate sits *before* an amp sim placed after the
plugin. It removes the noise the amp would amplify (pickups + TS808), but not the amp's own hiss.

### 2. Gate detector bug (audit)
`NoiseGate.cpp:118-121`: one-pole envelope with 8 ms release on a raw peak signal. Period of
B1 = 16 ms, F#1 = 21.7 ms. Between waveform peaks the envelope decays by exp(-16/8) ≈ -17 dB,
which exceeds the 5 dB hysteresis, and the 12 ms hold is shorter than one period → the gate
closes inside each cycle on sustained low notes near threshold (chatter / AM distortion).
**Fix:** sliding-window maximum over 23 ms (1 ms cells, Tone3000's approach in its pitch detector),
which is ripple-free for any fundamental ≥ 44 Hz, then a fast gain ramp (attack 0.15 ms,
release 6 ms). Close latency after a mute ≈ 23 ms + ramp: still tight.

### 3. Delay mismatch
The pitch shifter delays audio by 2.5–35 ms (drifting tap) while the detector sees the DI with zero
delay. The gate would close up to 35 ms before the delayed note ends. **Mitigation:** expose the
shifter's current tap delay and add it to the gate hold each block. Opening early is harmless
(acts as look-ahead).

### Other audit findings (out of scope, logged in TODO)
- `PitchShifter::findBestCorrelationLag` runs ~200k Hermite reads in one sample (burst CPU on splice).
- Gate `currentGainDb` read by UI thread without atomics.

## Decisions Made
| Decision | Choice | Rationale |
|---|---|---|
| Buses | 1 in / 1 out, mono or stereo, matching | DAW usability (user request) |
| Detector | 23 ms sliding max, 1 ms cells | No ripple on low notes, O(1)-ish cost |
| Delay comp | extra hold = shifter tap delay | Simple, correct direction |
