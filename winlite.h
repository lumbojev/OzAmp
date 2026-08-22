#pragma once

#ifdef __clang__
#define WINAPI __stdcall
#define CALLBACK __stdcall
#else
#define WINAPI __stdcall
#define CALLBACK __stdcall
#endif

#define NULLPTR ((void*)0)

typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HDC;
typedef void* HFONT;
typedef void* HPEN;
typedef void* HBITMAP;
typedef void* HGDIOBJ;
typedef void* HMENU;
typedef void* HDROP;
typedef void* HGLOBAL;
typedef void* HMODULE;
typedef void* HMONITOR;
typedef void* HGDIOBJ;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long LONG;
typedef int BOOL;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long DWORD_PTR;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef unsigned short ATOM;
typedef wchar_t WCHAR;
typedef const WCHAR* LPCWSTR;
typedef WCHAR* LPWSTR;
typedef const void* LPCVOID;
typedef void* LPVOID;
typedef long long LONG_PTR;
typedef unsigned long long UINT_PTR;
typedef unsigned long long SIZE_T;
typedef unsigned long long ULONGLONG;
typedef long long LONGLONG;

typedef LRESULT (CALLBACK *WNDPROC)(HWND,UINT,WPARAM,LPARAM);

struct POINT { LONG x; LONG y; };
struct RECT { LONG left, top, right, bottom; };
struct WINDOWPOS { HWND hwnd; HWND hwndInsertAfter; int x; int y; int cx; int cy; UINT flags; };
struct MONITORINFO { DWORD cbSize; RECT rcMonitor; RECT rcWork; DWORD dwFlags; };
struct FILETIME { DWORD dwLowDateTime, dwHighDateTime; };
struct PAINTSTRUCT { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore; BOOL fIncUpdate; BYTE rgbReserved[32]; };
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; DWORD lPrivate; };
struct WNDCLASSEXW {
    UINT cbSize; UINT style; WNDPROC lpfnWndProc; int cbClsExtra; int cbWndExtra;
    HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; HBRUSH hbrBackground;
    LPCWSTR lpszMenuName; LPCWSTR lpszClassName; HICON hIconSm;
};
struct OPENFILENAMEW {
    DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance; LPCWSTR lpstrFilter; LPWSTR lpstrCustomFilter;
    DWORD nMaxCustFilter; DWORD nFilterIndex; LPWSTR lpstrFile; DWORD nMaxFile; LPWSTR lpstrFileTitle;
    DWORD nMaxFileTitle; LPCWSTR lpstrInitialDir; LPCWSTR lpstrTitle; DWORD Flags; WORD nFileOffset;
    WORD nFileExtension; LPCWSTR lpstrDefExt; LPARAM lCustData; void* lpfnHook; LPCWSTR lpTemplateName;
    void* pvReserved; DWORD dwReserved; DWORD FlagsEx;
};
struct BROWSEINFOW { HWND hwndOwner; void* pidlRoot; LPWSTR pszDisplayName; LPCWSTR lpszTitle; UINT ulFlags; void* lpfn; LPARAM lParam; int iImage; };
struct WIN32_FIND_DATAW {
    DWORD dwFileAttributes; FILETIME ftCreationTime; FILETIME ftLastAccessTime; FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh; DWORD nFileSizeLow; DWORD dwReserved0; DWORD dwReserved1;
    WCHAR cFileName[260]; WCHAR cAlternateFileName[14]; DWORD dwFileType; DWORD dwCreatorType; WORD wFinderFlags;
};
struct SIZEW { LONG cx; LONG cy; };
struct BITMAPW { LONG bmType; LONG bmWidth; LONG bmHeight; LONG bmWidthBytes; WORD bmPlanes; WORD bmBitsPixel; LPVOID bmBits; };
struct GDIPLUS_STARTUP_INPUT { UINT GdiplusVersion; void* DebugEventCallback; BOOL SuppressBackgroundThread; BOOL SuppressExternalCodecs; };

