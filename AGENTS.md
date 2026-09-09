# PortPad project instructions

## Goal

PortPad is a lightweight gamepad-first text editor and file manager
for PortMaster-compatible Linux handhelds.

Initial target:
- R36S / RK3326
- 640x480
- aarch64

The project should eventually work on other PortMaster-supported devices.

## Technology

- C++17
- SDL2
- SDL2_ttf
- CMake
- Ninja
- Linux / PortMaster

Do not use:
- Electron
- Qt
- GTK
- X11-specific APIs
- Python runtime dependencies

## Input

The application must be fully usable without a physical keyboard.

Use native SDL controller input.

The application must provide its own graphical virtual keyboard.

Do not depend on gptokeyb for normal editor text entry.

## Features

v0.1:

- controller navigation
- file browser
- open text files
- edit text
- virtual keyboard
- save
- save as
- undo
- search
- confirmation before destructive actions

Supported text files should include:
- .txt
- .sh
- .cfg
- .ini
- .gptk
- .json
- .xml
- .md

## PortMaster

Avoid hardcoded firmware-specific paths.

Use PortMaster control.txt and environment information where appropriate.

The final package must:
- work offline
- contain all required runtime pieces
- install through PortMaster autoinstall
- target aarch64 first

## Architecture

Keep these concerns separate:
- application state
- editor/text buffer
- file system
- SDL rendering
- controller input
- virtual keyboard
- PortMaster launcher

Prefer small focused classes.

## Development workflow

For every implementation task:

1. Inspect existing code first.
2. Make the smallest coherent change.
3. Build the project.
4. Run tests.
5. Fix errors before finishing.
6. Do not silently change unrelated code.