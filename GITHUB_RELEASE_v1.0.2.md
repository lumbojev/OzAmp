# OzAmp 1.0.2

OzAmp **1.0.2** is the stable release following **1.0.1**.

This release rolls the latest tested package into the public stable line and includes **33 changed repository paths** since the previous stable release. Full package notes and the exact file inventory are recorded below.

## What's new

## Windows compatibility metadata

- Added an embedded application manifest declaring Windows 10/11 compatibility and normal-user execution (asInvoker).
- Added native FileVersion, ProductVersion, product name and original executable filename resources.
- Kept the existing system-DPI default and Spotify's per-monitor window handling.
- Regenerate all application resources on every build and validate the resources extracted from the linked executable. A missing manifest or version mismatch fails the build.

## GitHub release preparation

- Copy every source file to its exact relative destination, including existing docs, tools, skins and workflow directories.
- Repair known doubled directories created by the previous updater; preserve and stop on unknown files rather than deleting them.
- Preserve archived release logs and older changelog entries from GitHub. Audit active app/source/docs while allowing numbered development references in historical records and clearly marked Git inventories.
- Preserve the final newline and trailing whitespace of archived changelog entries. Fix the "Older changelog section changed" self-test failure without weakening the preservation check.
- Insert release-note text literally, including dollar signs, and check repeat preparation with LF, CRLF, missing final newlines, trailing whitespace, pending entries and Unreleased sections.
- Synchronize app, manifest, build output, CI metadata and README versions to the next stable tag selected from GitHub.
- Configure line endings within the temporary checkout and avoid pager pauses.
- Run offline regression checks in the installed Windows PowerShell before any GitHub operation.

## Existing features retained

Spotify Recent / Playlists / Queue / Devices, persistent Client ID and encrypted saved login, lyrics, artwork, karaoke, the native large workspace, local playback and EQ remain available. See CHANGELOG.md for the full earlier release history.

## Validation scope

The Windows x64 executable is cross-built and its embedded resource tree is inspected locally. Resource fixtures, browser behavior, window state and layout tests are automated. The original changelog failure was reproduced with PowerShell 7.4.19 on Linux, and the corrected updater's complete offline self-test passes in that runtime. Windows PowerShell 5.1 and the Windows Compatibility Assistant dialog have not been run here; the same offline self-test executes on the publishing Windows machine before the updater contacts GitHub.

## Complete repository change inventory since v1.0.1

<!-- OZAMP_HISTORY_START -->
- **Modified:** `.gitattributes`
- **Removed:** `.github/.github/ISSUE_TEMPLATE/bug_report.yml`
- **Removed:** `.github/.github/ISSUE_TEMPLATE/feature_request.yml`
- **Removed:** `.github/.github/PULL_REQUEST_TEMPLATE.md`
- **Removed:** `.github/.github/workflows/build-windows.yml`
- **Modified:** `.github/workflows/build-windows.yml`
- **Modified:** `CONTRIBUTING.md`
- **Modified:** `FEATURES.md`
- **Removed:** `GITHUB_POLISH_UPDATE.md`
- **Removed:** `GITHUB_RELEASE_CHECKLIST.md`
- **Modified:** `PACKAGE_NOTES.md`
- **Modified:** `README.md`
- **Modified:** `RELEASE_NOTES.md`
- **Removed:** `RELEASE_PREPARATION.md`
- **Removed:** `TEST1_NOTES.md`
- **Removed:** `TEST38_NOTES.md`
- **Modified:** `TEST_CHECKLIST.md`
- **Modified:** `build_linux_cross.sh`
- **Modified:** `build_windows_llvm.bat`
- **Removed:** `docs/docs/ARCHITECTURE.md`
- **Removed:** `docs/docs/assets/ozamp-banner.png`
- **Removed:** `docs/docs/assets/ozamp-screenshot.png`
- **Modified:** `main.cpp`
- **Added:** `ozamp.manifest`
- **Modified:** `ozamp.rc`
- **Modified:** `ozamp.res`
- **Removed:** `skins/skins/AmigaGreen.ozskin`
- **Removed:** `skins/skins/HotCopper.ozskin`
- **Removed:** `skins/skins/NordicFrost.ozskin`
- **Added:** `tests/test_resources.py`
- **Modified:** `tools/make_icon_res.py`
- **Removed:** `tools/tools/make_icon_res.py`
- **Added:** `tools/verify_exe.py`

## Git changes already on main since v1.0.1



The final release commit for **v1.0.2** is added by the updater after these notes are generated.

## Diff summary

```text
.gitattributes                                     |   7 +-
 .github/.github/ISSUE_TEMPLATE/bug_report.yml      |  47 -----
 .github/.github/ISSUE_TEMPLATE/feature_request.yml |  17 --
 .github/.github/PULL_REQUEST_TEMPLATE.md           |  13 --
 .github/.github/workflows/build-windows.yml        |  50 -----
 .github/workflows/build-windows.yml                |  28 ++-
 CONTRIBUTING.md                                    |   2 +-
 FEATURES.md                                        |   2 +-
 GITHUB_POLISH_UPDATE.md                            |  18 --
 GITHUB_RELEASE_CHECKLIST.md                        |  14 --
 PACKAGE_NOTES.md                                   |  65 ++-----
 README.md                                          |  36 ++--
 RELEASE_NOTES.md                                   | 205 +--------------------
 RELEASE_PREPARATION.md                             |   9 -
 TEST1_NOTES.md                                     |  29 ---
 TEST38_NOTES.md                                    |  20 --
 TEST_CHECKLIST.md                                  |   2 +-
 build_linux_cross.sh                               |   8 +-
 build_windows_llvm.bat                             |  14 +-
 docs/docs/ARCHITECTURE.md                          |  22 ---
 docs/docs/assets/ozamp-banner.png                  | Bin 1621212 -> 0 bytes
 docs/docs/assets/ozamp-screenshot.png              | Bin 39472 -> 0 bytes
 main.cpp                                           |  26 +--
 ozamp.manifest                                     |  26 +++
 ozamp.rc                                           |   1 +
 ozamp.res                                          | Bin 142336 -> 144428 bytes
 skins/skins/AmigaGreen.ozskin                      |  17 --
 skins/skins/HotCopper.ozskin                       |  17 --
 skins/skins/NordicFrost.ozskin                     |  17 --
 tests/test_resources.py                            |  55 ++++++
 tools/make_icon_res.py                             |  55 +++++-
 tools/tools/make_icon_res.py                       |  44 -----
 tools/verify_exe.py                                | 113 ++++++++++++
 33 files changed, 344 insertions(+), 635 deletions(-)
```
<!-- OZAMP_HISTORY_END -->

## Distribution

- Windows x64 executable: `OzAmp-1.0.2.exe`
- SHA-256 checksum: `OzAmp-1.0.2-SHA256.txt`
- Source snapshot: `OzAmp-1.0.2-source.zip`
- Source tag: `v1.0.2`

The Windows binary is built by GitHub Actions from the exact release commit. The release is published only if the Windows CI workflow succeeds.