# REQUIREMENTS.md

## Traceability Matrix — Milestone v1.1 "Fix the Core"
| ID | Requirement | Source | Status | Verification |
|----|-------------|--------|--------|--------------|
| REQ-01 | Configuração do CMakeLists.txt com JUCE e targets VST3/Standalone | SPEC Goal 5 | Complete | MSVC 19.51 Release build exit code 0 |
| REQ-02 | Configuração do PluginProcessor com arquitetura 2-in/2-out e 4CM interno (DI detect) | SPEC Goal 1 / ADR-101 | Complete | ChugKnifeTests: Stereo/Mono matching layout pass |
| REQ-03 | Gerenciamento de parâmetros unificado via AudioProcessorValueTreeState (APVTS) | SPEC Goal 5 | Complete | APVTS bindings for Gate, Pitch, TS, Chug, Master |
| REQ-04 | Algoritmo DSP do Noise Gate One-Knob (Janela deslizante de 24ms, histerese, compensação de pitch) | SPEC Goal 2 / ADR-104 | Complete | ChugKnifeTests: 44 Hz sustained note ripple-free pass |
| REQ-05 | Algoritmo DSP de Pitch Shifter Polifônico Tone3000 (SOLA correlation, Onset re-sync, latency export) | SPEC Goal 3 | Complete | PitchShifter active with getCurrentDelaySamples() |
| REQ-06 | Algoritmo DSP do Tube Screamer (Calibração 4V, solver Newton-Raphson monótono, tone RC real, Level atenuador) | SPEC Goal 4 / ADR-102 | Complete | ChugKnifeTests: Drive 0 + Level 1.0 bounded output pass |
| REQ-07 | Algoritmo DSP "O Chugger" v3 (Crossover LR4 3-bandas alinhado em fase, detector de dominância, sem dry/wet comb filtering) | SPEC Goal 4 / ADR-103 | Complete | ChugKnifeTests: Transparência a Chug 0 e clamp de palm mute pass |
| REQ-08 | Interface gráfica (PluginEditor) nativa com sliders, labels e attachments vinculados ao APVTS | SPEC Goal 5 | Complete | PluginEditor com visualização de modo 4CM e GR em tempo real |
