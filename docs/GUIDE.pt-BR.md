# Guia do jogador: Castlevania: Curse of Darkness Recomp

[English](GUIDE.md) · Português (Brasil)

Este guia explica como instalar, jogar e configurar tudo o que o port oferece. A parte técnica de mods (formato dos arquivos, packs de textura, `cheats.toml`) fica em [MODDING.pt-BR.md](../MODDING.pt-BR.md).

O menu de ferramentas começa em português. Os botões **PT-BR** e **EN** no canto superior direito trocam na hora; no controle, use Menu > Aparência > "Idioma do menu". A escolha fica salva em `settings.toml` (`menu_language`). Isso muda só o texto das ferramentas; o idioma do jogo é outra opção.

## Requisitos de PC

| | Mínimo | Testado em |
|---|---|---|
| Sistema | Windows 10 ou 11, 64 bits | Windows 11 Pro |
| Processador | x86-64 com SSE4.2 e POPCNT (o código é compilado com `-march=x86-64-v2`): Intel Core de 2008 em diante ou AMD de 2011 em diante | Intel Core i5-10400F (6 núcleos) |
| Placa de vídeo | Direct3D 12, nível 11_0, com driver atualizado | NVIDIA GeForce RTX 3060, 12 GB |
| Memória | Não medida | 32 GB |
| Disco | 6,6 GB com tudo: jogo 2,9 GB, HD4K 3,3 GB e PT-BR 0,5 GB. Sem o HD4K, cerca de 3,3 GB | SSD |
| Bibliotecas | Visual C++ Redistributable 2015–2022 **x64** | Instalado |

Na máquina de teste, o jogo roda em 4K (3840×2160) com resolução interna de 200% e limite de 60 FPS. Há quedas de FPS ocasionais ainda em investigação. Em PC mais fraco, comece com resolução interna 100% e sem o HD4K.

Linux: o Direct3D 12 roda pelo Proton (vkd3d), mas isso não foi testado.

## Instalar e abrir

**Pasta pronta (só jogar):** na pasta do projeto, rode no PowerShell:

```powershell
.\tools\make-portable.ps1
```

Ele cria "Castlevania Curse of Darkness Recomp" na área de trabalho, com o jogo, os mods, os saves e as opções. Clique em `CastlevaniaCOD.exe` para jogar.

- **Atualizar:** rode o script de novo depois de um build. Ele atualiza o jogo e os mods, mas não mexe nos saves nem nas opções da pasta pronta.
- **Outro disco ou PC:** copie a pasta inteira pelo Explorador, que mantém as datas dos arquivos. A dll precisa continuar mais nova que o exe; se não estiver, o jogo procura a pasta `compiler` e para.
- **Começar do zero:** apague a pasta do save em `hdd\E\UDATA\4b4e002d` (a que contém `C2Save.dat`).
- **Ícone:** um `assets\icon.ico` seu vira o ícone do exe e da janela no próximo build.

**Pelo projeto (desenvolvimento):** veja "Como instalar e rodar" no [README](../README.pt-BR.md). O exe fica em `out\build\release`.

A pasta tem os arquivos do seu jogo. Não compartilhe.

## Atalhos

| Ação | Controle | Teclado |
|---|---|---|
| Abrir e fechar o menu de ferramentas | L3 + R3 (clicar os dois analógicos) | F1 |
| Buscar uma opção no menu | Y abre o teclado na tela | Ctrl+F |
| Voltar ao título (perde o que não foi salvo) | Segurar Start + Back + LT + RT por 2 s | — |
| Capturar print e diagnóstico gráfico | — | F3 |
| Capturar e abrir o relato de bug | — | F8 ou Print Screen |
| Ligar e desligar o som | — | F9 |
| Tela cheia ou janela | — | Alt+Enter |
| Menu do jogo / voltar dentro dele | Start / B | Esc |

O menu de ferramentas, L3 + R3 e Start + Back + LT + RT usam sempre os botões físicos. Por isso funcionam mesmo com o controle remapeado.

