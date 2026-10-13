---
phase: 4
plan: 1
wave: 1
gap_closure: false
depends_on: [3]
files_modified:
  - Source/PluginEditor.h
  - Source/PluginEditor.cpp
autonomous: true
must_haves:
  truths:
    - "PluginEditor renders with a dark minimalist aesthetic inspired by Neural DSP and FabFilter (matte obsidian, 1px subtle borders, crisp modern typography)"
    - "Rotary sliders feature custom vector dials with distinct neon glowing arc rings (Cyan for Transpose, Emerald for Screamer, Crimson for Chugger, Amber for Gate)"
    - "The 4CM Noise Gate Threshold slider features a dynamic concentric Gain Reduction arc meter tracking real-time gating at 30 Hz"
    - "Bypass controls feature sleek hardware stomps with illuminated micro-LEDs"
    - "Horizontal linear rack flow accommodates all 5 sections cleanly within 920x420 dimensions"
  artifacts:
    - "Source/PluginEditor.h"
    - "Source/PluginEditor.cpp"
---

# Plan 4.1 (Milestone v1.2): Implementação da Interface Gráfica Dark Minimalist

## Objective
Build the custom native JUCE GUI adhering strictly to `DESIGN.md`. Implement a custom vector `ChugKnifeLookAndFeel`, modular horizontal rack layout (920x420 px), color-coded neon glowing slider arcs, hardware-style bypass switches with micro-LEDs, and real-time concentric Gain Reduction visualization for the 4CM Noise Gate.

## Context
- DESIGN.md
- Source/PluginEditor.h
- Source/PluginEditor.cpp
- Source/PluginProcessor.h

## Tasks

<task type="auto">
  <name>Task 1: Implement custom LookAndFeel and Dark Minimalist PluginEditor</name>
  <files>
    Source/PluginEditor.h
    Source/PluginEditor.cpp
  </files>
  <action>
    1. In PluginEditor.h and PluginEditor.cpp:
       - Implement `ChugKnifeLookAndFeel`:
         - Custom `drawRotarySlider`:
           - Dark metallic concave dial with smooth bevel.
           - Glowing neon arc track with rounded caps.
           - Support for custom accent colors per slider.
           - For the Threshold slider: concentric dynamic Gain Reduction ring that closes/dims with gating.
         - Custom `drawToggleButton`:
           - Minimalist hardware stomp / pill toggle with illuminated micro-LED.
         - Custom `drawLinearSlider`:
           - Precision vertical fader for Master Out.
       - Layout in PluginEditor:
         - Size: 920 x 420 px.
         - Top bar (45 px): "CHUGKNIFE // 4CM PRE-AMP WEAPON" + status.
         - 5 modular columns (Transpose, Screamer, Chugger, 4CM Gate, Master).
         - High-contrast, clean labels and value readouts.
       - Connect APVTS attachments and 30 Hz timer for smooth Gain Reduction visualizer.
  </action>
  <verify>
    cmake --build build --config Release --target ChugKnife_VST3 ChugKnife_Standalone
  </verify>
  <done>
    New dark minimalist UI builds cleanly and renders all 5 modules with custom vector controls.
  </done>
</task>

## Must-Haves
After task completes, verify:
- [ ] VST3 and Standalone compile with 0 errors
- [ ] Controls are interactive and correctly bound to APVTS parameters
- [ ] Gate visualizer animates dynamically
