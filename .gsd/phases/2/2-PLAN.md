---
phase: 2
plan: 1
wave: 1
gap_closure: false
depends_on: [1]
files_modified:
  - Source/dsp/Chugger.h
  - Source/dsp/Chugger.cpp
autonomous: true
must_haves:
  truths:
    - "At Chug = 0.0, the frequency response is completely transparent and flat across 20 Hz - 20 kHz"
    - "As Chug increases from 0.0 to 1.0, the pre-distortion Tight HPF cutoff moves from 20 Hz to 500 Hz (tightening flubby sub-bass for low tunings)"
    - "As Chug increases, high-mid pick attack bite around 2.2 kHz is boosted by up to +9 dB for knife-edge palm mute percussiveness"
    - "Punch around 90 Hz is maintained/enhanced to ensure chest-thumping palm mute impact without flub"
  artifacts:
    - "Source/dsp/Chugger.h"
    - "Source/dsp/Chugger.cpp"
---

# Plan 2.1 (Milestone v1.2): Chugger Rebirth — Modern Metal Tight & Attack Engine

## Objective
Rebuild "The Chugger" based directly on the provided engineering literature (*"A Anatomia do Chug"* and *"Pedais de Chug no Metal Moderno"*). Replace the complex, inaudible 3-band dynamic compressor with an authentic front-end electroacoustic shaping engine: variable pre-gain Tight HPF (sweeping up to 500 Hz), high-mid pick attack boost (2.2 kHz), and 90 Hz cabinet punch resonance, guaranteeing complete transparency at Chug = 0.

## Context
- .gsd/ROADMAP.md
- User reference PDF 1: *A Anatomia do Chug: Física, Circuitos e Timbre*
- User reference PDF 2: *Pedais de Chug no Metal Moderno: Circuitos & Aplicação*
- Source/dsp/Chugger.h
- Source/dsp/Chugger.cpp

## Tasks

<task type="auto">
  <name>Task 1: Redesign Chugger DSP architecture with Tight HPF, Attack Bite, and Punch</name>
  <files>
    Source/dsp/Chugger.h
    Source/dsp/Chugger.cpp
  </files>
  <action>
    1. In Chugger.h and Chugger.cpp:
       - Replace the 3-band Linkwitz-Riley compressor setup with a high-precision multi-stage front-end filter cascade:
         a) Variable Pre-Gain Tight HPF (2nd-order Butterworth or 1st-order analog style):
            - Chug 0.0: Cutoff at 20 Hz (flat/transparent).
            - Chug 0.3: Cutoff at ~220 Hz (ideal for E Standard / Drop D).
            - Chug 0.6: Cutoff at ~350 Hz (ideal for Drop C / Drop B / 7-String Drop A).
            - Chug 1.0: Cutoff at 500 Hz (ideal for Drop F / Drop E / 8-String extreme metal).
         b) High-Mid Pick Attack Peaking Filter:
            - Center frequency: 2200 Hz (Q = 1.3).
            - Chug 0.0: 0 dB boost.
            - Chug 1.0: +8.5 dB boost.
         c) Low-End Girth & Punch Peaking Filter:
            - Center frequency: 90 Hz (Q = 1.2).
            - Chug 0.0: 0 dB.
            - Chug 1.0: +3.0 dB punch bump.
       - Use smooth parameter interpolation (juce::SmoothedValue) to prevent zippering during knob sweeps.
       - Ensure at Chug = 0.0 that all filters produce 0 dB deviation across the audible spectrum.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnifeTests
  </verify>
  <done>
    Chugger processes audio with definitive, audible tightness and cutting pick attack as the Chug knob is turned.
  </done>
</task>

## Must-Haves
After task completes, verify:
- [ ] Chug = 0.0 has flat frequency response (deviation < 0.1 dB)
- [ ] Chug = 1.0 cuts sub-bass below 300 Hz while elevating 2.2 kHz pick bite by +8 to +9 dB
- [ ] All tests in `ChugKnifeTests` build and pass