### Teclado e mouse no jogo (padrão, remapeável)

| Comando | Tecla |
|---|---|
| Movimento | WASD |
| Câmera | Mouse |
| Direcional (menus do jogo) | Setas |
| A: confirmar ou pular / confirmar no menu | Espaço / Enter |
| B: voltar ou esquivar | E |
| X: ataque / Y: ataque especial | Clique esquerdo / clique direito |
| Preto / branco | Q / R |
| Gatilho esquerdo / direito | Shift / Ctrl |
| Start / Back (mapa) | P / Tab |

## Menu de ferramentas

O menu abre por cima do jogo, e o jogo continua rodando: os analógicos andam e giram a câmera, e os botões ficam com o menu. Para pausar tudo, ligue "Pausar o jogo com o menu aberto" em Menu > Aparência.

Navegação:
- L1/R1 trocam de aba.
- O direcional escolhe um título na coluna da esquerda, e A entra nas opções dele.
- Numa barra, A entra no ajuste; o direcional ou o analógico esquerdo arrasta, e B sai.
- B volta das opções para os títulos. Só L3 + R3 ou F1 fecham o menu.
- **Salvar opções** grava tudo em `settings.toml`.

**Busca:** o campo com a lupa, no topo, acha qualquer opção pelo nome, sem acento e com as palavras em qualquer ordem.
- No teclado, digite e aperte Enter para abrir a primeira opção encontrada.
- No controle, Y abre o teclado na tela, A digita e B fecha. O teclado tem as teclas Apagar, Limpar e "Abrir 1o".

### Jogabilidade

| Página | O que tem |
|---|---|
| Jogo e minimapa | Idioma do jogo (English / Português); minimapa (ligar, girar com a câmera, tamanho 0,6–1,6x, opacidade, posição horizontal e vertical, botão "canto superior direito") |
| Dificuldade | Presets Fácil, Normal, Difícil, Muito difícil e Pesadelo; barras "Dano recebido pelo jogador" e "Dano recebido pelo Innocent Devil". O golpe final e as mortes por roteiro continuam matando. |
| Inimigos: todos | Vida dos inimigos (0,5x–5x) e tamanho (0,5x–3x); combinações prontas Original, Variados, Elites, Gigantes e Caotico; botão "Restaurar padrão dos inimigos" |
| Inimigos: aleatório | Cada inimigo sorteia tamanho e vida ao aparecer, dentro das faixas mínima e máxima. A chance de elite (0–100%) e o tamanho e a vida extras da elite multiplicam por cima. "Trocar tipos de inimigo" ainda está em pesquisa. |
| Inimigos: por tipo | Vida e tamanho para cada tipo de inimigo já visto, como mod em `mods/Editor/enemies.toml`; valem com "Aplicar edições" ligado. Tem os botões "Voltar ao original" e "Voltar todos os tipos ao original". |
| Drops e itens | Taxa de drop de itens (0x–10x); chance de cristais dos Innocent Devils (0x–10x) e cristais por drop (1x–5x, no máximo 4 por drop); itens aleatórios tirados da tabela de drops do próprio jogo; tamanho dos itens no chão (0,25x–2x, botão "Itens pela metade") |
| Cheats | Godmode; speedhack (0,5x–2x); vida cheia; MP cheio; MP infinito; ouro máximo; ganho de ouro e de EXP (1x–10x); compras sem gastar ouro; "Simular save de Lament of Innocence" (libera a estátua Moai); `cheats.toml` próprio |
| HUD e pets | Mostrar HUD original; vida e MP sempre visíveis; escala do HUD; ocultar mais rápido fora de combate e tempo de espera |
| Câmera | Modos Original, Cinemática, Próxima, Personalizada, Livre e Orbital. Ajustes de distância, altura, campo de visão, ângulo horizontal, inclinação e rotação do horizonte, além de acompanhamento suave e velocidade da câmera livre. |
| Saves e sessão | Lista os arquivos de save; "Reiniciar no título" para carregar outro save. O menu não apaga nem troca saves. |

