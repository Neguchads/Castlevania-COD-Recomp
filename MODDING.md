# Ferramentas e mods

## Personalização do painel

Navegação em seis seções superiores: Jogo, Imagem, Som, Controles, Mods e cheats, Sistema. Esquerda/direita ou L1/R1 muda a seção; cima/baixo navega, A ajusta/confirma e B sai do ajuste ou volta. F1 abre/fecha o painel. Esc opera o menu original. O cabeçalho mostra os IDs internos de mapa/sala; nomes das áreas ainda não foram identificados.

Mouse horizontal/vertical têm inversão separada. O padrão foi invertido em relação ao build anterior por pedido do usuário. WASD movimenta, setas controlam câmera; os dois grupos podem ser remapeados. Restaurar controles padrão repõe as teclas do port.

Godmode agora intercepta também o setter absoluto de HP, além do delta: ambos passaram pelos wrappers originais em teste isolado, preservando cura e outros personagens. O segundo bypass foi reproduzido antes da correção. Reteste em combate continua necessário.

O painel pode ser movido pela barra de título e redimensionado pelo canto inferior direito. A aba Aparência ajusta opacidade, fundo, seleção e foco. Os controles numéricos de posição/tamanho também aceitam navegação por controle. Restaurar aparência padrão repõe a janela e a paleta preto/vermelho/dourado; não apaga saves nem remapeamentos.

Salvar opções grava as preferências em settings.toml. Fechar o painel grava também sua geometria. A aba Controles mostra a referência Xbox, a ação e a tecla em colunas separadas.

Saves e sessão lista apenas nomes/tamanhos dos arquivos de progresso. Reiniciar no título pede confirmação da perda de progresso não salvo e usa o reinício nativo do runtime; carregue um slot pelo Load Game original. Não restaura nem substitui arquivos de save enquanto o jogo roda.

Pausa completa, savestate, troca de personagem no meio da campanha e novos presets de cheats ainda não foram validados. O mundo continua rodando ao abrir este build do painel.

Abra o jogo pelo atalho **Castlevania COD Recomp**. **F1** ou **L3 + R3** abre/fecha o menu, **F3** captura print e diagnóstico gráfico e salva um relatório automático, **F8** abre o formulário de relato escrito e **F9** liga/desliga o som. L3/R3 são os cliques nos analógicos esquerdo/direito. Solte os botões antes de repetir o atalho.

O menu usa preto, vermelho profundo e dourado, fontes do Windows renderizadas em resolução maior e escala automática. No controle, cima/baixo navega, esquerda/direita ou L1/R1 troca de seção, A/cruz ajusta/confirma e B/círculo sai do ajuste ou volta. A combinação para abrir pode ser desativada em Controles.

Se B/Círculo já estiver segurado ao abrir, ele só passa a funcionar como Voltar depois que você o soltar; isso evita fechar as ferramentas sem querer.

O menu mantém o mundo rodando. Analógicos controlam personagem e câmera; direcional/A/B/L1/R1 operam o painel. Mouse e teclado ficam na interface e não acionam ataques/câmera do jogo. Pause pelo próprio jogo antes de abrir as ferramentas durante combate. Clique em **Salvar opções** para persistir controles, áudio, vídeo e mods em `settings.toml`.

Todos os caminhos abaixo ficam ao lado de `CastlevaniaCOD.exe`, normalmente em `out/build/release`.

F3 salva a imagem e o relatório depois que a captura gráfica termina. A confirmação aparece na tela só após os arquivos serem gravados. A imagem também vai para `reports/`; o rastreio completo fica em `captures/orange-marker/` por padrão. `XBE_CAPTURE_DIR` permite escolher outra pasta.

## Relatar problemas

1. Pressione F8 quando o problema aparecer.
2. Escreva título e descrição na aba Relatar bug.
3. Clique em Salvar relato local.

Cada pasta `reports/data-hora-identificador` contém `screenshot.bmp`, `report.txt` e `runtime.log`. A captura vem do quadro do jogo, sem o menu de ferramentas nem a Área de Trabalho. Nenhum arquivo é enviado automaticamente. O log corresponde ao momento em que você salva o relato. Capturas durante carregamento podem registrar um quadro preto; use Nova captura quando a imagem desejada estiver visível.

