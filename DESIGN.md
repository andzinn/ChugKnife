# DESIGN.md — ChugKnife Dark Minimalist Interface Specification

<!-- impeccable:design-schema 1 -->

## 1. Job & Audience
- **Visitor Mode**: *Operate* (Guitarrista gravando/tocando em DAW com foco total no timbre e zero atrito cognitivo).
- **Context**: Carregado como plugin insert principal de guitarra antes de amplificadores high-gain em monitores de estúdio (1080p a 4K).
- **Need**: Ajuste rápido e cirúrgico dos 4 módulos com feedback visual instantâneo do sinal e do noise gate 4CM.

## 2. Visual Authority & Aesthetics
- **Influence**: Neural DSP (Archetype series) + FabFilter (Pro-Q / Saturn polished vector precision).
- **Theme**: Dark Metal Minimalist / High-Tech Obsidian.
- **Palette**:
  - **Chassis Background**: `#111215` (Deep Carbon/Obsidian).
  - **Module Panels**: `#16181D` com bordas sutis de 1px em `#232730`.
  - **Knob Body**: Gradiente radial em metal fosco `#262933` a `#181A20` com chanfro sutil `#323744`.
  - **Module 1 (Transpose)**: Neon Electric Cyan (`#00E5FF`).
  - **Module 2 (TubeScreamer)**: Acid Emerald (`#00E676`).
  - **Module 3 (The Chugger)**: High-Gain Crimson (`#FF1744`).
  - **Module 4 (4CM Noise Gate)**: Amber/Gold Warning (`#FFB300`) com arco dinâmico integrado de Gain Reduction.
  - **Master Output**: Ice White / Silver (`#E0E6ED`).
  - **Typography**: Fonte sem serifa moderna de alta legibilidade, caixa alta para cabeçalhos técnicos, valores com unidades alinhadas.

## 3. Structural & Layout Architecture
- **Orientation**: Rack Linear Horizontal (Fluxo de sinal da esquerda para a direita).
- **Dimensões**: 920px × 420px (proporção dourada de tela de plugin moderna).
- **Top Header Bar (45px)**:
  - Logotipo e badge da versão: `CHUGKNIFE // 4CM PRE-AMP WEAPON`.
  - Status do roteamento: `INTERNAL 4CM: DI-KEYED (STEREO/MONO)`.
- **Main Rack (320px altura)** — 5 colunas modulares com divisores verticais discretos:
  1. **TRANSPOSE (170px)**:
     - Título e botão bypass com micro-LED ciano.
     - Grande knob rotativo de afinação (-12 a +12 semitons) com anel de arco ciano e indicador central de semitom.
  2. **SCREAMER (230px)**:
     - Título e botão bypass com micro-LED verde esmeralda.
     - 3 knobs estilizados alinhados em triângulo/linha: `DRIVE`, `TONE`, `LEVEL` com arcos verdes.
  3. **THE CHUGGER (180px)**:
     - Título e botão bypass com micro-LED carmesim.
     - Grande knob `CHUG` central com anel vermelho vivo (0% a 100%, 15 Hz a 500 Hz Tight HPF + 2.2 kHz Attack).
  4. **4CM NOISE GATE (180px)**:
     - Título e botão bypass com micro-LED âmbar.
     - Knob `THRESHOLD` com **anel concêntrico duplo**:
       - Anel externo fixo: marcação da posição do Threshold (-80 dB a 0 dB).
       - Anel interno dinâmico: display de Gain Reduction (GR) em tempo real que escurece/apaga conforme o gate fecha, proporcionando feedback imediato das palhetadas staccato.
  5. **MASTER (130px)**:
     - Slider vertical esguio com medidor estéreo e ganho Master (-24 dB a +12 dB).

## 4. Components & LookAndFeel Specification
- **Custom LookAndFeel (`ChugKnifeLookAndFeel`)**:
  - `drawRotarySlider`:
    - Fundo escuro côncavo com relevo micro-usinado.
    - Arco de valor (`track`) renderizado via `juce::Path` com cantos arredondados (`PathStrokeType::curved`).
    - Anel luminoso com espessura de 3.5px com gradiente de cor do módulo.
    - Ponteiro indicador minimalista (linha radial de 2px iluminada).
    - Valor numérico legível abaixo ou no centro do dial.
  - `drawToggleButton`:
    - Botão de Bypass moderno tipo "Pill Toggle" / Hardware Stomp switch minimalista com LED de ativação (aceso = ativo, escurecido = bypass).

## 5. States & Interaction
- **Hover & Drag**:
  - Knobs respondem a arrasto vertical e horizontal com sensibilidade calibrada.
  - Duplo clique reseta o parâmetro para o valor padrão (Default).
- **Bypass State**:
  - Ao desligar um módulo (Bypass ativo), o painel reduz levemente a luminosidade e os anéis neon diminuem a opacidade para 30%, mantendo clareza do estado desligado.
