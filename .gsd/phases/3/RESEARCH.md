---
phase: 3
level: 2
researched_at: 2026-10-12
---

# Phase 3 Research — What makes a palm mute "chug"

## Why the v2 Chugger failed (audit)
1. **Level-dependent detectors.** `attackDelta` and `envTightness` are linear amplitudes (~0.01–0.1
   on a DI). Multiplied by `chug*2.8` the "clank" boost was a fraction of a dB → inaudible. The
   behaviour changed with input level instead of with playing technique.
2. **Dry/wet blend of filtered signals.** `0.1·dry + 0.9·highpassed` is not a high-pass; it is a
   phase-smeared shelf that comb-filters against the dry → "estraga o timbre".
3. Triggered on *any* pick attack (open chords too), not on palm mutes.

## What the analog world does
- **TS808 / Maxon / Precision Drive / Fortin 33 in front of a high-gain amp**: cut lows *before*
  the amp's clipping stages (TS ≈ 720 Hz on the gain path; Precision Drive "Tight" = variable HPF;
  Fortin 33 "tight" switch), add mid push and pick attack. Lows into a cascaded high-gain preamp
  cause intermodulation and blocking ("flub"); the weight is restored later by the cab / power-amp
  resonance, where it is not distorted.
- **The cost**: the cut is static, so open chords and single notes also lose body.
- **Mix engineers' fix**: a *dynamic* EQ band at ~100–200 Hz that only clamps when palm mutes bloom.

## Palm-mute signature (physics)
Damping the strings at the bridge kills upper harmonics fast; the fundamental remains → during a
palm mute the low band **dominates** the spectrum. Open chords are harmonically rich → low band is
well below the rest. The ratio (low dB − rest dB) is level-independent and technique-dependent:
exactly the discriminator we need.

## Design (Chugger v3)
1. Phase-coherent 3-band LR4 split: low < 150 Hz, mid, high > 1.8 kHz; low band passed through an
   LR all-pass at 1.8 kHz so low+mid+high sums to an all-pass (flat magnitude).
2. **Tight**: low-band compressor keyed on dominance = envLow_dB − envRest_dB.
   GR = (dominance − thr)·(1 − 1/3), thr from +6 dB (Chug 0) to −10 dB (Chug 1), max 12 dB.
   Attack 4 ms (first cycle of the thump passes → punch), release 80 ms (bloom is held down).
3. **Attack**: high-band transient shaper: t = fastEnv_dB − slowEnv_dB (both of the high band),
   boost = clamp(t, 0, 12)·0.6·Chug dB → up to ~+7 dB in the first ~10 ms of each pick.
4. Detection on the mono sum, gains linked across channels (stable stereo image).
5. Chug = 0 → gains are exactly 1 → all-pass, magnitude-transparent. No dry/wet mixing.

## Honest limitation
Thresholds are first-pass values verified on synthetic palm-mute vs open-chord signals in the test
harness; final voicing needs a listening pass with a real DI + amp.