#define TRUE 1
#define FALSE 0
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define CS_HREDRAW 0x0002
#define CS_VREDRAW 0x0001
#define CS_DBLCLKS 0x0008
#define WS_POPUP 0x80000000UL
#define WS_CHILD 0x40000000UL
#define WS_BORDER 0x00800000UL
#define WS_TABSTOP 0x00010000UL
#define WS_OVERLAPPEDWINDOW 0x00CF0000UL
#define ES_AUTOHSCROLL 0x0080
#define BS_PUSHBUTTON 0x00000000UL
#define WS_VISIBLE 0x10000000UL
#define WS_MINIMIZEBOX 0x00020000UL
#define WS_CLIPCHILDREN 0x02000000UL
#define WS_EX_ACCEPTFILES 0x00000010UL
#define WS_EX_TOOLWINDOW 0x00000080UL
#define WS_EX_TOPMOST 0x00000008UL

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9

#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_SIZE 0x0005
#define WM_SETFOCUS 0x0007
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_ERASEBKGND 0x0014
#define WM_SETFONT 0x0030
#define WM_SETCURSOR 0x0020
#define WM_NCHITTEST 0x0084
#define WM_NCLBUTTONDBLCLK 0x00A3
#define WM_CONTEXTMENU 0x007B
#define WM_KEYDOWN 0x0100
#define WM_CHAR 0x0102
#define WM_TIMER 0x0113
#define WM_COMMAND 0x0111
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONUP 0x0205

#ifndef WM_SIZING
#define WM_SIZING 0x0214
#endif
#ifndef WM_MOVING
#define WM_MOVING 0x0216
#endif
#ifndef WM_ENTERSIZEMOVE
#define WM_ENTERSIZEMOVE 0x0231
#endif
#ifndef WM_EXITSIZEMOVE
#define WM_EXITSIZEMOVE 0x0232
#endif
#define WM_MOUSEWHEEL 0x020A
#define WM_DROPFILES 0x0233
#define WM_APPCOMMAND 0x0319
#define WM_SYSCOMMAND 0x0112

#define VK_SPACE 0x20
#define VK_CONTROL 0x11
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_F5 0x74
#define VK_F10 0x79
#define VK_RETURN 0x0D
#define VK_DELETE 0x2E
#define VK_ESCAPE 0x1B
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_HOME 0x24
#define VK_END 0x23
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_SHIFT 0x10
#define VK_F11 0x7A
#define VK_F12 0x7B

#define HTCLIENT 1
#define HTCAPTION 2

#define IDC_ARROW ((LPCWSTR)32512)
#define IDI_APPLICATION ((LPCWSTR)32512)

#define DT_LEFT 0x00000000
#define DT_CENTER 0x00000001
#define DT_RIGHT 0x00000002
#define DT_VCENTER 0x00000004
#define DT_SINGLELINE 0x00000020
#define DT_END_ELLIPSIS 0x00008000
#define DT_NOPREFIX 0x00000800
#define DT_WORDBREAK 0x00000010

#define TRANSPARENT 1
#define OPAQUE 2
#define PS_SOLID 0
#define FW_NORMAL 400
#define FW_SEMIBOLD 600
#define FW_BOLD 700
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define CLEARTYPE_QUALITY 5
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define SRCCOPY 0x00CC0020

#define OFN_EXPLORER 0x00080000
#define OFN_FILEMUSTEXIST 0x00001000
#define OFN_PATHMUSTEXIST 0x00000800
#define OFN_ALLOWMULTISELECT 0x00000200
#define OFN_HIDEREADONLY 0x00000004
#define OFN_OVERWRITEPROMPT 0x00000002
#define BIF_RETURNONLYFSDIRS 0x0001
#define BIF_NEWDIALOGSTYLE 0x0040
#define BIF_EDITBOX 0x0010

#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define GENERIC_READ 0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ 0x00000001
#define OPEN_EXISTING 3
#define CREATE_ALWAYS 2
#define FILE_BEGIN 0
#define FILE_END 2

#define CP_UTF8 65001
#define GMEM_MOVEABLE 0x0002
#define CF_UNICODETEXT 13

#define MF_STRING 0x0000
#define MF_SEPARATOR 0x0800
#define MF_CHECKED 0x0008
#define MF_GRAYED 0x0001
#define TPM_RETURNCMD 0x0100
#define TPM_RIGHTBUTTON 0x0002

