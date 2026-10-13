---
phase: 1
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
    - "TubeScreamer Drive 0 maintains the existing clean/mid boost behavior"
    - "TubeScreamer Drive > 0 provides rich, warm, dynamic analog overdrive distortion with responsive touch dynamics"
    - "TubeScreamer Tone control provides wide, audible tonal variation from dark/warm (0.0) to cutting/aggressive (1.0) without premature high-frequency choking"
  artifacts:
    - "Source/dsp/TubeScreamer.h"
    - "Source/dsp/TubeScreamer.cpp"
---

# Plan 1.1 (Milestone v1.2): TubeScreamer Overdrive & Wide-Range Tone Overhaul

## Objective
Revamp the TubeScreamer DSP module so that it functions both as an authentic clean/mid boost (at Drive 0) AND as a rich, singing, dynamic analog overdrive (as Drive increases). Eliminate the spurious cascading 723 Hz lowpass filter that choked all high-frequency harmonics, and reconstruct the active Tone control with an expressive ±15 dB range pivoting around 1.2 kHz.

## Context
Load these files for context:
- .gsd/ROADMAP.md
- .gsd/DECISIONS.md
- Source/dsp/TubeScreamer.h
- Source/dsp/TubeScreamer.cpp

## Tasks

<task type="auto">
  <name>Task 1: Recalibrate Drive scaling for rich, dynamic overdrive distortion</name>
  <files>
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
  </files>
  <action>
    1. In TubeScreamer.cpp:
       - Adjust the drive gain path and feedback loop:
         At Drive = 0, maintain current bounded boost characteristics.
         As Drive increases from 0.0 to 1.0, increase the input signal drive into the non-linear diode clipping network exponentially (providing up to +36 dB of extra drive into the clipping stage).
         Ensure that the diode saturation component scales musical distortion and sustain into the output, generating rich 2nd and 3rd harmonic overtones rather than being masked by the raw dry signal.
       - Preserve smooth sample-rate scaling and anti-aliasing via 4x oversampling.
    AVOID: Keeping the clipped diode voltage drowned out by clean unamplified input.
    USE: Musical non-linear gain scaling that allows the 1N4148 diodes to sing and compress organically when Drive is pushed.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3
  </verify>
  <done>
    Drive knob yields progressive, rich overdrive harmonics from 0% to 100%.
  </done>
</task>

<task type="auto">
  <name>Task 2: Eliminate choking 723 Hz LPF and implement expressive wide-range Tone control</name>
  <files>
    Source/dsp/TubeScreamer.h
    Source/dsp/TubeScreamer.cpp
  </files>
  <action>
    1. In TubeScreamer.h and TubeScreamer.cpp:
       - Remove the cascading `r7c5Filter` that was filtering the entire signal at 723 Hz and suffocating the tone stack.
       - Re-voice `toneFilter` as an active tilt/shelving network pivoting at 1.2 kHz:
         Tone 0.0: Smooth roll-off of highs (-14 dB at 4 kHz) for warm, dark, creamy neck tones.
         Tone 0.5: Natural, balanced response.
         Tone 1.0: Sharp, biting high-mid boost (+15 dB at 3.5 kHz) for modern aggressive cutting presence.
       - Retain only an ultrasonic RF protection lowpass filter at 16 kHz.
    AVOID: Cascading a lowpass at 723 Hz on the main audio path.
    USE: Wide-range active tone filtering with pivot at 1.2 kHz and full audible harmonic transmission.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnife_Standalone
  </verify>
  <done>
    Tone control produces dramatic, musical sonic variations across its full travel.
  </done>
</task>

## Must-Haves
After all tasks complete, verify:
- [ ] VST3 and Standalone targets build with exit code 0
- [ ] Drive 0 remains a clean boost; Drive 1.0 provides singing overdrive
- [ ] Tone knob from 0.0 to 1.0 spans from warm/dark to bright/cutting
