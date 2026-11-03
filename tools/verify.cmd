@echo off
rem Portao local antes de commit/PR: build incremental, testes C++ e Python, varredura de segredos.
rem Precisa de um build ja configurado (build.bat uma vez) e do clang/cmake no PATH.
setlocal
cd /d "%~dp0.."
if not exist out\build\release\build.ninja (
    echo verify: rode build.bat uma vez antes ^(out\build\release nao configurado^).
    exit /b 1
)
echo [1/3] build
cmake --build out\build\release || exit /b 1
echo [2/3] testes
call tools\test-local.cmd || exit /b 1
echo [3/3] segredos
where gitleaks >nul 2>nul || (
    echo verify: gitleaks nao encontrado; instale com "winget install Gitleaks.Gitleaks".
    exit /b 1
)
gitleaks git --no-banner --redact . || exit /b 1
gitleaks dir --no-banner --redact src tools || exit /b 1
echo verify: tudo passou.
exit /b 0