#define HWND_TOPMOST ((HWND)(LONG_PTR)-1)
#define HWND_NOTOPMOST ((HWND)(LONG_PTR)-2)
#define SWP_NOMOVE 0x0002
#define SWP_NOSIZE 0x0001
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define MONITOR_DEFAULTTONEAREST 0x00000002

#define APPCOMMAND_MEDIA_NEXTTRACK 11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK 12
#define APPCOMMAND_MEDIA_STOP 13
#define APPCOMMAND_MEDIA_PLAY_PAUSE 14

extern "C" {
__declspec(dllimport) HINSTANCE WINAPI GetModuleHandleW(LPCWSTR);
__declspec(dllimport) LPWSTR WINAPI GetCommandLineW();
__declspec(dllimport) HGLOBAL WINAPI LocalFree(HGLOBAL);
__declspec(dllimport) void WINAPI ExitProcess(UINT);
__declspec(dllimport) ULONGLONG WINAPI GetTickCount64();
__declspec(dllimport) DWORD WINAPI GetEnvironmentVariableW(LPCWSTR,LPWSTR,DWORD);
__declspec(dllimport) BOOL WINAPI CreateDirectoryW(LPCWSTR,void*);
__declspec(dllimport) HANDLE WINAPI CreateFileW(LPCWSTR,DWORD,DWORD,void*,DWORD,DWORD,HANDLE);
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE,LPVOID,DWORD,DWORD*,void*);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE,LPCVOID,DWORD,DWORD*,void*);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
__declspec(dllimport) DWORD WINAPI SetFilePointer(HANDLE,LONG,LONG*,DWORD);
__declspec(dllimport) DWORD WINAPI GetFileSize(HANDLE,DWORD*);
__declspec(dllimport) DWORD WINAPI GetFileAttributesW(LPCWSTR);
__declspec(dllimport) HANDLE WINAPI FindFirstFileW(LPCWSTR,WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL WINAPI FindNextFileW(HANDLE,WIN32_FIND_DATAW*);
__declspec(dllimport) BOOL WINAPI FindClose(HANDLE);
__declspec(dllimport) int WINAPI WideCharToMultiByte(UINT,DWORD,LPCWSTR,int,char*,int,const char*,BOOL*);
__declspec(dllimport) int WINAPI MultiByteToWideChar(UINT,DWORD,const char*,int,LPWSTR,int);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HMODULE,LPWSTR,DWORD);
__declspec(dllimport) DWORD WINAPI GetPrivateProfileIntW(LPCWSTR,LPCWSTR,int,LPCWSTR);
__declspec(dllimport) DWORD WINAPI GetPrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,DWORD,LPCWSTR);
__declspec(dllimport) BOOL WINAPI WritePrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
__declspec(dllimport) HGLOBAL WINAPI GlobalAlloc(UINT,SIZE_T);
__declspec(dllimport) LPVOID WINAPI GlobalLock(HGLOBAL);
__declspec(dllimport) BOOL WINAPI GlobalUnlock(HGLOBAL);

