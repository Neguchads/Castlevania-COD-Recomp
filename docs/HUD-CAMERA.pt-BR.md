# HUD e câmeras

[English](HUD-CAMERA.md) · Português (Brasil)

## HUD, Esc e modos de câmera (2026-10-24)

- Minimapa movido para Jogo → Opções de jogo. Mesmo bloco: Mostrar HUD original, escala0,35–1× (padrão0,5×), botão50%, ocultação rápida e atraso0,2–5s. Persistência de HUD/câmera/minimapa exercitada no runtime.
- F1/L3+R3 alternam o dev menu. Esc não alterna dev: gera pulso100ms de Start quando modal10000010 não está ativo, ou B quando ativo. Gate conferido com getteroriginal2F6230 em arena; abre/fecha no menu real ainda aguarda teste humano. Teclas de movimento no dev seguem separadas do personagem.
- HUD: hooks3575B0/357610 só para filhos da raizB251F0, parentescoobject+40. Alteração temporária de pos/escala spriteuserdata128, restauração após desenho. NomeInnocentDevil tratado em308E70 e callers308F02/308F48 dos renderers de texto. Não aplicar escala a inventário/menuoriginal. Cobertura visual total ainda pendente.
- Ocultação rápida usa3094E0modo1 e timerHUDdata4C (original5s). Mantém fade0,3s e gatilhos nativos; não inventa detecção de inimigos. Fixtureoriginal exige flagsroot130=30h para getter359600retornaruserdata; testeantesfalhou por ausência desses flags, corrigido e repetido sem afrouxar critério.
- Câmeras: Original(default), Cinemática inspiradaLoS1, Próxima inspiradaLoS2, Personalizada(distância/altura/FOVrelativo), Livre. Perfis simples, não cópia dos sistemasLoS. FOV não está em graus: ajuste relativo de focalinteger154. Todos parâmetros nativos temporários são restaurados depois das matrizes.
- Livre: fechar dev e usarWASD/QE/mouse ou analógicos/LT/RT. Personagem sem comandos, mundo continua. camera_live só captura controles com view/playerativos; título/mapa/debug conhecidos preservados. Não há colisão e gatecutscene completo; UI avisa que pode alterar cenas ativas.
- Build/test-local/verificadorruntime passaram. Provas logs/build-hud-camera.log, logs/tests-hud-camera.log, logs/runtime-hud-camera.log, logs/runtime-hud-camera-verification.log. Fixtures usam pilhaD0010000 e memóriaisolada; HUDscale/restauração/ocultar/menuguard e timer5→0,6/fade0,3 passaram. Matrizes originais ajustadas/restauradas e freecamrestauração passaram. Não equivalem a todos modos testados visualmente na campanha.
- PID46076 deixado aberto; jogo chegou ao cenário sob entrada humana. Screenshot da janela6882862 mostrou transição de cenário semHUD, não confirmou tamanho50%. Usuário foi convidado a testarHUD/câmeras/Esc; ainda sem resposta. Não encerrar processo nem enviar entradas durante teste humano.


Ver MODDING.pt-BR.md para controles. Fontes: src/game/hud.cpp, src/game/camera.cpp, src/common/presentation_math.h e src/common/dev_runtime.cpp. As preferências alteram só a apresentação/controles do port, não arquivos de save.
