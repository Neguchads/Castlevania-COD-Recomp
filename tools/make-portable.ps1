<#
Monta uma pasta pronta para jogar: exe, dll, jogo (assets), mods, saves e opcoes.
Padrao: "Castlevania Curse of Darkness Recomp" na area de trabalho. So clicar em CastlevaniaCOD.exe.

Rodar de novo atualiza exe, dll, assets e mods. Saves (hdd) so sao copiados na primeira montagem e
opcoes (settings.toml) so quando faltam: o que voce jogou ou apagou na pasta pronta fica assim.
A pasta tem os arquivos do seu jogo: nao compartilhe.

Prefira .\tools\release.ps1, que so chama este script depois do build e dos testes passarem.
Uso (PowerShell, na pasta do projeto):  .\tools\make-portable.ps1 [-Destination <pasta>] [-DryRun]
#>
param(
    [string]$Project = (Split-Path $PSScriptRoot -Parent),
    [string]$Destination = (Join-Path ([Environment]::GetFolderPath('Desktop')) 'Castlevania Curse of Darkness Recomp'),
    [switch]$DryRun
)
$ErrorActionPreference = 'Stop'
$release = Join-Path $Project 'out\build\release'
$assets = Join-Path $Project 'assets'

# O exe procura "assets" ao lado dele; a dll tem o codigo do jogo e precisa ficar junto.
foreach ($required in @((Join-Path $release 'CastlevaniaCOD.exe'), (Join-Path $release 'CastlevaniaCOD.dll'), (Join-Path $assets 'default.xbe'))) {
    if (-not (Test-Path $required)) { throw "Falta $required. Rode o build (e confira a pasta assets) antes." }
}
# Sem a pasta compiler, o SDK so usa a dll se ela for mais nova que o exe (game_code.cpp, Current); senao tenta recompilar.
function Test-DllCurrent([string]$folder) {
    (Get-Item (Join-Path $folder 'CastlevaniaCOD.dll')).LastWriteTime -gt (Get-Item (Join-Path $folder 'CastlevaniaCOD.exe')).LastWriteTime
}
if (-not (Test-DllCurrent $release)) { throw 'A dll esta mais velha que o exe no build: rode o build de novo antes.' }
if (Get-Process -Name 'CastlevaniaCOD' -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($Destination) }) {
    throw "O jogo esta aberto a partir de $Destination. Feche antes de atualizar."
}

function Measure-Folder([string]$path) {
    if (-not (Test-Path $path)) { return 0 }
    (Get-ChildItem $path -Recurse -File -Force -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum
}
$needed = (Measure-Folder $assets) + (Measure-Folder (Join-Path $release 'mods')) + (Measure-Folder (Join-Path $release 'hdd')) + 40MB
$drive = Get-PSDrive ((Split-Path -Qualifier $Destination).TrimEnd(':'))
'Destino: {0}' -f $Destination
'Tamanho: {0:N1} GB | livre no disco: {1:N1} GB' -f ($needed / 1GB), ($drive.Free / 1GB)
if ($drive.Free -lt $needed) { throw 'Espaco livre insuficiente no disco de destino.' }
if ($DryRun) { 'DryRun: nada copiado.'; return }

# robocopy: codigos abaixo de 8 sao sucesso (0 = nada a copiar, 1 = copiou, 2/3 = extras).
function Copy-Tree([string]$from, [string]$to, [string[]]$extra = @()) {
    $options = @($from, $to, '/E', '/R:1', '/W:1', '/MT:8', '/NFL', '/NDL', '/NJH', '/NJS', '/NP') + $extra
    & robocopy @options | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy falhou ($LASTEXITCODE): $from -> $to" }
}
New-Item -ItemType Directory -Force $Destination | Out-Null

'Copiando exe e dll...'
& robocopy $release $Destination 'CastlevaniaCOD.exe' 'CastlevaniaCOD.dll' 'CastlevaniaCOD.pdb' 'MODDING.md' 'MODDING.pt-BR.md' 'cheats.example.toml' /R:1 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy falhou ($LASTEXITCODE) no exe" }
# Guia do jogador: GUIA.md em portugues, GUIDE.md em ingles.
foreach ($pair in @(@('docs\GUIDE.pt-BR.md', 'GUIA.md'), @('docs\GUIDE.md', 'GUIDE.md'))) {
    $guide = Join-Path $Project $pair[0]
    if (Test-Path $guide) { Copy-Item $guide (Join-Path $Destination $pair[1]) }
}
if (-not (Test-DllCurrent $Destination)) { throw 'A copia perdeu a data da dll; o jogo tentaria recompilar. Rode de novo.' }
'Copiando o jogo (assets)...'
Copy-Tree $assets (Join-Path $Destination 'assets') @('/XF', '*.private.h')
'Copiando mods (HD4K, PT-BR...)...'
Copy-Tree (Join-Path $release 'mods') (Join-Path $Destination 'mods')
# Saves: so na primeira montagem. Depois a pasta hdd e da pasta pronta (jogo novo, saves apagados de proposito).
$saves = Join-Path $Destination 'hdd'
if (-not (Test-Path $saves)) {
    'Copiando saves do projeto (primeira montagem)...'
    Copy-Tree (Join-Path $release 'hdd') $saves
} else {
    'Saves da pasta pronta mantidos como estao.'
}
foreach ($name in @('settings.toml', 'cheats.toml')) {
    $from = Join-Path $release $name; $to = Join-Path $Destination $name
    if ((Test-Path $from) -and -not (Test-Path $to)) { Copy-Item $from $to }
}

$readme = @'
Castlevania: Curse of Darkness Recomp
Para jogar: clique duas vezes em CastlevaniaCOD.exe.
Menu de ferramentas: L3 + R3 no controle ou F1 no teclado.
Guia completo (atalhos, opcoes, mods, cheats): GUIA.md, abre no Bloco de Notas.
English guide: GUIDE.md.
Saves ficam em hdd\E\UDATA; opcoes em settings.toml. Para comecar do zero, apague a pasta
do save dentro de hdd\E\UDATA\4b4e002d (a que tem C2Save.dat).
Pastas: assets = arquivos do jogo, mods = texturas HD e traducao PT-BR.
Se o Windows reclamar de MSVCP140.dll ou VCRUNTIME140.dll, instale o
"Microsoft Visual C++ Redistributable 2015-2022 x64".
Para levar a outro HD ou PC: copie a pasta inteira (o Explorador mantem as datas).
A dll precisa continuar mais nova que o exe; se o jogo falar em "compiler folder",
a copia perdeu as datas: copie de novo a partir da pasta original.
Esta pasta tem os arquivos do seu jogo: nao compartilhe.
'@
Set-Content -Path (Join-Path $Destination 'LEIA-ME.txt') -Value $readme -Encoding utf8

$vc = (Test-Path "$env:WINDIR\System32\msvcp140.dll") -and (Test-Path "$env:WINDIR\System32\vcruntime140_1.dll")
if (-not $vc) { 'AVISO: Visual C++ Redistributable 2015-2022 x64 nao encontrado; instale antes de jogar.' }
'Pronto: {0:N1} GB em {1}' -f ((Measure-Folder $Destination) / 1GB), $Destination
'Para jogar: abra a pasta e clique em CastlevaniaCOD.exe.'
exit 0   # o robocopy deixa 1 ("copiou") em $LASTEXITCODE; aqui chegar e sucesso
