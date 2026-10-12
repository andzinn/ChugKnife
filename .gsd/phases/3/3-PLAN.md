---
phase: 3
plan: 1
wave: 1
gap_closure: false
depends_on: []
files_modified:
  - Source/dsp/Chugger.h
  - Source/dsp/Chugger.cpp
autonomous: true
must_haves:
  truths:
    - "Chugger uses a phase-coherent 3-band LR4 crossover (Low < 150 Hz, Mid 150 Hz - 1.8 kHz, High > 1.8 kHz) that sums with flat magnitude"
    - "Tightening is keyed on low-band dominance (envLow_dB - envRest_dB), making palm-mute detection level-independent"
    - "Pick attack transient boost is applied to the high band exclusively during attacks"
    - "At Chug = 0, processing is perfectly transparent (gains = 1.0, zero comb filtering, no dry/wet blending)"
  artifacts:
    - "Source/dsp/Chugger.h"
    - "Source/dsp/Chugger.cpp"
---

# Plan 3.1: Chugger v3 — Palm-Mute-Aware Dynamic Tightener

## Objective
Replace the flawed v2 Chugger (which suffered from level-dependent linear detectors, comb-filtering dry/wet blends, and tone discoloration on open chords) with Chugger v3: a phase-coherent 3-band Linkwitz-Riley 4th order (LR4) crossover with low-band allpass alignment, dynamic low-band compression keyed on level-independent palm-mute low dominance (`envLow_dB - envRest_dB`), and high-band transient pick attack shaping without any dry/wet blending.

## Context
Load these files for context:
- .gsd/SPEC.md
- .gsd/DECISIONS.md
- .gsd/phases/3/RESEARCH.md
- Source/dsp/Chugger.h
- Source/dsp/Chugger.cpp

## Tasks

<task type="auto">
  <name>Task 1: Build phase-coherent 3-band LR4 crossover architecture</name>
  <files>
    Source/dsp/Chugger.h
    Source/dsp/Chugger.cpp
  </files>
  <action>
    1. In Chugger.h and Chugger.cpp:
       - Implement 3-band Linkwitz-Riley 4th-order (LR4) crossover:
         a) Low crossover at 150 Hz (cascaded 2-pole Butterworth LP and HP).
         b) High crossover at 1800 Hz (cascaded 2-pole Butterworth LP and HP).
         c) The low band is filtered through an LR 2nd/4th order all-pass at 1800 Hz to match the phase shift of the mid-band high-pass, guaranteeing that Low + Mid + High sums perfectly to an all-pass with flat magnitude (0 dB at all frequencies).
       - Maintain separate per-channel filter states for low, mid, and high bands.
       - Verify that when band gains are 1.0 (Chug = 0), the output is mathematically identical to an all-passed version of the input, with zero magnitude coloration and zero comb filtering.
    AVOID: Dry/wet mixing of high-passed and unfiltered signals which causes severe comb filtering.
    USE: Exact Linkwitz-Riley phase-coherent multiband crossover with all-pass alignment.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    Phase-coherent 3-band crossover compiles cleanly and sums to flat magnitude at unity gain.
  </done>
</task>

<task type="auto">
  <name>Task 2: Implement low-dominance palm-mute clamp and transient attack shaper</name>
  <files>
    Source/dsp/Chugger.h
    Source/dsp/Chugger.cpp
  </files>
  <action>
    1. Mono Sum Detection:
       - Compute mono sum for detection so stereo imaging remains rock-solid and linked across channels.
    2. Low-Dominance Palm-Mute Detector:
       - Track RMS/peak envelope of low band (< 150 Hz) and rest of spectrum (> 150 Hz).
       - Convert to dB: envLowDb and envRestDb.
       - Dominance = envLowDb - envRestDb.
       - During palm mutes, fundamentals dominate (dominance > threshold); open chords are harmonically rich (dominance is low/negative).
       - Threshold scales with Chug knob: from +6 dB at Chug 0 down to -10 dB at Chug 1.
       - Low-band gain reduction: attack 4 ms (lets the first thump cycle punch through), release 80 ms (clamps the muddy low-end boom), max reduction 12 dB * Chug.
    3. High-Band Transient Attack Shaper:
       - High band (> 1.8 kHz) energy tracked with fast envelope (2 ms) and slow envelope (25 ms).
       - Transient delta = clamp(fastDb - slowDb, 0.0, 12.0 dB).
       - High boost = transient delta * 0.6 * Chug dB (up to +7.2 dB during initial 10 ms pick attack).
    4. Recombine:
       - output = low * lowGain + mid * 1.0 + high * highGain.
       - If Chug == 0.0: lowGain = 1.0, highGain = 1.0 -> completely transparent.
    AVOID: Level-dependent linear amplitude thresholds that fail on quiet or loud pickups.
    USE: Level-independent spectral ratio (dominance) in decibels.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnife_Standalone
  </verify>
  <done>
    Chugger v3 reacts specifically to palm mutes, injects pick bite on attacks, is transparent at Chug 0, and builds with exit code 0.
  </done>
</task>

## Must-Haves
After all tasks complete, verify:
- [ ] VST3 and Standalone targets build with exit code 0
- [ ] At Chug = 0, signal passes through crossover with flat magnitude response
- [ ] Palm mute tightening triggers when low band dominates, not simply when loud
- [ ] High-band transient attack activates on pick strikes without adding steady fizz
