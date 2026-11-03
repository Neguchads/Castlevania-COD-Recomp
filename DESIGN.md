# Curse of Darkness tools menu

English · [Português (Brasil)](#pt-br)

> **Note:** this page describes the code in [PR #2](https://github.com/Neguchads/Castlevania-COD-Recomp/pull/2) (`fix/visual-hud`), which the upcoming v0.1.0 release is built from. Until it is merged, some tools, scripts and options named here exist only on that branch.

A native in-game operating surface that refines the existing tabs. Visual reference: the gold Castlevania logo, the red subtitle and the dark backgrounds seen in the USA game. Do not change the game's rendering when applying the theme.

- Near-black background with red; selection in deep red, action/focus in gold, ivory text and beige secondary text.
- Local Segoe UI fonts for options and Georgia for the title; atlas with oversampling, without installing or redistributing Windows fonts.
- Scale to the available area, starting from 1280×800; support the existing 960×540 window and larger windows.
- Header with title, FPS and factor; side navigation; content with its own scrolling; footer with save/back and the controller combination.
- F1 and L3 + R3 toggle the tools. One activation per combination; both buttons must be released before the next one.
- The d-pad navigates; the sticks stay with the game; A/cross confirms, B/circle goes back; L1/R1 switch tabs. Visible gold focus.
- Do not require new services or dependencies. The menu keeps the world running: the sticks move the character/camera; d-pad and A/B/L1/R1 stay in the interface. Keyboard and mouse only operate the panel while it is open.

- Esc stays with the original menu (Start/B depending on the modal state); it does not toggle the tools menu.
- Game holds the minimap, HUD and cameras. HUD default 50%, visibility toggle and hide delay; experimental cameras with instructions and an Original restore.

---

<a id="pt-br"></a>

# Menu de ferramentas de Curse of Darkness

[English](#curse-of-darkness-tools-menu) · Português (Brasil)

> **Nota:** esta página descreve o código do [PR #2](https://github.com/Neguchads/Castlevania-COD-Recomp/pull/2) (`fix/visual-hud`), do qual sai a release v0.1.0. Até o merge, algumas ferramentas, scripts e opções citados aqui só existem nessa branch.

Superfície nativa de operação dentro do jogo, refinando as sete abas existentes. Referência visual: logo dourado de Castlevania, subtítulo vermelho e fundos escuros observados no jogo USA. Não mudar a renderização do jogo ao aplicar o tema.

- Fundo quase preto com vermelho; seleção em vermelho profundo, ação/foco em dourado, texto marfim e secundário bege.
- Fontes locais Segoe UI para opções e Georgia para o título; atlas com oversampling, sem instalar ou redistribuir fontes do Windows.
- Escala conforme a área disponível, partindo de 1280×800; suportar a janela 960×540 existente e janelas maiores.
- Cabeçalho com título, FPS e fator; navegação lateral; conteúdo com rolagem própria; rodapé com salvar/voltar e combinação do controle.
- F1 e L3 + R3 alternam as ferramentas. Uma ativação por combinação; os dois botões precisam ser soltos antes da próxima.
- Direcional navega; analógicos continuam no jogo; A/cruz confirma, B/círculo volta; L1/R1 troca de aba. Foco visível em dourado.
- Não exigir novos serviços/dependências. Menu mantém o mundo rodando: analógicos movem o personagem/câmera; direcional e A/B/L1/R1 ficam na interface. Teclado e mouse operam só o painel enquanto aberto.

- Esc fica com menu original (Start/B conforme estado modal); não alterna o dev menu.
- Jogo concentra minimapa, HUD e câmeras. HUD padrão50%, togglevisibilidade e atraso de ocultação; câmeras experimentais com instruções e restauração Original.
