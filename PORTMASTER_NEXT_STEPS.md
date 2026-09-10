> **Historical plan — superseded; do not use as current build instructions.**
> Reviewed 2026-09-11. The implemented workflow is documented in [BUILD.md](BUILD.md).
> Current output is `dist/portpad.zip`, generated from
> `dist/portmaster-source/port/portpad/` via an external official checkout's
> `ports/portpad/` and release tools. No SDL/system libraries are bundled.
> Submitted launchers use 0644; binaries use 0755 with runtime execute-bit repair.
> `collect-portmaster-libs.sh`, `portmaster/package/` and `dist/staging/` were
> proposed here but are not implemented or required. The concise port README
> contains no build instructions. Packaging needs a checkout argument or
> `PORTMASTER_REPO`. The editor milestone was implemented after this plan.
> The original text below is preserved for historical rationale only.

# PortPad — Codex Implementation Plan for ARM64 / PortMaster Packaging

## Context

PortPad already builds successfully in the PortMaster ARM64 Docker builder.

Verified environment:

- Target architecture: `aarch64`
- Builder image: `ghcr.io/monkeyx-net/portmaster-build-templates/portmaster-builder:aarch64-latest`
- GCC/G++: 9.4.0
- CMake: 3.31.3
- SDL2: 2.32.0
- SDL2_ttf: 2.0.15

Verified ARM64 binary:

```text
build-portmaster/portpad
ELF 64-bit ARM aarch64
GLIBC requirement: 2.17
GLIBCXX requirement: up to 3.4.26
```

Current runtime dependencies include:

```text
libSDL2_ttf-2.0.so.0
libSDL2-2.0.so.0
libstdc++.so.6
libgcc_s.so.1
libc.so.6
libfreetype.so.6
libm.so.6
libdl.so.2
libpthread.so.0
libpng16.so.16
libz.so.1
```

The development host is Windows + WSL2 Ubuntu + Docker Desktop.

The repository is:

```text
mamitsch/PortPad
```

## Goal

Implement a repeatable, Codex-driven ARM64 build and PortMaster packaging workflow.

The developer should **not need to manually create directories, copy runtime libraries, assemble ZIP files, or edit package structure by hand**.

The desired workflow should eventually be as simple as:

```bash
./scripts/build-portmaster.sh
./scripts/package-portmaster.sh
```

and produce a ready-to-copy PortMaster package under:

```text
dist/
```

## Important constraints

1. Do not break the existing desktop/x86_64 build.
2. Do not hardcode R36S-only paths in the C++ application.
3. The PortMaster launcher may use PortMaster-provided variables and `control.txt`.
4. The package must work offline after copying it to the handheld.
5. Do not bundle glibc or the Linux dynamic loader.
6. Do not bundle `libc`, `libpthread`, `libm`, or `libdl`.
7. Prefer a self-contained SDL runtime bundle where practical.
8. Keep generated build and packaging artifacts out of Git.
9. All directory creation and file copying must be scripted.
10. Add checks that fail clearly when prerequisites or expected files are missing.

---

# Phase 1 — Review current repository

Before changing anything:

1. Read `AGENTS.md`.
2. Inspect the current repository structure.
3. Inspect the current `CMakeLists.txt` and build target names.
4. Verify the ARM64 executable target is named `portpad`.
5. Verify the existing desktop build remains usable.
6. Identify any existing `scripts/`, `portmaster/`, `dist/`, or packaging files before creating replacements.
7. Do not duplicate existing functionality unnecessarily.

At the end of this phase, summarize what already exists and what will be added.

---

# Phase 2 — Add PortMaster build script

Create:

```text
scripts/build-portmaster.sh
```

The script must:

1. Run from any working directory by resolving the repository root.
2. Use the Docker image:

```text
ghcr.io/monkeyx-net/portmaster-build-templates/portmaster-builder:aarch64-latest
```

3. Bind-mount the repository into the container.
4. Build inside the container using ARM64.
5. Use a separate build directory:

```text
build-portmaster/
```

6. Use Ninja when available in the builder.
7. Configure and build the existing CMake project.
8. Verify the expected output exists:

