# Architecture

## `main.cpp`
Owns the Win32 application lifecycle, custom painting, playlists, queue, session/settings persistence, metadata UI, docking, system integration and error/recovery surfaces.

## `audio_engine.cpp` / `audio_engine.h`
Owns WASAPI output, Media Foundation decoding, PCM conversion/resampling, EQ/replay gain, gapless/crossfade preparation, spectrum/waveform data and audio endpoint state.

## `winlite.h`
A deliberately small Windows ABI/header surface used so OzAmp can be cross-built without depending on the full Windows SDK header graph.

## Data model
`Track` stores path, normalized display metadata, playback stats, loudness data and UI flags. Playlist queue entries are stored as paths rather than indexes so queue order survives playlist reordering.

## Device recovery
The selected endpoint is treated as the preferred endpoint. If it disappears during native playback, OzAmp reinitializes against Windows Default while preserving track position. Periodic endpoint checks restore the preferred device when it becomes available again.

## UI rendering
Core windows are owner-drawn into memory DCs and blitted to screen. Hover, pressed and active states are separate. Playlist docking is geometric and survives playlist resizing. Above/below docking stores horizontal alignment independently, allowing edge sliding and persistent right-edge magnetic alignment.

## Visualizer
The F12 visualizer is a single audio-reactive surface. It renders a fixed-palette plasma field from live PCM spectrum/waveform data, with restrained waveform and spectrum overlays; it does not expose visual mode-selection concepts.
