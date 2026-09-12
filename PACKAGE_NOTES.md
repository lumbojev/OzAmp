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
