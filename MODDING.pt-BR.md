# Ferramentas e mods

[English](MODDING.md) · Português (Brasil)

## Personalização do painel

O guia do jogador, com cada aba e opção, está em [docs/GUIDE.pt-BR.md](docs/GUIDE.pt-BR.md). Sete abas no topo: Jogabilidade (jogo e minimapa, dificuldade, inimigos, drops e itens, cheats, HUD e pets, câmera, saves), Gráficos, Áudio, Controles, Mods, Suporte (relatar bug, diagnóstico) e Menu (aparência, perfis). O campo com a lupa, no topo, busca qualquer opção pelo nome: Ctrl+F no teclado ou Y no controle, que abre um teclado na tela. L1/R1 ou clique trocam de aba. O direcional escolhe um título na coluna esquerda; A entra nas opções dele. Numa barra, A entra no ajuste: direcional ou analógico esquerdo arrasta, B sai do ajuste. B volta das opções para os títulos e nunca fecha o painel; só **L3 + R3** ou **F1** fecham. No teclado: setas, Enter e Esc fazem o mesmo. Com o painel aberto o jogo continua rodando: os analógicos andam e giram a câmera (também na Livre e na Orbital) e os botões ficam com o painel; enquanto uma barra é ajustada, o analógico esquerdo só mexe a barra. Para pausar tudo (jogo, relógio, vblank e som), ligue "Pausar o jogo com o menu aberto" em Menu > Aparência.

Idioma: Jogabilidade > Jogo e minimapa > Idioma do jogo troca entre English e Português (Brasil). Textos mudam na hora; imagens e vozes já carregadas, ao reiniciar. Japonês continua em Opções > Idioma no jogo.

Dificuldade (Jogabilidade > Dificuldade): presets Fácil, Normal, Difícil, Muito difícil e Pesadelo, ou as barras "Dano recebido pelo jogador" e "Dano recebido pelo Innocent Devil" (0,25x a 4x). O golpe final e mortes por roteiro continuam matando. A vida dos inimigos fica em Jogabilidade > Inimigos (abaixo).

Pacotes de textura: os nomes dos arquivos (`<hash de 16 hex>.dds` ou `.bmp`) são lidos uma vez ao carregar os mods ou trocar o idioma. Arquivo novo copiado com o jogo aberto só aparece depois de recarregar os mods.

Cheats: além de godmode e speedhack, "Vida cheia" (não revive), "MP cheio", "MP infinito" (só o Hector; o jogo não tem corações, as habilidades dos Devils gastam MP), "Ouro maximo" (9.999.999), "Ganho de ouro" e "Ganho de EXP" (1x a 10x) e "Compras nao gastam ouro". Ficam salvos nas opções. Códigos de nome do jogo: @CRAZY (modo Crazy) e @TREVOR (jogar com Trevor), segundo guias; não testados nesta versão.

Save de Lament of Innocence: no PS2, ter o save de LoI no memory card libera a estátua Moai. Em Cheats, "Simular save de Lament of Innocence" faz o jogo achar esse save; ligue, salve as opções e só então carregue o save ou comece um jogo novo pela tela de título (o log mostra `cheat: Lament of Innocence save simulated`). O bônus fica gravado no save ao salvar. Pelo código, ele libera um objeto numa sala (não cai direto no inventário); a sala ainda não foi identificada.

Minimapa: em Jogabilidade > Jogo e minimapa, "Posição horizontal/vertical do minimapa" (0 = esquerda/topo, 1 = direita/base) e o botão que volta ao canto superior direito. Salvar opções grava a posição.

Botões de PlayStation: ligue o mod **PS2-Botoes** (Mods) para ver ✕ ○ □ △ L1 L2 R1 R2 no lugar de A B X Y L preto R branco, e em Controles > Controle aperte **Layout PS2** para L1/R1 virarem os gatilhos e L2/R2 o preto/branco, como no PS2. **Layout Xbox** volta ao padrão; a linha "Layout atual" mostra Xbox, PS2 ou personalizado. Para gerar o pack de novo: `python tools\hd_pack_match.py buttons --pack <pasta replacements do 4K PS2>`.

