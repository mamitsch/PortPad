# Architecture and repository map

Source reviewed 2026-09-11. PortPad uses one synchronous application/event loop,
one browser and one open document. View and Edit share the same implementation.

## Components

| Source/header basename | Responsibility |
| --- | --- |
| `main` | SDL/TTF startup and teardown, command-line smoke flag, event dispatch, SDL text-input activation |
| `AppState` | Main screens, file View/Edit/Cancel dialog, routing and safe close/quit transitions |
| `FileSystem` | Read-only directory snapshot and folder-first sorting |
| `FileBrowser` | Current directory, selection, six-entry pages and selectable parent navigation |
| `TextBuffer` | UTF-32 scalar storage, LF line index, cursor/preferred column, UTF-8 boundary, dirty comparison, bounded undo/redo and search |
| `Utf8` | Strict scalar UTF-8 decoding and encoding |
| `DocumentIO` | Text eligibility, canonical load path, BOM/CRLF conversion, conflict checks, temporary writes, backup and replacement |
| `EditorState` | Document access guard, viewport, navigation, search, prompts, menu, confirmations and exit intent |
| `VirtualKeyboard` | Independent letters/digits/symbols layout, selection, latched/one-shot Shift and key results |
| `TextDialog` | Header-only wrapped error/path modal and scrolling state |
| `InputAction` | Ten device-independent navigation/selection actions |
| `Input`, `Keymap` | SDL controller lifetime/hotplug, action bindings and fixed desktop editor commands |
| `Renderer` | SDL window/render target, browser/main UI and shared text dialogs |
| `EditorRenderer` | Monospaced document, access/dirty markers, viewport, keyboard and editor menus |

Headers are in `include/`, implementations in `src/` (except header-only
InputAction/TextDialog and executable-only main). CMake's `portpad_state` contains
application/browser/document/keyboard state without SDL links. `portpad_ui` links
state and SDL2/SDL2_ttf. `portpad` links the UI target. POSIX file I/O belongs to
DocumentIO, never a renderer or SDL event callback.

`main` first lets Input process events for controller maintenance, then prioritizes
window-close, editor shortcuts and SDL_TEXTINPUT over ordinary InputAction dispatch.
AppState routes an active editor before other screens. EditorState modes are
Navigate, Keyboard, Menu, Prompt, Overwrite, CloseConfirm and EnableEditing.
Prompt owns a separate TextBuffer for Find, Save As or Go to line. Pending close
intent distinguishes closing a document from quitting the application.

The document buffer is exposed as const through EditorState. TextBuffer itself is
a reusable mutable model; **read-only is enforced by EditorState**, including
commands, virtual-keyboard paths and mutation availability. Renderer only reads
state. Successful saves update the saved comparison and original-byte snapshot;
failed saves keep the current in-memory document. There is no worker thread,
background load/save, file watcher, document tab system or persistent undo store.

## Repository structure

```text
CMakeLists.txt                 build targets and six CTest registrations
AGENTS.md                     project development constraints
README.md                     user-facing features and controls
BUILD.md                      maintained build / PortMaster reference
BUILDING.md                   compatibility link to BUILD.md
PORTMASTER_NEXT_STEPS.md       historical superseded packaging plan
include/                      model, state, input and renderer interfaces
src/                          C++ implementation and main.cpp
tests/                        browser, state, input, editor, editor_render tests
assets/fonts/                 DejaVuSans.ttf, DejaVuSansMono.ttf, LICENSE-DejaVu.txt
config/keymap.ini             shipped ten-action binding template
portmaster/                   PortPad.sh, port.json, gameinfo.xml, concise README.md
scripts/                      build/stage/export/package/validate/list/launcher tests
scripts/capture-portmaster-screenshot.cpp   development browser-capture harness
docs/ARCHITECTURE.md           this document
docs/CURRENT_STATE.md          current state and dated verification
docs/portmaster-review.md      preserved historical review
LICENSE                       PortPad MIT license
```

Generated, ignored output includes `build/`, `build-desktop-check/`,
`build-portmaster/` and `dist/`. Build trees contain their own copied assets/config.
`dist/portmaster-source/port/portpad/` is the generated submission source;
`dist/portpad.zip` is the installable release. The official checkout/tools remain
outside this repository. [BUILD.md](../BUILD.md) documents the two different layouts.

## Known implementation issues (not corrected in this documentation task)

- `EditorState::menuAvailable(0)` disables Save for a clean document, but the
  writable Ctrl+S command calls saveTo anyway and can overwrite the previous backup.
- Fixed editor shortcuts take precedence over INI actions; printable remapped
  Confirm/Menu bindings are suppressed during text entry. Some editor hints are
  literal strings rather than binding-derived labels. Default bindings work.
- `EditorCommand::Tab` ignores insertion failure at the buffer limit, whereas
  normal text and newline insertion show a notice. No data is inserted beyond
  the limit, but that particular failure can be silent.
- `FileBrowser::handle` retains the old “file opening is not available yet” message.
  AppState intercepts file confirmation first, so normal UI opening is implemented;
  a caller using FileBrowser alone still sees the legacy message.
- Directory enumeration/file I/O is synchronous and text textures are rebuilt
  every frame. Rendering also waits 16 ms in addition to requested vsync. Profile
  on RK3326 before deciding on a performance change.
- Path/dialog layout counts code points, not glyph widths. Some non-UTF-8 names,
  combining sequences or long binding labels can display incorrectly or clip.
- Save checks are not a locking protocol. The final external-change check and
  rename are separate; directory fsync can fail after replacement. New-file
  creation needs kernel no-replace rename or filesystem hard-link support.
- Existing launcher error branches and release reproducibility gaps are documented
  in [BUILD.md](../BUILD.md); these were not changed by this audit.

These findings require an explicitly approved code follow-up, not silent changes
as part of documentation maintenance.
