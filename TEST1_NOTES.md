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