Gráficos também têm brilho, contraste e saturação; Áudio tem "Silenciar quando a janela perde o foco". O cabeçalho e o Diagnóstico mostram o nome da área (tirado dos teleportes do jogo) com os IDs de mapa/sala; os mapas 0, 1, 7 e 12 ainda aparecem só com o ID. Em Suporte > Diagnóstico, "Pesquisar vida dos inimigos (log)" registra os campos do inimigo alvo que caem a cada golpe: ligue, mate 2 ou 3 inimigos e salve um relato.

Mouse horizontal/vertical têm inversão separada. O padrão foi invertido em relação ao build anterior por pedido do usuário. WASD movimenta, setas controlam câmera; os dois grupos podem ser remapeados. Restaurar controles padrão repõe as teclas do port.

Jogabilidade > Inimigos: todos > "Vida dos inimigos" (0,5x a 5x, chave `enemy_hp`) multiplica a vida máxima calculada pelo jogo para cada tipo de inimigo. Vale para inimigos que aparecem depois da mudança; troque de sala para ver.

Jogabilidade > Inimigos: todos > "Tamanho dos inimigos" (0,5x a 3x, chave `enemy_scale`) muda o tamanho de todos; o ajuste por tipo multiplica por cima. Jogabilidade > Inimigos: por tipo ajusta vida e tamanho por tipo de inimigo. Os ajustes ficam em `mods/Editor/enemies.toml` (`[enemy.N]` com `name`, `hp` de 0,5 a 5 e `scale` de 0,5 a 3) e só valem com o mod Editor ligado; nenhum arquivo do jogo muda. O tamanho é só visual: a área de acerto continua a original. O jogo reescala o inimigo depois que ele aparece; desde 26/10 o port reaplica o fator nessa reescala. Essa correção ainda não foi confirmada no jogo. A vida funciona.

Jogabilidade > Inimigos: aleatório sorteia tamanho e vida de cada inimigo ao aparecer (`enemy_random`, faixas `enemy_random_scale_min/max` e `enemy_random_hp_min/max`). A chance de elite (`enemy_elite_chance`, 0 a 100) multiplica por cima com `enemy_elite_scale` e `enemy_elite_hp`. As camadas se multiplicam (todos x tipo x sorteio), com limite final de 0,5x a 3x no tamanho e 0,5x a 5x na vida. Os botões Original, Variados, Elites, Gigantes e Caotico preenchem tudo de uma vez. Trocar um tipo de inimigo por outro ainda está em pesquisa.

O campo "Filtrar opções" no topo do menu dev (Ctrl+F) acha qualquer opção pelo nome, sem acento e em qualquer ordem de palavras; Enter abre a página dela.

Jogabilidade > Drops e itens: "Taxa de drop de itens" (`drop_rate`, 0x a 10x) multiplica a chance de cada item do inimigo; "Chance de cristais dos Innocent Devils" (`crystal_rate`) e "Cristais por drop" (`crystal_amount`, 1x a 5x, no máximo 4 cristais por drop) cuidam dos cristais de evolução; "Itens aleatórios nos drops" (`random_drops`) troca o item por outro da tabela de drops do jogo; "Tamanho dos itens no chão" (`item_scale`, 0,25x a 2x) muda só o visual dos itens que caem.

A opção "inverter câmera" das Opções do jogo só inverte o eixo vertical, como no Xbox original. Para inverter o horizontal do analógico direito, ligue "Inverter camera horizontal (analogico direito)" em Controles > Controle (`invert_stick_x` em `settings.toml`).

Godmode agora intercepta também o setter absoluto de HP, além do delta: ambos passaram pelos wrappers originais em teste isolado, preservando cura e outros personagens. O segundo bypass foi reproduzido antes da correção. Reteste em combate continua necessário.

