# DECISIONS.md — Architecture Decision Record (ADR)

## ADR-001: Arquitetura de barramentos 4CM via Sidechain Auxiliar
- **Contexto**: O usuário deseja recriar a experiência do método de 4 cabos onde a detecção do gate ocorre na DI limpa mas a atuação de atenuação ocorre após a distorção/amplificador.
- **Decisão**: Configurar `AudioProcessor` com 2 buses de entrada: `Input` (Principal, estéreo/mono) e `Sidechain` (Auxiliar, estéreo/mono). No código, se o Sidechain contiver áudio, o detector do gate usa essas amostras; caso contrário, usa a entrada limpa principal antes dos estágios de saturação.
- **Consequência**: Compatibilidade total com DAWs modernas (Reaper, Pro Tools, Cubase, Studio One, Ableton, Logic).

## ADR-002: Algoritmo de Transpose Polifônico (Tone3000 Style)
- **Contexto**: Phase vocoders causam latência de 50-100 ms e instabilidade de graves, inviabilizando monitoração ao vivo em afinações graves (Drop A, Drop C, etc.).
- **Decisão**: Utilizar uma linha de atraso com emenda correlacionada (SOLA) com interpolação Hermite e detecção de onset em altas frequências para re-sincronização do atraso para 2-4 ms a cada palhetada.
- **Consequência**: Resposta tátil imediata para o guitarrista nos ataques de palheta e transposição polifônica limpa.

## ADR-003: Algoritmo "O Chugger"
- **Contexto**: O usuário solicitou um botão único ("Chug") para peso percussivo de palm mute sem arruinar o timbre original.
- **Decisão**: Projeto de 4 estágios dinâmicos conjugados sob o knob:
  1. Expansão dinâmica de ressonância percussiva entre 85-100 Hz acionada pelo detector de transiente.
  2. Filtro dinâmico de corte da "lama" (mud scoop) a 280 Hz para manter o som estanque.
  3. Injetor de ataque de palheta ("clack/bite") a 2.5 kHz no início do transiente.
  4. Saturação assimétrica suave de sub-graves (<140 Hz) para peso harmônico valvulado.
