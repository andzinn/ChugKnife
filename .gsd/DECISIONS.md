# Decisions — v1.2

> *ADRs dos Milestones anteriores arquivados em `.gsd/milestones/v1.0/DECISIONS.md` e `.gsd/milestones/v1.1/DECISIONS.md`.*

## ADR-201: TS808 Overdrive Gain & Tone Range Calibration
O estágio de saturação do TubeScreamer deve permitir que a tensão ceifada pelos diodos gere distorção rica quando o Drive for aumentado (ganho de pequenos sinais até +40 dB), e o Tone deve atuar como filtro tilt ativo de amplo espectro sem filtros passa-baixas rígidos espúrios ceifando agudos a 720 Hz.

## ADR-202: Chugger como Unidade Cirúrgica de Pré-Ganho (Tight HPF + Attack Boost)
Baseado no manual técnico e pedais modernos (Precision Drive, Fortin 33, Solar Chug):
1. Filtro passa-altas de primeira ordem pré-ganho com corte ajustável de 150 Hz a 500 Hz (Tight knob).
2. Boost de ataque de palheta em 2.2 kHz de até +14 dB.
3. Ressonância de punch de gabinete na faixa de 85 a 105 Hz.
4. Totalmente transparente com knob em 0.
