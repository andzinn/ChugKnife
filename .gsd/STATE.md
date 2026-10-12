# STATE.md — Project Memory

## Current Position
- **Milestone**: v1.0
- **Current Phase**: Phase 1 - 4 (Arquitetura, DSP & UI Nativa Implementados)
- **Status**: Skeleton Completo e DSPs Funcionais

## Resumo dos Componentes Implementados
1. **CMakeLists.txt**:
   - Moderno (CMake 3.22+), padrão C++20, suporte VST3 e Standalone com FetchContent automático do JUCE 7.0.12.
2. **4CM Routing & AudioProcessor**:
   - Main In/Out + Sidechain Auxiliar em `Source/PluginProcessor.cpp`.
   - Detecção inteligente: se o Sidechain externo estiver ativo (ex: sinal DI da guitarra no DAW), o detector do Gate lê o Sidechain e atenua a saída pós-amplificador. Se não houver sidechain, usa a DI interna pré-ganho.
3. **One-Knob Noise Gate**:
   - Implementado em `Source/dsp/NoiseGate.h` e `Source/dsp/NoiseGate.cpp`.
   - Attack instantâneo (0.15 ms), histerese de 5 dB, hold de 12 ms, release de expansor de 8 ms e 90 dB de atenuação.
4. **Tone3000 Inspired Pitch Shifter**:
   - Implementado em `Source/dsp/PitchShifter.h` e `Source/dsp/PitchShifter.cpp`.
   - Ring buffer com interpolação Hermite 4-pontos, algoritmo SOLA de correlação de atraso e re-sync instantâneo de transientes (2-4 ms de latência perceptível nos ataques).
5. **Tube Screamer**:
   - Implementado em `Source/dsp/TubeScreamer.h` e `Source/dsp/TubeScreamer.cpp`.
   - Pré-corte a 720 Hz, saturação de diodos em antiparalelo com harmônicos pares sutis, tone tilt a 1 kHz e level.
6. **O Chugger ("Monster Knob")**:
   - Implementado em `Source/dsp/Chugger.h` e `Source/dsp/Chugger.cpp`.
   - Resonador dinâmico de 92 Hz, de-mud compressor a 280 Hz, transient attack clack a 2.4 kHz, corte sub a 65-78 Hz e saturação harmônica de sub-graves.
7. **APVTS & UI Nativa**:
   - Implementado em `Source/PluginEditor.h` e `Source/PluginEditor.cpp`.
   - Sliders e labels divididos em 5 seções operacionais com leitura de metering e status de sidechain em tempo real.
