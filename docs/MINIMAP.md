# Native minimap

English · [Português (Brasil)](MINIMAP.pt-BR.md)

## Automatic native minimap (2026-10-24)

- Delivered a minimap on by default: top-right corner, 260x150 px at 1080p, transparent background, blue rooms and a gold marker centered on the player. Settings in Imagem: visibility/size/opacity, saved in settings.toml. It uses a draw list without capturing input and keeps working with the menu closed.
- Real source: stmap_*.dar files through kernel::ReadGameFile, models 3/strips of 36 bytes with their own matrix; header 55BA00 for groups/rooms and the original transform 3464B0. DAR offsets are relative to the entry. The geometry does not get the 0.08 factor; only the cursor world position does. Do not invent rectangles or connections.
- Safety: cached position at player+D0/D8, pointer B41394, state 2 and flags EE&18; no mutating getter is called from the UI. The title screen without a player hides the overlay. Some cutscenes with an active player may keep it. Geometry and edges are cached; files are not read every frame.
- Verification: compile-local.cmd, test-local.cmd and verify-dev-runtime.ps1 passed. Evidence: logs/build-minimap.log, logs/tests-minimap.log, logs/runtime-minimap-verification.log and logs/runtime-minimap.log. The real-asset test covers 240 models/39316 triangles, matrix/rotation and invalid input. The runtime loaded 253 unique models, including common resources.
- SEEN IN GAMEPLAY in a window screenshot: character in a corridor, HUD HP 122/122, minimap visible at the top right with the marker and a transparent background. The user was already controlling the game; the automation sent no commands and loaded no save. Comparing movement and points across all rooms is still pending.
- Limits: shows the geometry of neighboring rooms regardless of discovery. No door/save/teleport icons, special colors or floor indicator. Some inner model lines still show. Do not claim the whole campaign or map is validated.
- The game was left open under the user's control. Do not close it without checking the state and getting permission. The F1/L3+R3 shortcut and the earlier controls are preserved.

## Where the structures come from

Header/map: generated/game/x86_recomp.101.cpp, sub 345400 and 345760/3457E0. Room transform: sub 345A50. Cursor: sub 3464B0. Player: sub 333870, sub 333230 and FB8F0; cached position returned by 35A8B0/3700F0. These paths were checked in the recompiled code and in the native files, without live Ghidra (backend unavailable).

DAR: magic 82734927, count at +10h, entry at 20h+i*16. Entry u32 id/type/offset/size; the resource starts at entry+offset. Model FFFF0003: +8 matrix, +20h mesh count, +24h mesh start, +2Ch length. Mesh +0 next/+8 packet. Packet +C vertex count/+1C index count/+2C vertex offset. Vertex of 36 bytes with XYZ floats; uint16 strip indices after the vertices, aligned to 4. Row-vector matrix, then the room rotation/translation.

Models are read only in the first game session; rooms are reprojected when the stage changes. This local code needs the user's own assets and is not a package for distribution.
