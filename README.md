# PortPad

PortPad 0.1.0 is a gamepad-first text editor and file browser for Linux handhelds.
The initial target is R36S / RK3326, aarch64, at 640×480. It uses C++17, SDL2 and
SDL2_ttf, works offline, and supplies its own graphical keyboard. No physical
keyboard, gptokeyb text input, Qt, GTK, Electron or Python runtime is required.

Documentation reflects the source reviewed on **2026-09-11**:
[build and PortMaster reference](BUILD.md), [architecture](docs/ARCHITECTURE.md),
[current status and validation](docs/CURRENT_STATE.md).

## Menus and file browser

The main menu contains **File Browser**, **Text Editor**, **About**, **Quit**.
The Text Editor entry only instructs you to open a file from File Browser; there
is no New document action or command-line filename argument. About is a short
project description. B/Start leaves a detail screen for the main menu; on the
main menu those buttons do not quit.

The browser starts in the process working directory (the installed `portpad/`
directory when launched through PortMaster). It lists hidden entries and sorts
folders first, then files, by case-sensitive filename. Directory symlinks can be
entered; broken links remain listed as files. Up/Down wraps the selection;
L/R moves six entries, clamping at either end. Six entries are visible per page.
Left/Right has no browser action. Select `..` to enter the parent directory and
restore selection on the child; filesystem root has no parent entry. B/Start
returns to the main menu, not the parent directory.

Directory errors preserve the last successful listing. Listings refresh on
opening/reopening a directory, not continuously or after Save As. Y shows the
full browser directory path. That dialog wraps at 16 code points and shows ten
lines; Up/Down scrolls one line, L/R ten lines, A/B/Start closes it.

Selecting valid supported text offers **View / Edit / Cancel**, with View selected
initially. Unsupported extensions, binary/control bytes, invalid encoding,
unreadable or oversized files produce an error instead of an Edit action.
Directory navigation is unchanged by this dialog. File contents are checked
before showing it and loaded again on View/Edit. There is no metadata viewer,
file copy/move/rename/delete operation, folder creation or script execution.

## Viewer and editor

Supported extensions (case-insensitive): `.txt`, `.sh`, `.cfg`, `.ini`, `.gptk`,
`.json`, `.xml`, `.md`. The same document/editor implementation handles View and
Edit. Text is strict UTF-8, with optional BOM, LF or consistent CRLF endings.
The buffer uses LF internally and saves using the loaded BOM/ending convention.
No final newline is automatically added; editing can explicitly add/remove one.

The screen shows a shortened path (full path through Start then Y), line numbers,
cursor, line/column and viewport position, encoding/ending style and modified
marker. DejaVu Sans Mono renders the body. Tabs display at four-column stops.
Long lines scroll horizontally without wrapping; 17 lines are visible normally,
eight above the keyboard. Left/Right moves one code point and stops at each line
boundary. Up/Down preserves the preferred character column over shorter/empty
lines; L/R moves 17 lines. Empty files have one editable empty line.

Editing supports insertion, Enter to split a line, Backspace/Delete to remove a
code point or join adjacent lines, and undo/redo. Undo history compares the current
text with the last successfully saved text to determine the modified marker.
A new edit clears redo history. History is not saved between sessions.

**Find** is case-sensitive literal search, highlights the match and wraps at the
end. Y performs Find Next, or opens Find if no query exists. There is no Find Next
menu entry. **Go to line** accepts digits and rejects empty, zero, overflowing or
out-of-range numbers; valid lines are 1 through the current line count.

The editor menu contains Save, Save As, Undo, Redo, Find, Go to line, Close file,
Quit PortPad, and Start editing / Set read-only. Unavailable actions stay visible
but disabled. Closing returns to the existing browser listing.

## Read-only behavior

View displays **READ ONLY**. Navigation, scrolling, Find, Find Next, Go to line
and full-path display remain available; text mutations and undo/redo are blocked
in `EditorState`, not only hidden by the renderer. A shows an explanatory notice.
Search/line-number prompts may use the keyboard without modifying the document.

**Start editing** opens “Switch to edit mode? This will allow changes to this
file.” Only **Yes** enables editing; **No** is the default. No controller shortcut
bypasses this confirmation. Edit from the browser opens writable immediately.

**Set read-only** takes effect immediately and retains any edits, history and
modified marker. A dirty read-only document can Save, Save As or Close file and
Discard. A clean read-only document cannot Save or Save As. Resuming edits still
requires Yes. This mode is an application guard; it does not change filesystem
permissions or make protected files writable.

## Saving, backups and errors

Save As starts with the current filename. Relative paths resolve against the
current document's folder; absolute paths are accepted. The destination parent
must already exist, and the filename must have a supported extension. Existing
destinations require Overwrite / Cancel confirmation, with Cancel selected.
Successful Save As makes that path the current document.

Saving writes a `.portpad-XXXXXX` temporary file in the destination directory,
flushes it with `fsync`, closes it, then atomically replaces the destination.
Before overwriting, one `filename.ext.bak` is prepared and replaces the previous
backup. New files have no backup; `.bak` files are not supported editor inputs or
save destinations, so no `.bak.bak` chain is created. Backups are always enabled.
Ordinary permission bits are preserved where supported; new files request mode
0600. FAT/exFAT may use fixed mount permissions. Ownership, ACLs, extended
attributes and special permission bits are not copied.