As camadas de inimigos se multiplicam: todos × tipo × sorteio. Tamanho e itens menores são só visuais; a área de acerto e a de pegar o item continuam as originais. A vida vale para os inimigos que aparecem depois da mudança, então troque de sala para ver.

### Gráficos, Áudio e Controles

| Aba | O que tem |
|---|---|
| Gráficos | Tela cheia ou janela; resolução; resolução interna; filtro anisotrópico; limite de FPS; vsync; shader de apresentação (Original, Escala de cinza, CRT leve); brilho, contraste e saturação; FPS na tela, com escolha do canto |
| Áudio | Volume geral; balanço esquerda/direita; sem som (F9); silenciar quando a janela perde o foco |
| Controles > Controle | Remapear cada botão do jogo (A em cima do botão e aperte o botão físico novo); Layout Xbox e Layout PS2; Trocar A e B; inverter a câmera horizontal do analógico direito (a opção do jogo só inverte a vertical); desligar o atalho L3 + R3 |
| Controles > Teclado e mouse | Remapear teclas; ligar o teclado; mouse como câmera; inverter o mouse em cada eixo; sensibilidade; restaurar o padrão |

### Mods, Suporte e Menu

| Aba | O que tem |
|---|---|
| Mods | Lista os mods em `mods/`, com caixa para ligar cada um; recarregar a lista; exportar texturas para criar um pack HD |
| Suporte | **Relatar bug:** captura do quadro do jogo, título e texto, salvo em `reports/`. **Capturas salvas:** as 12 mais novas. **Logs e travamentos:** últimas linhas do log e dumps de travamento. **Diagnóstico:** registro de desempenho, pesquisa da vida dos inimigos e área e sala atuais. Nada é enviado para a internet. |
| Menu | **Aparência:** idioma do menu, opacidade, cores, pausar com o menu aberto. **Perfis:** salvar e aplicar conjuntos de opções, perfil Dev e padrão vanilla. |

## Mods incluídos

| Mod | O que faz | Como ligar |
|---|---|---|
| PTBR-PS2 | Tradução de 2479 textos, imagens e dublagem em português, trazidas da ISO PS2 dos fãs | Opções > Idioma do jogo > Português (também em Jogabilidade > Jogo e minimapa) |
| HD4K | 2739 texturas 4K do pacote PS2 (PlatinumMimic), pareadas com as do Xbox | Aba Mods |
| PS2-Botoes | Ícones ✕ ○ □ △ L1 L2 R1 R2 no lugar dos botões do Xbox | Aba Mods; para os gatilhos, use também Controles > Layout PS2 |
| Editor | Ajustes de vida e tamanho por tipo de inimigo | Jogabilidade > Inimigos: por tipo > "Aplicar edições" |

Os mods nunca alteram os arquivos originais do jogo: substituem só na memória. Para criar o seu, veja [MODDING.pt-BR.md](../MODDING.pt-BR.md). Os créditos dos materiais de terceiros estão em [CREDITS.pt-BR.md](CREDITS.pt-BR.md).

## O que muda em relação ao Xbox original

- **Vídeo:** roda no Windows com Direct3D 12, em qualquer resolução, com resolução interna maior, filtro anisotrópico e FPS na tela. A sombra do personagem foi corrigida (multisample 4X).
- **Idioma:** tradução e dublagem em português, com troca de idioma durante o jogo.
- **Texturas:** pacote 4K e ícones de botão do PS2.
- **Controles:** teclado e mouse; controles de Xbox, PlayStation e genéricos (SDL3); remapeamento; inversão horizontal da câmera.
- **Ferramentas:** menu dev com busca, câmeras alternativas e minimapa na tela.
- **Jogabilidade:** dificuldade por dano; vida, tamanho, sorteio e elites de inimigos; taxa de drops, cristais e itens aleatórios; cheats de vida, MP, ouro e EXP; bônus Moai sem o save do PS2.
- **Suporte:** relato de bug com captura e log, registro de desempenho e dumps de travamento.

