# ROADMAP.md

> **Current Milestone**: v1.1 — "Fix the Core"
> **Goal**: Make the plugin usable in any DAW (2-in/2-out), make the TS808 gain-stage correct,
> and replace the Chugger with an algorithm that actually tightens palm mutes.
> v1.0 archived in `.gsd/milestones/v1.0/`.

## Must-Haves
- [ ] Plugin exposes exactly one stereo/mono input and one stereo/mono output
- [ ] Internal 4CM: gate detector keyed on the clean DI copied at block start; gain applied at block end
- [ ] Gate does not chatter on low notes (7/8-string range) and still closes tight
- [ ] TS808: Drive 0 + Level max is a boost (bounded output, no extra makeup gain); Level is an attenuator like the pedal
- [ ] TS808 tone stage simulated from the real RC network (R7/C5/RV/R8/C6/R9), not a generic shelf
- [ ] Chugger: transparent at 0; tightens low end only when lows dominate (palm mutes); boosts pick attack only on transients
- [ ] Automated DSP test executable with numeric pass/fail checks for all of the above

## Phases

### Phase 1: Routing 2-in/2-out + Internal 4CM + Gate Detector Fix
**Status**: ⬜ Not Started
**Objective**: Remove Send/Return buses, implement DI-keyed gate inside the block, fix the gate's
ripple-induced chatter on low notes, compensate pitch-shifter delay in the gate hold.

### Phase 2: TS808 Gain Staging and Circuit Model Rewrite
**Status**: ⬜ Not Started
**Objective**: Rewrite TubeScreamer with correct volt calibration, guaranteed-convergent diode solver,
nodal tone-stage simulation and Level as an audio-taper attenuator.

### Phase 3: Chugger v3 — Palm-Mute-Aware Tightener
**Status**: ⬜ Not Started
**Objective**: Phase-coherent 3-band split; level-independent low-dominance compressor + transient
attack shaper. No dry/wet blending.

### Phase 4: Test Harness and Verification
**Status**: ⬜ Not Started
**Objective**: `ChugKnifeTests` console target with measurable assertions; build VST3; record evidence.
