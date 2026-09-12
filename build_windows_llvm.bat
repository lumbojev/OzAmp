@echo off
setlocal
cd /d "%~dp0"
del /q main.obj audio_engine.obj OzAmp-1.0.1.exe 2>nul
where clang-cl >nul 2>nul || (echo ERROR: clang-cl not found. Install LLVM for Windows. & exit /b 1)
where lld-link >nul 2>nul || (echo ERROR: lld-link not found. Install LLVM for Windows. & exit /b 1)

rem Build the application icon resource. Python is normally available on GitHub Actions;
rem a checked-in ozamp.res is also included as a fallback for local builds.
where python >nul 2>nul && python tools\make_icon_res.py ozamp.ico ozamp.res
if not exist ozamp.res (echo ERROR: ozamp.res missing. Install Python or regenerate the icon resource. & exit /b 1)

rem Minimal import libraries used by OzAmp's raw Win32 build.
for %%D in (kernel32 user32 gdi32 comdlg32 shell32 ole32 winmm msacm32 gdiplus msvcrt mfplat mfreadwrite advapi32 winhttp ws2_32 crypt32) do (
  lld-link /machine:x64 /dll /noentry /def:%%D.def /out:%%D_stub.dll /implib:%%D.lib >nul || exit /b 1
)

clang-cl --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Foaudio_engine.obj audio_engine.cpp || exit /b 1
clang-cl --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Fomain.obj main.cpp || exit /b 1
lld-link main.obj audio_engine.obj ozamp.res kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib winmm.lib msacm32.lib gdiplus.lib msvcrt.lib mfplat.lib mfreadwrite.lib advapi32.lib winhttp.lib ws2_32.lib crypt32.lib /machine:x64 /subsystem:windows /entry:WinMainCRTStartup /out:OzAmp-1.0.1.exe || exit /b 1

del /q *_stub.dll 2>nul

echo Built OzAmp-1.0.1.exe
