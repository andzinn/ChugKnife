---
phase: 4
plan: 1
wave: 1
gap_closure: false
depends_on: []
files_modified:
  - CMakeLists.txt
  - Source/tests/ChugKnifeTests.cpp
autonomous: true
must_haves:
  truths:
    - "ChugKnifeTests console target builds and executes with 0 failures"
    - "NoiseGate test verifies zero ripple/chatter on 44 Hz sustained low note"
    - "TubeScreamer test verifies Drive 0 + Level max is bounded and Level acts as pure attenuator"
    - "Chugger test verifies flat magnitude response (< 0.1 dB deviation) at Chug = 0 and selective palm-mute clamping"
    - "PluginProcessor verifies 2-in / 2-out matching layout support"
  artifacts:
    - "CMakeLists.txt"
    - "Source/tests/ChugKnifeTests.cpp"
---

# Plan 4.1: Automated DSP Test Harness and Milestone Verification

## Objective
Implement an automated console test target (`ChugKnifeTests`) with measurable numeric assertions testing the core DSP behaviors: ripple-free low-frequency gating (44 Hz), Tube Screamer volt calibration and bounded output at Drive 0/Level max, Chugger flat magnitude at Chug 0 and palm-mute dominance compression, and PluginProcessor 2-in/2-out routing. Run the test suite and verify 100% pass rate.

## Context
Load these files for context:
- .gsd/SPEC.md
- .gsd/DECISIONS.md
- .gsd/ROADMAP.md
- CMakeLists.txt
- Source/PluginProcessor.h
- Source/dsp/NoiseGate.h
- Source/dsp/TubeScreamer.h
- Source/dsp/Chugger.h

## Tasks

<task type="auto">
  <name>Task 1: Add ChugKnifeTests executable target and test suite</name>
  <files>
    CMakeLists.txt
    Source/tests/ChugKnifeTests.cpp
  </files>
  <action>
    1. In CMakeLists.txt:
       - Add a console executable target `ChugKnifeTests` linking to `juce::juce_audio_basics`, `juce::juce_audio_processors`, `juce::juce_dsp`, `juce::juce_core`.
       - Add `Source/tests/ChugKnifeTests.cpp` and necessary DSP sources.
    2. In Source/tests/ChugKnifeTests.cpp:
       - Implement automated tests:
         a) `testNoiseGateLowFrequencyRipple()`: Feed sustained 44 Hz sine at -30 dBFS (threshold = -45 dBFS). Assert that gain reduction stays at 0.0 dB throughout (no intra-cycle chatter). Feed silence; assert gate attenuates to <= -80 dB within 50 ms.
         b) `testTubeScreamerGainStaging()`: Feed DI signal at Drive 0 and Level 1.0. Assert output peak is <= 1.0 FS (bounded, no wall of noise). Sweep Level from 0.0 to 1.0; assert monotonicity and that gain at Level 0.0 is silent and Level 1.0 is <= 1.0.
         c) `testChuggerFlatAtZero()`: Set Chug = 0. Feed test signals across 80 Hz, 500 Hz, 3 kHz. Assert magnitude deviation is < 0.1 dB (phase-coherent all-pass transparency).
         d) `testChuggerPalmMuteDominance()`: Feed low-dominant signal (100 Hz fundamental) vs balanced chord. Assert low band reduction engages specifically on low-dominant signal when Chug = 0.8.
         e) `testPluginProcessorBuses()`: Assert 2-in/2-out layout is supported; assert mismatched layout is rejected.
       - Return exit code 0 if all tests pass, non-zero on failure.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnifeTests
  </verify>
  <done>
    ChugKnifeTests builds cleanly.
  </done>
</task>

<task type="auto">
  <name>Task 2: Execute ChugKnifeTests and record empirical evidence</name>
  <files>
    .gsd/phases/4/4-SUMMARY.md
    .gsd/STATE.md
    .gsd/ROADMAP.md
  </files>
  <action>
    1. Run `build/Release/ChugKnifeTests.exe`.
    2. Confirm every assertion passes with exit code 0.
    3. Record test outputs and verification evidence in 4-SUMMARY.md, STATE.md, and mark all Must-Haves in ROADMAP.md.
  </action>
  <verify>
    build/Release/ChugKnifeTests.exe
  </verify>
  <done>
    Automated test harness reports 100% assertions passed with exit code 0.
  </done>
</task>

## Must-Haves
After all tasks complete, verify:
- [ ] ChugKnifeTests builds and runs successfully
- [ ] 100% of DSP and routing assertions pass with exit code 0
- [ ] VST3 and Standalone targets build with exit code 0