__declspec(dllimport) ATOM WINAPI RegisterClassExW(const WNDCLASSEXW*);
__declspec(dllimport) HWND WINAPI CreateWindowExW(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID);
__declspec(dllimport) LRESULT WINAPI DefWindowProcW(HWND,UINT,WPARAM,LPARAM);
__declspec(dllimport) BOOL WINAPI ShowWindow(HWND,int);
__declspec(dllimport) BOOL WINAPI UpdateWindow(HWND);
__declspec(dllimport) BOOL WINAPI GetMessageW(MSG*,HWND,UINT,UINT);
__declspec(dllimport) BOOL WINAPI TranslateMessage(const MSG*);
__declspec(dllimport) LRESULT WINAPI DispatchMessageW(const MSG*);
__declspec(dllimport) void WINAPI PostQuitMessage(int);
__declspec(dllimport) HDC WINAPI BeginPaint(HWND,PAINTSTRUCT*);
__declspec(dllimport) BOOL WINAPI EndPaint(HWND,const PAINTSTRUCT*);
__declspec(dllimport) BOOL WINAPI GetClientRect(HWND,RECT*);
__declspec(dllimport) BOOL WINAPI GetWindowRect(HWND,RECT*);
__declspec(dllimport) BOOL WINAPI InvalidateRect(HWND,const RECT*,BOOL);
__declspec(dllimport) UINT_PTR WINAPI SetTimer(HWND,UINT_PTR,UINT,void*);
__declspec(dllimport) BOOL WINAPI KillTimer(HWND,UINT_PTR);
__declspec(dllimport) HCURSOR WINAPI LoadCursorW(HINSTANCE,LPCWSTR);
__declspec(dllimport) HICON WINAPI LoadIconW(HINSTANCE,LPCWSTR);
__declspec(dllimport) BOOL WINAPI SetCapture(HWND);
__declspec(dllimport) BOOL WINAPI ReleaseCapture();
__declspec(dllimport) BOOL WINAPI GetCursorPos(POINT*);
__declspec(dllimport) BOOL WINAPI ScreenToClient(HWND,POINT*);
__declspec(dllimport) BOOL WINAPI SetWindowPos(HWND,HWND,int,int,int,int,UINT);
__declspec(dllimport) int WINAPI SetWindowRgn(HWND,HANDLE,BOOL);
__declspec(dllimport) BOOL WINAPI MoveWindow(HWND,int,int,int,int,BOOL);
__declspec(dllimport) int WINAPI MessageBoxW(HWND,LPCWSTR,LPCWSTR,UINT);
__declspec(dllimport) int WINAPI DrawTextW(HDC,LPCWSTR,int,RECT*,UINT);
__declspec(dllimport) int WINAPI FillRect(HDC,const RECT*,HBRUSH);
__declspec(dllimport) int WINAPI SetBkMode(HDC,int);
__declspec(dllimport) DWORD WINAPI SetTextColor(HDC,DWORD);
__declspec(dllimport) DWORD WINAPI SetBkColor(HDC,DWORD);
__declspec(dllimport) BOOL WINAPI SetForegroundWindow(HWND);
__declspec(dllimport) HWND WINAPI SetFocus(HWND);
__declspec(dllimport) BOOL WINAPI DestroyWindow(HWND);
__declspec(dllimport) HMENU WINAPI CreatePopupMenu();
__declspec(dllimport) BOOL WINAPI AppendMenuW(HMENU,UINT,UINT_PTR,LPCWSTR);
__declspec(dllimport) UINT WINAPI TrackPopupMenu(HMENU,UINT,int,int,int,HWND,const RECT*);
__declspec(dllimport) BOOL WINAPI DestroyMenu(HMENU);
__declspec(dllimport) BOOL WINAPI OpenClipboard(HWND);
__declspec(dllimport) BOOL WINAPI EmptyClipboard();
__declspec(dllimport) HANDLE WINAPI SetClipboardData(UINT,HANDLE);
__declspec(dllimport) BOOL WINAPI CloseClipboard();
__declspec(dllimport) BOOL WINAPI SetProcessDPIAware();
__declspec(dllimport) BOOL WINAPI IsWindowVisible(HWND);
__declspec(dllimport) BOOL WINAPI SetWindowTextW(HWND,LPCWSTR);
__declspec(dllimport) int WINAPI GetWindowTextW(HWND,LPWSTR,int);
__declspec(dllimport) BOOL WINAPI EnableWindow(HWND,BOOL);
__declspec(dllimport) int WINAPI GetSystemMetrics(int);
__declspec(dllimport) HMONITOR WINAPI MonitorFromWindow(HWND,DWORD);
__declspec(dllimport) BOOL WINAPI GetMonitorInfoW(HMONITOR,MONITORINFO*);
__declspec(dllimport) LRESULT WINAPI SendMessageW(HWND,UINT,WPARAM,LPARAM);
__declspec(dllimport) short WINAPI GetKeyState(int);

