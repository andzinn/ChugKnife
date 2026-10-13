# ROADMAP.md

> **Current Milestone**: v1.2 — "Tone Precision & Dark Minimalist UI"
> **Goal**: Entregar overdrive real e tom responsivo no TubeScreamer, redefinir o Chugger como a unidade cirúrgica definitiva de pré-ganho do metal moderno (Tight HPF 150-500Hz + Pick Attack 2.2kHz + Punch 90Hz baseados na literatura técnica fornecida), e criar a interface visual Dark Minimalist inspirada em Neural DSP e FabFilter.
> *(Milestones v1.0 e v1.1 arquivados em `.gsd/milestones/`)*

## Must-Haves
- [ ] TS808: Drive entrega saturação de overdrive rica e dinâmica quando aberto, mantendo boost limpo quando zerado
- [ ] TS808: Knob Tone possui alcance tonal dramático e expressivo (de aveludado e escuro a cortante e agressivo)
- [ ] Chugger: Implementado como filtro pré-ganho de alta precisão com corte Tight ajustável (150 Hz a 500 Hz) + Boost de estalo de palheta (2.2 kHz) + Soco de gabinete (90 Hz)
- [ ] Chugger: Transparência total em Chug = 0 e aperto percussivo evidente para afinações graves (Drop C até Drop E/8 cordas)
- [ ] Impeccable Shape: Design brief da interface completo com arquitetura de informação, paleta Dark Minimalist (Neural DSP style) e layout
- [ ] PluginEditor: Nova interface gráfica nativa em JUCE com visual moderno dark, knobs vetoriais customizados com anéis luminosos e medidores dinâmicos de alta precisão
- [ ] Automated Tests: Test harness `ChugKnifeTests` atualizado e com 100% das asserções aprovadas

## Phases

### Phase 1: TubeScreamer Overdrive & Wide-Range Tone Overhaul
**Status**: 🟩 Completed
**Objective**: Corrigir a faixa de ganho e saturação dos diodos no estágio de clipping para entregar overdrive real e dinâmico, e reformular o filtro de Tom para ampla variação tímbrica sem abafamento espúrio.

### Phase 2: Chugger Rebirth — Modern Metal Tight & Attack Engine
**Status**: 🟩 Completed
**Objective**: Reformular o Chugger com base no manual de engenharia fornecido: filtro passa-altas pré-ganho de corte ajustável (Tight 150 Hz a 500 Hz), boost de transiente em 2.2 kHz e ressonância de punch a 90 Hz.

### Phase 3: Impeccable Shape — Arquitetura de Design do PluginEditor
**Status**: 🟩 Completed
**Objective**: Definir o brief de design completo da interface: layout de 4 módulos em rack moderno, paleta de cores Neural DSP / FabFilter, tipografia técnica e especificações dos componentes visuais.

### Phase 4: Implementação da Interface Gráfica (Neural DSP / Dark Minimalist)
**Status**: 🟩 Completed
**Objective**: Construir o LookAndFeel customizado no JUCE C++, novos sliders rotativos vetoriais com arco neon, display de atenuação dinâmico do gate e layout responsivo polido.

### Phase 5: Testes Automatizados & Verificação Final
**Status**: ⬜ Not Started
**Objective**: Atualizar `ChugKnifeTests`, validar o comportamento sonoro de todos os módulos e compilar os binários VST3 e Standalone finais.
