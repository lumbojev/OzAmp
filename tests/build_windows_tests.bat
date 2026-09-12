@echo off
setlocal
cd /d "%~dp0.."
clang-cl --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Fospotify_storage_tests.obj tests\spotify_storage_windows.cpp || exit /b 1
lld-link spotify_storage_tests.obj audio_engine.obj ozamp.res kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib winmm.lib msacm32.lib gdiplus.lib msvcrt.lib mfplat.lib mfreadwrite.lib advapi32.lib winhttp.lib ws2_32.lib crypt32.lib /machine:x64 /subsystem:console /entry:SpotifyStorageTests /out:spotify_storage_tests.exe || exit /b 1
spotify_storage_tests.exe
if errorlevel 1 (echo Spotify DPAPI/file integration test failed with code %errorlevel%. & exit /b 1)
echo PASS: DPAPI round trip, Client ID binding, atomic replacement failure, rotation, deletion, corrupt ciphertext.