O painel pode ser movido pela barra de título e redimensionado pelo canto inferior direito. A aba Aparência ajusta opacidade, fundo, seleção e foco. Os controles numéricos de posição/tamanho também aceitam navegação por controle. Restaurar aparência padrão repõe a janela e a paleta preto/vermelho/dourado; não apaga saves nem remapeamentos.

Salvar opções grava as preferências em settings.toml. Fechar o painel grava também sua geometria. A aba Controles mostra a referência Xbox, a ação e a tecla em colunas separadas.

Saves e sessão lista apenas nomes/tamanhos dos arquivos de progresso. Reiniciar no título pede confirmação da perda de progresso não salvo e usa o reinício nativo do runtime; carregue um slot pelo Load Game original. Não restaura nem substitui arquivos de save enquanto o jogo roda.

Savestate, troca de personagem no meio da campanha e novos presets de cheats ainda não foram validados.

Abra o jogo pelo atalho **Castlevania COD Recomp**. **F1** ou **L3 + R3** abre/fecha o menu, **F3** captura print e diagnóstico gráfico e salva um relatório automático, **F8** abre o formulário de relato escrito e **F9** liga/desliga o som. L3/R3 são os cliques nos analógicos esquerdo/direito. Solte os botões antes de repetir o atalho. Para voltar ao título e carregar outro save, segure **Start + Back + LT + RT** por 2 segundos (o jogo reinicia; progresso não salvo é perdido).

O menu usa preto, vermelho profundo e dourado, fontes do Windows renderizadas em resolução maior e escala automática. No controle, cima/baixo navega, esquerda/direita ou L1/R1 troca de seção, A/cruz ajusta/confirma e B/círculo sai do ajuste ou volta. A combinação para abrir pode ser desativada em Controles.

Se B/Círculo já estiver segurado ao abrir, ele só passa a funcionar como Voltar depois que você o soltar; isso evita fechar as ferramentas sem querer.

O menu mantém o mundo rodando. Analógicos controlam personagem e câmera; direcional/A/B/L1/R1 operam o painel. Mouse e teclado ficam na interface e não acionam ataques/câmera do jogo. Pause pelo próprio jogo antes de abrir as ferramentas durante combate. Clique em **Salvar opções** para persistir controles, áudio, vídeo e mods em `settings.toml`.

Todos os caminhos abaixo ficam ao lado de `CastlevaniaCOD.exe`, normalmente em `out/build/release`.

F3 salva a imagem e o relatório depois que a captura gráfica termina. A confirmação aparece na tela só após os arquivos serem gravados. A imagem também vai para `reports/`; o rastreio completo fica em `captures/orange-marker/` por padrão. `XBE_CAPTURE_DIR` permite escolher outra pasta.

## Relatar problemas

1. Pressione Print Screen ou F8 quando o problema aparecer. O painel abre em Suporte > Relatar bug com a imagem capturada.
2. Escreva título e descrição.
3. Clique em Salvar relato local.

Cada pasta `reports/data-hora-identificador` contém `screenshot.bmp`, `report.txt` e `runtime.log`. A captura vem do quadro do jogo, sem o menu de ferramentas nem a Área de Trabalho. Nenhum arquivo é enviado automaticamente. O log corresponde ao momento em que você salva o relato. Capturas durante carregamento podem registrar um quadro preto; use Nova captura quando a imagem desejada estiver visível.

Outras páginas da aba Suporte:

- **Capturas salvas**: as 12 mais novas. Passe o direcional pela lista para ver a imagem; Abrir pasta e Copiar relato agem sobre a selecionada.
- **Logs e travamentos**: últimas 200 linhas do log desta sessão (Copiar log, Abrir pasta do log). Cada travamento grava `CrashDumps/<data>.dmp` e uma cópia do log com o mesmo nome; abra o item para ver as últimas linhas.

No Windows 11, se a opção "Usar a tecla Print Screen para abrir a captura de tela" estiver ligada, a Ferramenta de Captura pode abrir junto. Desligue a opção em Configurações > Acessibilidade > Teclado ou use F8.

## Mapeamento do controle