Opening resolves symlinks to a canonical file path. Save As rejects a symlink
as the destination itself. Saves reject nonregular files, existing destinations
with no write bits, hard-linked destinations and detected external changes.
Use another Save As path to recover. Changes are checked before replacement, but
there is no file locking or complete protection from simultaneous external writes.

Save errors keep the in-memory text and its previous dirty state. A directory
flush failure can be reported **after** the destination has been replaced: the
buffer remains dirty and storage should be checked before retrying. Temporary
files are removed on ordinary return paths, not guaranteed after a crash/power loss.
Close file, B and Quit (including window close) ask Save / Discard / Cancel for
modified text, with Cancel selected. Discard closes without writing; it is not a
separate menu action. There is no autosave or crash recovery.

## Controls

Defaults below use native SDL GameController button names; device labels depend
on its SDL mapping. No analog-stick navigation or held-controller-button repeat
is implemented. The first recognized controller is used; hotplug/disconnect
selects another available controller. Keyboard fallback also works without one.

| Context / action | Controller | Desktop fallback |
| --- | --- | --- |
| Browser/menu/keyboard selection | D-pad Up/Down (keyboard also Left/Right) | Arrows |
| Editor cursor | D-pad: line / character | Arrows |
| Confirm / open text input | A | Return |
| Back / close document | B | Escape |
| Menu / close keyboard or cancel prompt | Start | Tab |
| Page up/down | L1 / R1 | Page Up / Page Down |
| Browser directory path | Y | P |
| Editor Find Next | Y | P |
| Editor full file path | Start, then Y | Tab, then P |
| Keyboard: press selected key | A | Return |
| Keyboard: backspace; close at beginning | B | Escape |
| Keyboard: next text key uppercase | L1 | Page Up |
| Keyboard: letters / symbols | R1 | Page Down |

The five-row keyboard has letters, digits, shell/config punctuation, a latched
Shift key, Space, Tab, Enter, Backspace, Delete and Done. L1 toggles one-shot Shift;
this is not a held modifier. Done closes document input or submits a prompt;
Enter inserts a newline or submits a prompt. Start closes input without saving,
and does not undo characters already inserted. Prompts are limited to 4096 bytes;
Find/Save As reject newlines and tabs. D-pad moves key selection, not the prompt
cursor: prompt editing currently appends and backspaces at the end.

With input open, desktop typing inserts SDL text events. Return still activates
the selected virtual key. Fixed editor shortcuts (not configurable in the INI):

| Shortcut | Action |
| --- | --- |
| Backspace / Delete | Delete backward / forward |
| Ctrl+Enter / Ctrl+Tab | Newline (submit prompt) / literal tab |
| Ctrl+Z / Ctrl+Y or Ctrl+Shift+Z | Undo / redo |
| Ctrl+S / Ctrl+Shift+S | Save / Save As |
| Ctrl+F / Ctrl+G | Find / Go to line |

These shortcuts have priority over configured navigation bindings. Unlike the
disabled clean-buffer Save menu item, Ctrl+S in writable mode currently saves
again even with no edits, replacing the backup. Read-only guards still apply.

## Configurable bindings

`config/keymap.ini` beside the executable is loaded at startup; restart after
editing it. CMake seeds the build copy only if absent. The application does not
automatically rewrite settings, but the editor can explicitly edit this INI like
any other supported file. An absolute override avoids working-directory ambiguity:

```sh
PORTPAD_KEYMAP=/path/to/keymap.ini ./build/portpad
```

Replace `/path/to/keymap.ini` with an existing file. A relative override is
resolved from the process working directory. `[keyboard]` and `[controller]`
must each map all ten `InputAction` names exactly once, with unique physical keys
per section. SDL key/button names are used. Whole-line `#`/`;` comments, blank
lines, UTF-8 BOM and CRLF are accepted. Invalid/missing/incomplete bindings produce
a stderr warning and load all defaults; no in-app configuration error is shown.

Most hints follow the active mapping, switching keyboard/controller labels when
a controller connects. Some editor status/menu hints still use fixed names.
Printable custom keyboard bindings are suppressed during text entry, and fixed
editor shortcuts may override them. Prefer the shipped defaults for editor use.
PortPad's action INI is separate from SDL's device/GUID mappings and gptokeyb
`.gptk` files. PortMaster supplies `SDL_GAMECONTROLLERCONFIG`; PortPad calls no
custom controller-database loader and does not start gptokeyb.

## Limits and planned work

Files and encoded saves are limited to 1 MiB. Undo/redo each retain at most 100
snapshots with a 16 MiB text-storage budget; other buffers/objects use additional
memory. Mixed LF/CRLF, bare CR, UTF-16, invalid UTF-8 and control characters other
than tab/newline are rejected. CRLF expansion/BOM counts toward the saved-byte
limit. Code-point navigation is not grapheme-aware; complex scripts and missing
glyphs may display imperfectly. The virtual keyboard supplies ASCII letters and
symbols, not arbitrary Unicode entry.

Not implemented: New document, clipboard/selection editing, replace/regex,
syntax highlighting, soft wrapping, encoding/line-ending conversion, backup
settings, file management operations and session recovery. These are potential
future work, not features of this release. Priorities and known code issues are
in [CURRENT_STATE.md](docs/CURRENT_STATE.md).

Build/launch instructions and all test commands are in [BUILD.md](BUILD.md).
The R36S installation/browser/controller/cleanup baseline was reported working
on real hardware; the editor and broader firmware matrix still require testing.
The [dated PortMaster review](docs/portmaster-review.md) and
[original packaging plan](PORTMASTER_NEXT_STEPS.md) are retained as history.
