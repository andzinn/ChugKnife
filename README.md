# 🔪 ChugKnife — The Definitive 4CM Heavy Guitar Pre-Amp Weapon

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Vibecoded](https://img.shields.io/badge/Vibecoded-100%25%20AI%20Assisted-ff69b4.svg)](https://github.com/andzinn/ChugKnife)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![JUCE 7](https://img.shields.io/badge/JUCE-7.0.12-orange.svg)](https://juce.com/)
[![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20Standalone-green.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()
[![Tests](https://img.shields.io/badge/Tests-43%2F43%20Passing-brightgreen.svg)]()

> **O canivete suíço definitivo para guitarristas de metal moderno, djent e rock pesado.**  
> Transposição polifônica autêntica baseada no motor do Tone3000 (±24 semitons), modelagem física do circuito analógico TS808, esculpimento cirúrgico pré-ganho de palm mutes (*The Chugger*) e Noise Gate One-Knob guiado pelo sinal limpo (4CM interno) — tudo em um único plugin nativo 2-in / 2-out.

---

## ⚡ Por Que o ChugKnife Existe?

No metal contemporâneo (Djent, Deathcore, Thall, Metalcore Progressivo), guitarristas precisam encadear múltiplos plugins de terceiros antes do amplificador:
1. Um pitch shifter para afinações baixas (Drop C, Drop A, Drop F, Drop D1) ou afinações agudas.
2. Um pedal de overdrive clássico (estilo TS808) para apertar os médios e dar ganho inicial.
3. Um modelador pré-ganho cirúrgico (estilo Precision Drive ou Fortin 33) para secar o subgrave e acentuar o estalo mecânico da palheta.
4. Um noise gate no final da cadeia com roteamento de cabos de sidechain complexo.

O **ChugKnife** consolida toda essa cadeia de front-end com processamento DSP de alta fidelidade e emulação do **Método de 4 Cabos (4CM)** de forma interna e transparente.

---

## 🎛️ Os 4 Pilares do Plugin

### 1. Transpose (Tone3000 Correlation-Spliced Engine)
* **Alcance Total**: **-24 a +24 semitons** (duas oitavas completas para cima ou para baixo).
* **Motor DSP**: Baseado no aclamado algoritmo do [TONE3000](https://github.com/tone-3000/tone3000-plugin), usando linha de atraso com emenda correlacionada (*correlation-spliced delay line*):
  * **Rate-Cost Lag Selection**: Escolha ótima de ponto de emenda baseada em dano-por-salto, prevenindo saltos microscópicos e distorção em frequências graves e acordes complexos.
  * **Crossfade Adaptativo**: Transições dinâmicas de 30 ms a 120 ms com lei de potência normalizada pela qualidade da correlação cruzada, eliminando *chuffing* em acordes abertos.
  * **Onset Re-Sync**: Detector de transientes de energia em 600 Hz com limite duplo (9 dB / 6 dB) que ressincroniza o tap nos ataques de palheta, mantendo a latência sentida abaixo de **5 ms** em palm mutes.
* Permite transitar de *E Standard* para afinações pesadas de 7 e 8 cordas instantaneamente sem artefatos sonoros.

### 2. Screamer (Virtual Analog TS808 Circuit)
* **Modelagem Física**: Equações nodais em tempo real da malha de realimentação de um Op-Amp com diodos de silício em antiparalelo (**1N4148**), resolvidos via método analítico monotonicamente convergente de Newton-Raphson.
* **Clean Boost (Drive = 0)**: Comportamento autêntico de boost limpo para empurrar o canal de distorção do amp.
* **Overdrive Saturado (Drive > 0)**: Saturação analógica rica em harmônicos de 2ª e 3ª ordem, compressão natural e sustain aveludado.
* **Active Wide-Range Tone**: Filtro de tom ativo com ampla faixa de $\pm 14\text{ dB}$ centrado em 1.4 kHz (de aveludado e quente a cortante e agressivo).
* **Sobreamostragem 4x (Polyphase IIR)**: Elimina artefatos de aliasing não-linear.

### 3. The Chugger (Tight & Pick Attack Shaper)
Baseado na engenharia de pedais modernos de alto ganho:
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

## 🤖 100% Vibecodado (AI-Assisted Pair Programming)

Este projeto é um manifesto de engenharia de software contemporânea: **100% do código do ChugKnife foi desenvolvido no modelo "Vibecoding"**, com direção arquitetural e curadoria humana por [@andzinn](https://github.com/andzinn) em pair-programming interativo com modelos de Inteligência Artificial de última geração.

Cada linha de DSP C++20, simulação de circuitos analógicos, testes unitários, shaders visuais e interface gráfica JUCE foi iterada através de prompts de alta precisão e validação empírica contínua.

---

## 📜 Créditos & Código Aberto (Open Source Credits)

O ChugKnife é construído sobre o ecossistema de código aberto:

1. **TONE3000 Pitch Engine**:
   * O algoritmo do módulo de transposição polifônica (`Source/dsp/PitchShifter.cpp` e `PitchShifter.h`) foi adaptado a partir do projeto **[TONE3000](https://github.com/tone-3000/tone3000-plugin)**.
   * Copyright (c) 2026 TONE3000.
   * Distribuído sob os termos da licença **MIT License**.
2. **JUCE Framework**:
   * Framework de áudio C++ multiplataforma de referência mundial.
   * Copyright (c) 2022 - Raw Material Software Limited.

---

## ⚖️ Termos de Uso e Isenção de Responsabilidade (Legal Disclaimer)

* **Segurança e Propriedade Intelectual**: O ChugKnife é distribuído gratuitamente sob a licença **MIT**. Você tem total liberdade para usar, estudar, modificar e distribuir o software.
* **Marcas Comerciais de Terceiros**: Todos os nomes de produtos, marcas registradas e referências sonoras citados neste repositório (incluindo, sem limitação, *Ibanez*, *TS808*, *Tube Screamer*, *Digitech Drop*, *Horizon Devices Precision Drive*, *Fortin 33*, *Solar Guitars*, *Neural DSP*) são marcas registradas de seus respectivos detentores e são utilizados aqui **exclusivamente para fins informativos, descritivos e de identificação de timbre**.
* **Sem Associação**: O ChugKnife é um projeto de código aberto independente e **não possui qualquer afiliação, patrocínio ou endosso** por parte das empresas proprietárias das referidas marcas registradas.
* **Garantia**: O software é fornecido *"NO ESTADO EM QUE SE ENCONTRA"* (*AS IS*), sem garantias de qualquer tipo, expressas ou implícitas, conforme detalhado no arquivo [LICENSE](LICENSE).

---

## 📦 Download & Instalação

Você pode baixar a versão compilada diretamente na aba de **[Releases](https://github.com/andzinn/ChugKnife/releases)**:
* **ChugKnife-v1.0.0-win64.zip**: Contém tanto o arquivo **`ChugKnife.vst3`** (para sua DAW) quanto o **`ChugKnife.exe`** (aplicativo Standalone).

### Instalação no Windows
* **VST3**: Descompacte a pasta `ChugKnife.vst3` em:
  `C:\Program Files\Common Files\VST3\`
* **Standalone**: Execute diretamente o `ChugKnife.exe`.

---

## 🛠️ Como Compilar do Código-Fonte

### Pré-requisitos
* **CMake** 3.22 ou superior
* Compilador **C++20**: Visual Studio 2022 (MSVC v143 ou superior) no Windows
* Git

### Comandos de Build

```powershell
# 1. Configurar o projeto com CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 2. Compilar VST3, Standalone e Testes
cmake --build build --config Release

# 3. Executar a suíte completa de testes automatizados
.\build\Release\ChugKnifeTests.exe
```

---

## 🧪 Testes Automatizados (43/43 Passando)

A suíte de testes [`ChugKnifeTests.cpp`](Source/tests/ChugKnifeTests.cpp) valida:
* Estabilidade numérica do motor PitchShift em toda a faixa de -24 a +24 semitons sem NaNs ou Infs.
* Recuperação instantânea do detector de onset em palm mutes bruscos.
* Ausência de ripple no Noise Gate em notas de 44 Hz (Drop F / 8 cordas).
* Convergência matemática do solver de Newton-Raphson do TS808 com diodos 1N4148.
* Comportamento do atenuador passivo de saída do pedal.
* Faixa tonal de $\ge 15\text{ dB}$ no tone stack.
* Transparência $< 0.15\text{ dB}$ em Chug = 0 e atenuação de subgrave $\le -10\text{ dB}$ em Chug = 1.0.
* Barramentos 2-In / 2-Out mono e estéreo.

---

## 📄 Licença
Distribuído sob a licença **MIT**. Consulte o arquivo [LICENSE](LICENSE) para mais detalhes.
