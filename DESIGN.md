# Curse of Darkness tools menu

English · [Português (Brasil)](DESIGN.pt-BR.md)

A native in-game operating surface that refines the existing tabs. Visual reference: the gold Castlevania logo, the red subtitle and the dark backgrounds seen in the USA game. Do not change the game's rendering when applying the theme.

- Near-black background with red; selection in deep red, action/focus in gold, ivory text and beige secondary text.
- Local Segoe UI fonts for options and Georgia for the title; atlas with oversampling, without installing or redistributing Windows fonts.
- Scale to the available area, starting from 1280×800; support the existing 960×540 window and larger windows.
- Header with title, FPS and factor; side navigation; content with its own scrolling; footer with save/back and the controller combination.
- F1 and L3 + R3 toggle the tools. One activation per combination; both buttons must be released before the next one.
- The d-pad navigates; the sticks stay with the game; A/cross confirms, B/circle goes back; L1/R1 switch tabs. Visible gold focus.
- Do not require new services or dependencies. The menu keeps the world running: the sticks move the character/camera; d-pad and A/B/L1/R1 stay in the interface. Keyboard and mouse only operate the panel while it is open.

- Esc stays with the original menu (Start/B depending on the modal state); it does not toggle the tools menu.
- Game holds the minimap, HUD and cameras. HUD default 50%, visibility toggle and hide delay; experimental cameras with instructions and an Original restore.
