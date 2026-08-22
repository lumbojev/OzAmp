# OzAmp 1.0.0 GitHub release checklist

- [x] Add MIT `LICENSE`.
- [x] Clean source tree of build outputs and runtime state.
- [x] Update repository documentation for 1.0.0.
- [x] Configure Windows x64 GitHub Actions build.
- [ ] Run `TEST_CHECKLIST.md` on the final Windows 11 build.
- [ ] Push the clean source tree to the `main` branch.
- [ ] Confirm the `build-windows` GitHub Actions workflow passes.
- [ ] Create tag `v1.0.0` from the tested `main` commit.
- [ ] Create a GitHub Release titled `OzAmp 1.0.0`.
- [ ] Attach `OzAmp-1.0.0.exe` and `OzAmp-1.0.0-SHA256.txt`.
- [ ] Paste the contents of `GITHUB_RELEASE_v1.0.0.md` into the release description.
- [ ] Mark the release as **Latest** and ensure **Pre-release** is disabled.