```text
build-portmaster/portpad
```

9. Run these validation checks automatically:

```bash
file build-portmaster/portpad
readelf -h build-portmaster/portpad
readelf --version-info build-portmaster/portpad
readelf -d build-portmaster/portpad
```

10. Fail if the binary is not AArch64.
11. Print a clear success message with the resulting binary path.

The developer should not have to manually enter the Docker container.

---

# Phase 3 — Create PortMaster package source layout

Create and maintain this source-controlled structure automatically if it does not yet exist:

```text
portmaster/
├── PortPad.sh
├── port.json
├── README.md
└── package/
    └── .gitkeep
```

Do not place generated binaries or copied runtime libraries in the tracked `portmaster/package/` tree unless there is a strong reason.

Prefer staging generated content under `dist/staging/`.

## Launcher requirements

Create:

```text
portmaster/PortPad.sh
```

The launcher should follow current PortMaster conventions and should:

1. Locate and source PortMaster `control.txt` using standard compatible paths.
2. Call `get_controls`.
3. Resolve the game/port directory using PortMaster variables rather than hardcoding one firmware layout.
4. Set:

```bash
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
```

5. Set:

```bash
export LD_LIBRARY_PATH="$GAMEDIR/libs.aarch64:$LD_LIBRARY_PATH"
```

6. `cd` into the PortPad game directory.
7. Launch:

```text
./portpad
```

8. Call `pm_finish` when appropriate.
9. Quote shell variables safely.
10. Exit with a useful error if the executable cannot be found.

The launcher must not assume only dArkOS or only the R36S.

---

# Phase 4 — Create PortMaster metadata

Create:

```text
portmaster/port.json
```

Use a valid current PortMaster metadata structure.

Use these project details unless the repository already defines better values:

```text
Name: PortPad
Genre/Category: Utility / Tool
Description: Gamepad-first text editor and file manager for PortMaster-compatible handhelds.
License: MIT
Architecture: aarch64 initially
```

If a required PortMaster metadata field cannot be determined confidently, document it rather than inventing misleading data.

Also create/update:

```text
portmaster/README.md
```

with:

- what PortPad is
- supported initial target
- controls summary placeholder
- build/package instructions
- installation instructions
- troubleshooting notes

---

# Phase 5 — Runtime library collection

Create a script or implement this as part of packaging.

Preferred file:

```text
scripts/collect-portmaster-libs.sh
```

The script must run using the same PortMaster ARM64 Docker image as the build.

Collect these runtime libraries into staging:

```text
libSDL2-2.0.so.0
libSDL2_ttf-2.0.so.0
libfreetype.so.6
libpng16.so.16
libz.so.1
```

Do **not** automatically bundle:

```text
libc.so.6
libpthread.so.0
libm.so.6
libdl.so.2
ld-linux-aarch64.so.1
```

Do not bundle `libstdc++.so.6` or `libgcc_s.so.1` initially unless a later device test proves they are required.

The script should:

1. Resolve the real files behind symlinks where necessary.
2. Preserve or recreate required SONAME symlink names inside the package.
3. Validate each collected library using `file`.
4. Ensure all collected libraries are ARM64.
5. Print exactly which files are packaged.
6. Fail clearly if a required runtime library cannot be found.

Destination in the staged package:

```text
portpad/libs.aarch64/
```

---

# Phase 6 — Packaging script

Create:

```text
scripts/package-portmaster.sh
```

This script must do everything required to create the distributable package.

It should:

1. Resolve repository root automatically.
2. Optionally call `build-portmaster.sh` if the ARM64 binary is missing or stale.
3. Delete/recreate a clean staging directory:

```text
dist/staging/
```

4. Create the full package structure automatically.
5. Copy the ARM64 executable.
6. Copy the launcher.
7. Copy `port.json` and documentation as appropriate.
8. Copy application assets required at runtime.
9. Collect/copy runtime libraries.
10. Ensure shell launchers are executable.
11. Validate the staged binary and runtime libraries.
12. Create the final ZIP automatically.

Target output:

```text
dist/PortPad.zip
```