__declspec(dllimport) HBRUSH WINAPI CreateSolidBrush(DWORD);
__declspec(dllimport) HPEN WINAPI CreatePen(int,int,DWORD);
__declspec(dllimport) HGDIOBJ WINAPI SelectObject(HDC,HGDIOBJ);
__declspec(dllimport) BOOL WINAPI DeleteObject(HGDIOBJ);
__declspec(dllimport) BOOL WINAPI Rectangle(HDC,int,int,int,int);
__declspec(dllimport) BOOL WINAPI RoundRect(HDC,int,int,int,int,int,int);
__declspec(dllimport) HANDLE WINAPI CreateRoundRectRgn(int,int,int,int,int,int);
__declspec(dllimport) BOOL WINAPI MoveToEx(HDC,int,int,POINT*);
__declspec(dllimport) BOOL WINAPI LineTo(HDC,int,int);
__declspec(dllimport) HFONT WINAPI CreateFontW(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
__declspec(dllimport) HDC WINAPI CreateCompatibleDC(HDC);
__declspec(dllimport) HBITMAP WINAPI CreateCompatibleBitmap(HDC,int,int);
__declspec(dllimport) BOOL WINAPI DeleteDC(HDC);
__declspec(dllimport) BOOL WINAPI BitBlt(HDC,int,int,int,int,HDC,int,int,DWORD);
__declspec(dllimport) BOOL WINAPI StretchBlt(HDC,int,int,int,int,HDC,int,int,int,int,DWORD);
__declspec(dllimport) BOOL WINAPI GetTextExtentPoint32W(HDC,LPCWSTR,int,SIZEW*);
__declspec(dllimport) int WINAPI GetObjectW(HGDIOBJ,int,LPVOID);

__declspec(dllimport) BOOL WINAPI GetOpenFileNameW(OPENFILENAMEW*);
__declspec(dllimport) BOOL WINAPI GetSaveFileNameW(OPENFILENAMEW*);

__declspec(dllimport) void WINAPI DragAcceptFiles(HWND,BOOL);
__declspec(dllimport) UINT WINAPI DragQueryFileW(HDROP,UINT,LPWSTR,UINT);
__declspec(dllimport) void WINAPI DragFinish(HDROP);
__declspec(dllimport) void* WINAPI SHBrowseForFolderW(BROWSEINFOW*);
__declspec(dllimport) BOOL WINAPI SHGetPathFromIDListW(const void*,LPWSTR);
__declspec(dllimport) HINSTANCE WINAPI ShellExecuteW(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,int);
__declspec(dllimport) LPWSTR* WINAPI CommandLineToArgvW(LPCWSTR,int*);

__declspec(dllimport) void WINAPI CoTaskMemFree(LPVOID);

__declspec(dllimport) DWORD WINAPI mciSendStringW(LPCWSTR,LPWSTR,UINT,HWND);
__declspec(dllimport) BOOL WINAPI mciGetErrorStringW(DWORD,LPWSTR,UINT);
}


extern "C" {
__declspec(dllimport) int WINAPI GdiplusStartup(ULONG_PTR*,const GDIPLUS_STARTUP_INPUT*,void*);
__declspec(dllimport) void WINAPI GdiplusShutdown(ULONG_PTR);
__declspec(dllimport) int WINAPI GdipLoadImageFromFile(LPCWSTR,void**);
__declspec(dllimport) int WINAPI GdipCreateHBITMAPFromBitmap(void*,HBITMAP*,DWORD);
__declspec(dllimport) int WINAPI GdipDisposeImage(void*);
}

static inline DWORD RGBc(BYTE r,BYTE g,BYTE b){ return ((DWORD)r)|((DWORD)g<<8)|((DWORD)b<<16); }
static inline int LOWORDi(LPARAM l){ return (short)(l & 0xffff); }
static inline int HIWORDi(LPARAM l){ return (short)((l>>16)&0xffff); }
static inline int GET_WHEEL_DELTA_WPARAM(WPARAM w){ return (short)((w>>16)&0xffff); }

// OzAmp 3.0 extended Win32/audio declarations.
typedef long HRESULT;
typedef unsigned long ULONG;
typedef unsigned long long ULONG64;
typedef long long REFERENCE_TIME;
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
struct GUID { DWORD Data1; WORD Data2; WORD Data3; BYTE Data4[8]; };
static_assert(sizeof(GUID)==16, "GUID ABI must match Windows");

