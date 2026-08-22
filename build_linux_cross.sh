#!/bin/bash
set -e
cd "$(dirname "$0")"
for D in kernel32 user32 gdi32 comdlg32 shell32 ole32 winmm msacm32 gdiplus msvcrt mfplat mfreadwrite; do
  /usr/local/swift/usr/bin/lld-link /machine:x64 /dll /noentry /def:${D}.def /out:${D}_stub.dll /implib:${D}.lib >/dev/null
done
/usr/local/swift/usr/bin/clang-cl --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Fo:audio_engine.obj audio_engine.cpp
/usr/local/swift/usr/bin/clang-cl --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Fo:main.obj main.cpp
/usr/local/swift/usr/bin/lld-link main.obj audio_engine.obj kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib winmm.lib msacm32.lib gdiplus.lib msvcrt.lib mfplat.lib mfreadwrite.lib /machine:x64 /subsystem:windows /entry:WinMainCRTStartup /out:OzAmp-1.0.0.exe
