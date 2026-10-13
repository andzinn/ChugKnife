---
phase: 1
plan: 1
wave: 1
gap_closure: false
depends_on: []
files_modified:
  - Source/dsp/PitchShifter.h
  - Source/dsp/PitchShifter.cpp
autonomous: true
must_haves:
  truths:
    - "PitchShifter integrates the authentic Tone3000 correlation-spliced delay engine (MIT License)"
    - "PitchShifter supports -24 to +24 semitones (-2 to +2 octaves)"
    - "Rate-cost lag selection (1 - ncc)/jump avoids micro-jumps and prevents chord/bass distortion"
    - "Adaptive crossfade (30 ms to 120 ms) based on match quality eliminates chuffing on sustained chords"
    - "Onset re-sync preserves pick attacks with sub-5ms perceptual latency"
  artifacts:
    - "Source/dsp/PitchShifter.h"
    - "Source/dsp/PitchShifter.cpp"
---

# Plan 1.1 (Milestone v1.3): Tone3000 Pitch Engine Integration (-24 to +24 st)

## Objective
Replace the provisional pitch shifter with the authentic, battle-tested low-latency pitch engine from TONE3000 (MIT License). Implement rate-cost lag scoring, adaptive crossfade (30–120 ms), two-octave buffer geometry budgeting (-24 to +24 semitones), and onset re-sync for drop-tuning and whammy sweeps.

## Tasks

<task type="auto">
  <name>Task 1: Implement Tone3000 Pitch Engine in Source/dsp/PitchShifter</name>
  <files>
    Source/dsp/PitchShifter.h
    Source/dsp/PitchShifter.cpp
  </files>
  <action>
    1. In Source/dsp/PitchShifter.h and Source/dsp/PitchShifter.cpp:
       - Incorporate Tone3000's correlation-spliced delay line implementation with full MIT copyright attribution.
       - Support semitones from -24 to +24.
       - Maintain clean JUCE audio processor interface: prepare(), reset(), setSemitones(), setBypassed(), getCurrentDelaySamples(), process().
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    PitchShifter compiles cleanly with Tone3000 engine supporting -24 to +24 semitones.
  </done>
</task>