// Windows mmreg.h is byte-packed (pshpack1.h). These structures MUST NOT use
// the compiler's default x64 alignment or WAVEFORMATEXTENSIBLE fields move by
// two bytes and WASAPI mix-format parsing becomes invalid.
#pragma pack(push,1)
struct WAVEFORMATEX { WORD wFormatTag; WORD nChannels; DWORD nSamplesPerSec; DWORD nAvgBytesPerSec; WORD nBlockAlign; WORD wBitsPerSample; WORD cbSize; };
struct MPEGLAYER3WAVEFORMAT { WAVEFORMATEX wfx; WORD wID; DWORD fdwFlags; WORD nBlockSize; WORD nFramesPerBlock; WORD nCodecDelay; };
#pragma pack(pop)
static_assert(sizeof(WAVEFORMATEX)==18, "WAVEFORMATEX must match Windows mmreg.h pack(1)");
static_assert(sizeof(MPEGLAYER3WAVEFORMAT)==30, "MPEGLAYER3WAVEFORMAT ABI must match Windows mmreg.h");

struct ACMSTREAMHEADER { DWORD cbStruct; DWORD fdwStatus; DWORD_PTR dwUser; BYTE* pbSrc; DWORD cbSrcLength; DWORD cbSrcLengthUsed; DWORD_PTR dwSrcUser; BYTE* pbDst; DWORD cbDstLength; DWORD cbDstLengthUsed; DWORD_PTR dwDstUser; DWORD dwReservedDriver[10]; };
struct PROPERTYKEY { GUID fmtid; DWORD pid; };

// PROPVARIANT is 24 bytes on Win64: 8-byte header + a 16-byte value union.
// The older 16-byte declaration could let COM overwrite adjacent stack memory
// in IPropertyStore / Media Foundation calls.
struct PROPVARIANT {
 WORD vt; WORD wReserved1,wReserved2,wReserved3;
 union {
  ULONGLONG raw64[2];
  LPWSTR pwszVal; ULONG ulVal; LONG lVal; ULONGLONG uhVal; void* punkVal;
 };
};
static_assert(sizeof(PROPVARIANT)==24, "PROPVARIANT Win64 ABI must be 24 bytes");
struct STARTUPINFOW { DWORD cb; LPWSTR lpReserved; LPWSTR lpDesktop; LPWSTR lpTitle; DWORD dwX,dwY,dwXSize,dwYSize,dwXCountChars,dwYCountChars,dwFillAttribute,dwFlags; WORD wShowWindow,cbReserved2; BYTE* lpReserved2; HANDLE hStdInput,hStdOutput,hStdError; };
struct PROCESS_INFORMATION { HANDLE hProcess; HANDLE hThread; DWORD dwProcessId; DWORD dwThreadId; };

#define S_OK ((HRESULT)0)
#define FAILED(hr) ((HRESULT)(hr)<0)
#define SUCCEEDED(hr) ((HRESULT)(hr)>=0)
#define COINIT_MULTITHREADED 0x0
#define CLSCTX_INPROC_SERVER 1
#define CLSCTX_ALL 23
#define STGM_READ 0
#define DEVICE_STATE_ACTIVE 0x00000001
#define WAVE_FORMAT_PCM 0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#define AUDCLNT_SHAREMODE_SHARED 0
#define AUDCLNT_BUFFERFLAGS_SILENT 0x2
#define ACM_STREAMOPENF_NONREALTIME 0x00000004
#define ACM_STREAMSIZEF_SOURCE 0x00000000
#define ACM_STREAMCONVERTF_BLOCKALIGN 0x00000004
#define ACM_STREAMCONVERTF_START 0x00000010
#define ACM_STREAMCONVERTF_END 0x00000020
#define HEAP_ZERO_MEMORY 0x00000008
#define CREATE_NO_WINDOW 0x08000000
#define STARTF_USESHOWWINDOW 0x00000001
#define SW_HIDE 0
#define INFINITE 0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define VT_LPWSTR 31
#define WM_HOTKEY 0x0312
#define MOD_ALT 0x0001
#define MOD_CONTROL 0x0002
#define MOD_NOREPEAT 0x4000
#define NIM_ADD 0x00000000
#define NIM_MODIFY 0x00000001
#define NIM_DELETE 0x00000002
#define NIF_MESSAGE 0x00000001
#define NIF_ICON 0x00000002
#define NIF_TIP 0x00000004
#define NIF_INFO 0x00000010
#define NIIF_INFO 0x00000001
#define WM_APP 0x8000
#define MF_POPUP 0x0010
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define BN_CLICKED 0

