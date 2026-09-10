# Current state — 2026-09-11

## Implemented

PortPad 0.1.0: directory browser with View/Edit/Cancel, one shared UTF-8 viewer/editor,
state-enforced read-only mode and explicit editing confirmation, controller keyboard,
undo/redo, literal Find/Find Next, Go to line, atomic replacement saves, Save As,
.bak backups and dirty-close confirmation. Native SDL input/hotplug and desktop
fallback, 640×480 monospaced editor, official aarch64 PortMaster export/release tools.

## Known problems and limits

1 MiB text files; consistent LF or CRLF only; code-point rather than grapheme layout;
ASCII virtual-keyboard entry; synchronous I/O; no autosave/recovery or file locking.
Clean writable Ctrl+S can replace the backup despite disabled Save in the menu.
Custom keyboard bindings can conflict with fixed editor shortcuts/text-entry rules;
some hints remain hardcoded. Ctrl+Tab at the size limit can fail without a notice.
See [ARCHITECTURE.md](ARCHITECTURE.md) for code locations and
[BUILD.md](../BUILD.md) for launcher/storage/reproducibility limitations.
No productive code was changed during this documentation audit.

## Verification

Audit on **2026-09-11 (Europe/Berlin)**, existing working tree; all attempted build
and test workflows below succeeded. Prior 2026-09-10 editor validation also passed.

| Check | Result |
| --- | --- |
| Desktop Debug configure/build, `build-desktop-check` | Passed; no compilation needed |
| Desktop CTest | 6/6 passed |
| Desktop dummy-video `--smoke-test` | Passed |
| `scripts/build-portmaster.sh` | ARM64 Release build and 6/6 tests passed |
| `scripts/package-portmaster.sh /tmp/portpad-PortMaster-New` | Rebuild/stage/export, official prepare/check/build and validator passed |
| Standalone `scripts/validate-portmaster-package.sh` | Passed, no bundled shared libraries; GLIBC_2.17 / GLIBCXX_3.4.26 |
| `scripts/test-portmaster-launcher.sh` | Passed |
| `scripts/list-portmaster-package.sh` | 11 files, 719459 bytes |
| Markdown local links, `git diff --check`, non-Markdown source hash comparison | Passed; productive source unchanged |

Exact build/test commands executed from the repository root:

```sh
cmake -S . -B build-desktop-check -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build-desktop-check
ctest --test-dir build-desktop-check --output-on-failure
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build-desktop-check/portpad --smoke-test
./scripts/build-portmaster.sh
./scripts/package-portmaster.sh /tmp/portpad-PortMaster-New
./scripts/validate-portmaster-package.sh
./scripts/test-portmaster-launcher.sh
./scripts/list-portmaster-package.sh
```

The package wrapper ran `tools/prepare_repo.sh`,
`python3 tools/build_release.py --do-check`, then `python3 tools/build_release.py`
in the external checkout; upstream reported zero broken ports. No upstream tools
were modified or copied into PortPad. Temporary logs are
`/tmp/portpad-docs-arm64.log` and `/tmp/portpad-docs-package.log` (not permanent records).

Desktop: GCC 15.2.0, CMake 4.2.3, SDL2 2.32.10, SDL2_ttf 2.24.0.
ARM64 builder: GCC 9.4.0, CMake 3.31.3, SDL2 2.32.0, SDL2_ttf 2.0.15.
Builder image ID/repository digest:
`sha256:c6783785614cf8cb57e265976c180b6cc087e305f9d709637c5df6b47a90528c`.
Official checkout: `79e6ff94b3c1aeeae930fec6525328ea6bc49d75`.
PortPad HEAD: `398aa05d4c6ae12b230b9a3a2b2bab51edee48cf`, **with existing uncommitted
implementation files plus this documentation update**; HEAD alone cannot reproduce it.
`dist/portpad.zip` SHA-256:
`95bc0b5a09bb2d8a5f24b70d351015e02eafce832bf699f7b87618e5e29e290b`.

No failed build/test attempts in this documentation audit. Not performed:
interactive GUI/controller and handheld tests (no physical device available),
fault injection for power loss/full media, and the wider firmware matrix.
Dependency installation/fresh clone examples were checked against the workflow but
not repeated because prerequisites and the external checkout already existed.

## Open tasks and next steps

1. Validate the editor on R36S: read-only/dirty transitions, keyboard, overwrite,
   backup recovery and FAT/exFAT save behavior; record exact firmware/version.
2. Agree on fixes for the documented shortcut/Save/hint inconsistencies before
   changing code; extend focused tests with those fixes.
3. Pin builder/release inputs for reproducibility; preserve logs, hashes and source
   changes. Current scripts use a mutable image tag and live release metadata.
4. Profile rendering/I/O on RK3326 and complete the firmware/resolution matrix.
5. Planned candidates, not implemented: New document, clipboard/selection, replace,
   configurable backups, encoding conversion, syntax highlighting, file-management
   operations and session recovery. No delivery dates are claimed.

Hardware baseline: the user reported installation, launch, browser, controller
mappings and cleanup working on R36S. That does not establish new-editor support
or separate passes for ArkOS/dArkOS versions; other firmware/device tests are open.
