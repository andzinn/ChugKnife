# RESEARCH.md — Pesquisa Técnica e Arquitetura DSP

## 1. O Roteamento 4-Cable Method (4CM) no JUCE

### O Conceito
No mundo analógico, o método de 4 cabos conecta pedais de pré-ganho (overdrive, pitch shift, entrada do gate) na frente do amplificador e pedais de pós-ganho (atenuação do noise gate, delays, reverbs) no loop de efeitos (FX Loop) do amplificador.
O truque mágico do Noise Gate em 4CM é:
- **Detector (Key/Sidechain)**: Conectado diretamente na guitarra limpa (DI). Sinal com SNR altíssimo, dinâmicas puras e sem ruído de válvulas ou hiss de distorção.
- **Atenuador (VCA)**: Conectado no loop de efeitos após o pré-amplificador de alto ganho. Quando o guitarrista para de tocar, o gate silencia instantaneamente o ruído e hiss gerados pelo amplificador.

### Implementação JUCE
Configuramos `BusesProperties()` no `AudioProcessor`:
```cpp
AudioProcessor (BusesProperties()
    .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
    .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
    .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
)
```
No método `processBlock`:
- Se o canal auxiliar de Sidechain estiver ativo (`getBusBuffer (buffer, true, 1)` não vazio e com sinal), o detector do `NoiseGate` lê essas amostras da DI limpa.
- Se não houver sidechain externo roteado pelo DAW, o detector pode ler o áudio de entrada antes dos módulos de distorção (`TubeScreamer` / `Chugger`), atuando como um 4CM interno virtual.
- O ganho calculado do gate é aplicado suavemente no áudio do bus principal.

---

## 2. O Noise Gate "One Knob" para Metal / Djent

### Por que a maioria dos gates falha no Djent?
Gates com tempos lentos deixam escapar uma cauda de ruído após um palm mute staccato ("chug"). Gates rápidos demais causam trepidação (*chattering*) em notas que sustentam, cortando o final da nota em pedaços.

### A Fórmula do "One Knob":
- **Detector com Band-Pass Filter**: HPF a 90 Hz (12 dB/oct) para rejeitar rumble e ruído de 60 Hz da rede elétrica + LPF a 4 kHz (6 dB/oct) para ignorar chiado de alta frequência.
- **Envelope Follower Assimétrico Instantâneo**:
  - Tempo de subida (Attack): ~0.15 ms. O gate abre instantaneamente no primeiro ciclo da palhetada.
  - Histerese: Abertura em $T_{open} = \text{Threshold}$ dB; fechamento em $T_{close} = \text{Threshold} - 5$ dB.
- **Hold Inteligente**: 12 ms de tempo de espera antes de iniciar a queda. Isso garante que intervalos minúsculos entre palhetadas rápidas em tercinas/semicolcheias não engasguem.
- **Release Exponencial com Curva de Expansor (Downward Expander)**:
  - Queda de ganho com constante de tempo de 8 ms para paradas abruptas, suavizando para notas longas em decaimento.
  - Range de atenuação: 90 dB (silêncio absoluto no high-gain).

---

## 3. O Transpose (Pitch Shifter) Baseado no Tone3000

### O Problema do Pitch Shifting Tradicional
- **Phase Vocoders (ex: FFT, RubberBand, Signalsmith)**: Janelas curtas (30 ms) produzem instabilidade em notas graves (o E grave da guitarra é 82 Hz e do baixo é 41 Hz). Janelas longas (60-100 ms) ultrapassam o limite de latência de monitoração ao vivo (20-25 ms) e borram o ataque percussivo da palhetada.

### A Solução Tone3000 (SOLA com Onset Re-Sync):
1. **Linha de Atraso Fracionária**:
   - Ring buffer com interpolação de Hermite de 4 pontos.
   - O read tap avança na velocidade proporcional à razão de afinação:
     $$r = 2^{(\text{semitones} / 12)}$$
   - Se baixamos a afinação ($r < 1$), o tap de leitura anda mais devagar que a cabeça de gravação, então o atraso cresce linearmente de um piso mínimo ($d_{\min} \approx 2\text{ ms}$) até o tamanho do buffer ($d_{\max} \approx 30\text{ ms}$).
