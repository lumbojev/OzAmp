# Security and privacy

OzAmp is local-first without telemetry or analytics. Local playback requires no account. Optional network features mean this is not a wholly offline application.

- Spotify authorization/control contacts Spotify and requires a configured Client ID, Spotify account and available device. Provider/account restrictions apply; audio remains in Spotify.
- Client ID is stored in OzAmp settings. Saved refresh tokens use Windows DPAPI for the current Windows user with Client ID binding. No Client Secret or access token is stored on disk. Disconnect deletes the saved session. DPAPI does not protect against malicious software running as the same user.
- Lyrics lookup sends track metadata to LRCLIB. Online artwork contacts external providers and caches images locally. Spotify artwork uses URLs returned by Spotify.
- Release checks/downloads contact GitHub. Automatic checking can be changed in Settings > Updates.
- Personal settings/session files remain in the user's AppData directory and are excluded from the package.

See the included Windows checklist and session-storage tests. Cross-compilation alone does not verify Windows runtime behavior.
