# Castlevania: Curse of Darkness Recomp

Port experimental para Windows baseado no XboxXBERecomp. O executável hospedeiro adapta serviços do Xbox ao Windows; o código recompilado e os arquivos do jogo são preparados localmente a partir de uma imagem USA compatível fornecida pelo usuário.

## O projeto inclui

- Menu de desenvolvimento com configurações de vídeo, áudio, controles, minimapa, câmera, cheats e relatórios locais.
- Hooks experimentais para HUD, velocidade, localização no mapa e seleção de idioma.
- Código-fonte do runtime, documentação e testes focados. A configuração gerada pelo XBE é criada localmente e não fica versionada.

## Não inclui

Este repositório não distribui ISO/XBE, assets extraídos, código C++ gerado da imagem do jogo, DLL recompilada, saves ou capturas. O catálogo PT-BR derivado dos textos do jogo também fica fora. Forneça sua própria edição Xbox USA; o runtime valida o XBE e prepara a recompilação localmente.

## Desenvolvimento

Coloque `default.xbe` e os arquivos extraídos da sua cópia em `assets/`, mantenha o checkout `XboxXBERecomp` ao lado deste projeto e execute `build.bat`. O SDK usado como fallback fica fixado no commit `e0de3e44d10443c1c9aad512f36baf133cdd4a60`. O primeiro build gera localmente a configuração e o código recompilado. Os arquivos do jogo e o catálogo privado de tradução ficam ignorados pelo Git. Os scripts Python usam somente a biblioteca padrão.

Depois do primeiro build, execute `tools/test-local.cmd` para rodar os testes Python e C++.

O port é experimental e a campanha inteira não foi validada. Permanecem pendentes as faixas horizontais de textura, a tradução completa e a redução da seta de localização para salas de save. Veja [MODDING.md](MODDING.md).

Ainda não foi escolhida uma licença para as alterações deste projeto. Tornar o repositório público permite visualizar o código, mas não concede automaticamente permissão de reutilização. Partes do runtime derivadas de XboxXBERecomp seguem o aviso MIT em [LICENSES/XboxXBERecomp-MIT.txt](LICENSES/XboxXBERecomp-MIT.txt); o SDK e as demais dependências mantêm suas licenças próprias.
