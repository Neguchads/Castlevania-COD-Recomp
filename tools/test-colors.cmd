@echo off
cd /d "%~dp0.."
findstr /c:"graphics: color conversion self-test passed" out\build\release\logs\castlevania_cod.log
exit /b %errorlevel%