Em F1 > Controles > Controle, selecione um botão do jogo, aperte A (ou clique) e pressione o botão físico que deve acioná-lo. O botão que já usava esse físico fica com o antigo, então nenhum comando some. Esc cancela; sem resposta em 6 s, a captura desiste. "Trocar A e B" e "Restaurar padrão" fazem o que dizem. Salvar opções grava o mapa (chave `controller_map`), e os perfis salvos o levam junto.

O menu dev, L3 + R3 e Start + Back + LT + RT usam sempre os botões físicos. Só vale para o jogador 1 e só para botões; os analógicos não mudam.

## Teclado e mouse

| Comando | Padrão |
|---|---|
| Movimento | WASD |
| Câmera | Mouse, simulando o analógico direito |
| Direcional (navega nos menus do jogo) | Setas |
| Abrir menu do jogo / voltar dentro dele | Esc |
| A, confirmar no menu | Enter |
| A, confirmar/pular | Espaço |
| B, voltar/esquivar | E |
| X, ataque | Clique esquerdo |
| Y, ataque especial | Clique direito |
| Preto / branco | Q / R |
| Gatilhos esquerdo / direito | Shift / Ctrl |
| Start / Back | P / Tab |

Na aba Controles, clique na tecla de uma ação e pressione a nova tecla ou botão do mouse. F1/F8/F9/Esc são reservados. Configuração salva antes desta versão com Enter no Start passa o Start para P. A câmera conserva as limitações e inversões do analógico original do jogo. F1 e a perda de foco liberam o cursor. O controle físico continua disponível; quando houver movimento de mouse, ele controla a câmera.

## Mods de arquivos

Crie `mods/NomeDoMod/files/` e reproduza o caminho relativo dos arquivos em `assets/`. Exemplo: um arquivo original `assets/DATA/exemplo.bin` recebe a substituição `mods/NomeDoMod/files/DATA/exemplo.bin`.

Abra Mods, recarregue a lista, marque o mod e salve as opções. Reinicie o jogo para trocar arquivos já carregados. O arquivo substituto é aberto somente para leitura. A camada só atende leituras do disco do jogo; saves em `hdd/` ficam fora dela. Não altere os assets originais.

Se dois mods oferecerem o mesmo arquivo, o nome em ordem alfabética mais alta vence. Desmarcar e reiniciar remove o efeito. Caminhos absolutos, `..` e saídas da pasta do mod por links são rejeitados.

## Packs de texturas

1. Marque Exportar texturas na aba Mods e passe pela tela que deseja alterar.
2. As primeiras 200 texturas compatíveis da execução aparecem em `texture-dumps/`.
3. Edite o BMP preservando seu nome de 16 dígitos hexadecimais.
4. Coloque-o em `mods/MeuPack/textures/0123456789ABCDEF.bmp`.
5. Recarregue a lista, ative o pack e salve. Reinicie se o conteúdo já estava em cache.

Aceita BMP sem compressão, 24 ou 32 bits, até 16384×16384. Também aceita `<hash>.dds` em BC7_UNORM (com ou sem mipmaps), que ocupa 1/4 da memória de vídeo; quando os dois existem, o DDS vence. Texturas em `mods/PTBR-PS2/textures` valem só com o idioma Português, mesmo com os packs desligados.

Pacote 4K do PS2 (PlatinumMimic, ver [CREDITS.pt-BR.md](docs/CREDITS.pt-BR.md)): `python tools/hd_pack_match.py --pack <pasta replacements>` pareia as imagens com as texturas do Xbox; `... build --pack <pasta>` gera `mods/HD4K/textures` em DDS BC7 (precisa de `.local/build-tools/texconv.exe`); `... near --pack <pasta>` aproveita as quase iguais (detalhe do 4K onde a imagem bate, original ampliada onde difere; `--dry-run --preview <pasta>` para conferir antes); `... ptbr --pack <pasta>` monta as texturas traduzidas em alta. Depois ligue `HD4K` em F1 > Mods. Use 32 bits para manter o canal alpha das transparências. O identificador depende do conteúdo original, formato, dimensões e paleta, não do endereço temporário na memória. Texturas de cubo, volume e render targets ficam fora da substituição. A exportação atende formatos convertidos pelo backend para BGRA8. Packs aceitam BMP 24/32 bits e DDS BC7 com mipmaps; PNG não é lido pelo jogo (converta com a ferramenta).

