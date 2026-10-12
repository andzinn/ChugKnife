# STATE.md — Project Memory

## Current Position
- **Milestone**: v1.0
- **Current Phase**: Phase 1 - 4 (Initial Full Blueprint & Implementation)
- **Status**: Em andamento

## Context Summary
- Projeto inicializado com os 4 pilares: 4CM Routing com sidechain no JUCE, One-Knob Noise Gate Djent, Transpose Polifônico inspirado no Tone3000 (SOLA + Onset Resync), Tube Screamer e o módulo "O Chugger".
- APVTS com parâmetros definidos.
- Arquitetura de DSP modular na pasta `Source/dsp/`.
- UI Nativa configurada em `Source/PluginEditor.cpp`.

## Blockers / Risks
- Nenhum bloqueador ativo.

## Next Steps
- Implementar CMakeLists.txt.
- Implementar os headers e sources de DSP (`NoiseGate`, `PitchShifter`, `TubeScreamer`, `Chugger`).
- Implementar `PluginProcessor` e `PluginEditor`.
