# REQUIREMENTS.md

## Format
| ID | Requirement | Source | Status |
|----|-------------|--------|--------|
| REQ-01 | Configuração do CMakeLists.txt com JUCE e targets VST3/Standalone | SPEC Goal 5 | Pending |
| REQ-02 | Configuração do PluginProcessor com arquitetura de barramentos 4CM (Main In/Out + Sidechain In) | SPEC Goal 1 | Pending |
| REQ-03 | Gerenciamento de parâmetros unificado via AudioProcessorValueTreeState (APVTS) | SPEC Goal 5 | Pending |
| REQ-04 | Algoritmo DSP do Noise Gate One-Knob (Threshold, Histerese, Hold de 12ms, Release de 8ms, Sidechain ou DI detect) | SPEC Goal 2 | Pending |
| REQ-05 | Algoritmo DSP de Pitch Shifter Polifônico de baixa latência inspirado no Tone3000 (Ring buffer Hermite, SOLA correlation splice, Onset re-sync) | SPEC Goal 3 | Pending |
| REQ-06 | Algoritmo DSP do Tube Screamer (Drive, Tone, Level com HPF de pré-corte a 720 Hz e saturação de diodos) | SPEC Goal 4 | Pending |
| REQ-07 | Algoritmo DSP "O Chugger" (Knob único modulando ressonância dinâmica a 95 Hz, corte dinâmico de 280 Hz, transient attack clank a 2.5 kHz e saturação de sub-graves) | SPEC Goal 4 | Pending |
| REQ-08 | Interface gráfica (PluginEditor) nativa com sliders, labels e attachments vinculados ao APVTS | SPEC Goal 5 | Pending |
