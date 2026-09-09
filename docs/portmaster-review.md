# PortMaster readiness review — 2026-09-09

Verdict: the current prototype has a suitable foundation, but is not yet a
validated PortMaster release. No handheld build or hardware execution was performed.

## Verified application behavior

- C++17, CMake, SDL2 and SDL2_ttf; no direct X11 APIs or Python runtime requirement.
- Native SDL controller events and PC keyboard events share InputAction.
- Application state, browser listing/navigation and modal state have no SDL dependency.
- Read-only browsing and selectable `..`; Back does not navigate the filesystem.
- Controller hotplug, no-controller keyboard use, and custom controller Confirm tested.
- Assets and default config resolve beside the executable. PORTPAD_KEYMAP overrides
  the config location without firmware-specific paths or runtime writes.
- Config loads atomically; malformed, missing, incomplete and conflicting files
  retain working defaults. All ten actions must be mapped in both sections.
- CRLF and UTF-8 BOM are accepted. On-screen hints use configured bindings.

## Fixed during this review

- Reconfiguring CMake could overwrite the editable build-directory keymap. It now
  seeds only a missing file. Tests use the source template independently of user edits.
- SDL's default input-category log filter suppressed invalid-config warnings.
  These now go directly to stderr, verified in an actual startup process.
- Added configuration tests for physical-key conflicts, incomplete/missing files,
  Windows text format, actual remapped controller events and controller hint labels.

## PortMaster release work still required

The official [packaging guide](https://portmaster.games/packaging.html) calls for
port metadata, screenshot, gameinfo, a launch script, and a port data directory.
PortPad currently has none of that release scaffolding. Before release:

- Add a launcher sourcing control.txt and applicable firmware mod files, calling
  get_controls, propagating SDL mappings, and using platform setup/cleanup helpers.
- Build aarch64 and name/select the binary by architecture. Bundle required
  compatible libraries in the architecture-specific directory, with license notices
  for code, libraries and assets in the package's licenses directory.
- Generate and validate package metadata and an installable archive; verify offline
  autoinstall and subsequent startup on the R36S.

These are release requirements, not claims of completed implementation. Native SDL
input should remain native; a launcher must not add duplicate keyboard emulation.
The [current control.txt](https://github.com/PortsMaster/PortMaster-GUI/blob/main/PortMaster/control.txt)
provides controller-environment setup; PortPad's action INI is a separate layer from
SDL's device/GUID database. PORTPAD_KEYMAP does not replace get_controls or
SDL_GAMECONTROLLERCONFIG / SDL_GAMECONTROLLERCONFIG_FILE.

## Remaining code risks and debt

- `file` identifies the tested binary as x86-64. `ldd` shows host SDL/font/system
  dependencies; copying this build to aarch64 will not work. Host SDL also links
  desktop backends, which is not a direct X11 dependency in PortPad source but is
  another reason not to ship host libraries. Target ABI and SDL video backend need
  validation with the chosen handheld toolchain/runtime.
- Directory enumeration is synchronous. Slow storage or large folders can pause
  input. Listings are snapshots until reopened.
- Text surfaces/textures are rebuilt every frame; the loop adds a 16 ms delay even
  with vsync. Profile frame pacing and memory/CPU use on RK3326 before optimizing.
- Long controller binding names and error messages can be clipped at 640x480.
  Dialog wrapping counts code points, not glyph widths; arbitrary non-UTF-8 Linux
  filenames and missing font glyphs need an explicit display-encoding policy.
- Default bindings are duplicated between fallback event switches, hint arrays and
  the INI template. Tests detect template/event drift, but centralization would
  reduce maintenance risk. No analog navigation or held-button repeat exists.
- Config warnings appear in stderr, not an in-app dialog. A launcher must retain
  logs; user-visible configuration diagnostics would be useful later.
- Physical controller mapping, framebuffer/KMSDRM behavior, device orientation,
  display readability, suspend/resume and exit-to-launcher remain untested.

## Validation performed

- Debug configure/build and all four CTest suites passed.
- Separate Release build with -Wall -Wextra -Wpedantic: no warnings; all four suites passed.
- Dummy-video smoke test renders menu, browser and path dialog.
- Isolated temporary-directory checks verified CMake preserves custom config,
  PORTPAD_KEYMAP startup works from outside the project, and missing config emits
  the fallback warning while startup succeeds.
- No physical gamepad or R36S testing; no autoinstall/package validation yet.
