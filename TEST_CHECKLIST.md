# OzAmp 1.0.0 Windows release smoke test

## Startup and persistence
- [ ] Start OzAmp on Windows 11 without errors.
- [ ] Add local tracks and restart; playlist/session restore works.
- [ ] Select an EQ preset, restart, and confirm preset name, curve and EQ state persist.

## Playback
- [ ] Play, pause/resume, stop, previous and next work.
- [ ] Seek works by clicking/dragging the seek area.
- [ ] Volume, balance, mute and media keys work.
- [ ] Queue / Play Next works across multiple tracks.

## Playlist
- [ ] Search/filter works.
- [ ] Ctrl/Shift multiselect works.
- [ ] Playlist resize from the lower-right grows to the right normally.
- [ ] Playlist right edge snaps correctly to the EQ/main right edge.
- [ ] Double-clicking the playlist title aligns it to the EQ width/right edge.
- [ ] The final visible track is never covered by helper/status text.

## EQ and devices
- [ ] EQ presets audibly apply and persist after restart.
- [ ] Selected output device works.
- [ ] Disconnecting the selected device falls back safely when possible.

## Visualizer and windows
- [ ] F12 opens/closes the visualizer.
- [ ] Double-click/F11 enters true monitor fullscreen.
- [ ] Escape/F11 restores the previous visualizer window.
- [ ] Playlist, EQ and other windows restore sensible positions.

## Packaging
- [ ] GitHub Actions Windows build passes.
- [ ] SHA-256 published for the exact release EXE matches the attached binary.
