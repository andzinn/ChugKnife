---
phase: 1
plan: 1
wave: 1
gap_closure: false
depends_on: []
files_modified:
  - Source/PluginProcessor.h
  - Source/PluginProcessor.cpp
  - Source/PluginEditor.h
  - Source/PluginEditor.cpp
  - Source/dsp/NoiseGate.h
  - Source/dsp/NoiseGate.cpp
  - Source/dsp/PitchShifter.h
  - Source/dsp/PitchShifter.cpp
autonomous: true
must_haves:
  truths:
    - "Plugin exposes standard 2-in / 2-out (Stereo/Mono matching) audio buses"
    - "Noise gate detector runs on clean DI captured at block start and attenuates at block end"
    - "Noise gate detector uses a 23 ms sliding-window maximum eliminating ripple on low notes down to 44 Hz"
    - "Pitch shifter delay is compensated in the gate hold window"
  artifacts:
    - "Source/PluginProcessor.cpp"
    - "Source/dsp/NoiseGate.cpp"
    - "Source/dsp/PitchShifter.h"
---

# Plan 1.1: Routing 2-In/2-Out, Internal 4CM and Ripple-Free Gate Detector

## Objective
Remove the complex and broken Send/Return 4-bus architecture, restoring standard single-track 2-in/2-out (stereo/mono) DAW compatibility. Key the noise gate internally from the pristine DI buffer copied at block start, rewrite the gate envelope detector to use a 23 ms sliding-window peak detector that eliminates intra-cycle ripple/chatter on low guitar tunings (down to 44 Hz), and dynamically compensate pitch-shifter latency in the gate hold timer.

## Context
Load these files for context:
- .gsd/SPEC.md
- .gsd/DECISIONS.md
- .gsd/phases/1/RESEARCH.md
- Source/PluginProcessor.h
- Source/PluginProcessor.cpp
- Source/dsp/NoiseGate.h
- Source/dsp/NoiseGate.cpp
- Source/dsp/PitchShifter.h

## Tasks

<task type="auto">
  <name>Task 1: Simplify bus routing to 2-In/2-Out and implement internal 4CM pipeline</name>
  <files>
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
  </files>
  <action>
    1. In PluginProcessor.h and PluginProcessor.cpp:
       - Change BusesProperties constructor to only declare Main Input (stereo) and Main Output (stereo). Remove Return and Send auxiliary buses.
       - In isBusesLayoutSupported(), enforce: main input equals main output, and set is either mono or stereo. Remove all checks for aux bus 1.
       - Remove returnConnected, preAmpBuffer, and postAmpBuffer scratch buffers. Keep cleanDiBuffer and ensure it is sized in prepareToPlay (maxChannels, samplesPerBlock) and cleared in releaseResources.
       - In processBlock:
         a) Determine inChannels and outChannels from main buffer.
         b) Copy input audio to cleanDiBuffer at the very start of the block.
         c) Process the main buffer in-place sequentially: PitchShifter -> TubeScreamer -> Chugger.
         d) Feed cleanDiBuffer as the detector source to NoiseGate::process(buffer, &cleanDiBuffer).
         e) Apply master output gain.
       - Remove isReturnConnected() method or update to return false; update PluginEditor to show clean status message without return bus polling.
    AVOID: Keeping extra audio buses or dynamic heap allocation in processBlock.
    USE: Single in-place main buffer processing with pre-allocated cleanDiBuffer.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    PluginProcessor has only 1 input and 1 output bus; compiles cleanly without aux bus dependencies.
  </done>
</task>

<task type="auto">
  <name>Task 2: Rewrite NoiseGate detector with 23 ms sliding-window maximum and atomic reduction</name>
  <files>
    Source/dsp/NoiseGate.h
    Source/dsp/NoiseGate.cpp
  </files>
  <action>
    1. In NoiseGate.h and NoiseGate.cpp:
       - Replace the leaky single-pole envelope follower with a 23 ms sliding-window peak tracker (23 x 1 ms circular ring of cell maximums) plus ongoing sub-cell peak tracking.
       - At any sample, detected peak = max(cell_ring_max, current_cell_peak). This is guaranteed to be flat and ripple-free for notes >= 44 Hz (period <= 22.7 ms).
       - Maintain the bandpass filter on detector input (90 Hz HPF, 4.2 kHz LPF) to keep low rumble and high fizz out of detection.
       - Update the state machine: hysteresis opens at thresholdDb, closes at thresholdDb - 5.0 dB.
       - Hold counter: base hold is 12 ms, plus extraHoldSamples passed from pitch shifter delay.
       - Gain smoothing: attack 0.15 ms, release 6 ms downward expander down to -90 dB.
       - Change currentGainDb to std::atomic<float> for thread-safe GUI meter reading without data races.
       - Add setExtraHoldSamples(int extraSamples) method.
    AVOID: One-pole leaky peak follower on the raw audio signal which drops 17 dB between cycle peaks on low B/F# strings.
    USE: Sliding-window block-maximum over 23 ms.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    NoiseGate operates with 23 ms sliding window peak detector, thread-safe atomic metering, and compiles cleanly.
  </done>
</task>

<task type="auto">
  <name>Task 3: Expose PitchShifter latency and wire delay compensation to NoiseGate</name>
  <files>
    Source/dsp/PitchShifter.h
    Source/dsp/PitchShifter.cpp
    Source/PluginProcessor.cpp
  </files>
  <action>
    1. In PitchShifter.h and PitchShifter.cpp:
       - Add float getCurrentDelaySamples() const method returning (writePos - tapA) when active, or 0.0f if bypassed or semitones == 0.
    2. In PluginProcessor.cpp:
       - Retrieve the shifter latency via pitchShifter.getCurrentDelaySamples() and pass it to noiseGate.setExtraHoldSamples() before calling noiseGate.process().
    3. Verify and compile both VST3 and Standalone targets.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnife_Standalone
  </verify>
  <done>
    Pitch shifter tap latency is communicated to noise gate hold timer; both VST3 and Standalone targets build with exit code 0.
  </done>
</task>

## Must-Haves
After all tasks complete, verify:
- [ ] VST3 and Standalone targets compile successfully with exit code 0
- [ ] Plugin processor defines 1 input bus and 1 output bus (mono/stereo)
- [ ] Noise gate detector does not decay between cycles on low frequencies (sliding window >= 23 ms)
- [ ] cleanDiBuffer is isolated from drive/distortion stages and passed to gate detector

## Success Criteria
- [ ] All 3 tasks verified passing
- [ ] Must-haves confirmed
- [ ] No compilation warnings or regressions
