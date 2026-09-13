# Changes since OzAmp 1.0.1

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