## Recursos experimentais e estado

"Validado no jogo" quer dizer que alguém jogou e viu funcionar. "Testado" quer dizer que os testes automáticos ou o autoteste do runtime passaram, mas ninguém confirmou jogando. Relate qualquer coisa estranha com F8.

### Funciona, validado no jogo

| Recurso | Observação |
|---|---|
| Texto, imagens e dublagem em português | Troca de idioma durante o jogo; imagens e vozes já carregadas mudam ao reiniciar. |
| Pacote HD4K e HD da tela de título | Texturas pareadas carregam sem queda de FPS. |
| Vida dos inimigos (todos) | 0,5x–5x pela fórmula de vida máxima do próprio jogo (185 → 925 com 5x). |
| Voltar ao título (Start + Back + LT + RT) | O progresso não salvo se perde. |

### Feito e testado, falta confirmar no jogo

| Recurso | O que ainda não se sabe |
|---|---|
| Tamanho dos inimigos (todos, por tipo, sorteio, elites) | Corrigido em 26/10 para sobreviver à reescala do próprio jogo; ainda não foi visto funcionando. O tamanho é só visual. |
| Sorteio e elites de inimigos | Igual ao tamanho; a vida sorteada usa o retorno nativo da vida máxima. |
| Taxa de drop e cristais dos Innocent Devils | Só as 5 checagens de drop da rotina de morte do inimigo mudam. |
| Itens aleatórios nos drops | O log mostra trocas (`drops: random item 230 -> 242`), mas o jogador ainda não percebeu. |
| Tamanho dos itens no chão | Só visual; a área de pegar continua a original. |
| Busca do menu com teclado na tela | Ctrl+F e Y; Enter ou "Abrir 1o" abre o primeiro resultado. |
| Menu de ferramentas em inglês (botões PT-BR / EN, canto superior direito) | 402 textos; um teste sem janela clica nos botões. Alguns textos longos em inglês podem ficar cortados em botões de largura fixa. |
| Godmode | Dano bloqueado numa arena isolada com as rotinas originais; mortes por roteiro não foram comprovadas. |
| Speedhack 0,5x–2x | Relógios e timers medidos; a campanha inteira em cada velocidade, não. |
| Bônus do save de *Lament of Innocence* | O objeto do bônus é liberado, mas a sala onde ele aparece não foi identificada. |

### Experimental por natureza

| Recurso | Limites |
|---|---|
| Câmeras Cinemática, Próxima e Personalizada | Só perfis de enquadramento; ângulos grandes entortam o movimento e algumas cenas podem ser afetadas. |
| Câmera Livre | Sem colisão; o personagem fica parado enquanto ela está ligada. |
| Câmera Orbital | Colisão nativa testada num único triângulo; cantos e salas apertadas não foram testados. |
| Escala do HUD e ocultação rápida | Inventário e menus originais não mudam de escala; cobertura visual completa pendente. |
| Minimapa | Mostra salas ainda não exploradas; sem ícones de porta, save ou teleporte. |
| Perfis próprios em `cheats.toml` | Só com endereços que você mesmo validou; endereço errado pode estragar o estado do jogo. |

### Problemas conhecidos e o que ainda não existe

- **Sombra do personagem:** corrigida para o modo multisample 4X, mas ainda aparece errada em alguns lugares. Envie uma captura F3 quando acontecer.
- **Quedas de FPS ocasionais:** em investigação; ligue o registro de desempenho e envie um relato.
- **Áudio:** só estéreo; os efeitos DSP do Xbox são reproduzidos em parte.
- **Em pesquisa:** trocar tipos de inimigo, mais inimigos por sala e mover inimigos.
- **Não existe:** savestate.
- **Não testados nesta versão:** códigos de nome @CRAZY e @TREVOR e troca de personagem no meio da campanha.
