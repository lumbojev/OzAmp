> See [changes since 1.0.0](PACKAGE_NOTES.md) and the [Windows test checklist](TEST38_NOTES.md). The supplied EXE is TEST38; the Windows release updater selects the public version and rebuilds it through CI.

## Karaoke lyrics view — TEST38

Click **KARAOKE** beside Lyrics in the Spotify workspace to show large centered lyrics in the same window. Click the same button again to restore the library and sidebar lyrics with the previous tab, search text and list position. Transport controls remain available; mode changes send no playback/restart commands.

Timed lyrics highlight the current line; plain lyrics remain manually scrollable without fake timing. Scroll to browse, then click Sync to resume following. This is a lyric presentation mode, not vocal removal or word-level karaoke. Clicking a library tab exits karaoke. F11/Esc retains its existing return-to-normal-player behavior.

## Spotify and fullscreen workspace

Connect through **F10 → Spotify** using your Client ID. Connection preserves your normal player and side windows. **F11 / F8** or **SPOTIFY / FULLSCREEN** opens the unified workspace; **Esc / Normal Player** returns without restarting playback.

Recent, Playlists, Queue and Devices share one window with album art, track details, synchronized lyrics and a bottom transport bar. Search filters loaded items; Load More retrieves additional supported pages. Scroll lyrics manually and click Sync to resume following. Spotify audio stays in Spotify, so OzAmp's local EQ does not affect it.

Client ID persists; saved login is encrypted with Windows DPAPI. Disconnect removes the saved session. Optional Spotify, lyrics, online artwork and update checks use network services; local playback remains available without a Spotify account. See [privacy details](SECURITY_AND_PRIVACY.md).

<p align="center">
  <img src="docs/assets/ozamp-banner.png" alt="OzAmp — native Windows audio player" width="100%">
</p>

<p align="center">
  <a href="https://github.com/lumbojev/OzAmp/releases/latest">
    <img alt="Latest release" src="https://img.shields.io/github/v/release/lumbojev/OzAmp?display_name=tag&style=for-the-badge">
  </a>
  <a href="https://github.com/lumbojev/OzAmp/actions/workflows/build-windows.yml">
    <img alt="Windows build" src="https://img.shields.io/github/actions/workflow/status/lumbojev/OzAmp/build-windows.yml?branch=main&style=for-the-badge&label=Windows%20build">
  </a>
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-native-6688AE?style=for-the-badge&logo=cplusplus&logoColor=white">
  <img alt="MIT License" src="https://img.shields.io/badge/license-MIT-477A60?style=for-the-badge">
</p>

<p align="center">
  <strong>OzAmp is a compact native Windows audio player created by Oskar Lumbojev.</strong><br>
  Fast local playback, a focused Win32 interface, persistent EQ, playlist workflow and audio-reactive visuals — without accounts, telemetry or unnecessary layers.
</p>

<p align="center">
  <a href="https://github.com/lumbojev/OzAmp/releases/latest"><strong>⬇ Download OzAmp for Windows x64</strong></a>
  &nbsp;•&nbsp;
  <a href="https://github.com/lumbojev/OzAmp/releases">Releases</a>
  &nbsp;•&nbsp;
  <a href="RELEASE_NOTES.md">Release notes</a>
  &nbsp;•&nbsp;
  <a href="docs/ARCHITECTURE.md">Architecture</a>
</p>

---

## Screenshot

<p align="center">
  <img src="docs/assets/ozamp-screenshot.png" alt="OzAmp 1.0.1 running on Windows" width="720">
</p>

## Why OzAmp?

OzAmp started from a simple idea: **a local music player should feel immediate, focused and personal**.

It is deliberately built as a native Windows desktop application rather than a browser shell. The UI stays compact, playback remains local, and the core features are designed around everyday listening instead of accounts, cloud services or telemetry.

## Highlights

- **Native C++ / Win32** desktop application
- **WASAPI** audio output with selectable devices and fallback handling
- **Windows Media Foundation** decoding
- Dockable and resizable playlist with search/filter and multi-selection
- Play Next / ordered queue workflow
- **10-band equalizer** with persistent presets
- Session and window-position restore
- Hardware media keys and global hotkeys
- Local media library, album art and track information
- Fullscreen audio-reactive visualizer
- Compact shade mode
- `.ozskin` skin support
- No account requirement, telemetry or analytics

<!-- OZAMP_CURRENT_RELEASE_START -->
## Current release — 1.0.1

OzAmp **1.0.1** is the current stable release.

**[Download OzAmp 1.0.1 →](https://github.com/lumbojev/OzAmp/releases/tag/v1.0.1)**

### What changed since 1.0.0

- Latest tested OzAmp source promoted to a stable GitHub release
- Application, build script and Windows CI version synchronized to **1.0.1**
- **24** repository paths changed since the previous stable tag
- Release notes, changelog, checksum and source archive regenerated for this release

For the complete change list, see [RELEASE_NOTES.md](RELEASE_NOTES.md) and [CHANGELOG.md](CHANGELOG.md).
<!-- OZAMP_CURRENT_RELEASE_END -->

## Download
### Windows x64

The recommended way to install or update OzAmp is through the latest GitHub release:

**[Download the latest OzAmp release →](https://github.com/lumbojev/OzAmp/releases/latest)**

For v1.0.1 specifically:

**[Download OzAmp-1.0.1.exe](https://github.com/lumbojev/OzAmp/releases/download/v1.0.1/OzAmp-1.0.1.exe)**

> Windows may show a SmartScreen warning for an unsigned independent executable. Verify the SHA-256 checksum published with the release if desired.

## Platform

OzAmp 1.0.1 targets **64-bit Windows**. Windows 10 and Windows 11 are the intended desktop environments.

## Build from source

LLVM/Clang for Windows is required. From a Developer Command Prompt or terminal where `clang-cl` and `lld-link` are available:

```bat
build_windows_llvm.bat
```

Expected output:

```text
OzAmp-1.0.1.exe
```

A GitHub Actions workflow in `.github/workflows/build-windows.yml` performs the same Windows x64 build in CI.

## Repository layout

| Path | Purpose |
| --- | --- |
| `main.cpp` | Win32 application, UI, playlist, queue and persistence |
| `audio_engine.cpp/.h` | WASAPI / Media Foundation playback and PCM processing |
| `winlite.h` | Compact Windows ABI/header surface used by the project |
| `ozamp.ico` / `ozamp.res` | Application icon and compiled Windows resource |
| `skins/` | Bundled `.ozskin` themes |
| `docs/ARCHITECTURE.md` | High-level implementation overview |
| `TEST_CHECKLIST.md` | Release smoke-test checklist |

## Privacy

OzAmp is designed as a **local-first desktop application**. It does not require an account and is built without telemetry or analytics.

See [`SECURITY_AND_PRIVACY.md`](SECURITY_AND_PRIVACY.md) for details.

## Contributing

Bug reports and focused pull requests are welcome. Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before submitting code.

## License

OzAmp is released under the **MIT License**. See [`LICENSE`](LICENSE).

Copyright © 2026 Oskar Lumbojev.
