# SPEC.md — Project Specification

> **Status**: `FINALIZED`

## Vision
Desenvolver o **ChugKnife**, um plugin VST3 de alta performance construído em C++ e JUCE com suporte a CMake, projetado para ser o canivete suíço definitivo para guitarristas de peso (Modern Metal / Djent). O plugin atua nos pontos críticos pré e pós-amplificador (sem simulação de amplificador embutida), integrando o Método de 4 Cabos (4CM) dentro do DAW com sidechain de baixíssima latência, um Noise Gate "One-Knob" ultra-rápido com detecção cirúrgica, um Pitch Shifter polifônico com latência imperceptível inspirado na arquitetura SOLA/re-sync do Tone3000, um clássico Tube Screamer e o inovador processador de transientes e graves dinâmicos "O Chugger".

## Goals
1. **Roteamento 4CM Real no DAW**: Permitir que o detector do Noise Gate leia a DI limpa da guitarra (via sidechain auxiliar ou antes da cadeia de ganho) enquanto a atenuação do gate é aplicada no áudio pós-amplificador ou na saída principal, eliminando ruído de alto ganho sem cortar a sustentação natural.
2. **Noise Gate Cirúrgico "One Knob"**: Algoritmo com apenas o controle de Threshold exposto, com constantes internas de attack (<0.2 ms), hold (12 ms) e release dinâmico programado para cortes secos de palm mutes modernos sem chattering.
3. **Drop Tuning Polifônico de Baixa Latência (Tone3000)**: Implementar transpositor polifônico baseado em linha de atraso com emenda correlacionada (SOLA/correlation-spliced delay line) e re-sincronização instantânea de transientes (onset re-sync), alcançando 2-5 ms de latência perceptível nos ataques de palheta.
4. **Módulo de Distorção Híbrido (Screamer + Chugger)**:
   - *Screamer*: Overdrive clássico estilo TS (Drive, Tone, Level) com curva de saturação suave simétrica/assimétrica e corte pré-clipping de graves.
   - *O Chugger*: Knob único inovador que esculpe dinamicamente o palm mute: compressão dinâmica do "mud" (280 Hz), expansor de ressonância percussiva de graves (90-100 Hz), acentuação transitória de "clack" de palhetada (2.4 kHz) e saturação harmônica de sub-graves.
5. **Arquitetura Técnica JUCE/CMake Moderna**:
   - Parâmetros gerenciados inteiramente via `juce::AudioProcessorValueTreeState` (APVTS).
   - CMakeLists.txt moderno com download automático do JUCE via FetchContent / CPM.
   - UI nativa funcional e limpa com sliders e botões conectados via attachments.
   - Código modular com DSP desacoplado em classes dedicadas e sem alocações no thread de áudio.

## Non-Goals (Out of Scope)
- Simulação de amplificador valvulado ou carregador de Impulse Response de gabinete (o foco é a cadeia auxiliar pré/pós amp).
- Modelagem de modulação complexa (chorus, flanger, delay, reverb).
- Interface gráfica personalizada de alta complexidade com skins skeuomórficas na primeira versão (o foco inicial é a arquitetura de áudio, DSP e UI nativa funcional).

## Users
Guitarristas de metal moderno, djent, deathcore, prog metal e produtores musicais que utilizam amplificadores reais com loadboxes ou amplificadores em software (NAM, Neural DSP, Amplitube, STL Tones) e necessitam de uma solução coesa de afinação rápida, aperto dinâmico de graves e silenciamento estanque de ruídos.

## Constraints
- **Linguagem e Padrão**: C++20.
- **Framework**: JUCE 7/8/9.
- **Formato**: VST3 (e Standalone).
- **Tempo Real**: Proibição estrita de alocações de memória (`new`, `malloc`, `std::vector::resize`) ou bloqueios (mutex, I/O) no método `processBlock`.
- **Compatibilidade de SO**: Windows (com suporte multiplataforma no CMake para macOS/Linux).

## Success Criteria
- [ ] CMakeLists.txt configurado e compatível com compilação JUCE VST3 e Standalone.
- [ ] PluginProcessor configurado com buses principais e sidechain auxiliar (4CM).
- [ ] Noise Gate operando com apenas 1 knob de threshold, ouvindo o sidechain quando conectado ou a entrada limpa interna.
- [ ] Algoritmo de Transpose polifônico de baixa latência funcional com semitons de -12 a +12.
- [ ] Algoritmos de Screamer e "Chugger" totalmente codificados com a matemática dinâmica descrita.
- [ ] Gerenciamento de estado via APVTS e UI nativa conectada.
