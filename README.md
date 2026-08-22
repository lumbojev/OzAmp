# OzAmp 1.0.0

**OzAmp** is a compact native Windows audio player created by **Oskar Lumbojev**.

It is built around a simple idea: local music playback should feel immediate, focused and personal. OzAmp combines a custom Win32 interface with native Windows audio, a dockable playlist, persistent equalizer presets, queue management, session restore and a restrained audio-reactive visualizer.

## Highlights

- Native C++ / Win32 desktop application
- WASAPI audio output with selectable devices and fallback handling
- Windows Media Foundation decoding
- Dockable and resizable playlist with search/filter and multi-selection
- Play Next / ordered queue workflow
- 10-band equalizer with persistent presets
- Session and window-position restore
- Hardware media keys and global hotkeys
- Local media library, album art and track information
- Fullscreen audio-reactive visualizer
- Compact shade mode
- `.ozskin` skin support
- No account requirement, telemetry or analytics

## Platform

OzAmp 1.0.0 targets **64-bit Windows**. Windows 10 and Windows 11 are the intended desktop environments.

## Download

For normal use, download the latest Windows executable or release package from the repository's **Releases** page.

## Build from source

LLVM/Clang for Windows is required. From a Developer Command Prompt or terminal where `clang-cl` and `lld-link` are available:

```bat
build_windows_llvm.bat
```

Expected output:

```text
OzAmp-1.0.0.exe
```

A GitHub Actions workflow in `.github/workflows/build-windows.yml` performs the same Windows x64 build in CI.

## Repository layout

- `main.cpp` — Win32 application, UI, playlist, queue and persistence
- `audio_engine.cpp/.h` — WASAPI / Media Foundation playback and PCM processing
- `winlite.h` — compact Windows ABI/header surface used by the project
- `skins/` — bundled `.ozskin` themes
- `docs/ARCHITECTURE.md` — high-level implementation overview
- `TEST_CHECKLIST.md` — release smoke-test checklist

## Runtime data

OzAmp stores settings, playlist and library state locally at runtime. Generated runtime files are excluded by `.gitignore` and are not part of the source repository.

## Privacy

OzAmp is designed as a local-first desktop application. It does not require an account and is built without telemetry or analytics. See `SECURITY_AND_PRIVACY.md` for details.

## Contributing

Bug reports and focused pull requests are welcome. Please read `CONTRIBUTING.md` before submitting code.

## License

OzAmp is released under the **MIT License**. See [`LICENSE`](LICENSE).

Copyright © 2026 Oskar Lumbojev.