O mod `PTBR-PS2` não tem checkbox próprio: ele liga com **Opções → Idioma →
Português (Brasil)** no jogo e não desliga outros mods.
Veja [TRANSLATION.pt-BR.md](docs/TRANSLATION.pt-BR.md) para montagem local e limites de cobertura.

Filtro anisotrópico e resolução interna estão em Gráficos. Eles melhoram a amostragem; detalhes HD exigem arquivos de um pack. Os shaders disponíveis são Original, Escala de cinza e CRT leve, aplicados à apresentação final.

## Cheats de Xbox

**Godmode** está em Jogabilidade > Cheats. Ele protege a vida do personagem principal, bloqueando deltas negativos na rotina original de HP desta versão Xbox USA. A cura e os outros atores continuam funcionando. O programa confere a assinatura da rotina antes de habilitar a opção. Godmode começa desligado a cada execução e mostra quantos danos bloqueou.

A implementação consulta o mapa de IDs do jogo para proteger o slot do jogador. O ID lógico pode ser 0 ou 1; não é a posição física na lista. O teste usa as rotinas originais de construção desse mapa e de aplicação de dano em uma arena separada: dano normal 100→90, Godmode 100→100, cura 100→110 e outro ator 100→90, inclusive quando o ID 0 pertence a outro slot. Não remove animações de impacto nem revive personagem já morto. Mortes comandadas por scripts ou outros caminhos não foram comprovadas em gameplay.

Para perfis adicionais, copie `cheats.example.toml` para `cheats.toml` e preencha apenas endereços que você validou para o Xbox USA deste projeto. Códigos de PS2/GameShark não podem ser colados diretamente.

Cada entrada precisa de `name`, `address`, `width` (1, 2 ou 4 bytes), `expected` e `value`. O endereço é relativo à memória Xbox. O carregador aceita dados entre `0x10000` e `0x04000000`; exige uma página gravável e o valor esperado antes da primeira aplicação. O cabeçalho SHA-256 precisa corresponder ao `default.xbe` USA.

Cheats começam desligados. Ativados, repetem a escrita a cada quadro. Ao desligar, restauram o valor original somente se ainda encontrarem o valor escrito pelo patch. Isso não valida que um endereço represente vida ou dinheiro. Um perfil errado pode alterar o estado do jogo; teste com cópia do seu save.

## Velocidade e limites

**Speedhack**, em Jogabilidade > Cheats, varia de 0,5× a 2×. Agora escala RDTSC/QPC, tempo do sistema Xbox, esperas, timers, VBlank, apresentação e consumo das amostras de áudio. A troca de velocidade mantém o relógio contínuo. Vsync é temporariamente suspenso fora de 1× e a preferência salva volta a valer em 1×.

O som muda de tom junto com a velocidade; não há preservação de pitch. A saída XAudio2 permanece estéreo 48 kHz. O jogo sempre inicia em 1×. Use Normal para restaurar.

Testes dentro do runtime confirmaram que 60 ms virtuais levaram aproximadamente 61 ms reais em 1×, 31 ms em 2× e 122 ms em 0,5×. Um timer já armado também respondeu à mudança de fator. Isso não comprova campanha inteira, toda a física ou todas as cenas de vídeo em cada fator. Surround nativo, DSP completo e perfis adicionais continuam pendentes.

## Referências

