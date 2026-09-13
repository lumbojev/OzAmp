# Changelog

## 1.0.2 — Stable release

This release rolls the latest tested package into the public stable line and includes **33 changed repository paths** since the previous stable release. Full package notes and the exact file inventory are recorded below.

### What's new

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

### Complete change inventory since 1.0.1

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
<!-- OZAMP_HISTORY_END -->

---

## 1.0.1 — Stable release

This release rolls the latest tested package into the public stable line and includes **24 changed repository paths** since the previous stable release. Full package notes and the exact file inventory are recorded below.

### What's new

### Package note: TEST38_NOTES.md

# TEST38 Windows checklist

The supplied EXE is a development build. Close older OzAmp before starting it.

1. Connect/restart Spotify: Client ID persists; art, lyrics, EQ and other normal windows stay as configured.
2. F11/F8 or the title button opens the maximized native window with caption and taskbar access. Esc/Normal Player returns. Repeat while music plays; no restart should occur.
3. Check all four sections, search, Load More, track/playlist playback and device switching.
4. Check artwork, lyric sync/manual scroll, seeking and supported device volume.
5. Check your actual monitor, Windows DPI scaling and additional displays.
6. Restart: saved login should restore. Disconnect: saved login is removed. Esc restores the windows from before large mode.
7. Verify local audio, EQ, library, skins and visualizer.
8. Restore Panels reopens EQ, art and lyrics if TEST36 had already saved them hidden.
9. Minimize/maximize/restore, drag or Snap the native workspace; check text at your DPI scale.
10. Click KARAOKE beside Lyrics. Check centered timed lines and manual scrolling. SYNC resumes following.
11. Click KARAOKE again: the prior tab/search/list position should return, with uninterrupted playback.
12. Check lyrics missing/loading/plain cases and switching tracks while karaoke is open.

Cross-build and local browser/layout tests passed. Actual Windows GUI, live Spotify and Windows DPAPI have not been run here. Test before stable release.

For publication use the supplied UPDATE-GITHUB.cmd, starting with -Preview. See RELEASE_PREPARATION.md. Do not use an older Bash prerelease script.


### Package note: TEST1_NOTES.md

# OzAmp 1.1.0 TEST1

Development test build for local Windows testing. Do not publish this build as the stable GitHub release.

## 1.1 test scope

- Media Library with persistent watched folders. `WATCH DIR` adds a folder and remembers it across restarts; `RESCAN` re-scans watched folders and removes missing files.
- Favorites remain persistent and are available through the `FAV` Library view.
- `RECENT` is now based on actual playback order, persisted per track, rather than library-add order.
- `MOST` uses persisted play counts.
- Queue 2.0 keeps an ordered queue, Play Next, Add/Remove Queue, Clear Queue, and Move Earlier/Move Later controls from the playlist context menu.
- Mini Player is the compact shade interface and is now available from the main menu or F9.
- Per-user Windows file association registration is available from the OzAmp main menu for MP3, FLAC, WAV, M4A/AAC, WMA, OGG/Opus, AIFF/ALAC, M3U and M3U8. Modern Windows may still ask the user to confirm a default app.
- FLAC remains on the native WASAPI + Media Foundation playback path and now reads FLAC STREAMINFO plus Vorbis-comment metadata for title, artist, album, genre and date/year. Track info can display FLAC bit depth/sample rate/channel mode.
- MP3, WAV and M4A/AAC continue through the existing native playback paths.

## What to test first

1. Play several `.flac` files, including 16/44.1 and 24/96 if available. Test seek, EQ, pause/resume, next track, waveform and F12.
2. Add a music folder with `Media Library -> WATCH DIR`, restart OzAmp, then use `RESCAN`.
3. Favorite tracks, restart, and check `FAV`.
4. Play several tracks and confirm `RECENT` puts the newest playback first; verify `MOST` increments.
5. Add 3+ tracks to Queue, move one earlier/later, then let playback advance naturally.
6. Press F9 repeatedly and verify Mini Player restores the normal player correctly.
7. Use `OZAMP -> Register audio file associations`, then double-click a test FLAC/MP3 in Explorer.

## Important

This is a test build. Keep the public `v1.0.0` release unchanged until the 1.1 test cycle is complete.


### Package note: PACKAGE_NOTES.md

# Changes since OzAmp 1.0.0

This package contains the TEST38 development build. Its public release number is selected by the Windows updater from GitHub's latest stable release; no public version is reserved by this document.

## Karaoke lyrics view — TEST38

Click **KARAOKE** beside Lyrics in the Spotify workspace to show large centered lyrics in the same window. Click the same button again to restore the library and sidebar lyrics with the previous tab, search text and list position. Transport controls remain available; mode changes send no playback/restart commands.

Timed lyrics highlight the current line; plain lyrics remain manually scrollable without fake timing. Scroll to browse, then click Sync to resume following. This is a lyric presentation mode, not vocal removal or word-level karaoke. Clicking a library tab exits karaoke. F11/Esc retains its existing return-to-normal-player behavior.

## Spotify integration

