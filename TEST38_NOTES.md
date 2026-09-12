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