## Teclado e mouse

| Comando | Padrão |
|---|---|
| Movimento | WASD ou setas |
| Câmera | Mouse, simulando o analógico direito |
| A, confirmar/pular | Espaço |
| B, voltar/esquivar | E |
| X, ataque | Clique esquerdo |
| Y, ataque especial | Clique direito |
| Preto / branco | Q / R |
| Gatilhos esquerdo / direito | Shift / Ctrl |
| Start / Back | Enter / Tab |

Na aba Controles, clique na tecla de uma ação e pressione a nova tecla ou botão do mouse. F1/F8/F9/Esc são reservados. WASD e setas são fixos nesta versão. A câmera conserva as limitações e inversões do analógico original do jogo. F1 e a perda de foco liberam o cursor. O controle físico continua disponível; quando houver movimento de mouse, ele controla a câmera.

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

Aceita BMP sem compressão, 24 ou 32 bits, até 4096×4096. Use 32 bits para manter o canal alpha das transparências. O identificador depende do conteúdo original, formato, dimensões e paleta, não do endereço temporário na memória. Texturas de cubo, volume e render targets ficam fora da substituição. A exportação atende formatos convertidos pelo backend para BGRA8. Mipmaps e PNG/DDS não são suportados nesta versão.

Filtro anisotrópico e resolução interna estão em Gráficos. Eles melhoram a amostragem; detalhes HD exigem arquivos de um pack. Os shaders disponíveis são Original, Escala de cinza e CRT leve, aplicados à apresentação final.

## Cheats de Xbox

**Godmode** está pronto nas abas Desenvolvimento e Cheats. Ele protege a vida do personagem principal, bloqueando deltas negativos na rotina original de HP desta versão Xbox USA. A cura e os outros atores continuam funcionando. O programa confere a assinatura da rotina antes de habilitar a opção. Godmode começa desligado a cada execução e mostra quantos danos bloqueou.

A implementação consulta o mapa de IDs do jogo para proteger o slot do jogador. O ID lógico pode ser 0 ou 1; não é a posição física na lista. O teste usa as rotinas originais de construção desse mapa e de aplicação de dano em uma arena separada: dano normal 100→90, Godmode 100→100, cura 100→110 e outro ator 100→90, inclusive quando o ID 0 pertence a outro slot. Não remove animações de impacto nem revive personagem já morto. Mortes comandadas por scripts ou outros caminhos não foram comprovadas em gameplay.

Para perfis adicionais, copie `cheats.example.toml` para `cheats.toml` e preencha apenas endereços que você validou para o Xbox USA deste projeto. Códigos de PS2/GameShark não podem ser colados diretamente.

Cada entrada precisa de `name`, `address`, `width` (1, 2 ou 4 bytes), `expected` e `value`. O endereço é relativo à memória Xbox. O carregador aceita dados entre `0x10000` e `0x04000000`; exige uma página gravável e o valor esperado antes da primeira aplicação. O cabeçalho SHA-256 precisa corresponder ao `default.xbe` USA.

Cheats começam desligados. Ativados, repetem a escrita a cada quadro. Ao desligar, restauram o valor original somente se ainda encontrarem o valor escrito pelo patch. Isso não valida que um endereço represente vida ou dinheiro. Um perfil errado pode alterar o estado do jogo; teste com cópia do seu save.

## Velocidade e limites

**Speedhack**, em Desenvolvimento e Cheats, varia de 0,5× a 2×. Agora escala RDTSC/QPC, tempo do sistema Xbox, esperas, timers, VBlank, apresentação e consumo das amostras de áudio. A troca de velocidade mantém o relógio contínuo. Vsync é temporariamente suspenso fora de 1× e a preferência salva volta a valer em 1×.

O som muda de tom junto com a velocidade; não há preservação de pitch. A saída XAudio2 permanece estéreo 48 kHz. O jogo sempre inicia em 1×. Use Normal para restaurar.

