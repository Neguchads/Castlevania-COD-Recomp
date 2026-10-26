# HUD and cameras

English · [Português (Brasil)](HUD-CAMERA.pt-BR.md)

## HUD, Esc and camera modes (2026-10-24)

- The minimap moved to Game → game options. Same block: show original HUD, scale 0.35–1× (default 0.5×), 50% button, fast hiding and a 0.2–5 s delay. HUD/camera/minimap persistence was exercised in the runtime.
- F1/L3+R3 toggle the tools menu. Esc does not toggle it: it sends a 100 ms Start pulse when modal 10000010 is not active, or B when it is. The gate was checked against the original getter 2F6230 in an arena; open/close in the real menu still awaits human testing. Movement keys in the tools menu stay separate from the character.
- HUD: hooks 3575B0/357610 apply only to children of the root B251F0, with parenting at object+40. Sprite position/scale (userdata 128) is changed temporarily and restored after drawing. The Innocent Devil name is handled in 308E70 and in callers 308F02/308F48 of the text renderers. Scale is not applied to the inventory or the original menu. Full visual coverage is still pending.
- Fast hiding uses 3094E0 mode 1 and the HUD timer at data 4C (originally 5 s). It keeps the 0.3 s fade and the native triggers; it does not invent enemy detection. The original fixture requires flags root+130 = 30h for getter 359600 to return userdata; an earlier test failed because those flags were missing, which was fixed and repeated without loosening the criterion.
- Cameras: Original (default), Cinemática inspired by Lords of Shadow 1, Próxima inspired by Lords of Shadow 2, Personalizada (distance/height/relative FOV), Livre. Simple profiles, not copies of the LoS systems. FOV is not in degrees: it is a relative adjustment of the focal integer at 154. All temporary native parameters are restored after the matrices.
- Livre (free): close the tools menu and use WASD/QE/mouse or the sticks/LT/RT. The character gets no commands; the world keeps running. camera_live only captures controls with view/player active; known title/map/debug states are preserved. There is no collision and no complete cutscene gate; the UI warns that it may change active scenes.
- Build, test-local and the runtime verifier passed. Evidence: logs/build-hud-camera.log, logs/tests-hud-camera.log, logs/runtime-hud-camera.log, logs/runtime-hud-camera-verification.log. Fixtures use the D0010000 stack and isolated memory; HUD scale/restore/hide/menu guard and timer 5→0.6/fade 0.3 passed. Original matrices adjusted/restored and free-camera restore passed. This is not the same as every mode being visually tested in the campaign.
- The game was left open and reached gameplay under human input. A window screenshot showed a scene transition without the HUD and did not confirm the 50% size. The user was invited to test HUD/cameras/Esc. Do not close the process or send input during human testing.


See MODDING.md for controls. Sources: src/game/hud.cpp, src/game/camera.cpp, src/common/presentation_math.h and src/common/dev_runtime.cpp. The preferences only change the port's presentation and controls, not save files.
