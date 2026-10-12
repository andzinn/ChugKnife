# STATE.md — Project Memory

## Current Position
- **Milestone**: v1.1 — "Fix the Core"
- **Status**: Milestone Complete & Empirically Verified 🎉 (19/19 Tests Passed, Exit Code 0)

## Milestone v1.1 Accomplishments
1. **Routing & Internal 4CM (Phase 1)**:
   - Restored standard 2-in / 2-out (Stereo & Mono matching) architecture.
   - Decoupled gate detection: clean DI copied at block start keys the detector; gate attenuates post-distortion at block end.
   - Fixed low-frequency gate chatter on 7/8-string tunings (down to 41.6 Hz) via 24 ms sliding-window peak tracker.
   - Dynamic compensation of pitch-shifter latency in gate hold timer.
   - Thread-safe atomic reduction metering for UI.

2. **Tube Screamer Physical Modeling (Phase 2)**:
   - Physical volt calibration: 1.0 FS = 4.0 V peak (matching +12 dBu interface Hi-Z inputs).
   - 4x oversampling with polyphase half-band IIR filter for alias-free diode harmonics.
   - SPICE 1N4148 model with monotonically convergent Newton-Raphson solver started from analytic upper bound.
   - Real dual-branch RC tone network (`R7=1k`, `C5=220nF`, `R8=220`, `C6=220nF`, `R9=1k`) modeled with exact corner frequencies.
   - Level control implemented as true passive audio-taper attenuator (maximum 0 dB, zero artificial makeup gain or wall of noise).

3. **Chugger v3 (Phase 3)**:
   - Phase-coherent 3-band Linkwitz-Riley 4th order (LR4) crossover (splits at 150 Hz and 1800 Hz) with 150 Hz high-band all-pass alignment.
   - Flat magnitude all-pass (0 dB across all audible frequencies) at Chug = 0.
   - Level-independent palm-mute detector keyed on low-band dominance (`envLow_dB - envRest_dB`) from mono sum.
   - Punch-preserving low-band compressor (4 ms attack lets thump wavefront through; 80 ms release clamps flub).
   - Transient pick attack shaper (+7.2 dB bite during first 10-15 ms).
   - Zero dry/wet blending — no phase smearing or comb-filtering.

4. **Test Harness & Empirical Verification (Phase 4)**:
   - Automated test executable `ChugKnifeTests` with 19 numeric assertions verifying all the above.
   - 19 / 19 tests passed with exit code 0.
   - VST3 and Standalone binaries built and verified in Release mode.

## Verified Binaries
- **VST3 Plugin**: `D:\Vst Plguins\Plugin 1\build\ChugKnife_artefacts\Release\VST3\ChugKnife.vst3`
- **Standalone App**: `D:\Vst Plguins\Plugin 1\build\ChugKnife_artefacts\Release\Standalone\ChugKnife.exe`
- **Test Executable**: `D:\Vst Plguins\Plugin 1\build\Release\ChugKnifeTests.exe`
