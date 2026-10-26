<#
Gera a pasta de jogar (release) so depois do build e dos testes passarem.

Ordem: build (cmake) -> testes (tools\test-local.cmd) -> make-portable.ps1 -> atalho na area de trabalho.
Se o build ou um teste falhar, para antes de tocar na pasta de jogar.

Destino: -Destination, ou o ultimo usado (guardado em .local\release-destination.txt), ou a area de
trabalho. Pastas extras de ferramentas (ex.: o LLVM) vao uma por linha em .local\tool-path.txt.
Os dois arquivos ficam em .local, fora do git, porque sao caminhos desta maquina.

Uso (PowerShell, na pasta do projeto):  .\tools\release.ps1 [-Destination <pasta>] [-NoShortcut]
#>
param(
    [string]$Project = (Split-Path $PSScriptRoot -Parent),
    [string]$Destination,
    [switch]$NoShortcut
)
$ErrorActionPreference = 'Stop'
$local = Join-Path $Project '.local'
$remembered = Join-Path $local 'release-destination.txt'
if (-not $Destination -and (Test-Path $remembered)) { $Destination = (Get-Content $remembered -TotalCount 1).Trim() }
if (-not $Destination) { $Destination = Join-Path ([Environment]::GetFolderPath('Desktop')) 'Castlevania Curse of Darkness Recomp' }

# ferramentas do projeto (cmake, texconv) e as pastas extras desta maquina na frente do PATH
$tools = @((Join-Path $local 'build-tools\cmake\data\bin'), (Join-Path $local 'build-tools\bin'))
$extra = Join-Path $local 'tool-path.txt'
if (Test-Path $extra) { $tools = @(Get-Content $extra | Where-Object { $_.Trim() }) + $tools }
$env:PATH = (@($tools | Where-Object { Test-Path $_ }) + @($env:PATH)) -join ';'

function Invoke-Step([string]$name, [scriptblock]$step) {
    "== $name"
    & $step
    if ($LASTEXITCODE -ne 0) { throw "$name falhou (exit $LASTEXITCODE). A pasta de jogar nao foi alterada." }
}

Push-Location $Project
try {
    Invoke-Step 'Build' { cmake --build out\build\release 2>&1 | Select-String -Pattern 'error|FAILED' | Select-Object -First 20 }
    Invoke-Step 'Testes' { cmd /c tools\test-local.cmd 2>&1 | Select-String -Pattern 'FAIL|failed|error|Ran |^OK' | Select-Object -Last 10 }
} finally { Pop-Location }

"== Pasta de jogar: $Destination"
& (Join-Path $PSScriptRoot 'make-portable.ps1') -Project $Project -Destination $Destination
New-Item -ItemType Directory -Force $local | Out-Null
Set-Content -Path $remembered -Value $Destination -Encoding utf8

$desktop = [Environment]::GetFolderPath('Desktop')
if (-not $NoShortcut -and -not $Destination.StartsWith($desktop, [StringComparison]::OrdinalIgnoreCase)) {
    # atalho com o icone do proprio exe; trabalha na pasta de jogar para o exe achar assets e saves
    $shell = New-Object -ComObject WScript.Shell
    $link = $shell.CreateShortcut((Join-Path $desktop 'Castlevania Curse of Darkness Recomp.lnk'))
    $link.TargetPath = Join-Path $Destination 'CastlevaniaCOD.exe'
    $link.WorkingDirectory = $Destination
    $link.IconLocation = (Join-Path $Destination 'CastlevaniaCOD.exe') + ',0'
    $link.Save()
    'Atalho na area de trabalho: Castlevania Curse of Darkness Recomp'
}
'Release pronta.'
exit 0
