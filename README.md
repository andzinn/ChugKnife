# 🔪 ChugKnife — The Definitive 4CM Heavy Guitar Pre-Amp Weapon

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![JUCE 7](https://img.shields.io/badge/JUCE-7.0.12-orange.svg)](https://juce.com/)
[![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20Standalone-green.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()
[![Tests](https://img.shields.io/badge/Tests-22%2F22%20Passing-brightgreen.svg)]()

> **O canivete suíço definitivo para guitarristas de metal moderno, djent e rock pesado.**  
> Transposição instantânea polifônica, modelagem analógica TS808, esculpimento cirúrgico pré-ganho de palm mutes (*The Chugger*) e Noise Gate One-Knob guiado pelo sinal limpo (4CM interno) — tudo em um único plugin nativo 2-in / 2-out.

---

## ⚡ Por Que o ChugKnife Existe?

No metal contemporâneo (Djent, Deathcore, Thall, Metalcore Progressivo), guitarristas precisam encadear de 3 a 5 plugins de terceiros antes do amplificador:
1. Um pitch shifter para afinações baixas (Drop C, Drop A, Drop F).
2. Um pedal de overdrive estilo TS808 para apertar os médios.
3. Um pedal cirúrgico pré-ganho (como Precision Drive ou Fortin 33) para secar o subgrave e acentuar o estalo da palheta.
4. Um noise gate no final da cadeia com roteamento de cabos de sidechain complexo.

O **ChugKnife** consolida toda essa cadeia de front-end com processamento DSP de alta fidelidade e emulação do **Método de 4 Cabos (4CM)** de forma interna e transparente.

---

## 🎛️ Os 4 Pilares do Plugin

### 1. Transpose (Tone3000 Pitch Engine)
* **Alcance**: -12 a +12 semitons.
* **Algoritmo**: Transposição polifônica sincronizada no domínio do tempo com emenda correlacionada e alinhamento de fase de transientes.
* Permite alternar de afinação padrão (*E Standard*) para afinações pesadas de 7 e 8 cordas instantaneamente sem alterar a afinação física da guitarra.

### 2. Screamer (Virtual Analog TS808 Circuit)
* **Modelagem Física**: Equações nodais em tempo real da malha de realimentação de um Op-Amp real com diodos de silício em antiparalelo (**1N4148**), resolvidos via método analítico monotonicamente convergente de Newton-Raphson.
* **Clean Boost (Drive = 0)**: Comportamento autêntico de boost limpo para empurrar o canal de distorção do amp.
* **Overdrive Saturado (Drive > 0)**: Saturação analógica rica em harmônicos de 2ª e 3ª ordem, compressão natural e sustain aveludado.
* **Active Wide-Range Tone**: Filtro de tom ativo com ampla faixa de $\pm 14\text{ dB}$ centrado em 1.4 kHz (de aveludado e quente a cortante e agressivo).
* **Sobreamostragem 4x (Polyphase IIR)**: Elimina artefatos de aliasing não-linear.

### 3. The Chugger (Tight & Pick Attack Shaper)
Baseado na engenharia moderna de pedais de referência (*Horizon Devices Precision Drive*, *Fortin 33*, *Solar Guitars CHUG*):
* **Filtro Pré-Ganho Tight HPF (1ª Ordem, 6 dB/oitava)**:
  * Varredura contínua de **10 Hz a 500 Hz** acoplada ao knob *Chug*.
  * Elimina o subgrave destrutivo (60 Hz a 180 Hz) antes de entrar no amp, prevenindo a compressão fofa (*blocking distortion*) e liberando headroom dinâmico.
* **Pick Attack Clank Boost (2.2 kHz)**:
  * Peaking filter cirúrgico ($Q = 1.3$) que injeta até **$+8.5\text{ dB}$** de estalo mecânico da ponta da palheta contra a corda de aço.
* **Cabinet Punch Resonance (90 Hz)**:
  * Compensação acústica na frequência de ressonância mecânica de gabinetes fechados 4x12 ($+3.0\text{ dB}$ em 90 Hz).
* **Transparência Absoluta**: Em Chug = 0%, o efeito possui desvio de magnitude $< 0.07\text{ dB}$, mantendo o sinal 100% puro.

### 4. Noise Gate 4CM (Keyed pelo DI Limpo)
* **Roteamento Interno 4CM**: O detector monitora exclusivamente o sinal DI elétrico não-distorcido da guitarra na entrada do bloco, mas aplica a atenuação dinâmica de corte no final da cadeia.
* **Janela Deslizante de 24 ms**: Rastreamento contínuo de pico livre de *ripple* (ondulação residual) mesmo em fundamentais ultra-graves (até 41.6 Hz / afinações de 8 cordas).
* **Compensação Dinâmica de Latência**: O temporizador de *Hold* do gate compensa automaticamente a latência do pitch shifter.
* **Feedback Visual Imediato**: Anel concêntrico dinâmico ao redor do knob de Threshold que acende em vermelho-fogo em tempo real indicando a redução de ganho (*Gain Reduction*).

### 5. Master Output
* Controle de volume de saída master calibrado de **-24 dB a +12 dB**.

---

## 🎨 Interface Gráfica Dark Minimalist

Inspirada no design de hardware/software de estúdio moderno (**Neural DSP Archetype Series** e **FabFilter**):
* **Tema**: *Obsidian Carbon* (`#111215`) com painéis modulares em rack horizontal (920px × 420px).
* **Knobs Vetoriais Customizados**: Dials metálicos com chanfro usinado, arcos luminosos neon e ponteiros radiais iluminados.
* **Acentos de Cor por Módulo**:
  * 🟦 **Transpose**: Ciano Elétrico (`#00E5FF`)
  * 🟩 **Screamer**: Verde Esmeralda (`#00E676`)
  * 🟥 **The Chugger**: Carmesim High-Gain (`#FF1744`)
  * 🟨 **4CM Gate**: Âmbar / Ouro (`#FFAB00`) com anel dinâmico em Laranja/Vermelho (`#FF3D00`)
  * ⬜ **Master**: Prata Polar (`#E0E6ED`)
* **Bypass Stomps**: Botões de hardware com micro-LEDs de status (*ACTIVE* / *BYPASS*).

---

## 📁 Estrutura do Repositório

```
ChugKnife/
├── CMakeLists.txt              # Configuração de build JUCE e CMake
├── README.md                   # Documentação completa
├── DESIGN.md                   # Especificação técnica de UI e Design Tokens
├── PRODUCT.md                  # Visão de produto e manifesto
├── Source/
│   ├── PluginProcessor.h/cpp   # Motor principal JUCE e roteamento 4CM
│   ├── PluginEditor.h/cpp      # Interface gráfica e ChugKnifeLookAndFeel
│   ├── dsp/
│   │   ├── NoiseGate.h/cpp     # Noise gate com janela deslizante de 24 ms
│   │   ├── PitchShifter.h/cpp  # Algoritmo de transposição polifônica
│   │   ├── TubeScreamer.h/cpp  # Circuito analógico TS808 com diodos 1N4148
│   │   └── Chugger.h/cpp       # Filtro pré-ganho Tight HPF, Attack e Punch
│   └── tests/
│       └── ChugKnifeTests.cpp  # Suíte automatizada de testes DSP
└── .gsd/                       # Memória de desenvolvimento e roadmap
```

---

## 🛠️ Como Compilar

### Pré-requisitos
* **CMake** 3.22 ou superior
* Compilador **C++20**:
  * Windows: Visual Studio 2022 (MSVC v143 ou superior)
  * macOS: Xcode 14+ / Clang
* Git (o CMake fará o download do JUCE 7 automaticamente caso não esteja instalado localmente)

### Comandos de Build

```powershell
# 1. Configurar o projeto com CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 2. Compilar VST3, Standalone e Testes
cmake --build build --config Release

# 3. Executar a suíte de testes automatizados
.\build\Release\ChugKnifeTests.exe
```

Os artefatos finais serão gerados em:
* **VST3 Plugin**: `build/ChugKnife_artefacts/Release/VST3/ChugKnife.vst3`
* **Standalone App**: `build/ChugKnife_artefacts/Release/Standalone/ChugKnife.exe`

---

## 🎧 Como Usar na DAW (Reaper, Cubase, Ableton, Studio One)

1. Conecte sua guitarra em uma entrada Hi-Z da sua interface de áudio.
2. Na pista da DAW, insira o **ChugKnife** como o **PRIMEIRO PLUGIN** da cadeia de inserts.
3. Insira seu simulador de amplificador de alto ganho favorito logo em seguida (Neural DSP Archetype, Helix Native, Amplitube, STL ToneHub, etc.).
4. Ajuste:
   * **Transpose**: Escolha a afinação desejada sem desafinar a guitarra.
   * **Screamer**: Use com Drive baixo para boost clássico ou abra o Drive para empurrar o amp com overdrive cremoso.
   * **The Chugger**: Gire o knob até os graves ficarem secos e percussivos nas palhetadas abafadas (*palm mutes*).
   * **4CM Gate**: Ajuste o Threshold até o anel de Gain Reduction fechar perfeitamente nos silêncios sem cortar o sustain das notas agudas.

---

## 🧪 Testes Automatizados

O projeto conta com o executável de teste unitário e validação de DSP [`ChugKnifeTests.cpp`](Source/tests/ChugKnifeTests.cpp), verificando:
* Ausência total de ripple em notas graves sustentadas (44 Hz / 8 cordas).
* Saturação dinâmica do TS808 e isolamento do atenuador de Level passivo.
* Faixa tonal de $\ge 15\text{ dB}$ no tone stack.
* Transparência $< 0.15\text{ dB}$ em Chug = 0 e atenuação de subgrave $\le -10\text{ dB}$ em Chug = 1.0.
* Compatibilidade com layouts de barramento estéreo e mono padrão 2-In / 2-Out.

---

## 📄 Licença
Desenvolvido por [andzinn](https://github.com/andzinn). Distribuído sob a licença MIT.