struct NOTIFYICONDATAW {
 DWORD cbSize; HWND hWnd; UINT uID; UINT uFlags; UINT uCallbackMessage; HICON hIcon; WCHAR szTip[128];
 DWORD dwState; DWORD dwStateMask; WCHAR szInfo[256]; union { UINT uTimeout; UINT uVersion; }; WCHAR szInfoTitle[64]; DWORD dwInfoFlags;
 GUID guidItem; HICON hBalloonIcon;
};

extern "C" {
__declspec(dllimport) HANDLE WINAPI GetProcessHeap();
__declspec(dllimport) LPVOID WINAPI HeapAlloc(HANDLE,DWORD,SIZE_T);
__declspec(dllimport) BOOL WINAPI HeapFree(HANDLE,DWORD,LPVOID);
__declspec(dllimport) LPVOID WINAPI HeapReAlloc(HANDLE,DWORD,LPVOID,SIZE_T);
__declspec(dllimport) HANDLE WINAPI CreateThread(void*,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,DWORD*);
__declspec(dllimport) void WINAPI Sleep(DWORD);
__declspec(dllimport) BOOL WINAPI DeleteFileW(LPCWSTR);
__declspec(dllimport) BOOL WINAPI CreateProcessW(LPCWSTR,LPWSTR,void*,void*,BOOL,DWORD,LPVOID,LPCWSTR,STARTUPINFOW*,PROCESS_INFORMATION*);
__declspec(dllimport) DWORD WINAPI WaitForSingleObject(HANDLE,DWORD);
__declspec(dllimport) HRESULT WINAPI CoInitializeEx(LPVOID,DWORD);
__declspec(dllimport) void WINAPI CoUninitialize();
__declspec(dllimport) HRESULT WINAPI CoCreateInstance(const GUID*,LPVOID,DWORD,const GUID*,LPVOID*);
__declspec(dllimport) HRESULT WINAPI PropVariantClear(PROPVARIANT*);
__declspec(dllimport) BOOL WINAPI RegisterHotKey(HWND,int,UINT,UINT);
__declspec(dllimport) BOOL WINAPI UnregisterHotKey(HWND,int);
__declspec(dllimport) BOOL WINAPI Shell_NotifyIconW(DWORD,NOTIFYICONDATAW*);
__declspec(dllimport) UINT WINAPI acmStreamOpen(void**,void*,WAVEFORMATEX*,WAVEFORMATEX*,void*,DWORD_PTR,DWORD_PTR,DWORD);
__declspec(dllimport) UINT WINAPI acmStreamClose(void*,DWORD);
__declspec(dllimport) UINT WINAPI acmStreamSize(void*,DWORD,DWORD*,DWORD);
__declspec(dllimport) UINT WINAPI acmStreamPrepareHeader(void*,ACMSTREAMHEADER*,DWORD);
__declspec(dllimport) UINT WINAPI acmStreamUnprepareHeader(void*,ACMSTREAMHEADER*,DWORD);
__declspec(dllimport) UINT WINAPI acmStreamConvert(void*,ACMSTREAMHEADER*,DWORD);
__declspec(dllimport) double __cdecl sin(double);
__declspec(dllimport) double __cdecl cos(double);
__declspec(dllimport) double __cdecl sqrt(double);
__declspec(dllimport) double __cdecl log10(double);
__declspec(dllimport) double __cdecl pow(double,double);
__declspec(dllimport) double __cdecl fabs(double);
}

// Media Foundation decoding (OzAmp 3.0).
#define MF_VERSION 0x00020070
#define MFSTARTUP_FULL 0
#define MF_SOURCE_READER_FIRST_AUDIO_STREAM 0xFFFFFFFDu
#define MF_SOURCE_READER_ALL_STREAMS 0xFFFFFFFEu
#define MF_SOURCE_READERF_ERROR 0x00000001u
#define MF_SOURCE_READERF_ENDOFSTREAM 0x00000002u
extern "C" {
__declspec(dllimport) HRESULT WINAPI MFStartup(ULONG,DWORD);
__declspec(dllimport) HRESULT WINAPI MFShutdown();
__declspec(dllimport) HRESULT WINAPI MFCreateMediaType(void**);
__declspec(dllimport) HRESULT WINAPI MFCreateSourceReaderFromURL(LPCWSTR,void*,void**);
}
