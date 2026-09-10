# PortPad

## Notes

Gamepad-first text editor and file manager for PortMaster-compatible Linux handhelds.

Current prototype status:
- file browser with View / Edit selection
- native SDL controller input
- text editing, virtual keyboard, undo/redo, search and safe saving

Controls:
- D-pad: navigate
- A: confirm / open text input
- B: back; keyboard backspace
- Start: menu
- L/R: page; keyboard Shift / symbols
- Y: browser full path / editor Find Next
- Start then Y: editor full path

View is read-only. Use Start editing and confirm Yes to enable changes.
Set read-only retains unsaved edits; Close file offers Save / Discard / Cancel.

Bindings are stored in `config/keymap.ini`.

Thanks to the SDL, DejaVu, and PortMaster contributors.
PortMaster packaging by mamitsch.
