# Decisions — v1.1 Archive

## ADR-101: Internal 4CM supersedes Send/Return buses
Send/Return (ADR v1.0) broke DAW usability. 1-in/1-out; DI copied at block start keys the gate,
gate applied at block end. Trade-off: amp-sim self-noise after the plugin is not gated.

## ADR-102: TS808 calibrated at 1.0 FS = 4 V, Level is an attenuator
Removes the fictitious +18.7 dB makeup gain that caused the distortion wall.

## ADR-103: Chugger keyed on low-band dominance, not on amplitude
Level-independent, palm-mute-specific; phase-coherent band split instead of dry/wet blending.

## ADR-104: Gate detector = 23 ms sliding maximum
Removes intra-cycle chatter on fundamentals down to ~44 Hz.
