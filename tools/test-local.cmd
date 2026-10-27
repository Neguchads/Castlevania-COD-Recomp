@echo off
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs
for /f "usebackq tokens=*" %%v in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%v"
if not defined VSDIR goto :novs
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul
if exist "%VSDIR%\VC\Tools\Llvm\x64\bin\clang++.exe" set "PATH=%VSDIR%\VC\Tools\Llvm\x64\bin;%PATH%"
if exist "%ProgramFiles%\LLVM\bin\clang++.exe" set "PATH=%ProgramFiles%\LLVM\bin;%PATH%"
if not defined CLANGXX set "CLANGXX=clang++"
if exist "%CLANGXX%" goto :clangfound
where "%CLANGXX%" >nul 2>nul || goto :noclang
:clangfound
if not exist .local mkdir .local
rem SDK: XBE_SDK_DIR, ou a primeira linha de .local\sdk-path.txt (relativa a esta pasta), ou ..\XboxXBERecomp
set "SDK=..\XboxXBERecomp"
if exist .local\sdk-path.txt set /p SDK=<.local\sdk-path.txt
if defined XBE_SDK_DIR set "SDK=%XBE_SDK_DIR%"
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_translation_pack.cpp -o .local\test_translation_pack.exe
if errorlevel 1 exit /b 1
.local\test_translation_pack.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_shadow_volume.cpp -o .local\test_shadow_volume.exe -ld3d12 -ldxgi -ld3dcompiler
if errorlevel 1 exit /b 1
.local\test_shadow_volume.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_final_copy.cpp -o .local\test_final_copy.exe
if errorlevel 1 exit /b 1
.local\test_final_copy.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_hud_render.cpp -o .local\test_hud_render.exe
if errorlevel 1 exit /b 1
.local\test_hud_render.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_capture_path.cpp -o .local\test_capture_path.exe
if errorlevel 1 exit /b 1
.local\test_capture_path.exe
if errorlevel 1 exit /b 1
set "PYTHONPATH=%CD%\tools;%PYTHONPATH%"
python -m unittest tools.test_extract_xiso tools.test_game_features tools.test_graphics_signature tools.test_pitch_signature tools.test_ptbr tools.test_ps2_ptbr_import tools.test_ps2_movie_dub tools.test_ps2_voice_dub tools.test_ps2_texture_import tools.test_dar_texture tools.test_hd_pack_match tools.test_menu_search_index tools.test_menu_strings tools.test_public_hygiene
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_audio_math.cpp -o .local\test_audio_math.exe
if errorlevel 1 exit /b 1
.local\test_audio_math.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_indexed_up.cpp -o .local\test_indexed_up.exe
if errorlevel 1 exit /b 1
.local\test_indexed_up.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_dev_rules.cpp -o .local\test_dev_rules.exe
if errorlevel 1 exit /b 1
.local\test_dev_rules.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_texture_pack.cpp -o .local\test_texture_pack.exe
if errorlevel 1 exit /b 1
.local\test_texture_pack.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_report_gallery.cpp -o .local\test_report_gallery.exe
if errorlevel 1 exit /b 1
.local\test_report_gallery.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror -I"%SDK%\runtime" tools\test_controller_map.cpp -o .local\test_controller_map.exe
if errorlevel 1 exit /b 1
.local\test_controller_map.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_damage_rules.cpp -o .local\test_damage_rules.exe
if errorlevel 1 exit /b 1
.local\test_damage_rules.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_texture_lookup.cpp -o .local\test_texture_lookup.exe
if errorlevel 1 exit /b 1
.local\test_texture_lookup.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_multisample_rules.cpp -o .local\test_multisample_rules.exe
if errorlevel 1 exit /b 1
.local\test_multisample_rules.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_area_names.cpp -o .local\test_area_names.exe
if errorlevel 1 exit /b 1
.local\test_area_names.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_enemy_probe.cpp -o .local\test_enemy_probe.exe
if errorlevel 1 exit /b 1
.local\test_enemy_probe.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_enemy_editor.cpp -o .local\test_enemy_editor.exe
if errorlevel 1 exit /b 1
.local\test_enemy_editor.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_menu_search.cpp -o .local\test_menu_search.exe
if errorlevel 1 exit /b 1
.local\test_menu_search.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_menu_language.cpp -o .local\test_menu_language.exe
if errorlevel 1 exit /b 1
.local\test_menu_language.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_drop_rules.cpp -o .local\test_drop_rules.exe
if errorlevel 1 exit /b 1
.local\test_drop_rules.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror -I"%SDK%\runtime" tools\test_menu_defaults.cpp -o .local\test_menu_defaults.exe
if errorlevel 1 exit /b 1
.local\test_menu_defaults.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_economy_rules.cpp -o .local\test_economy_rules.exe
if errorlevel 1 exit /b 1
.local\test_economy_rules.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_scaled_clock.cpp -o .local\test_scaled_clock.exe
if errorlevel 1 exit /b 1
.local\test_scaled_clock.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_frame_skip.cpp -o .local\test_frame_skip.exe
if errorlevel 1 exit /b 1
.local\test_frame_skip.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_menu_preferences.cpp -o .local\test_menu_preferences.exe
if errorlevel 1 exit /b 1
.local\test_menu_preferences.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_guest_memory.cpp -o .local\test_guest_memory.exe
if errorlevel 1 exit /b 1
.local\test_guest_memory.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -I"%SDK%\runtime" -I"out\build\release\_deps\imgui-src" tools\test_menu_input.cpp out\build\release\_deps\imgui-src\imgui.cpp out\build\release\_deps\imgui-src\imgui_draw.cpp out\build\release\_deps\imgui-src\imgui_tables.cpp out\build\release\_deps\imgui-src\imgui_widgets.cpp -o .local\test_menu_input.exe
if errorlevel 1 exit /b 1
.local\test_menu_input.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -I"%SDK%\runtime" -I"out\build\release\_deps\imgui-src" tools\test_menu_nav.cpp out\build\release\_deps\imgui-src\imgui.cpp out\build\release\_deps\imgui-src\imgui_draw.cpp out\build\release\_deps\imgui-src\imgui_tables.cpp out\build\release\_deps\imgui-src\imgui_widgets.cpp -o .local\test_menu_nav.exe
if errorlevel 1 exit /b 1
.local\test_menu_nav.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -I"out\build\release\_deps\imgui-src" tools\test_menu_language_ui.cpp out\build\release\_deps\imgui-src\imgui.cpp out\build\release\_deps\imgui-src\imgui_draw.cpp out\build\release\_deps\imgui-src\imgui_tables.cpp out\build\release\_deps\imgui-src\imgui_widgets.cpp -o .local\test_menu_language_ui.exe
if errorlevel 1 exit /b 1
.local\test_menu_language_ui.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_minimap.cpp -o .local\test_minimap.exe
if errorlevel 1 exit /b 1
.local\test_minimap.exe
if errorlevel 1 exit /b 1
"%CLANGXX%" -std=c++20 -Wall -Wextra -Werror tools\test_presentation_math.cpp -o .local\test_presentation_math.exe
if errorlevel 1 exit /b 1
.local\test_presentation_math.exe
if errorlevel 1 exit /b 1
exit /b 0

:novs
echo Visual Studio C++ tools were not found. Install the Desktop development with C++ workload.
exit /b 1

:noclang
echo Clang++ was not found. Install the Visual Studio Clang tools or LLVM and add it to PATH.
exit /b 1
