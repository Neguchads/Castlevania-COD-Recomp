param([string]$ReleaseDir = (Join-Path $PSScriptRoot '../out/build/release'), [switch]$RequireControllerShortcut)
$ErrorActionPreference = 'Stop'
$release = (Resolve-Path -LiteralPath $ReleaseDir).Path
$runtime = Get-Content -LiteralPath (Join-Path $release 'logs/castlevania_cod.log') -Raw
$required = @(
    'graphics: color conversion self-test passed',
    'dev selftest: settings roundtrip=1',
    'dev selftest: user profile save/serialization/no recursion=1',
    'dev selftest: real game file overlay=1 path guard=1',
    'dev selftest: file overlay write/delete guard=1',
    'dev selftest: allocated-memory cheat apply/revert=1 expected-value guard=1',
    'dev selftest: godmode original damage/heal/other-actor/off=1',
    'dev selftest: godmode mapped original off=90 on=100 heal=110 other=90 id0=100 passed=1',
    'dev selftest: godmode absolute off=90 on=100 heal=110 other=90 passed=1',
    'dev selftest: godmode virtual-stack=D0010000 passed=1',
    'dev selftest: HUD hook scale/restore/hide/menu-guard=1 timer=5.0/0.6 fade=0.3',
    'dev selftest: pet scratch sprite scale/restore/hide=1',
    'dev selftest: camera native matrices/restore=1 freecam/restore=1',
    'dev selftest: orbit native collision=1 hit=1 correctionZ=-52.00 miss=0',
    'dev selftest: orbit player pose native getter/negative height=1',
    'dev selftest: orbit mode5 native matrices/cardinal target/height=1',
    'dev selftest: native menu original layout preserved=1',
    'dev selftest: white indicator 20pct scale/position/source/caller guard=1',
    'dev selftest: minimap native room type/exploration getters=1',
    'dev selftest: PTBR complete catalog allocation/token-safe lookup=1',
    'dev selftest: native menu live masks/restore/caller-guard=1',
    'dev selftest: native character selection passed=1',
    'dev selftest: speed 1.0x clocks=1 wait=1 timer=1',
    'dev selftest: speed 2.0x clocks=1 wait=1 timer=1',
    'dev selftest: speed 0.5x clocks=1 wait=1 timer=1',
    'dev selftest: speed runtime=1 armed-timer rate-change=1',
    'dev selftest: screenshot=1 report=1'
)
foreach ($proof in $required) {
    if (!$runtime.Contains($proof)) { throw "Falta evidencia na execucao: $proof" }
}
if ($RequireControllerShortcut -and !$runtime.Contains('dev: menu toggled by Options/Start + R3')) {
    throw 'Nenhuma ativacao real do atalho no controle foi registrada nesta execucao.'
}
$report = Get-ChildItem -LiteralPath (Join-Path $release 'reports') -Directory |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (!$report) { throw 'Nenhum relatorio encontrado.' }
foreach ($name in @('report.txt', 'runtime.log', 'screenshot.bmp')) {
    $file = Get-Item -LiteralPath (Join-Path $report.FullName $name)
    if ($file.Length -eq 0) { throw "Arquivo de relatorio vazio: $name" }
}
$image = [IO.File]::ReadAllBytes((Join-Path $report.FullName 'screenshot.bmp'))
if ($image.Length -lt 54 -or $image[0] -ne 66 -or $image[1] -ne 77) { throw 'Captura BMP invalida.' }
$width = [BitConverter]::ToInt32($image, 18)
$height = [Math]::Abs([BitConverter]::ToInt32($image, 22))
if ($width -le 0 -or $height -le 0) { throw 'Captura sem dimensoes validas.' }
Write-Output "PASSOU: Godmode original, speedhack/relogios/timers, opcoes, mods, cores e relato com captura ${width}x${height}."
if ($RequireControllerShortcut) { Write-Output 'PASSOU: Options/Start + R3 acionou o menu nesta execucao.' }