Organização das opções inspirada nos projetos oficiais [PCSX2](https://github.com/PCSX2/pcsx2) e [xemu](https://github.com/xemu-project/xemu). Implementação local sobre o SDK XboxXBERecomp MIT, usando ImGui e toml++ já presentes. Nenhum código dos emuladores foi incorporado.

Identificação da rotina de HP conferida com o pacote [Game Trainers do XBMC4Gamers](https://www.xbmc4gamers.co.uk/xbox/urldownloader/other/game%20trainers.zip), entrada T0150.etm, e com os bytes do XBE USA local. Nenhum trainer foi instalado ou executado; o hook é implementação própria na recompilação.


## Minimapa

Ligado por padrão durante a partida, no canto superior direito. Fundo transparente, salas azuis e marcador dourado centrado no jogador. Usa malhas stmap e transformação do cursor original, sem abrir Back nem escrever no save.

Em Jogo: ligar/desligar, tamanho0,6–1,6× e opacidade0,2–1. Salvar opções mantém preferências no próximo início. Orientação fixa; os contornos se deslocam com o personagem. Pode mostrar salas vizinhas ainda não exploradas. Descoberta e ícones/cores especiais de save/teleporte ainda não existem. Algumas cenas com jogador ativo podem continuar mostrando o mapa.


## HUD, menu original e câmera

Em Jogo → Opções de jogo: minimapa, Mostrar HUD original, escala0,35–1,0× (padrão0,5×), botão50%, ocultação rápida e espera0,2–5s. A espera substitui o prazo de5s da rotina original; o fade original0,3s e seus gatilhos permanecem. Não é um detector novo de inimigos.

F1 ou L3+R3 abre/fecha o painel dev. Esc envia Start ao jogo e B quando o gerenciador modal/pausa está aberto, para sair/voltar no menu original. Esc não alterna o painel dev.

Câmera: Original; Cinemática inspirada em Lords of Shadow1; Próxima inspirada em Lords of Shadow2; Personalizada; Livre. Os perfis aproximam enquadramento e não reproduzem os sistemas desses jogos. Personalizada ajusta distância, altura e campo de visão relativo. Personalizada e Livre têm ainda ângulo horizontal (±180°), inclinação (±60°) e rotação do horizonte (±45°), somados aos ângulos do próprio jogo; "Zerar" volta ao normal. Na Personalizada, ângulos horizontais grandes deixam o analógico no referencial antigo.

FPS na tela: F1 > Gráficos > Mostrar FPS na tela (escolha o canto). Verde a partir de 55, amarelo de 30 a 55, vermelho abaixo. Quedas de FPS: em F1 > Suporte > Diagnóstico, ligue "Registrar desempenho no log", jogue o trecho lento por 1 minuto e salve um relato; o log com pior quadro e quadros lentos vai junto.

Câmera livre: fechar painel, WASD move, Q/E desce/sobe e mouse olha. Controle: analógico esquerdo move, direito olha e LT/RT descem/sobem. Velocidade no menu. Enquanto ativa na partida, os comandos deixam de mover o personagem; o mundo continua rodando. No título e na seleção de personagem os comandos nativos permanecem disponíveis. Voltar a Original restaura o sistema nativo.

Câmeras são experimentais: não há colisão da câmera livre nem identificação completa de cutscenes. Algumas cenas podem ter enquadramento alterado enquanto o modo estiver ativo. Mapas e câmera debug identificados ficam excluídos. Todos os parâmetros nativos alterados temporariamente são restaurados após gerar a matriz do quadro.


## Atualização: menus restaurados e progresso experimental

Menus nativos permanecem100% após relato de desalinhamento em40%. A redução e o mundoao fundo foram adiados a pedido; HUD50% é independente e continua ajustável. Não há controle ativo de escala dos menus nesta revisão.

Em Jogo: Girar minimapa conforme câmera liga/desliga a rotação. Câmera → Orbital em terceira pessoa ativa free look e acompanhamento suave, com consulta nativa de colisão. Colisão foi testada em triângulo isolado; cantos, áreas e movimento relativo precisam teste emgameplay. Original restaura câmera nativa.

Português (Brasil): escolha em **Opções → Idioma** no jogo. Liga o catálogo de 2479 textos da ISO PS2 traduzida e o mod `PTBR-PS2` (imagens e dublagem). Detalhes em [TRANSLATION.pt-BR.md](docs/TRANSLATION.pt-BR.md).
