# ROADMAP.md

> **Current Phase**: Phase 1
> **Milestone**: v1.0

## Must-Haves (from SPEC)
- [ ] Roteamento de áudio 4CM funcional via Sidechain / bus auxiliar
- [ ] Noise Gate One-Knob cirúrgico para metal moderno
- [ ] Pitch Shifter polifônico com lógica do Tone3000 (SOLA + Onset Re-sync)
- [ ] Tube Screamer clássico de 3 knobs
- [ ] O Chugger com DSP dinâmico de graves, médios e transientes
- [ ] APVTS com controle completo de parâmetros e UI nativa
- [ ] CMakeLists.txt moderno pronto para compilação

## Phases

### Phase 1: Arquitetura do Sistema e Estrutura CMake
**Status**: 🔄 In Progress
**Objective**: Configurar CMakeLists.txt, esqueleto do PluginProcessor com suporte a barramento Sidechain (4CM) e declaração completa dos parâmetros no APVTS.
**Requirements**: REQ-01, REQ-02, REQ-03

### Phase 2: Módulos DSP de Front-End (Noise Gate & Pitch Shifter)
**Status**: 🔄 In Progress
**Objective**: Implementar a classe de NoiseGate cirúrgico One-Knob e a classe de PitchShifter polifônico inspirado no Tone3000 em arquivos DSP dedicados.
**Requirements**: REQ-04, REQ-05

### Phase 3: Módulos DSP de Distorção (Tube Screamer & O Chugger)
**Status**: 🔄 In Progress
**Objective**: Implementar o TubeScreamer e o revolucionário processador "O Chugger" em arquivos DSP dedicados e integrá-los ao pipeline de áudio.
**Requirements**: REQ-06, REQ-07

### Phase 4: Interface Gráfica Nativa e Integração Final
**Status**: 🔄 In Progress
**Objective**: Construir o PluginEditor com layout nativo intuitivo, agrupando os 4 blocos e conectando todos os parâmetros ao APVTS via attachments.
**Requirements**: REQ-08
