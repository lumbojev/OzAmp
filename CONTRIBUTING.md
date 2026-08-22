# Contributing to OzAmp

Thanks for your interest in OzAmp.

## Before submitting a change

1. Build `OzAmp-1.0.0.exe` successfully with `build_windows_llvm.bat`.
2. Run the relevant checks in `TEST_CHECKLIST.md` on Windows.
3. Keep UI changes consistent with the existing OzAmp visual language.
4. Avoid adding telemetry, accounts or network dependencies without explicit project discussion.
5. Keep pull requests focused and describe the user-visible behavior that changed.
6. Do not commit generated `.obj`, `.lib`, executable or runtime-state files.

## Pull requests

A useful pull request should explain:

- what problem it solves;
- what user-visible behavior changes;
- how it was tested;
- whether persistence, audio output, playlist geometry or window behavior is affected.

By contributing code to this repository, you agree that your contribution may be distributed under the project's MIT License.