- Added Spotify authorization and remote playback control, with a configurable Client ID and local callback.
- Added Recent, Playlists, Queue and Devices browsing, track/playlist playback and device selection.
- Added search within loaded items, pagination for supported lists, per-tab caches and refresh of the visible tab after 60 seconds.
- Distinguish API/network failures from empty results, preserve cached lists on failed refresh, respect rate-limit retry delays and reject stale asynchronous session results.
- Persist Client ID and encrypt the saved refresh token with Windows DPAPI, bound to the Windows user and Client ID. Restore login in the background; disconnect removes the saved session.
- Save rotated refresh tokens atomically and report settings/session write failures. No Client Secret or access token is stored on disk.

## TEST37 fixes to the large workspace

- Corrected TEST36 startup/reconnect hiding of art, lyrics and EQ. Normal window preferences are preserved during temporary workspace transitions.
- Removed whole-bitmap StretchBlt scaling: text and controls are rasterized at physical resolution using DPI-sized fonts.
- Native title bar with minimize/maximize/restore controls and taskbar access. Resize/Windows Snap are supported; close returns to the normal player.
- Shared OzAmp palette/font family and denser native-resolution layout instead of a separate fixed-scale theme.
- Restore Panels returns to normal and explicitly reopens EQ, artwork and lyrics if the previous build saved them hidden.

## Unified Spotify large workspace

- Connecting and saved-login restoration preserve the normal player and all side-window preferences.
- F11, F8 or the title-bar button opens a native maximized, resizable workspace on the player's monitor, with title bar and taskbar access. Esc or Normal Player returns to compact mode without issuing playback restart commands.
- Left navigation, searchable library, album art and track details, synchronized lyrics on the right, and one bottom transport bar.
- Seek by clicking/dragging the timeline; adjust supported device volume; scroll lists and lyrics independently and resume lyric following with Sync.
- Auxiliary window shortcuts return to compact mode; returning to the normal player restores the side-window visibility from before entering the workspace.

## Lyrics, artwork and updates

- Added LRCLIB lyric lookup, synchronized/plain lyrics, automatic following and a dedicated lyrics panel for local playback.
- Added online artwork lookup/cache and Spotify artwork display alongside local artwork support.
- Added automatic/manual stable GitHub release checks and an in-app update/download workflow.

## Local player and reliability

- Expanded EQ choices to 28 while retaining existing preset IDs and saved Custom settings; grouped preset menu and preamp headroom for higher-boost curves.
- Added clearer full-width output-device status and fallback/error feedback in Settings.
- Corrected compiler object output paths and clean-object builds to prevent stale-object linking.
- Added production browser fixture tests, offline Windows DPAPI/file tests, and fullscreen layout/hit-test coverage.
- Retained WASAPI/Media Foundation playback, gapless/crossfade, ReplayGain, queue, library, skins and visualizer features already present in 1.0.0; these are not claimed as newly introduced.

## Validation and limitations

Windows x64 cross-build, browser fixtures with ASan/UBSan, layout/hit-test checks at eight monitor dimensions and four DPI scales, plus repeated window-state transition tests passed here. The local audio engine is unchanged from TEST35. Actual Windows rendering/focus/DPI, live Spotify and DPAPI execution have not been tested in this environment. Run the included Windows checklist before publishing a stable release; green CI alone does not verify live Spotify or visual quality.

Spotify audio remains in Spotify; OzAmp is a controller. Local EQ/balance do not process Spotify audio. Spotify account/device/API restrictions still apply. Search filters loaded items, not Spotify's global catalog. Network features contact their respective providers; local playback does not require a Spotify account.


### Complete change inventory since 1.0.0

- **Modified:** `.github/workflows/build-windows.yml`
- **Modified:** `.gitignore`
- **Modified:** `CHANGELOG.md`
- **Modified:** `FEATURES.md`
- **Added:** `GITHUB_POLISH_UPDATE.md`
- **Modified:** `README.md`
- **Modified:** `RELEASE_NOTES.md`
- **Modified:** `SECURITY_AND_PRIVACY.md`
- **Modified:** `build_linux_cross.sh`
- **Modified:** `build_windows_llvm.bat`
- **Added:** `docs/assets/ozamp-banner.png`
- **Added:** `docs/assets/ozamp-screenshot.png`
- **Modified:** `gdiplus.def`
- **Modified:** `kernel32.def`
- **Modified:** `main.cpp`
- **Added:** `ozamp-icon.png`
- **Added:** `ozamp.ico`
- **Added:** `ozamp.rc`
- **Added:** `ozamp.res`
- **Added:** `resource.h`
- **Modified:** `shell32.def`
- **Added:** `tools/make_icon_res.py`
- **Modified:** `user32.def`
- **Modified:** `winlite.h`

---

## 1.0.0 — First public release

- First public release of OzAmp.
- Native Windows playback through WASAPI and Media Foundation.
- Dockable/resizable playlist with filtering, multi-selection and queue support.
- Persistent 10-band EQ presets and session restore.
- Media keys, device selection/fallback and local library features.
- Fullscreen audio-reactive visualizer and compact shade mode.
- Custom `.ozskin` themes.
- Playlist docking, resize behavior and EQ-preset persistence stabilized during the 3.5.x development line.
- Repository documentation and Windows CI prepared for public GitHub release.
- Project licensed under the MIT License.

### Presentation update
- Added a native multi-resolution OzAmp application icon for Explorer, taskbar, window classes and tray icon.