Testes dentro do runtime confirmaram que 60 ms virtuais levaram aproximadamente 61 ms reais em 1×, 31 ms em 2× e 122 ms em 0,5×. Um timer já armado também respondeu à mudança de fator. Isso não comprova campanha inteira, toda a física ou todas as cenas de vídeo em cada fator. Surround nativo, DSP completo, perfis adicionais e tradução PT-BR continuam pendentes.

## Referências

Organização das opções inspirada nos projetos oficiais [PCSX2](https://github.com/PCSX2/pcsx2) e [xemu](https://github.com/xemu-project/xemu). Implementação local sobre o SDK XboxXBERecomp MIT, usando ImGui e toml++ já presentes. Nenhum código dos emuladores foi incorporado.

Identificação da rotina de HP conferida com o pacote [Game Trainers do XBMC4Gamers](https://www.xbmc4gamers.co.uk/xbox/urldownloader/other/game%20trainers.zip), entrada T0150.etm, e com os bytes do XBE USA local. Nenhum trainer foi instalado ou executado; o hook é implementação própria na recompilação.


## Minimapa

Ligado por padrão durante a partida, no canto superior direito. Fundo transparente, salas azuis e marcador dourado centrado no jogador. Usa malhas stmap e transformação do cursor original, sem abrir Back nem escrever no save.

Em Jogo: ligar/desligar, tamanho0,6–1,6× e opacidade0,2–1. Salvar opções mantém preferências no próximo início. Orientação fixa; os contornos se deslocam com o personagem. Pode mostrar salas vizinhas ainda não exploradas. Descoberta e ícones/cores especiais de save/teleporte ainda não existem. Algumas cenas com jogador ativo podem continuar mostrando o mapa.


## HUD, menu original e câmera

Em Jogo → Opções de jogo: minimapa, Mostrar HUD original, escala0,35–1,0× (padrão0,5×), botão50%, ocultação rápida e espera0,2–5s. A espera substitui o prazo de5s da rotina original; o fade original0,3s e seus gatilhos permanecem. Não é um detector novo de inimigos.

F1 ou L3+R3 abre/fecha o painel dev. Esc envia Start ao jogo e B quando o gerenciador modal/pausa está aberto, para sair/voltar no menu original. Esc não alterna o painel dev.

Câmera: Original; Cinemática inspirada em Lords of Shadow1; Próxima inspirada em Lords of Shadow2; Personalizada; Livre. Os perfis aproximam enquadramento e não reproduzem os sistemas desses jogos. Personalizada ajusta distância, altura e campo de visão relativo; não há conversão validada para graus.

Câmera livre: fechar painel, WASD move, Q/E desce/sobe e mouse olha. Controle: analógico esquerdo move, direito olha e LT/RT descem/sobem. Velocidade no menu. Enquanto ativa na partida, os comandos deixam de mover o personagem; o mundo continua rodando. No título e na seleção de personagem os comandos nativos permanecem disponíveis. Voltar a Original restaura o sistema nativo.

Câmeras são experimentais: não há colisão da câmera livre nem identificação completa de cutscenes. Algumas cenas podem ter enquadramento alterado enquanto o modo estiver ativo. Mapas e câmera debug identificados ficam excluídos. Todos os parâmetros nativos alterados temporariamente são restaurados após gerar a matriz do quadro.


## Atualização: menus restaurados e progresso experimental

Menus nativos permanecem100% após relato de desalinhamento em40%. A redução e o mundoao fundo foram adiados a pedido; HUD50% é independente e continua ajustável. Não há controle ativo de escala dos menus nesta revisão.

Em Jogo: Girar minimapa conforme câmera liga/desliga a rotação. Câmera → Orbital em terceira pessoa ativa free look e acompanhamento suave, com consulta nativa de colisão. Colisão foi testada em triângulo isolado; cantos, áreas e movimento relativo precisam teste emgameplay. Original restaura câmera nativa.

Português(Brasil) parcial traduz102textos ASCII, preserva tokens e não altera assets. Semacentos por enquanto, títulos em textura permanecemEN. Há um seletor no dev e integração experimental na tela nativa; a linha Spoken ainda exige separar idioma dasvozes e textos. Cancelamento/aplicação do idioma precisa refinamento. Progresso e limites em HANDOFF.md.
