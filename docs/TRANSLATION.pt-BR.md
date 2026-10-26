# Tradução PT-BR

[English](TRANSLATION.md) · Português (Brasil)

Escolha no próprio jogo: **Opções → Idioma → Português (Brasil)** e confirme com **Sim**.
As opções são English, Japanese e Português (Brasil). A mesma tela fica na pausa
(Opções → Idioma) durante a partida.

Português liga tudo de uma vez:

- textos: catálogo da ISO PS2 traduzida, embutido no EXE;
- imagens e dublagem: mod `mods/PTBR-PS2`, copiado do patch PS2 por `tools/ps2_*.py`.

A escolha fica salva. Textos mudam na hora. Imagens e vozes já carregadas mudam
depois de reiniciar o jogo. Sem a pasta `PTBR-PS2`, Português traduz só os textos.

O pacote segue só o idioma: na tela Mods ele aparece travado.

## Gerar o mod a partir das ISOs

```powershell
python -m pip install -r tools/requirements-audio.txt
python tools/ps2_movie_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
python tools/ps2_voice_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
python tools/ps2_texture_import.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
```

## Editar uma textura do mod

```powershell
python tools/dar_texture.py export --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png titulo.png
python tools/dar_texture.py import --id 14452 --png titulo.png
```

Mantenha o tamanho da imagem. O import troca o ID em todos os DARs de
`mods/PTBR-PS2/files/data` e guarda os anteriores em `.local/ptbr-ps2/dar-backup`.

Para alta resolução (qualquer tamanho até 16384), sem mexer no DAR:

```powershell
python tools/dar_texture.py hd --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png remake.png
```

Grava `mods/PTBR-PS2/textures/<hash>.bmp`. O hash vem da textura que está no DAR;
se reimportar o ID depois, gere o HD de novo. Vale só com Português ativo.

Relatórios ficam em `.local/ptbr-ps2/`. O que o patch não cobre está em
`.local/ptbr-ps2/FALTANDO.md` (local). Não publique arquivos derivados do jogo
junto ao código do projeto.

O runtime substitui os textos por ID, sem modificar os arquivos extraídos ou os saves.
O catálogo local fica em `assets/ptbr_strings.private.h`; o repositório público inclui
somente o adaptador, com catálogo vazio quando esse arquivo privado está ausente.
Não distribua o catálogo derivado dos textos do jogo junto ao código público.

## Verificar o catálogo local

Após preparar seus dados locais:

```powershell
python tools/ptbr_catalog.py --require-complete
python -m unittest tools.test_ptbr
```

A auditoria lê a tabela `assets/data/strdataue.dar`, confere todos os IDs não vazios,
códigos de botões, cores, parâmetros, duplicatas e espaço disponível no runtime.
O relatório fica em `.local/ptbr-work/coverage.json`, sem copiar as frases para a saída
do terminal. Nomes próprios, símbolos e termos definidos pelo glossário podem permanecer iguais.

## Limites da cobertura

Cobrir a tabela inteira não comprova traduzir todo texto visível.
O relatório sinaliza expansões de comprimento; ele não mede a largura real dos glifos.
Ajuste de contexto, alinhamento, legendas e espaço nos painéis exige teste visual em jogo.

Antes de alterar uma entrada, preserve uma cópia privada do catálogo anterior.
Nunca substitua IDs, remova códigos de botão ou traduza parâmetros como `[port]` e `[size]`.
Os dados extraídos e os saves devem continuar intactos.
