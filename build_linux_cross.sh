#!/bin/bash
set -e
cd "$(dirname "$0")"
if [[ -n ${CLANG_CL:-} ]]; then
  compiler=("$CLANG_CL")
elif command -v clang-cl >/dev/null; then
  compiler=(clang-cl)
else
  compiler=(clang --driver-mode=cl)
fi
linker=${LLD_LINK:-lld-link}
rm -f -- main.obj audio_engine.obj OzAmp-1.0.1.exe
python3 tools/make_icon_res.py ozamp.ico ozamp.res
for D in kernel32 user32 gdi32 comdlg32 shell32 ole32 winmm msacm32 gdiplus msvcrt mfplat mfreadwrite advapi32 winhttp ws2_32 crypt32; do
  "$linker" /machine:x64 /dll /noentry /def:${D}.def /out:${D}_stub.dll /implib:${D}.lib >/dev/null
done
"${compiler[@]}" --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Foaudio_engine.obj audio_engine.cpp
"${compiler[@]}" --target=x86_64-pc-windows-msvc /c /GS- /Gs9999999 /Zl /EHs-c- /GR- /O2 /Ob0 /Fomain.obj main.cpp
"$linker" main.obj audio_engine.obj ozamp.res kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib winmm.lib msacm32.lib gdiplus.lib msvcrt.lib mfplat.lib mfreadwrite.lib advapi32.lib winhttp.lib ws2_32.lib crypt32.lib /machine:x64 /subsystem:windows /entry:WinMainCRTStartup /out:OzAmp-1.0.1.exe

rm -f -- *_stub.dll
