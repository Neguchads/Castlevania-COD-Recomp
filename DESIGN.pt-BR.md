# Menu de ferramentas de Curse of Darkness

[English](DESIGN.md) · Português (Brasil)

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