2. **Emenda por Correlação Cruzada (Correlation Splice)**:
   - Ao aproximar-se do limite da janela, o algoritmo busca o ponto no passado onde a forma de onda melhor se correlaciona com a atual (NCC - Normalized Cross Correlation).
   - Função de custo: Penaliza saltos curtos repetitivos usando $(1 - \text{NCC}) / \text{jump}$, forçando saltos de múltiplos inteiros do período fundamental e evitando batimento de fase em acordes.
   - Aplica crossfade de cosseno elevado de 10 a 30 ms entre os taps.
3. **Onset Re-Sync (O Segredo da Latência Perceptível)**:
   - Um detector de transientes com filtro passa-altas de 600 Hz monitora a energia dos ataques de palheta.
   - Ao detectar um novo ataque forte (+9 dB sobre o mínimo recente), o tap é instantaneamente re-sincronizado para próximo da cabeça de gravação ($d_{\min}$) com um crossfade minúsculo de 2 ms.
   - **Resultado**: O guitarrista ouve o ataque da nota com apenas **2 a 4 ms** de latência! O atraso médio de 15 ms só existe no sustain, onde o cérebro humano não percebe latência.

---

## 4. O Módulo de Distorção: Screamer e "O Chugger"

### O Tube Screamer
- **Pré-EQ**: Passa-altas RC a 720 Hz (corta sub-graves antes de distorcer, deixando o amp responder rápido).
- **Clipping**: Saturação com diodos em antiparalelo modelada por $\tanh(k \cdot x)$ com ligeira assimetria par.
- **Tone Stack**: Filtro tilt ativo centrado em 1 kHz.
- **Level**: Ganho de saída compensatório.

### O Algoritmo Genial do "Chugger" (Monster Knob)
Na guitarra base de metal moderno, o som de "chug" (palm mute) é caracterizado por:
1. **Impacto Físico (Chest Thump)**: O abafamento com a palma gera uma ressonância percussiva entre 80 Hz e 110 Hz.
2. **Corte do "Mud" / Tubby (250-350 Hz)**: Essa faixa acumula lama e faz o amplificador soar opaco e embolado.
3. **O "Clack" da Palhetada (1.8 kHz - 3.2 kHz)**: O ataque metálico e cortante que dá definição em afinações baixas (Drop A, Drop E, 8 cordas).
4. **Saturação de Sub-Harmônicos**: Dá densidade e peso psicoacústico que é audível mesmo em fones ou monitores pequenos.

#### A Matemática por trás do Knob "Chug" ($C \in [0.0, 1.0]$):
- **Dynamic Low-End Resonator (85-100 Hz)**:
  Filtro bell ressonante ($Q \approx 2.5$) cujo ganho varia dinamicamente de acordo com a detecção de transiente e nível de sinal:
  $$G_{\text{low}}(t) = G_{\text{base}} + C \cdot 9.0 \cdot \text{Env}_{\text{transient}}(t)\text{ dB}$$
- **Dynamic Mud Scooper (280 Hz)**:
  Um filtro dinâmico que atenua a região de 280 Hz ($Q \approx 1.8$) proporcionalmente à compressão e ao valor do knob:
  $$G_{\text{mud}}(t) = - (C \cdot 6.0 \cdot \text{Env}_{\text{low}}(t))\text{ dB}$$
- **Transient Attack Shaper / Clack Injector (2.5 kHz)**:
  Separador de transientes por subtração de envelope rápido e envelope lento:
  $$\Delta(t) = \max(0, \text{Env}_{\text{fast}}(t) - \text{Env}_{\text{slow}}(t))$$
  Injeção de ganho transitório no pico de ataque:
  $$y_{\text{bite}}(t) = y(t) + C \cdot 0.6 \cdot \text{BPF}_{2.5\text{kHz}}(\Delta(t) \cdot x(t))$$
- **Low-Band Asymmetric Soft Saturator**:
  Sub-graves abaixo de 140 Hz passam por um saturador com bias par:
  $$f_{\text{sat}}(u) = \frac{u + 0.15 \cdot u^2}{1 + |u| + 0.15 \cdot u^2}$$
  Isso adiciona 2º e 3º harmônicos aos graves, dando a sensação de um amplificador valvulado cuspindo ar!
