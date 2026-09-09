# PortPad
PortPad is a controller-first text editor and file manager for
PortMaster-compatible handheld Linux devices like the R36S.

Initial target: R36S / RK3326 / 640x480.

## Read-only file browser milestone

M1 provides a 640x480 SDL2 window, SDL2_ttf text, controller detection and
hotplug support, and four menu entries: File Browser, Text Editor, About,
and Quit. File Browser now browses directories; Text Editor is still a placeholder.
Editing, file opening, the virtual keyboard, and PortMaster packaging are future work.

The browser starts in the process working directory and shows its current path.
Directories appear before files, with case-sensitive filename sorting within each
group. Hidden entries are included; directory symlinks can be entered. Up/Down
wrap the selection and scroll through six visible rows. Confirm enters the selected
directory; confirming a file only displays a read-only notice. Select the first
entry, `..`, and Confirm to open the parent and restore selection on the child.
The parent entry is omitted at filesystem root. Back and Menu return to the main
menu; neither navigates to the parent. Directory errors appear as a message while
preserving the last successful listing. No files are modified.

Page Up/Down (controller left/right shoulder) move six entries and stop at the
first or last entry. Press P (controller Y) to show the full current path in a
modal dialog occupying the left two-thirds of the screen. Text wraps without
splitting UTF-8 characters; Up/Down scroll one line and Page Up/Down scroll ten
lines. Escape/B or Enter/A closes it; Tab/Start also closes the dialog without
changing the underlying screen. Browser navigation is suspended while it is open.
Long names and messages in the browser remain clipped to the available width.

The application works fully with a PC keyboard; no gamepad is required.
The first available SDL game controller is used and its name is displayed.
If it disconnects, another available controller is selected automatically.
Devices must be recognized by SDL's GameController mappings; additional mappings
can be supplied through SDL_GAMECONTROLLERCONFIG.

| Action | Controller | Desktop |
| --- | --- | --- |
| Previous item (wraps) | D-pad up/left | Up/left arrows |
| Next item (wraps) | D-pad down/right | Down/right arrows |
| Confirm | A | Enter |
| Back | B | Escape |
| Menu | Start | Tab |
| Page up/down | Left/right shoulder | Page Up/Page Down |
| Full path dialog | Y | P |
| Exit | Select Quit from menu | Select Quit or close window |

Menu and Back return to the main menu and preserve menu selection. On the main menu
both leave it open. Escape does not exit the application.

## Build and run on Ubuntu / WSL

Requires a C++17 compiler, CMake 3.16+, Ninja, pkg-config, SDL2 2.0.14+,
and SDL2_ttf development packages. On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build pkg-config libsdl2-dev libsdl2-ttf-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/portpad
```

On WSL, run these commands inside Ubuntu with WSLg available for the graphical
window. Without a graphical display, use the headless smoke test below.
To manually check keyboard operation: move with all four arrows, press Enter
to enter a directory in File Browser, select `..` to go to its parent, and Tab to return to the menu from
another screen. Select Quit and press Enter to exit.

CMake copies the bundled DejaVu Sans font and its license into `build/assets`.
Keep that directory beside the executable when moving it; assets are resolved
relative to the executable, so launching from a different working directory works.
Building and running require no font downloads or Python runtime.

Tests cover directory sorting, empty/missing/inaccessible directories, browser
navigation, non-SDL application state, keyboard-only navigation, SDL input with
virtual controllers (no physical controller needed), and menu/browser rendering
using SDL's dummy video driver. The permission-denied check is skipped when the
test process has privileges to read mode-000 directories.
For a manual headless startup check:

```sh
SDL_VIDEODRIVER=dummy ./build/portpad --smoke-test
```

`InputAction` defines navigation, Confirm, Back, Menu, PageUp, PageDown, and Details without SDL
dependencies. Unmapped events produce no action; window close is handled as a
separate lifecycle event. `AppState` owns menu state and transitions, `Input` owns SDL controller lifetime
and translates events into actions, and `Renderer` owns the window and text
rendering. `main.cpp` initializes SDL and runs the event loop.
`FileSystem` provides read-only directory listing with error reporting; `FileBrowser`
owns the current path, entries, selection, and navigation. Both are SDL-independent.
`TextDialog` provides reusable SDL-independent wrapped text and modal scroll state.
Listings are synchronous snapshots, refreshed on directory entry or reopening the
browser; very large or slow directories may pause the UI during enumeration.

The initial handheld target remains aarch64 R36S/RK3326. This milestone builds
for the host architecture; an offline PortMaster runtime bundle and launcher
are not included yet. Hardware validation should check D-pad/A/B navigation,
the displayed controller name, disconnect/reconnect, and readability at 640x480.

## Configurable bindings

Edit `build/config/keymap.ini` and restart to change the running build's controls.
`config/keymap.ini` is the source template used by CMake only when the build's
runtime config does not yet exist. Reconfiguration preserves your edits.
The default runtime path is `config/keymap.ini` beside the executable, independent
of the working directory. To keep personal settings outside the build directory:

```sh
PORTPAD_KEYMAP=/path/to/keymap.ini ./build/portpad
```

The INI file has `[keyboard]` and `[controller]` sections. Each maps all ten action
names to one unique input per section, for example `Confirm=Space` or `Confirm=x`.
Keyboard values use SDL key names; controller values use native SDL button names
such as `a`, `b`, `dpup`, `start`, and `leftshoulder`. Blank lines and whole-line
`#` or `;` comments are supported. Duplicate, unknown, incomplete, or unreadable
configurations produce a console warning and use the complete built-in defaults.
UTF-8 BOM and Windows CRLF line endings are accepted.
The app never writes the config. Visible control hints follow the loaded bindings,
showing keyboard names when no controller is connected and controller names otherwise.

A future PortMaster launcher can export `PORTPAD_KEYMAP` to a file in its portable
port directory. This is PortPad's own format, not gptokeyb's `.gptk` format;
controller input remains native SDL. There is no parent-directory action or binding:
parent navigation always uses the selectable `..` entry.

See [the PortMaster readiness review](docs/portmaster-review.md) for verified
behavior, remaining release requirements, and technical debt.