The ZIP should be structured for PortMaster offline/autoinstall usage.

The final archive should contain a layout equivalent to:

```text
PortPad.zip
├── PortPad.sh
└── portpad/
    ├── portpad
    ├── libs.aarch64/
    │   ├── libSDL2-2.0.so.0
    │   ├── libSDL2_ttf-2.0.so.0
    │   ├── libfreetype.so.6
    │   ├── libpng16.so.16
    │   └── libz.so.1
    ├── assets/
    ├── port.json
    ├── README.md
    └── LICENSE
```

Adjust exact metadata placement if current PortMaster packaging rules require a different location.

Before finalizing, inspect current PortMaster packaging conventions rather than relying on stale assumptions.

---

# Phase 7 — Packaging validation

Add:

```text
scripts/validate-portmaster-package.sh
```

The validator should check at least:

- `dist/PortPad.zip` exists
- ZIP is readable
- launcher exists
- launcher is executable in staging
- ARM64 executable exists
- ARM64 executable is AArch64
- all bundled `.so` files are AArch64
- required metadata exists
- required assets referenced by PortPad exist
- no desktop/x86_64 executable accidentally entered the package
- no build directory accidentally entered the ZIP
- no `.git`, `.vscode`, test output, or source tree accidentally entered the ZIP

Also print the ZIP contents in a readable tree/list at the end.

---

# Phase 8 — Update `.gitignore`

Ensure these generated paths are ignored:

```text
build-portmaster/
dist/
```

Preserve any existing useful `.gitignore` content.

Do not ignore source-controlled PortMaster launcher/metadata files.

---

# Phase 9 — Documentation

Create or update:

```text
BUILDING.md
```

Include three workflows.

## Desktop build

Document the existing local WSL development build.

## PortMaster ARM64 build

The developer should be able to run:

```bash
./scripts/build-portmaster.sh
```

## PortMaster package

The developer should be able to run:

```bash
./scripts/package-portmaster.sh
```

and receive:

```text
dist/PortPad.zip
```

Document offline installation on the handheld using the PortMaster `autoinstall` workflow.

Do not assume a single hardcoded `/roms` layout unless explicitly required by PortMaster documentation.

---

# Phase 10 — Do not implement unrelated features

For this task, do **not** add or redesign:

- file editor functionality
- file browser functionality
- virtual keyboard functionality
- controller UX
- fonts/UI design
- application architecture unrelated to build/package portability

Only make application-code changes when required to make ARM64/PortMaster packaging work correctly.

---

# Phase 11 — Run everything

After implementation, Codex must actually execute the workflow.

Run:

```bash
./scripts/build-portmaster.sh
./scripts/package-portmaster.sh
./scripts/validate-portmaster-package.sh
```

Fix all errors until they pass.

Then report:

1. ARM64 binary path
2. ARM64 architecture validation result
3. maximum GLIBC requirement
4. maximum GLIBCXX requirement
5. bundled runtime libraries
6. final ZIP path
7. final ZIP size
8. exact ZIP structure
9. any remaining assumptions that require validation on a real R36S

---

# Phase 12 — Real-device test preparation

Do not claim the port is confirmed working until it has been tested on the handheld.

Prepare the final instructions for the first R36S test:

```text
1. Copy dist/PortPad.zip to the PortMaster autoinstall directory.
2. Start PortMaster and let it install the package.
3. Return to Ports/Tools as appropriate.
4. Launch PortPad.
5. Record any stdout/stderr/log output if it exits unexpectedly.
```

If possible, make the launcher write a small diagnostic log under the PortPad data directory during early testing, but do not leave excessive debug logging enabled permanently.

---

# Acceptance criteria

This task is complete only when:

- existing desktop build still works
- ARM64 build is scripted
- no manual Docker shell is required
- no manual directory creation is required
- no manual library copying is required
- packaging is scripted
- package validation is scripted
- `dist/PortPad.zip` is generated successfully
- generated artifacts are excluded from Git
- documentation explains the complete workflow
- Codex has executed the scripts successfully

Do not stop after merely creating scripts. Run them and fix them.
