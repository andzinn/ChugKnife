---
phase: 5
plan: 1
wave: 1
gap_closure: false
depends_on: [4]
files_modified:
  - Source/tests/ChugKnifeTests.cpp
autonomous: true
must_haves:
  truths:
    - "Test runner ChugKnifeTests compiles and executes with 100% assertions passed"
    - "Release VST3 artifact ChugKnife.vst3 is present and ready for DAW insertion"
    - "Release Standalone artifact ChugKnife.exe is present and runnable"
  artifacts:
    - "build/ChugKnife_artefacts/Release/VST3/ChugKnife.vst3"
    - "build/ChugKnife_artefacts/Release/Standalone/ChugKnife.exe"
---

# Plan 5.1 (Milestone v1.2): Testes Automatizados & Verificação Final

## Objective
Run empirical verification across the entire DSP pipeline (TS808 analog overdrive & tone stack, Chugger variable Tight HPF & pick attack clank, 4CM noise gate low-frequency tracking, 2-in/2-out standard buses) and validate the production binaries for VST3 and Standalone.

## Tasks
<task type="auto">
  <name>Task 1: Execute full test suite and verify binaries</name>
  <files>
    Source/tests/ChugKnifeTests.cpp
  </files>
  <action>
    1. Run build\Release\ChugKnifeTests.exe and confirm 22/22 passing assertions.
    2. Confirm existence and integrity of ChugKnife.vst3 and ChugKnife.exe.
  </action>
  <verify>
    build\Release\ChugKnifeTests.exe
  </verify>
  <done>
    All assertions pass, Milestone v1.2 must-haves are 100% satisfied.
  </done>
</task>
