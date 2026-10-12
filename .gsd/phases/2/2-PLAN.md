---
phase: 2
plan: 1
wave: 1
gap_closure: false
depends_on: []
files_modified:
  - Source/dsp/TubeScreamer.h
  - Source/dsp/TubeScreamer.cpp
autonomous: true
must_haves:
  truths:
    - "1.0 FS is calibrated to 4.0 V peak in and out; diodes operate at real physical voltages"
    - "Diode solver uses physical 1N4148 parameters (N=1.752, Is=2.52 nA) and converges monotonically from analytic upper bound"
    - "Tone stage simulates the real TS808 dual-branch RC network rather than a generic shelf"
    - "Level control is a true audio-taper attenuator (<= 0 dB; 0.5 = -20 dB, 1.0 = 0 dB) without fictitious makeup gain"
  artifacts:
    - "Source/dsp/TubeScreamer.h"
    - "Source/dsp/TubeScreamer.cpp"
---

# Plan 2.1: TS808 Gain Staging and Physical Circuit Model Rewrite

## Objective
Rewrite the Tube Screamer virtual analog processor to eliminate the wall of distortion caused by fictitious makeup gain and uncalibrated digital scaling. Calibrate the volt interface to 1.0 FS = 4.0 V peak, upgrade oversampling to 4x, deploy an unconditionally convergent Newton-Raphson diode solver with SPICE 1N4148 parameters, implement the genuine TS808 active tone RC network (R7/C5/RV/R8/C6/R9), and make the Level knob operate as a true passive audio-taper attenuator.

## Context
Load these files for context:
- .gsd/SPEC.md
- .gsd/DECISIONS.md
- .gsd/phases/2/RESEARCH.md
- Source/dsp/TubeScreamer.h
- Source/dsp/TubeScreamer.cpp

## Tasks

<task type="auto">
  <name>Task 1: Implement 4V volt calibration, 4x oversampling, and guaranteed-convergent diode solver</name>
  <files>
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
  </files>
  <action>
    1. Volt Calibration:
       - Define kVoltFullScale = 4.0 V.
       - vin = sample * 4.0; vout is converted back to digital as vout / 4.0.
       - Remove the arbitrary 0.45 input attenuation and 2.2 output multiplier.
    2. 4x Oversampling:
       - Update oversampling to factor 2 (2^2 = 4x) using filterHalfBandPolyphaseIIR.
       - Run clipping stage and feedback loop at internalSampleRate = 4 * sampleRate.
    3. Diode Solver:
       - Use SPICE 1N4148 parameters: Is = 2.52e-9 A, N = 1.752, Vt = 0.02585 V (vDenom = N * Vt = 0.04529 V).
       - Diode current: Id(vd) = 2 * Is * sinh(vd / vDenom).
       - Solve g(vd) = Geq * vd + 2 * Is * sinh(vd / vDenom) - iTarget = 0.
       - Because g(v) is odd, strictly monotonic and convex for v > 0, start Newton-Raphson from the analytic upper bound:
         v_upper = min(iTarget / Geq, vDenom * asinh(iTarget / (2 * Is))) for iTarget >= 0 (and negative counterpart for iTarget < 0).
       - Newton from an upper bound converges monotonically without oscillation or damping hacks.
       - Smooth drive parameter per sample at internal oversampled rate.
    AVOID: Arbitrary makeup gain multipliers or damping heuristics that mask divergence.
    USE: Exact physical voltage scaling and monotonic Newton-Raphson from analytic bound.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    Clipping stage is physically calibrated to 4V, runs at 4x oversampling, and diode solver converges monotonically.
  </done>
</task>

<task type="auto">
  <name>Task 2: Model TS808 RC tone network and audio-taper Level attenuator</name>
  <files>
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
  </files>
  <action>
    1. TS808 Tone Stage RC Network:
       - Circuit: R7 = 1 k, C5 = 220 nF to GND (723 Hz LP). 20 k linear tone potentiometer RV; wiper to R8 = 220 ohm in series with C6 = 220 nF to GND. Feedback resistor R9 = 1 k across inverting op-amp.
       - Compute exact bilinear-transform coefficients for the tone stage transfer function based on pot setting (p in [0, 1]).
       - Post-filter: Output buffer passive LP (R = 10 k, C = 0.1 uF -> 159 Hz / output stage, or 10k/0.22uF).
    2. Level Control:
       - The real Level control is a 100k audio-taper potentiometer divider: it attenuates output, NEVER boosts.
       - Map levelParam in [0, 1] to audio taper dB:
         level == 0.0 -> -inf dB (silence/mute)
         level == 0.5 -> -20.0 dB
         level == 1.0 -> 0.0 dB (unity with clipping stage output)
         Use standard logarithmic/audio taper curve: gain = pow(level, 3.0) or smoothed Decibels mapping.
       - Smooth level per-sample at host rate.
    AVOID: Level makeup gain of (0.4 + level * 3.5) which added up to +18.7 dB of artificial distortion and noise.
    USE: True potentiometer attenuation (maximum 0 dB).
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnife_Standalone
  </verify>
  <done>
    Tone network accurately reproduces TS808 frequency response and Level control attenuates cleanly without noise amplification.
  </done>
</task>

## Must-Haves
After all tasks complete, verify:
- [ ] VST3 and Standalone build with exit code 0
- [ ] Drive 0 + Level max does not clip into digital distortion (output bounded <= 0 dBFS)
- [ ] Level is strictly attenuating (gain <= 1.0 at Level 1.0, 0.0 at Level 0.0)
- [ ] Diode solver runs with physical 1N4148 parameters and 4V calibration
