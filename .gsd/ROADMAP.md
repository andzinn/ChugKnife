# ROADMAP.md

> **Current Milestone**: v1.3 — "Tone3000 Engine Integration, -24/+24 Range & GitHub Release"
> **Goal**: Integrar a implementação autêntica do motor de pitch shift do Tone3000 (sob Licença MIT) para estabilidade impecável em acordes e afinações baixas (-24 a +24 st), adicionar documentação de segurança jurídica/créditos de open-source/vibecoding, compilar binários e publicar a primeira Release oficial no GitHub.
> *(Milestones v1.0, v1.1 e v1.2 arquivados em `.gsd/milestones/`)*

## Must-Haves
- [x] Integrar o motor PitchShift genuíno do Tone3000 (MIT) com rate-cost lag selection, adaptive crossfade (30-120 ms) e onset re-sync
- [x] Suporte estendido a ±24 semitons (-24 a +24 st) com geometria de buffer de 2 oitavas
- [x] APVTS e UI atualizados: Transpose slider opera de -24 a +24 st de forma fluida
- [x] Segurança Jurídica: Documentação completa de créditos, licença MIT, disclaimer de IA/vibecoding e marcas de terceiros
- [x] Testes Automatizados: Suíte ChugKnifeTests atualizada validando transposição de -24 a +24 semitons (43/43 assertions passing)
- [x] Release no GitHub: Pacote compactado com ChugKnife.vst3 e ChugKnife.exe gerado (`ChugKnife-v1.0.0-win64.zip`) e pronto para publicação

## Phases

### Phase 1: Tone3000 Pitch Engine Integration (-24 to +24 st)
**Status**: 🟢 Complete
**Objective**: Adicionar a implementação do PitchShift do Tone3000 ao ChugKnife (Source/dsp/PitchShifter), adaptando para a cadeia e estendendo o alcance para -24 a +24 semitons.

### Phase 2: Parameter & UI Updates (Transpose ±24 st)
**Status**: 🟢 Complete
**Objective**: Atualizar os parâmetros do APVTS e os componentes do PluginEditor para a faixa de -24 a +24 semitons, mantendo o visual Dark Minimalist.

### Phase 3: Legal Safety, Credits & Vibecoding Documentation
**Status**: 🟢 Complete
**Objective**: Estruturar a licença MIT do projeto, adicionar atribuição e créditos formais ao projeto TONE3000 e aos autores, e incluir disclaimer de vibecoding/IA e marcas.

### Phase 4: Automated Verification, Release Packaging & GitHub Publish
**Status**: 🟢 Complete
**Objective**: Atualizar os testes, compilar os binários finais Release x64, empacotar em ZIP e criar a Release v1.0.0 no GitHub via Git Tag / GitHub Release.
