# Product

<!-- impeccable:product-schema 1 -->

## Platform

adaptive

## Stack

C++20, JUCE 7.0.12, CMake, VST3 / Standalone (Windows x64 / macOS)

## Users

Guitarristas de metal moderno, djent e rock pesado gravando em DAWs (Reaper, Cubase, Studio One, Ableton Live) com simuladores de amplificador high-gain ou amplificadores reais, necessitando de afinação instantânea, saturação de pré-estágio, aperto de graves cirúrgico e gate transparente sem ruído.

## Product Purpose

ChugKnife é a "solução definitiva" em formato de canivete suíço tudo-em-um para guitarristas de peso: substitui a necessidade de encadear múltiplos plugins de terceiros antes do amplificador, integrando transposição polifônica instantânea (Tone3000 style), overdrive de modelagem analógica clássica (TS808), aperto percussivo dinâmico de palm mutes (The Chugger v3) e noise gate cirúrgico One-Knob guiado pelo sinal limpo da guitarra (4CM interno).

## Positioning

O primeiro plugin de front-end para guitarra pesada que combina drop-tuning polifônico de baixíssima latência com corte cirúrgico de flub em palm mutes via crossover Linkwitz-Riley LR4 e noise gate com detecção pré-distorção (4CM interno) em formato nativo 2-in/2-out sem requerer roteamento complexo de cabos auxiliares.

## Operating Context

Carregado como primeiro insert na pista de guitarra em DAWs modernas, diretamente após a interface de áudio (sinal DI limpo) e imediatamente antes do estágio de amplificação (VSTs de amps como Neural DSP, Helix Native, Amplitube, ou envio de reamp para amplificadores valvulados físicos). Também operável em modo Standalone para estúdio, ensaios e palcos.

## Capabilities and Constraints

- Roteamento 2-in / 2-out padrão (estéreo e mono) com emulação interna do Método de 4 Cabos (4CM): o gate ouve o DI limpo e atenua o sinal pós-distorção.
- Transpose polifônico de -12 a +12 semitons com emenda correlacionada e re-sincronização de ataque.
- Tube Screamer com calibração física de 4V, sobreamostragem 4x e atenuador de Level genuíno (sem paredes de ruído artificial).
- The Chugger v3 com crossover 3-bandas Linkwitz-Riley 4ª ordem alinhado em fase (transparência total em Chug = 0) e compressão de graves sensível à dominância espectral de palm mutes.
- Noise gate com janela deslizante de 24 ms imune a chatter em afinações graves (até 41.6 Hz).
- Interface gráfica nativa JUCE com resposta em tempo real e baixo consumo de CPU.

## Brand Commitments

- **Nome**: ChugKnife
- **Identidade Visual**: Tech Minimalista / Cyberpunk — painel preto fosco (matte carbon/obsidian), tipografia técnica contemporânea, medidores dinâmicos de alta densidade e acentos luminosos funcionais (ciano elétrico, verde-ácido e âmbar).

## Evidence on Hand

- Algoritmos DSP verificados e validados por suíte de testes com 19/19 asserções em `Source/tests/ChugKnifeTests.cpp`.
- Binários VST3 e Standalone funcionais gerados com sucesso em Release x64.

## Product Principles

1. **Transparência e Respeito ao Sinal Original**: Em valores neutros ou bypass, o processamento deve manter magnitude plana sem desfasamentos ou comb filtering indesejado.
2. **Controle Direto e Focado no Músico**: Interface direta, sem menus aninhados ou complexidade desnecessária; controles essenciais com curso musical afinado para timbres pesados.
3. **Precisão Cirúrgica e Resposta Instantânea**: O gate estanca imediatamente nas paradas de palheta e o detector de palm mutes reage apenas quando os graves realmente dominam.
4. **Feedback Visual Imediato**: A interface deve comunicar instantaneamente a redução de ganho do gate e o status dos módulos através de elementos visuais dinâmicos.
