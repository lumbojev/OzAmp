#include "winlite.h"
#include "audio_engine.h"

// OzAmp 1.0.0 - compact native C++/Win32 music player.
// Custom-drawn UI, native WASAPI/PCM playback, DirectShow compatibility for ordinary audio, MCI for MIDI, no network code.

extern "C" void* memset(void* p, int v, SIZE_T n){ unsigned char* d=(unsigned char*)p; while(n--) *d++=(unsigned char)v; return p; }
extern "C" void* memcpy(void* d0,const void* s0,SIZE_T n){ unsigned char*d=(unsigned char*)d0; const unsigned char*s=(const unsigned char*)s0; while(n--) *d++=*s++; return d0; }
extern "C" int memcmp(const void* a0,const void* b0,SIZE_T n){ const unsigned char*a=(const unsigned char*)a0,*b=(const unsigned char*)b0; while(n--){ if(*a!=*b) return *a-*b; a++; b++; } return 0; }
extern "C" int _fltused=0;

static const wchar_t* APP=L"OzAmp";
static const wchar_t* VER=L"1.0.0";
static inline LONG AtomicExchange(volatile LONG* p, LONG v){ return __atomic_exchange_n(p,v,__ATOMIC_SEQ_CST); }
static const int MAIN_W=500, MAIN_H=182, SHADE_H=34, PL_DEFAULT_W=500, PL_DEFAULT_H=300, PL_MIN_W=420, PL_MIN_H=160;
static const int EQ_W=500, EQ_H=188, SETTINGS_W=620, SETTINGS_H=440, ABOUT_W=500, ABOUT_H=330, INFO_W=560, INFO_H=360;
static const int MAX_TRACKS=2048, MAX_LIBRARY=8192, MAXP=512, MAXD=384;

struct Track { wchar_t path[MAXP]; wchar_t display[MAXD]; wchar_t title[160]; wchar_t artist[128]; wchar_t album[128]; wchar_t genre[64]; wchar_t year[16]; double replayGainDb; double peak; int playCount; int rating; int lengthMs; int addedOrder; int bitrateKbps; int sampleRateHz; int channelMode; int id3v2Major; int id3v2Minor; bool hasId3v1; bool marked; bool bookmark; };
struct Hit { int x1,y1,x2,y2,id; };

enum HitId {
 H_NONE=0,H_MIN,H_CLOSE,H_PREV,H_PLAY,H_PAUSE,H_STOP,H_NEXT,H_EJECT,H_SHUFFLE,H_REPEAT,H_AB,H_PL,H_VOL,H_BAL,H_SEEK,H_TITLE,
 H_PL_CLOSE,H_PL_ADD,H_PL_DIR,H_PL_REMOVE,H_PL_CLEAR,H_PL_LOAD,H_PL_SAVE,H_PL_LIST,H_PL_SCROLL,H_PL_SEARCH,H_TIME,H_VIS,H_MUTE,H_EQ,H_SETTINGS
};

static HINSTANCE g_inst=0; static HWND g_main=0,g_pl=0,g_eq=0,g_lib=0,g_art=0,g_viz=0,g_settings=0,g_about=0,g_info=0,g_error=0;
static HFONT g_font=0,g_small=0,g_bold=0,g_led=0;
static Track g_tracks[MAX_TRACKS]; static int g_count=0,g_current=-1,g_selected=-1,g_scroll=0;
static Track g_library[MAX_LIBRARY]; static int g_libCount=0,g_libSelected=-1,g_libScroll=0,g_libView=0,g_libraryAddSerial=0; static wchar_t g_libSearch[96]=L"";
static bool g_playing=false,g_paused=false,g_shuffle=false,g_top=false,g_plVisible=true,g_eqVisible=false,g_libVisible=false,g_artVisible=false,g_vizVisible=false,g_shade=false,g_muted=false,g_timeRemaining=false,g_doubleSize=false,g_nativeAudio=false;
static int g_skin=0; // 0 Midnight Ice, 1 Classic LCD, 2 Nordic Blue
static int g_repeat=0; // 0 off,1 all,2 one
static int g_volume=78,g_balance=0,g_preMuteVolume=78; // balance -100..100
static bool g_eqEnabled=false; static int g_preampDb=0; static int g_eqBands[10]={0,0,0,0,0,0,0,0,0,0}; static int g_eqDrag=-99;
static int g_eqPreset=0; static bool g_eqCustomSaved=false,g_eqCustomDirty=false; static int g_eqCustomPreamp=0,g_eqCustomBands[10]={0,0,0,0,0,0,0,0,0,0};
static int g_hoverHit=H_NONE,g_eqHover=0,g_plHover=0;
static int g_pressedHit=H_NONE,g_eqPressed=0,g_plPressed=0;
static wchar_t g_feedback[96]=L"";
static ULONGLONG g_feedbackUntil=0;
static int g_length=0,g_pos=0; static wchar_t g_mode[32]=L"READY";
static int g_abState=0,g_a=0,g_b=0; static ULONGLONG g_sleepUntil=0; static int g_sleepMin=0; static bool g_sleepAfterCurrent=false;
static int g_crossfadeSec=4,g_preparedNext=-1; static bool g_gapless=true,g_replayGainEnabled=true,g_trackNotify=true;
static int g_marquee=0,g_visMode=0,g_queued=-1; static unsigned long long g_rng=0x0A2A4D5E12345678ULL;
static wchar_t g_queuePaths[128][MAXP]; static int g_queueCount=0;
static wchar_t g_plFilter[96]=L""; static bool g_plFilterActive=false; static int g_filterMap[MAX_TRACKS]; static int g_filterCount=0; static int g_markAnchor=-1;
static Track g_reorderTemp[MAX_TRACKS];
static int g_actionFlashHit=H_NONE; static ULONGLONG g_actionFlashUntil=0;
static int g_restoreMainX=140,g_restoreMainY=120,g_restorePlX=140,g_restorePlY=340,g_restoreEqX=650,g_restoreEqY=120,g_restoreLibX=140,g_restoreLibY=430,g_restoreArtX=650,g_restoreArtY=300,g_restoreVizX=220,g_restoreVizY=220;
static int g_errorTrack=-1; static wchar_t g_errorTitle[96]=L"",g_errorBody[420]=L"";
static ULONGLONG g_lastDeviceCheck=0; static wchar_t g_preferredDeviceId[256]=L"";
static bool g_jump=false,g_ignoreJumpChar=false; static wchar_t g_jumpText[96]=L""; static int g_restoreIndex=-1,g_restorePos=0,g_restoreState=0;
static bool g_dragSeek=false,g_dragVol=false,g_dragBal=false;
static int g_plW=PL_DEFAULT_W,g_plH=PL_DEFAULT_H,g_plDockEdge=1,g_plDockTarget=0,g_plDockAlign=2,g_plDockOffset=0,g_plRightAnchorTarget=0,g_plResize=0; // edge: 0 free,1 bottom,2 top,3 right,4 left; target: 0 main,1 EQ; align: 0 slide,1 left,2 right
static bool g_plWindowDrag=false; static POINT g_plWindowDragStart={0,0}; static RECT g_plWindowDragRect={0,0,0,0}; static int g_plWindowDragLastRawRight=0;
static POINT g_plResizeStart={0,0}; static RECT g_plResizeStartRect={0,0,0,0}; static int g_plResizeStartW=0,g_plResizeStartH=0,g_plResizeLastRawRight=0; static bool g_dockMove=false;
static wchar_t g_appDir[MAXP],g_dataDir[MAXP],g_ini[MAXP],g_session[MAXP],g_statsIni[MAXP],g_libraryFile[MAXP],g_skinFile[MAXP],g_deviceId[256];
static wchar_t g_playlistFile[MAXP];
static bool g_globalHotkeys=true,g_trayAdded=false,g_scanRunning=false; static volatile LONG g_scanProgress=0,g_scanTotal=0,g_scanDone=0;
static int g_tickCounter=0;
static Track g_undo[64]; static int g_undoCount=0,g_undoAt=0;
static int g_plDragIndex=-1,g_plDragTarget=-1; static bool g_plDrag=false,g_plScrollDrag=false; static int g_plScrollGrab=0;
static ULONG_PTR g_gdiplusToken=0; static HBITMAP g_coverBmp=0; static wchar_t g_coverPath[MAXP]=L"";
static bool g_vizFull=false; static HWND g_vizFullWnd=0; static RECT g_vizRestore={0,0,800,450};
static int g_eqDockEdge=3,g_eqDockTarget=0,g_libDockEdge=1,g_artDockEdge=3,g_vizDockEdge=0; static POINT g_lastMainPos={0,0};
static void* g_taskbar=0;
static void* g_dsGraph=0,*g_dsControl=0,*g_dsSeek=0,*g_dsAudio=0; static bool g_dsActive=false;
static HWND g_tag=0,g_tagTitle=0,g_tagArtist=0,g_tagAlbum=0,g_tagGenre=0,g_tagYear=0; static int g_tagIndex=-1;
static HWND g_toast=0; static ULONGLONG g_toastUntil=0;
static int g_settingsTab=0,g_settingsDeviceScroll=0,g_settingsHover=0,g_settingsPendingDevice=-1; static wchar_t g_outputSwitchInfo[192]=L""; static HRESULT g_outputSwitchHr=S_OK;
static bool g_aboutEgg=false; static int g_aboutLogoClicks=0; static ULONGLONG g_aboutLogoClickUntil=0; static int g_infoIndex=-1;

static DWORD C_BG=0,C_PANEL=0,C_PANEL2=0,C_EDGE=0,C_TEXT=0,C_MUTED=0,C_ACCENT=0,C_LED=0,C_LED2=0,C_RED=0,C_BLACK=0;static bool g_externalSkin=false;static wchar_t g_fontName[64]=L"Segoe UI",g_ledFontName[64]=L"Consolas";


static void InitColors();
static void ToggleDoubleSize();
static void NotifyTrack();
static void LoadCoverForCurrent();
static void SnapTool(HWND,int&);
static void DockPlaylist();
static void DockEQ();
static void SnapEQ();
static bool SaveLibrary();
static void LoadLibrary();
static LRESULT CALLBACK TagProc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK ToastProc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK SettingsProc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK AboutProc(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK InfoProc(HWND,UINT,WPARAM,LPARAM);
static void ToggleSettings();
static void ToggleAbout();
static void ShowTrackInfo(int);
static void RoundWindow(HWND,int);
static void ShowTrackToast();
static void ShowOzError(const wchar_t*,const wchar_t*,int);
static LRESULT CALLBACK ErrorProc(HWND,UINT,WPARAM,LPARAM);
static void SaveWindowPositions();
static void RestoreWindowPositions();
static void SyncQueuedIndex();
static void QueueAddPath(const wchar_t*,bool);
static void QueueRemovePath(const wchar_t*);
static int QueueFirstIndex();
static void QueuePopFirst();
static void ClearMarks();
static void RememberIndices(wchar_t*,wchar_t*,wchar_t*);
static void RestoreIndices(const wchar_t*,const wchar_t*,const wchar_t*);
static bool FileExists(const wchar_t*);

static int WLen(const wchar_t*s){ int n=0; if(!s)return 0; while(s[n])n++; return n; }
static void WCopy(wchar_t*d,const wchar_t*s,int cap){ if(cap<1)return; int i=0; if(s) while(s[i]&&i<cap-1){d[i]=s[i];i++;} d[i]=0; }
static void WCat(wchar_t*d,const wchar_t*s,int cap){ int n=WLen(d),i=0; if(!s)return; while(s[i]&&n<cap-1)d[n++]=s[i++]; d[n]=0; }
static bool WEqI(const wchar_t*a,const wchar_t*b){ int i=0; for(;;i++){ wchar_t x=a[i],y=b[i]; if(x>='A'&&x<='Z')x+=32; if(y>='A'&&y<='Z')y+=32; if(x!=y)return false; if(!x)return true; } }
static bool WEndsI(const wchar_t*s,const wchar_t*e){ int a=WLen(s),b=WLen(e); if(b>a)return false; return WEqI(s+a-b,e); }
static int WToInt(const wchar_t*s){ int n=0; if(!s)return 0; while(*s>='0'&&*s<='9'){n=n*10+(*s-'0');s++;} return n; }
static void IntToW(int v,wchar_t*out){ wchar_t t[32]; int n=0; if(v==0){out[0]=L'0';out[1]=0;return;} bool neg=v<0; if(neg)v=-v; while(v){t[n++]=(wchar_t)(L'0'+v%10);v/=10;} int p=0;if(neg)out[p++]=L'-';while(n)out[p++]=t[--n];out[p]=0; }
static void Hex32(DWORD v,wchar_t*out){static const wchar_t*h=L"0123456789ABCDEF";out[0]=L'0';out[1]=L'x';for(int i=0;i<8;i++)out[2+i]=h[(v>>(28-i*4))&15];out[10]=0;}
static void FormatTime(int ms,wchar_t*out){ int s=ms/1000,m=s/60; s%=60; int p=0; if(m>=100){ wchar_t t[16];IntToW(m,t);WCopy(out,t,32);p=WLen(out);} else {out[p++]=(wchar_t)(L'0'+(m/10)%10);out[p++]=(wchar_t)(L'0'+m%10);} out[p++]=L':';out[p++]=(wchar_t)(L'0'+s/10);out[p++]=(wchar_t)(L'0'+s%10);out[p]=0; }
static bool PtIn(int x,int y,int x1,int y1,int x2,int y2){return x>=x1&&x<x2&&y>=y1&&y<y2;}
static int Clamp(int v,int a,int b){return v<a?a:(v>b?b:v);}
static int MaxI(int a,int b){return a>b?a:b;}
static int MinI(int a,int b){return a<b?a:b;}
static int UIScale(){return g_doubleSize?2:1;}
static int LX(int x){return x/UIScale();}
static int LY(int y){return y/UIScale();}
static bool CtrlDown(){return (GetKeyState(VK_CONTROL)&0x8000)!=0;}
static bool ShiftDown(){return (GetKeyState(VK_SHIFT)&0x8000)!=0;}
static int AbsI(int v){return v<0?-v:v;}
static bool Overlap1D(int a1,int a2,int b1,int b2){return a1<b2&&b1<a2;}

static wchar_t LowerW(wchar_t c){if(c>=L'A'&&c<=L'Z')return c+32;return c;}
static int WCompareI(const wchar_t*a,const wchar_t*b){int i=0;for(;;i++){wchar_t x=LowerW(a[i]),y=LowerW(b[i]);if(x<y)return -1;if(x>y)return 1;if(!x)return 0;}}
static bool WContainsI(const wchar_t*s,const wchar_t*q){if(!q||!q[0])return true;int ns=WLen(s),nq=WLen(q);for(int i=0;i+nq<=ns;i++){bool ok=true;for(int j=0;j<nq;j++)if(LowerW(s[i+j])!=LowerW(q[j])){ok=false;break;}if(ok)return true;}return false;}
static void TrimW(wchar_t*s){int n=WLen(s);while(n>0&&(s[n-1]==L' '||s[n-1]==L'\t'||s[n-1]==L'\r'||s[n-1]==L'\n'||s[n-1]==0))s[--n]=0;int a=0;while(s[a]==L' '||s[a]==L'\t')a++;if(a){int i=0;while(s[a])s[i++]=s[a++];s[i]=0;}}

static unsigned long long PathHash(const wchar_t*s){unsigned long long h=1469598103934665603ULL;for(int i=0;s&&s[i];i++){unsigned v=(unsigned)LowerW(s[i]);h^=(BYTE)(v&255);h*=1099511628211ULL;h^=(BYTE)((v>>8)&255);h*=1099511628211ULL;}return h;}
static void Hex64(unsigned long long v,wchar_t*out){static const wchar_t*H=L"0123456789ABCDEF";for(int i=0;i<16;i++){out[15-i]=H[v&15];v>>=4;}out[16]=0;}
static void DoubleToW(double v,wchar_t*out){bool neg=v<0;if(neg)v=-v;int q=(int)(v*100.0+0.5);wchar_t a[24];IntToW(q/100,a);int p=0;if(neg)out[p++]=L'-';for(int i=0;a[i];i++)out[p++]=a[i];out[p++]=L'.';out[p++]=(wchar_t)(L'0'+(q/10)%10);out[p++]=(wchar_t)(L'0'+q%10);out[p]=0;}
static int HexNib(wchar_t c){if(c>=L'0'&&c<=L'9')return c-L'0';if(c>=L'a'&&c<=L'f')return c-L'a'+10;if(c>=L'A'&&c<=L'F')return c-L'A'+10;return -1;}
static bool ParseColor(const wchar_t*s,DWORD&c){if(!s)return false;if(*s==L'#')s++;if(WLen(s)!=6)return false;int n[6];for(int i=0;i<6;i++){n[i]=HexNib(s[i]);if(n[i]<0)return false;}c=RGBc((BYTE)(n[0]*16+n[1]),(BYTE)(n[2]*16+n[3]),(BYTE)(n[4]*16+n[5]));return true;}
static void StatsKey(const wchar_t*path,wchar_t*k){Hex64(PathHash(path),k);}
static void LoadTrackStats(Track&t){wchar_t k[24],v[192];StatsKey(t.path,k);t.replayGainDb=0;t.peak=0;t.playCount=0;t.rating=0;t.lengthMs=0;t.addedOrder=-1;t.bookmark=false;t.marked=false;GetPrivateProfileStringW(k,L"rg",L"0",v,192,g_statsIni);int rg=WToInt(v+(v[0]=='-'?1:0));if(v[0]=='-')rg=-rg;t.replayGainDb=rg/100.0;GetPrivateProfileStringW(k,L"peak",L"0",v,192,g_statsIni);t.peak=WToInt(v)/10000.0;t.playCount=(int)GetPrivateProfileIntW(k,L"plays",0,g_statsIni);t.rating=Clamp((int)GetPrivateProfileIntW(k,L"rating",0,g_statsIni),0,5);t.lengthMs=(int)GetPrivateProfileIntW(k,L"length",0,g_statsIni);t.bookmark=GetPrivateProfileIntW(k,L"bookmark",0,g_statsIni)!=0;t.addedOrder=(int)GetPrivateProfileIntW(k,L"added",-1,g_statsIni);GetPrivateProfileStringW(k,L"title",L"",v,192,g_statsIni);if(v[0])WCopy(t.title,v,160);GetPrivateProfileStringW(k,L"artist",L"",v,192,g_statsIni);if(v[0])WCopy(t.artist,v,128);GetPrivateProfileStringW(k,L"album",L"",v,192,g_statsIni);if(v[0])WCopy(t.album,v,128);GetPrivateProfileStringW(k,L"genre",L"",v,192,g_statsIni);if(v[0])WCopy(t.genre,v,64);GetPrivateProfileStringW(k,L"year",L"",v,192,g_statsIni);if(v[0])WCopy(t.year,v,16);if(t.title[0]&&t.artist[0]){WCopy(t.display,t.artist,MAXD);WCat(t.display,L" - ",MAXD);WCat(t.display,t.title,MAXD);}else if(t.title[0])WCopy(t.display,t.title,MAXD);}
static void SaveTrackStats(const Track&t){wchar_t k[24],v[64];StatsKey(t.path,k);IntToW((int)(t.replayGainDb*100.0),v);WritePrivateProfileStringW(k,L"rg",v,g_statsIni);IntToW((int)(t.peak*10000.0),v);WritePrivateProfileStringW(k,L"peak",v,g_statsIni);IntToW(t.playCount,v);WritePrivateProfileStringW(k,L"plays",v,g_statsIni);IntToW(t.rating,v);WritePrivateProfileStringW(k,L"rating",v,g_statsIni);IntToW(t.lengthMs,v);WritePrivateProfileStringW(k,L"length",v,g_statsIni);WritePrivateProfileStringW(k,L"bookmark",t.bookmark?L"1":L"0",g_statsIni);if(t.addedOrder>=0){IntToW(t.addedOrder,v);WritePrivateProfileStringW(k,L"added",v,g_statsIni);}WritePrivateProfileStringW(k,L"title",t.title,g_statsIni);WritePrivateProfileStringW(k,L"artist",t.artist,g_statsIni);WritePrivateProfileStringW(k,L"album",t.album,g_statsIni);WritePrivateProfileStringW(k,L"genre",t.genre,g_statsIni);WritePrivateProfileStringW(k,L"year",t.year,g_statsIni);}

static const wchar_t* BaseName(const wchar_t*p){ const wchar_t*b=p; for(int i=0;p&&p[i];i++)if(p[i]=='\\'||p[i]=='/')b=p+i+1; return b; }
static void DirName(const wchar_t*p,wchar_t*out,int cap){ WCopy(out,p,cap); int n=WLen(out); while(n>0&&out[n-1]!='\\'&&out[n-1]!='/')n--; if(n>0)out[n-1]=0; else out[0]=0; }
static bool Supported(const wchar_t*p){ return WEndsI(p,L".mp3")||WEndsI(p,L".wav")||WEndsI(p,L".flac")||WEndsI(p,L".ogg")||WEndsI(p,L".oga")||WEndsI(p,L".opus")||WEndsI(p,L".wma")||WEndsI(p,L".mid")||WEndsI(p,L".midi")||WEndsI(p,L".aac")||WEndsI(p,L".m4a")||WEndsI(p,L".mp4")||WEndsI(p,L".alac")||WEndsI(p,L".aif")||WEndsI(p,L".aiff"); }
static bool IsDir(const wchar_t*p){ DWORD a=GetFileAttributesW(p); return a!=0xffffffffUL && (a&FILE_ATTRIBUTE_DIRECTORY)!=0; }

static void FileNameDisplay(const wchar_t*path,wchar_t*out){
 WCopy(out,BaseName(path),MAXD); int n=WLen(out); for(int i=n-1;i>=0;i--){ if(out[i]=='.'){ out[i]=0; break; } if(out[i]=='\\'||out[i]=='/')break; }
}
static int SyncSafe(const unsigned char*p){return ((p[0]&0x7f)<<21)|((p[1]&0x7f)<<14)|((p[2]&0x7f)<<7)|(p[3]&0x7f);}
static int BE32(const unsigned char*p){return ((int)p[0]<<24)|((int)p[1]<<16)|((int)p[2]<<8)|(int)p[3];}
static void DecodeID3Text(const unsigned char*p,int n,wchar_t*out,int cap){out[0]=0;if(!p||n<=1||cap<2)return;int enc=*p++;n--;if(enc==3){int m=MultiByteToWideChar(CP_UTF8,0,(const char*)p,n,out,cap-1);if(m>0)out[m]=0;}else if(enc==0){int m=MinI(n,cap-1);for(int i=0;i<m;i++)out[i]=(wchar_t)p[i];out[m]=0;}else{bool be=enc==2;int off=0;if(n>=2&&p[0]==0xff&&p[1]==0xfe){be=false;off=2;}else if(n>=2&&p[0]==0xfe&&p[1]==0xff){be=true;off=2;}int j=0;for(int i=off;i+1<n&&j<cap-1;i+=2){unsigned v=be?((unsigned)p[i]<<8|p[i+1]):((unsigned)p[i+1]<<8|p[i]);if(!v)break;out[j++]=(wchar_t)v;}out[j]=0;}TrimW(out);}
static bool IdEq(const unsigned char*p,const char*id){return p[0]==(unsigned char)id[0]&&p[1]==(unsigned char)id[1]&&p[2]==(unsigned char)id[2]&&p[3]==(unsigned char)id[3];}
static bool ParseMP3TechHeader(const unsigned char*p,int&br,int&sr,int&mode){
 if(!p||p[0]!=0xff||(p[1]&0xe0)!=0xe0)return false;unsigned h=((unsigned)p[0]<<24)|((unsigned)p[1]<<16)|((unsigned)p[2]<<8)|p[3];int ver=(h>>19)&3,layer=(h>>17)&3,bi=(h>>12)&15,si=(h>>10)&3;if(ver==1||layer!=1||bi==0||bi==15||si==3)return false;
 static const int br1[16]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};static const int br2[16]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};static const int baseSr[3]={44100,48000,32000};br=(ver==3?br1[bi]:br2[bi]);sr=baseSr[si];if(ver==2)sr/=2;else if(ver==0)sr/=4;mode=(h>>6)&3;return br>0&&sr>0;
}
static void FormatKHz(int hz,wchar_t*out,int cap){out[0]=0;if(hz<=0)return;int a=hz/1000,r=hz%1000;wchar_t n[20];IntToW(a,n);WCopy(out,n,cap);if(r){WCat(out,L".",cap);if(r%100==0){IntToW(r/100,n);WCat(out,n,cap);}else if(r%10==0){if(r<100)WCat(out,L"0",cap);IntToW(r/10,n);WCat(out,n,cap);}else{if(r<100)WCat(out,L"0",cap);if(r<10)WCat(out,L"0",cap);IntToW(r,n);WCat(out,n,cap);}}WCat(out,L" kHz",cap);}
static void BuildTrackTechLine(const Track&t,wchar_t*out,int cap){out[0]=0;if(!WEndsI(t.path,L".mp3"))return;WCopy(out,L"MP3",cap);wchar_t n[24];if(t.bitrateKbps>0){WCat(out,L"  •  ",cap);IntToW(t.bitrateKbps,n);WCat(out,n,cap);WCat(out,L" kbps",cap);}if(t.sampleRateHz>0){wchar_t khz[24];FormatKHz(t.sampleRateHz,khz,24);WCat(out,L"  •  ",cap);WCat(out,khz,cap);}WCat(out,L"  •  ",cap);const wchar_t*cm=t.channelMode==3?L"Mono":(t.channelMode==1?L"Joint stereo":(t.channelMode==2?L"Dual channel":L"Stereo"));WCat(out,cm,cap);WCat(out,L"  •  ",cap);if(t.id3v2Major>0){WCat(out,L"ID3v2.",cap);IntToW(t.id3v2Major,n);WCat(out,n,cap);if(t.id3v2Minor>0){WCat(out,L".",cap);IntToW(t.id3v2Minor,n);WCat(out,n,cap);}if(t.hasId3v1)WCat(out,L" + ID3v1/128B",cap);}else if(t.hasId3v1)WCat(out,L"ID3v1/128B",cap);else WCat(out,L"No ID3 tag",cap);}
static void LoadMetadata(const wchar_t*path,Track& t){
 t.title[0]=t.artist[0]=t.album[0]=t.genre[0]=t.year[0]=0;t.bitrateKbps=0;t.sampleRateHz=0;t.channelMode=0;t.id3v2Major=0;t.id3v2Minor=0;t.hasId3v1=false;FileNameDisplay(path,t.display);if(!WEndsI(path,L".mp3"))return;
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return;static unsigned char buf[131072];DWORD r=0;ReadFile(h,buf,sizeof(buf),&r,0);DWORD audioOff=0;
 if(r>=10&&buf[0]=='I'&&buf[1]=='D'&&buf[2]=='3'){int ver=buf[3];t.id3v2Major=ver;t.id3v2Minor=buf[4];int tag=SyncSafe(buf+6);audioOff=(DWORD)(10+tag);int end=MinI((int)r,10+tag),pos=10;while(pos+10<=end){unsigned char*f=buf+pos;if(f[0]==0)break;int fs=ver==4?SyncSafe(f+4):BE32(f+4);if(fs<=0||pos+10+fs>end)break;if(IdEq(f,"TIT2"))DecodeID3Text(f+10,fs,t.title,160);else if(IdEq(f,"TPE1"))DecodeID3Text(f+10,fs,t.artist,128);else if(IdEq(f,"TALB"))DecodeID3Text(f+10,fs,t.album,128);else if(IdEq(f,"TCON"))DecodeID3Text(f+10,fs,t.genre,64);else if(IdEq(f,"TYER")||IdEq(f,"TDRC"))DecodeID3Text(f+10,fs,t.year,16);pos+=10+fs;}}
 DWORD sz=GetFileSize(h,0);if(audioOff<sz){SetFilePointer(h,(LONG)audioOff,0,FILE_BEGIN);static unsigned char scan[65536];DWORD srn=0;if(ReadFile(h,scan,sizeof(scan),&srn,0)){for(DWORD i=0;i+4<=srn;i++){int br=0,sr=0,mode=0;if(ParseMP3TechHeader(scan+i,br,sr,mode)){t.bitrateKbps=br;t.sampleRateHz=sr;t.channelMode=mode;break;}}}}
 if(sz>=128){SetFilePointer(h,-128,0,FILE_END);unsigned char v1[128];DWORD rr=0;if(ReadFile(h,v1,128,&rr,0)&&rr==128&&v1[0]=='T'&&v1[1]=='A'&&v1[2]=='G'){t.hasId3v1=true;wchar_t tmp[64];if(!t.title[0]){for(int i=0;i<30;i++)tmp[i]=(wchar_t)v1[3+i];tmp[30]=0;TrimW(tmp);WCopy(t.title,tmp,160);}if(!t.artist[0]){for(int i=0;i<30;i++)tmp[i]=(wchar_t)v1[33+i];tmp[30]=0;TrimW(tmp);WCopy(t.artist,tmp,128);}if(!t.album[0]){for(int i=0;i<30;i++)tmp[i]=(wchar_t)v1[63+i];tmp[30]=0;TrimW(tmp);WCopy(t.album,tmp,128);}if(!t.year[0]){for(int i=0;i<4;i++)t.year[i]=(wchar_t)v1[93+i];t.year[4]=0;TrimW(t.year);}if(!t.genre[0]){wchar_t gn[16];IntToW((int)v1[127],gn);WCopy(t.genre,L"ID3 #",64);WCat(t.genre,gn,64);}}}CloseHandle(h);
 if(t.title[0]&&t.artist[0]){WCopy(t.display,t.artist,MAXD);WCat(t.display,L" - ",MAXD);WCat(t.display,t.title,MAXD);}else if(t.title[0])WCopy(t.display,t.title,MAXD);
}

static void InitPaths(){
 wchar_t exe[MAXP]; GetModuleFileNameW(0,exe,MAXP); WCopy(g_appDir,exe,MAXP); int n=WLen(g_appDir); while(n>0&&g_appDir[n-1]!='\\')n--; if(n)g_appDir[n-1]=0;
 wchar_t appdata[MAXP]; if(GetEnvironmentVariableW(L"APPDATA",appdata,MAXP)>0){ WCopy(g_dataDir,appdata,MAXP); WCat(g_dataDir,L"\\OzAmp",MAXP); CreateDirectoryW(g_dataDir,0); } else WCopy(g_dataDir,g_appDir,MAXP);
 WCopy(g_ini,g_dataDir,MAXP); WCat(g_ini,L"\\ozamp.ini",MAXP);
 WCopy(g_session,g_dataDir,MAXP); WCat(g_session,L"\\session.m3u8",MAXP);
 WCopy(g_statsIni,g_dataDir,MAXP); WCat(g_statsIni,L"\\track_stats.ini",MAXP);
 WCopy(g_libraryFile,g_dataDir,MAXP); WCat(g_libraryFile,L"\\library.m3u8",MAXP);
 WCopy(g_playlistFile,g_dataDir,MAXP);WCat(g_playlistFile,L"\\playlist1.m3u8",MAXP);
}

static void SetStatus(const wchar_t*s){ WCopy(g_mode,s,32); InvalidateRect(g_main,0,FALSE); }
static void Feedback(const wchar_t*s,int ms=1450){WCopy(g_feedback,s,96);g_feedbackUntil=GetTickCount64()+(ULONGLONG)ms;if(g_main)InvalidateRect(g_main,0,FALSE);}static void FlashAction(int id){g_actionFlashHit=id;g_actionFlashUntil=GetTickCount64()+150;if(g_main)InvalidateRect(g_main,0,FALSE);}
static void FeedbackBalance(){
 wchar_t b[64],n[24];
 if(g_balance==0)WCopy(b,L"BALANCE  CENTER",64);
 else{WCopy(b,L"BALANCE  ",64);WCat(b,g_balance<0?L"L ":L"R ",64);IntToW(AbsI(g_balance),n);WCat(b,n,64);WCat(b,L"%",64);}
 Feedback(b,1000);
}

static void PrepareNextTrack();
// DirectShow compatibility backend. Used only when the native PCM decoder cannot
// decode a non-MIDI file. It avoids the old MCI driver path that fails on some
// modern Windows installations.
static const GUID CLSID_FilterGraph={0xE436EBB3,0x524F,0x11CE,{0x9F,0x53,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
static const GUID IID_IGraphBuilder={0x56A868A9,0x0AD4,0x11CE,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
static const GUID IID_IMediaControl={0x56A868B1,0x0AD4,0x11CE,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
static const GUID IID_IBasicAudio={0x56A868B3,0x0AD4,0x11CE,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
static const GUID IID_IMediaSeeking={0x36B73880,0xC2C8,0x11CF,{0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60}};
struct DUnknownV{HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);};
struct GraphV{HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*AddFilter)(void*,void*,LPCWSTR);HRESULT(WINAPI*RemoveFilter)(void*,void*);HRESULT(WINAPI*EnumFilters)(void*,void**);HRESULT(WINAPI*FindFilterByName)(void*,LPCWSTR,void**);HRESULT(WINAPI*ConnectDirect)(void*,void*,void*,void*);HRESULT(WINAPI*Reconnect)(void*,void*);HRESULT(WINAPI*Disconnect)(void*,void*);HRESULT(WINAPI*SetDefaultSyncSource)(void*);HRESULT(WINAPI*Connect)(void*,void*,void*);HRESULT(WINAPI*Render)(void*,void*);HRESULT(WINAPI*RenderFile)(void*,LPCWSTR,LPCWSTR);HRESULT(WINAPI*AddSourceFilter)(void*,LPCWSTR,LPCWSTR,void**);HRESULT(WINAPI*SetLogFile)(void*,DWORD_PTR);HRESULT(WINAPI*Abort)(void*);HRESULT(WINAPI*ShouldOperationContinue)(void*);};
struct DispatchHead{HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*GetTypeInfoCount)(void*,UINT*);HRESULT(WINAPI*GetTypeInfo)(void*,UINT,DWORD,void**);HRESULT(WINAPI*GetIDsOfNames)(void*,const GUID*,wchar_t**,UINT,DWORD,LONG*);HRESULT(WINAPI*Invoke)(void*,LONG,const GUID*,DWORD,WORD,void*,void*,void*,UINT*);};
struct MediaControlV{DispatchHead d;HRESULT(WINAPI*Run)(void*);HRESULT(WINAPI*Pause)(void*);HRESULT(WINAPI*Stop)(void*);HRESULT(WINAPI*GetState)(void*,LONG,LONG*);HRESULT(WINAPI*RenderFile)(void*,void*);HRESULT(WINAPI*AddSourceFilter)(void*,void*,void**);HRESULT(WINAPI*get_FilterCollection)(void*,void**);HRESULT(WINAPI*get_RegFilterCollection)(void*,void**);HRESULT(WINAPI*StopWhenReady)(void*);};
struct BasicAudioV{DispatchHead d;HRESULT(WINAPI*put_Volume)(void*,LONG);HRESULT(WINAPI*get_Volume)(void*,LONG*);HRESULT(WINAPI*put_Balance)(void*,LONG);HRESULT(WINAPI*get_Balance)(void*,LONG*);};
struct MediaSeekingV{HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*GetCapabilities)(void*,DWORD*);HRESULT(WINAPI*CheckCapabilities)(void*,DWORD*);HRESULT(WINAPI*IsFormatSupported)(void*,const GUID*);HRESULT(WINAPI*QueryPreferredFormat)(void*,GUID*);HRESULT(WINAPI*GetTimeFormat)(void*,GUID*);HRESULT(WINAPI*IsUsingTimeFormat)(void*,const GUID*);HRESULT(WINAPI*SetTimeFormat)(void*,const GUID*);HRESULT(WINAPI*GetDuration)(void*,LONGLONG*);HRESULT(WINAPI*GetStopPosition)(void*,LONGLONG*);HRESULT(WINAPI*GetCurrentPosition)(void*,LONGLONG*);HRESULT(WINAPI*ConvertTimeFormat)(void*,LONGLONG*,const GUID*,LONGLONG,const GUID*);HRESULT(WINAPI*SetPositions)(void*,LONGLONG*,DWORD,LONGLONG*,DWORD);HRESULT(WINAPI*GetPositions)(void*,LONGLONG*,LONGLONG*);HRESULT(WINAPI*GetAvailable)(void*,LONGLONG*,LONGLONG*);HRESULT(WINAPI*SetRate)(void*,double);HRESULT(WINAPI*GetRate)(void*,double*);HRESULT(WINAPI*GetPreroll)(void*,LONGLONG*);};
template<class V> static V* DVT(void*p){return p?*(V**)p:0;}
static void DSRel(void*&p){if(p){DVT<DUnknownV>(p)->Release(p);p=0;}}
static void DSClose(){if(g_dsControl)DVT<MediaControlV>(g_dsControl)->Stop(g_dsControl);DSRel(g_dsAudio);DSRel(g_dsSeek);DSRel(g_dsControl);DSRel(g_dsGraph);g_dsActive=false;}
static void DSApplyAudio(){if(!g_dsActive||!g_dsAudio)return;LONG vol=-10000;if(!g_muted&&g_volume>0){double x=(double)g_volume/100.0;vol=(LONG)(2000.0*log10(x));if(vol<-10000)vol=-10000;if(vol>0)vol=0;}DVT<BasicAudioV>(g_dsAudio)->put_Volume(g_dsAudio,vol);LONG bal=(LONG)Clamp(g_balance*100,-10000,10000);DVT<BasicAudioV>(g_dsAudio)->put_Balance(g_dsAudio,bal);}
static bool DSOpen(const wchar_t*path,bool autoplay){DSClose();void*gr=0;HRESULT hr=CoCreateInstance(&CLSID_FilterGraph,0,CLSCTX_INPROC_SERVER,&IID_IGraphBuilder,&gr);if(FAILED(hr)||!gr)return false;if(FAILED(DVT<GraphV>(gr)->RenderFile(gr,path,0))){DSRel(gr);return false;}void*mc=0,*ms=0,*ba=0;DVT<GraphV>(gr)->QueryInterface(gr,&IID_IMediaControl,&mc);DVT<GraphV>(gr)->QueryInterface(gr,&IID_IMediaSeeking,&ms);DVT<GraphV>(gr)->QueryInterface(gr,&IID_IBasicAudio,&ba);if(!mc||!ms){if(ba)DSRel(ba);if(ms)DSRel(ms);if(mc)DSRel(mc);DSRel(gr);return false;}g_dsGraph=gr;g_dsControl=mc;g_dsSeek=ms;g_dsAudio=ba;g_dsActive=true;LONGLONG dur=0;if(SUCCEEDED(DVT<MediaSeekingV>(g_dsSeek)->GetDuration(g_dsSeek,&dur)))g_length=(int)(dur/10000LL);else g_length=0;g_pos=0;DSApplyAudio();if(autoplay){if(FAILED(DVT<MediaControlV>(g_dsControl)->Run(g_dsControl))){DSClose();return false;}g_playing=true;g_paused=false;}else{g_playing=false;g_paused=false;}return true;}
static void DSPlay(){if(g_dsActive&&g_dsControl&&SUCCEEDED(DVT<MediaControlV>(g_dsControl)->Run(g_dsControl))){g_playing=true;g_paused=false;}}
static void DSPause(){if(g_dsActive&&g_dsControl){DVT<MediaControlV>(g_dsControl)->Pause(g_dsControl);g_playing=false;g_paused=true;}}
static void DSStop(){if(g_dsActive&&g_dsControl){DVT<MediaControlV>(g_dsControl)->Stop(g_dsControl);LONGLONG p=0;DVT<MediaSeekingV>(g_dsSeek)->SetPositions(g_dsSeek,&p,1,0,0);g_pos=0;g_playing=false;g_paused=false;}}
static void DSSeekMs(int ms){if(!g_dsActive||!g_dsSeek)return;LONGLONG p=(LONGLONG)Clamp(ms,0,g_length)*10000LL;DVT<MediaSeekingV>(g_dsSeek)->SetPositions(g_dsSeek,&p,1,0,0);g_pos=ms;if(g_playing)DVT<MediaControlV>(g_dsControl)->Run(g_dsControl);}
static int DSPosMs(){if(!g_dsActive||!g_dsSeek)return 0;LONGLONG p=0;if(SUCCEEDED(DVT<MediaSeekingV>(g_dsSeek)->GetCurrentPosition(g_dsSeek,&p)))return (int)(p/10000LL);return g_pos;}

static DWORD MCI(const wchar_t*cmd,wchar_t*ret=0,UINT cap=0){ return mciSendStringW(cmd,ret,cap,g_main); }
static void MciError(DWORD e){ wchar_t t[256]; if(mciGetErrorStringW(e,t,256))ShowOzError(L"PLAYBACK ERROR",t,g_current); }
static void ApplyAudio(){
 if(g_current<0)return;
 if(g_nativeAudio){OzAudioSetVolume(g_volume,g_balance,g_muted);OzAudioSetEQ(g_eqEnabled,g_preampDb,g_eqBands);OzAudioSetReplayGainDb(g_replayGainEnabled?g_tracks[g_current].replayGainDb:0.0);return;}if(g_dsActive){DSApplyAudio();return;}
 int effective=g_muted?0:g_volume; int left=effective*10,right=effective*10; if(g_balance<0) right=right*(100+g_balance)/100; if(g_balance>0) left=left*(100-g_balance)/100;
 wchar_t c[128],n[16]; WCopy(c,L"setaudio ozamp_track left volume to ",128);IntToW(left,n);WCat(c,n,128);MCI(c);
 WCopy(c,L"setaudio ozamp_track right volume to ",128);IntToW(right,n);WCat(c,n,128);MCI(c);
}
static void CloseTrack(){if(g_nativeAudio)OzAudioUnload();if(g_dsActive)DSClose();MCI(L"close ozamp_track");g_nativeAudio=false;g_playing=false;g_paused=false;g_length=0;g_pos=0;if(g_main)SetWindowTextW(g_main,L"OzAmp");}
static bool OpenTrack(int idx,bool autoplay){
 if(idx<0||idx>=g_count)return false;if(!FileExists(g_tracks[idx].path)){ShowOzError(L"TRACK UNAVAILABLE",L"The file was moved, renamed or is no longer available. Locate it again or remove it from the playlist.",idx);SetStatus(L"TRACK UNAVAILABLE");return false;} CloseTrack(); g_current=idx;g_selected=idx;g_pos=0;
 if(OzAudioLoad(g_tracks[idx].path)){g_nativeAudio=true;g_length=OzAudioLengthMs();g_tracks[idx].lengthMs=g_length;SaveTrackStats(g_tracks[idx]);ApplyAudio();wchar_t wt[MAXD+32];WCopy(wt,L"OzAmp - ",MAXD+32);WCat(wt,g_tracks[idx].display,MAXD+32);SetWindowTextW(g_main,wt);LoadCoverForCurrent();OzAudioSetReplayGainDb(g_replayGainEnabled?g_tracks[idx].replayGainDb:0.0);if(autoplay){OzAudioPlay();g_playing=true;g_paused=false;g_tracks[idx].playCount++;SaveTrackStats(g_tracks[idx]);SetStatus(L"WASAPI PLAY");NotifyTrack();}else SetStatus(L"WASAPI READY");PrepareNextTrack();g_marquee=0;InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);return true;}
 bool isMidi=WEndsI(g_tracks[idx].path,L".mid")||WEndsI(g_tracks[idx].path,L".midi");
 // Never hide a broken WASAPI core behind DirectShow. 3.0.x/3.1.0 did that,
 // which made a renderer-initialization ABI bug look like a codec problem.
 // Compatibility fallback is allowed only when the WASAPI render service itself
 // is healthy and the specific file cannot be decoded into OzAmp PCM.
 if(!isMidi&&!OzAudioReady()){
  wchar_t msg[320],hx[16];WCopy(msg,L"The selected Windows audio output could not be initialized.\r\n",320);WCat(msg,OzAudioLastErrorStage(),320);Hex32((DWORD)OzAudioLastError(),hx);WCat(msg,L"  ",320);WCat(msg,hx,320);WCat(msg,L"\r\nOpen Settings > Audio to choose another output.",320);
  ShowOzError(L"AUDIO OUTPUT ERROR",msg,idx);SetStatus(L"AUDIO OUTPUT ERROR");g_current=-1;InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);return false;
 }
 // Compatibility path: DirectShow only for an actual per-file decoder fallback;
 // MCI sequencer remains reserved for MIDI.
 if(!isMidi){if(DSOpen(g_tracks[idx].path,autoplay)){g_tracks[idx].lengthMs=g_length;SaveTrackStats(g_tracks[idx]);wchar_t wt[MAXD+32];WCopy(wt,L"OzAmp - ",MAXD+32);WCat(wt,g_tracks[idx].display,MAXD+32);SetWindowTextW(g_main,wt);LoadCoverForCurrent();SetStatus(L"DIRECTSHOW COMPAT");if(autoplay){g_tracks[idx].playCount++;SaveTrackStats(g_tracks[idx]);NotifyTrack();}g_marquee=0;InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);return true;}SetStatus(L"UNSUPPORTED AUDIO");ShowOzError(L"COULD NOT PLAY TRACK",L"OzAmp could not decode this audio file with the native engine or compatibility decoder.",idx);g_current=-1;InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);return false;}
 wchar_t c[MAXP+160],r[64];WCopy(c,L"open \\\"",MAXP+160);WCat(c,g_tracks[idx].path,MAXP+160);WCat(c,L"\\\" type sequencer alias ozamp_track",MAXP+160);DWORD e=MCI(c);
 if(e){SetStatus(L"MIDI OPEN ERROR");MciError(e);g_current=-1;return false;}
 MCI(L"set ozamp_track time format milliseconds");if(!MCI(L"status ozamp_track length",r,64))g_length=WToInt(r);else g_length=0;g_tracks[idx].lengthMs=g_length;SaveTrackStats(g_tracks[idx]);ApplyAudio();wchar_t wt[MAXD+32];WCopy(wt,L"OzAmp - ",MAXD+32);WCat(wt,g_tracks[idx].display,MAXD+32);SetWindowTextW(g_main,wt);LoadCoverForCurrent();if(autoplay){e=MCI(g_repeat==2?L"play ozamp_track repeat":L"play ozamp_track");if(e){MciError(e);SetStatus(L"MIDI PLAY ERROR");return false;}g_playing=true;g_paused=false;g_tracks[idx].playCount++;SaveTrackStats(g_tracks[idx]);SetStatus(L"MIDI PLAY");NotifyTrack();}else SetStatus(L"MIDI READY");g_marquee=0;InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);return true;
}
static void Play(){FlashAction(H_PLAY);if(g_current<0){if(g_count)OpenTrack(0,true);return;}if(g_nativeAudio){OzAudioPlay();g_playing=true;g_paused=false;SetStatus(L"WASAPI PLAY");return;}if(g_dsActive){DSPlay();SetStatus(L"DIRECTSHOW PLAY");return;}if(g_paused){MCI(L"resume ozamp_track");g_paused=false;g_playing=true;SetStatus(L"PLAYING");}else if(!g_playing){MCI(g_repeat==2?L"play ozamp_track repeat":L"play ozamp_track");g_playing=true;SetStatus(L"PLAYING");}}
static void Pause(){FlashAction(H_PAUSE);if(g_current<0)return;if(g_playing){if(g_nativeAudio)OzAudioPause();else if(g_dsActive)DSPause();else MCI(L"pause ozamp_track");g_paused=true;g_playing=false;SetStatus(L"PAUSED");}else if(g_paused){Play();}}
static void TogglePlay(){if(g_playing)Pause();else Play();}
static void Stop(){FlashAction(H_STOP);if(g_current>=0){if(g_nativeAudio)OzAudioStop();else if(g_dsActive)DSStop();else{MCI(L"stop ozamp_track");MCI(L"seek ozamp_track to start");}g_pos=0;g_playing=false;g_paused=false;SetStatus(L"STOPPED");InvalidateRect(g_main,0,FALSE);}}
static int RandIndex(int max){ if(max<=1)return 0; g_rng=g_rng*6364136223846793005ULL+1442695040888963407ULL+GetTickCount64(); return (int)((g_rng>>17)%((unsigned)max)); }
static int SmartRandom(){if(g_count<=1)return 0;int n=RandIndex(g_count);if(g_current>=0&&g_tracks[g_current].artist[0]){for(int tries=0;tries<12;tries++){int c=RandIndex(g_count);if(c!=g_current&&(!g_tracks[c].artist[0]||!WEqI(g_tracks[c].artist,g_tracks[g_current].artist))){n=c;break;}}}if(n==g_current)n=(n+1)%g_count;return n;}
static int PickNextIndex(){if(!g_count||g_current<0)return -1;int qi=QueueFirstIndex();if(qi>=0&&qi<g_count&&qi!=g_current)return qi;if(g_shuffle)return SmartRandom();int n=g_current+1;if(n>=g_count){if(g_repeat==1)n=0;else return -1;}return n;}
static void PrepareNextTrack(){g_preparedNext=-1;OzAudioClearNext();if(!g_nativeAudio||g_repeat==2||g_sleepAfterCurrent||(!g_gapless&&g_crossfadeSec<=0))return;int n=PickNextIndex();if(n<0||n>=g_count)return;double rg=g_replayGainEnabled?g_tracks[n].replayGainDb:0.0;OzAudioSetCrossfadeMs(g_crossfadeSec*1000);if(OzAudioPrepareNext(g_tracks[n].path,rg))g_preparedNext=n;}
static void Next(bool user=true){FlashAction(H_NEXT);if(!g_count)return;if(!user&&g_sleepAfterCurrent){g_sleepAfterCurrent=false;Stop();SetStatus(L"SLEEP AFTER TRACK");return;}int n;if(user&&g_preparedNext>=0)n=g_preparedNext;else{int qi=QueueFirstIndex();if(qi>=0&&qi<g_count&&qi!=g_current)n=qi;else if(g_shuffle)n=SmartRandom();else{n=g_current+1;if(n>=g_count){if(g_repeat==1)n=0;else if(!user){Stop();return;}else n=0;}}}if(g_queueCount>0&&n==QueueFirstIndex())QueuePopFirst();g_preparedNext=-1;OzAudioClearNext();OpenTrack(n,true);}
static void Prev(){FlashAction(H_PREV);if(!g_count)return;int n=g_current-1;if(n<0)n=g_count-1;OpenTrack(n,true);}
static void SeekTo(int ms){if(g_current<0)return;ms=Clamp(ms,0,g_length);if(g_nativeAudio){OzAudioSeekMs(ms);g_pos=ms;}else if(g_dsActive){DSSeekMs(ms);}else{wchar_t c[128],n[32];WCopy(c,L"seek ozamp_track to ",128);IntToW(ms,n);WCat(c,n,128);MCI(c);g_pos=ms;if(g_playing)MCI(g_repeat==2?L"play ozamp_track repeat":L"play ozamp_track");}InvalidateRect(g_main,0,FALSE);}

static int AddPath(const wchar_t*p);
static int FindTrackPath(const wchar_t*p){for(int i=0;i<g_count;i++)if(WEqI(g_tracks[i].path,p))return i;return -1;}
static void SyncQueuedIndex(){g_queued=-1;while(g_queueCount>0){int ix=FindTrackPath(g_queuePaths[0]);if(ix>=0){g_queued=ix;break;}for(int i=0;i<g_queueCount-1;i++)WCopy(g_queuePaths[i],g_queuePaths[i+1],MAXP);g_queueCount--;}if(g_queueCount<=0)g_queueCount=0;}
static bool QueueHasPath(const wchar_t*p){for(int i=0;i<g_queueCount;i++)if(WEqI(g_queuePaths[i],p))return true;return false;}
static void QueueAddPath(const wchar_t*p,bool playNext){if(!p||!p[0])return;for(int i=0;i<g_queueCount;i++)if(WEqI(g_queuePaths[i],p)){if(playNext&&i>0){wchar_t tmp[MAXP];WCopy(tmp,g_queuePaths[i],MAXP);for(int q=i;q>0;q--)WCopy(g_queuePaths[q],g_queuePaths[q-1],MAXP);WCopy(g_queuePaths[0],tmp,MAXP);}SyncQueuedIndex();PrepareNextTrack();return;}if(g_queueCount>=128)return;if(playNext){for(int i=g_queueCount;i>0;i--)WCopy(g_queuePaths[i],g_queuePaths[i-1],MAXP);WCopy(g_queuePaths[0],p,MAXP);g_queueCount++;}else WCopy(g_queuePaths[g_queueCount++],p,MAXP);SyncQueuedIndex();PrepareNextTrack();}
static void QueueRemovePath(const wchar_t*p){for(int i=0;i<g_queueCount;i++)if(WEqI(g_queuePaths[i],p)){for(int q=i;q<g_queueCount-1;q++)WCopy(g_queuePaths[q],g_queuePaths[q+1],MAXP);g_queueCount--;i--;}SyncQueuedIndex();PrepareNextTrack();}
static int QueueFirstIndex(){SyncQueuedIndex();return g_queued;}
static void QueuePopFirst(){if(g_queueCount>0){for(int i=0;i<g_queueCount-1;i++)WCopy(g_queuePaths[i],g_queuePaths[i+1],MAXP);g_queueCount--;}SyncQueuedIndex();}
static void QueueClear(){g_queueCount=0;g_queued=-1;PrepareNextTrack();}
static bool TrackMatchesFilter(const Track&t){if(!g_plFilter[0])return true;return WContainsI(t.display,g_plFilter)||WContainsI(t.title,g_plFilter)||WContainsI(t.artist,g_plFilter)||WContainsI(t.album,g_plFilter)||WContainsI(t.genre,g_plFilter);}
static void BuildFilterMap(){g_filterCount=0;for(int i=0;i<g_count&&g_filterCount<MAX_TRACKS;i++)if(TrackMatchesFilter(g_tracks[i]))g_filterMap[g_filterCount++]=i;}
static int PlaylistVisibleCount(){BuildFilterMap();return g_filterCount;}
static int VisibleTrackAt(int pos){BuildFilterMap();return pos>=0&&pos<g_filterCount?g_filterMap[pos]:-1;}
static int VisiblePosOfTrack(int ix){BuildFilterMap();for(int i=0;i<g_filterCount;i++)if(g_filterMap[i]==ix)return i;return -1;}
static int QueuePositionForPath(const wchar_t*p){for(int i=0;i<g_queueCount;i++)if(WEqI(g_queuePaths[i],p))return i+1;return 0;}
static void MarkRange(int a,int b){if(a>b){int t=a;a=b;b=t;}ClearMarks();for(int i=a;i<=b&&i<g_count;i++)if(i>=0)g_tracks[i].marked=true;}
static int MarkedCount(){int n=0;for(int i=0;i<g_count;i++)if(g_tracks[i].marked)n++;return n;}
static void MoveMarkedBlock(int target){int mc=MarkedCount();if(mc<=0||target<0||target>=g_count)return;wchar_t cur[MAXP],sel[MAXP],q[MAXP];RememberIndices(cur,sel,q);int markedBefore=0;for(int i=0;i<target;i++)if(g_tracks[i].marked)markedBefore++;int insertAt=target-markedBefore;if(insertAt<0)insertAt=0;int out=0,un=0;for(int i=0;i<g_count;i++)if(!g_tracks[i].marked){if(un==insertAt){for(int k=0;k<g_count;k++)if(g_tracks[k].marked)g_reorderTemp[out++]=g_tracks[k];}g_reorderTemp[out++]=g_tracks[i];un++;}if(out<g_count)for(int k=0;k<g_count;k++)if(g_tracks[k].marked)g_reorderTemp[out++]=g_tracks[k];for(int i=0;i<g_count;i++)g_tracks[i]=g_reorderTemp[i];RestoreIndices(cur,sel,q);SyncQueuedIndex();}
static int TrackSortCmp(const Track&a,const Track&b,int mode){if(mode==4){if(a.lengthMs<b.lengthMs)return -1;if(a.lengthMs>b.lengthMs)return 1;return WCompareI(a.display,b.display);}const wchar_t*aa=a.display,*bb=b.display;if(mode==1){aa=a.artist[0]?a.artist:a.display;bb=b.artist[0]?b.artist:b.display;}else if(mode==2){aa=a.title[0]?a.title:a.display;bb=b.title[0]?b.title:b.display;}else if(mode==3){aa=a.album[0]?a.album:a.display;bb=b.album[0]?b.album:b.display;}int c=WCompareI(aa,bb);return c?c:WCompareI(a.display,b.display);}
static void SortPlaylistBy(int mode){wchar_t cur[MAXP],sel[MAXP],q[MAXP];RememberIndices(cur,sel,q);for(int i=1;i<g_count;i++){Track t=g_tracks[i];int j=i-1;while(j>=0&&TrackSortCmp(g_tracks[j],t,mode)>0){g_tracks[j+1]=g_tracks[j];j--;}g_tracks[j+1]=t;}RestoreIndices(cur,sel,q);SyncQueuedIndex();g_scroll=0;InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}
static int AddFolder(const wchar_t*dir,int depth){
 if(depth>24||g_count>=MAX_TRACKS)return -1;int first=-1;wchar_t pat[MAXP];WCopy(pat,dir,MAXP);WCat(pat,L"\\*",MAXP);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return -1;
 do{ if(fd.cFileName[0]=='.'&&(fd.cFileName[1]==0||(fd.cFileName[1]=='.'&&fd.cFileName[2]==0)))continue; wchar_t p[MAXP];WCopy(p,dir,MAXP);WCat(p,L"\\",MAXP);WCat(p,fd.cFileName,MAXP);int ix=(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?AddFolder(p,depth+1):AddPath(p);if(first<0&&ix>=0)first=ix; }while(FindNextFileW(h,&fd));FindClose(h);return first;
}
static int AddPath(const wchar_t*p){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return -1;}if(!p||!p[0]||g_count>=MAX_TRACKS)return -1;if(IsDir(p))return AddFolder(p,0);if(!Supported(p))return -1;int old=FindTrackPath(p);if(old>=0)return old;int ix=g_count;memset(&g_tracks[ix],0,sizeof(Track));WCopy(g_tracks[ix].path,p,MAXP);LoadMetadata(p,g_tracks[ix]);LoadTrackStats(g_tracks[ix]);g_count++;if(g_selected<0)g_selected=0;InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);return ix;}
static void RemoveAt(int ix){if(ix<0||ix>=g_count)return;wchar_t removed[MAXP];WCopy(removed,g_tracks[ix].path,MAXP);bool was=(ix==g_current);if(was)CloseTrack();for(int i=ix;i<g_count-1;i++)g_tracks[i]=g_tracks[i+1];g_count--;if(g_current>ix)g_current--;else if(was)g_current=-1;QueueRemovePath(removed);SyncQueuedIndex();}
static bool AnyMarked(){for(int i=0;i<g_count;i++)if(g_tracks[i].marked)return true;return false;}
static void ClearMarks(){for(int i=0;i<g_count;i++)g_tracks[i].marked=false;}
static void RemoveSelected(){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return;}if(g_selected<0||g_selected>=g_count)return;g_undoCount=0;g_undoAt=0;bool marks=AnyMarked();for(int i=g_count-1;i>=0;i--){if((marks&&g_tracks[i].marked)||(!marks&&i==g_selected)){if(g_undoCount<64)g_undo[g_undoCount++]=g_tracks[i];RemoveAt(i);}}if(g_selected>=g_count)g_selected=g_count-1;if(g_selected>=0&&g_count)g_tracks[g_selected].marked=true;if(g_scroll>g_selected)g_scroll=MaxI(0,g_selected);InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}
static void UndoRemove(){if(!g_undoCount)return;for(int i=g_undoCount-1;i>=0&&g_count<MAX_TRACKS;i--){if(FindTrackPath(g_undo[i].path)<0)g_tracks[g_count++]=g_undo[i];}g_undoCount=0;g_selected=g_count?g_count-1:-1;InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}
static void ClearAll(){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return;}CloseTrack();g_count=0;g_current=-1;g_selected=-1;g_queued=-1;g_queueCount=0;g_scroll=0;InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}

static void OpenFilesDialog(){
 static wchar_t buf[65536];memset(buf,0,sizeof(buf));OPENFILENAMEW o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=g_main;o.lpstrFile=buf;o.nMaxFile=65536;o.lpstrTitle=L"Open music in OzAmp";o.lpstrFilter=L"Audio files\0*.mp3;*.wav;*.flac;*.ogg;*.oga;*.opus;*.wma;*.mid;*.midi;*.aac;*.m4a;*.mp4;*.alac;*.aif;*.aiff\0All files\0*.*\0\0";o.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_HIDEREADONLY;
 if(!GetOpenFileNameW(&o))return;int play=-1;wchar_t*first=buf;int fl=WLen(first);wchar_t*second=first+fl+1;if(!*second){play=AddPath(first);}else{wchar_t dir[MAXP];WCopy(dir,first,MAXP);for(wchar_t*p=second;*p;p+=WLen(p)+1){wchar_t full[MAXP];WCopy(full,dir,MAXP);WCat(full,L"\\",MAXP);WCat(full,p,MAXP);int ix=AddPath(full);if(play<0)play=ix;}}if(play>=0)OpenTrack(play,true);
}
static void OpenFolderDialog(){wchar_t display[MAX_PATH],path[MAXP];BROWSEINFOW b;memset(&b,0,sizeof(b));b.hwndOwner=g_main;b.pszDisplayName=display;b.lpszTitle=L"Choose a music folder";b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_EDITBOX;void*pid=SHBrowseForFolderW(&b);if(pid){if(SHGetPathFromIDListW(pid,path)){int ix=AddFolder(path,0);if(ix>=0)OpenTrack(ix,true);}CoTaskMemFree(pid);}}

static bool SaveM3UPath(const wchar_t*path){
 HANDLE h=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;const char head[]="#EXTM3U\r\n";DWORD w;WriteFile(h,head,sizeof(head)-1,&w,0);
 static char u[8192];for(int i=0;i<g_count;i++){int n=WideCharToMultiByte(CP_UTF8,0,g_tracks[i].path,-1,u,8190,0,0);if(n>1){u[n-1]='\r';u[n]='\n';WriteFile(h,u,n+1,&w,0);}}CloseHandle(h);return true;
}
static void SaveM3UDialog(){static wchar_t p[MAXP];memset(p,0,sizeof(p));WCopy(p,L"playlist.m3u8",MAXP);OPENFILENAMEW o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=g_pl;o.lpstrFile=p;o.nMaxFile=MAXP;o.lpstrFilter=L"M3U8 playlist\0*.m3u8\0M3U playlist\0*.m3u\0\0";o.lpstrDefExt=L"m3u8";o.Flags=OFN_EXPLORER|OFN_OVERWRITEPROMPT;if(GetSaveFileNameW(&o))SaveM3UPath(p);}
static bool LoadM3UPath(const wchar_t*path,bool clearFirst){
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;DWORD sz=GetFileSize(h,0);if(sz==0xffffffffUL||sz>8*1024*1024){CloseHandle(h);return false;}static char data[8*1024*1024+4];DWORD r=0;if(!ReadFile(h,data,sz,&r,0)){CloseHandle(h);return false;}CloseHandle(h);data[r]=0;if(clearFirst)ClearAll();wchar_t base[MAXP];DirName(path,base,MAXP);char*line=data;if(r>=3&&(unsigned char)data[0]==0xEF&&(unsigned char)data[1]==0xBB&&(unsigned char)data[2]==0xBF)line+=3;while(*line){char*e=line;while(*e&&*e!='\r'&&*e!='\n')e++;char save=*e;*e=0;if(*line&&*line!='#'){wchar_t w[MAXP];int n=MultiByteToWideChar(CP_UTF8,0,line,-1,w,MAXP);if(n>0){bool abs=(WLen(w)>2&&w[1]==':')||(w[0]=='\\'&&w[1]=='\\');if(abs)AddPath(w);else{wchar_t f[MAXP];WCopy(f,base,MAXP);WCat(f,L"\\",MAXP);WCat(f,w,MAXP);AddPath(f);}}}*e=save;while(*e=='\r'||*e=='\n')e++;line=e;}return true;
}
static void LoadM3UDialog(){static wchar_t p[MAXP];memset(p,0,sizeof(p));OPENFILENAMEW o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=g_pl;o.lpstrFile=p;o.nMaxFile=MAXP;o.lpstrFilter=L"Playlists\0*.m3u;*.m3u8\0All files\0*.*\0\0";o.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST;if(GetOpenFileNameW(&o)){if(LoadM3UPath(p,true)&&g_count)OpenTrack(0,true);}}


static void CopyText(const wchar_t*s){if(!s)return;int n=WLen(s);HGLOBAL h=GlobalAlloc(GMEM_MOVEABLE,(n+1)*sizeof(wchar_t));if(!h)return;wchar_t*p=(wchar_t*)GlobalLock(h);if(p){WCopy(p,s,n+1);GlobalUnlock(h);if(OpenClipboard(g_main)){EmptyClipboard();SetClipboardData(CF_UNICODETEXT,h);CloseClipboard();return;}}}
static void OpenContaining(const wchar_t*p){wchar_t a[MAXP+32];WCopy(a,L"/select,\"",MAXP+32);WCat(a,p,MAXP+32);WCat(a,L"\"",MAXP+32);ShellExecuteW(g_main,L"open",L"explorer.exe",a,0,SW_SHOWNORMAL);}

static void ToggleMute(){if(g_muted){g_muted=false;if(g_volume==0)g_volume=g_preMuteVolume>0?g_preMuteVolume:78;}else{g_muted=true;if(g_volume>0)g_preMuteVolume=g_volume;}ApplyAudio();InvalidateRect(g_main,0,FALSE);}
static void RememberIndices(wchar_t*cur,wchar_t*sel,wchar_t*q){cur[0]=sel[0]=q[0]=0;if(g_current>=0)WCopy(cur,g_tracks[g_current].path,MAXP);if(g_selected>=0)WCopy(sel,g_tracks[g_selected].path,MAXP);if(g_queued>=0)WCopy(q,g_tracks[g_queued].path,MAXP);}
static void RestoreIndices(const wchar_t*cur,const wchar_t*sel,const wchar_t*q){g_current=cur[0]?FindTrackPath(cur):-1;g_selected=sel[0]?FindTrackPath(sel):(g_count?0:-1);g_queued=q[0]?FindTrackPath(q):-1;}
static void SwapTrack(int a,int b){if(a==b)return;Track t=g_tracks[a];g_tracks[a]=g_tracks[b];g_tracks[b]=t;}
static void SortAZ(){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return;}wchar_t c[MAXP],s1[MAXP],q[MAXP];RememberIndices(c,s1,q);for(int i=1;i<g_count;i++){int j=i;while(j>0&&WCompareI(g_tracks[j-1].display,g_tracks[j].display)>0){SwapTrack(j-1,j);j--;}}RestoreIndices(c,s1,q);g_scroll=0;InvalidateRect(g_pl,0,FALSE);}
static void ReversePlaylist(){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return;}wchar_t c[MAXP],s1[MAXP],q[MAXP];RememberIndices(c,s1,q);for(int i=0;i<g_count/2;i++)SwapTrack(i,g_count-1-i);RestoreIndices(c,s1,q);g_scroll=0;InvalidateRect(g_pl,0,FALSE);}
static void RandomizePlaylist(){if(g_scanRunning){SetStatus(L"WAIT FOR RG SCAN");return;}wchar_t c[MAXP],s1[MAXP],q[MAXP];RememberIndices(c,s1,q);for(int i=g_count-1;i>0;i--)SwapTrack(i,RandIndex(i+1));RestoreIndices(c,s1,q);g_scroll=0;InvalidateRect(g_pl,0,FALSE);}
static void RemoveMissing(){for(int i=g_count-1;i>=0;i--)if(GetFileAttributesW(g_tracks[i].path)==0xffffffffUL){g_selected=i;RemoveSelected();}InvalidateRect(g_pl,0,FALSE);}
static void TrackInfo(int ix){ShowTrackInfo(ix);}

static void PutID3Field(unsigned char*d,int n,const wchar_t*s){for(int i=0;i<n;i++)d[i]=' ';if(!s)return;for(int i=0;i<n&&s[i];i++){unsigned v=(unsigned)s[i];d[i]=(unsigned char)(v<256?v:'?');}}
static bool WriteID3v1(const Track&t){if(!WEndsI(t.path,L".mp3"))return false;HANDLE h=CreateFileW(t.path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;DWORD sz=GetFileSize(h,0);unsigned char tag[128];memset(tag,0,sizeof(tag));tag[0]='T';tag[1]='A';tag[2]='G';PutID3Field(tag+3,30,t.title);PutID3Field(tag+33,30,t.artist);PutID3Field(tag+63,30,t.album);PutID3Field(tag+93,4,t.year);tag[127]=255;bool has=false;if(sz>=128){SetFilePointer(h,-128,0,FILE_END);unsigned char old[3];DWORD r=0;if(ReadFile(h,old,3,&r,0)&&r==3&&old[0]=='T'&&old[1]=='A'&&old[2]=='G')has=true;}SetFilePointer(h,has?-128:0,0,FILE_END);DWORD w=0;bool ok=WriteFile(h,tag,128,&w,0)&&w==128;CloseHandle(h);return ok;}
static void SyncEditedTrack(const Track&t){for(int i=0;i<g_count;i++)if(WEqI(g_tracks[i].path,t.path)){WCopy(g_tracks[i].title,t.title,160);WCopy(g_tracks[i].artist,t.artist,128);WCopy(g_tracks[i].album,t.album,128);WCopy(g_tracks[i].genre,t.genre,64);WCopy(g_tracks[i].year,t.year,16);if(t.title[0]&&t.artist[0]){WCopy(g_tracks[i].display,t.artist,MAXD);WCat(g_tracks[i].display,L" - ",MAXD);WCat(g_tracks[i].display,t.title,MAXD);}else if(t.title[0])WCopy(g_tracks[i].display,t.title,MAXD);SaveTrackStats(g_tracks[i]);}for(int i=0;i<g_libCount;i++)if(WEqI(g_library[i].path,t.path)){WCopy(g_library[i].title,t.title,160);WCopy(g_library[i].artist,t.artist,128);WCopy(g_library[i].album,t.album,128);WCopy(g_library[i].genre,t.genre,64);WCopy(g_library[i].year,t.year,16);if(t.title[0]&&t.artist[0]){WCopy(g_library[i].display,t.artist,MAXD);WCat(g_library[i].display,L" - ",MAXD);WCat(g_library[i].display,t.title,MAXD);}else if(t.title[0])WCopy(g_library[i].display,t.title,MAXD);SaveTrackStats(g_library[i]);}InvalidateRect(g_pl,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);if(g_main)InvalidateRect(g_main,0,FALSE);}
static HWND MkCtl(HWND p,const wchar_t*cls,const wchar_t*txt,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowExW(0,cls,txt,WS_CHILD|WS_VISIBLE|style,x,y,w,h,p,(HMENU)(UINT_PTR)id,g_inst,0);if(c&&g_font)SendMessageW(c,WM_SETFONT,(WPARAM)g_font,TRUE);return c;}
static void OpenTagEditor(int ix){if(ix<0||ix>=g_count)return;g_tagIndex=ix;if(!g_tag){g_tag=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpTag30",L"OzAmp Metadata Editor",WS_POPUP|WS_CLIPCHILDREN,250,180,500,338,g_main,0,g_inst,0);RoundWindow(g_tag,14);}SetWindowTextW(g_tagTitle,g_tracks[ix].title);SetWindowTextW(g_tagArtist,g_tracks[ix].artist);SetWindowTextW(g_tagAlbum,g_tracks[ix].album);SetWindowTextW(g_tagGenre,g_tracks[ix].genre);SetWindowTextW(g_tagYear,g_tracks[ix].year);ShowWindow(g_tag,SW_SHOW);SetForegroundWindow(g_tag);SetFocus(g_tagTitle);InvalidateRect(g_tag,0,FALSE);}

static void JumpFind(){if(!g_jumpText[0])return;for(int i=0;i<g_count;i++){if(WContainsI(g_tracks[i].display,g_jumpText)){g_selected=i;g_scroll=Clamp(i-3,0,MaxI(0,g_count-12));InvalidateRect(g_pl,0,FALSE);return;}}}

static DWORD WINAPI RGScanThread(LPVOID p){HRESULT ci=CoInitializeEx(0,COINIT_MULTITHREADED);int mode=(int)(LONG_PTR)p;if(mode==1){int ix=g_selected;if(ix>=0&&ix<g_count){g_scanTotal=1;g_scanProgress=0;double pk=0;double rg=OzAudioScanReplayGain(g_tracks[ix].path,&pk);g_tracks[ix].replayGainDb=rg;g_tracks[ix].peak=pk;SaveTrackStats(g_tracks[ix]);g_scanProgress=1;}}else{int n=g_count;g_scanTotal=n;g_scanProgress=0;for(int i=0;i<n;i++){double pk=0;double rg=OzAudioScanReplayGain(g_tracks[i].path,&pk);g_tracks[i].replayGainDb=rg;g_tracks[i].peak=pk;SaveTrackStats(g_tracks[i]);g_scanProgress=i+1;}}AtomicExchange(&g_scanDone,1);g_scanRunning=false;if(SUCCEEDED(ci))CoUninitialize();return 0;}
static void StartRGScan(bool all){if(g_scanRunning)return;AtomicExchange(&g_scanDone,0);g_scanRunning=true;SetStatus(all?L"LOUDNESS SCAN ALL":L"LOUDNESS SCAN");HANDLE h=CreateThread(0,0,RGScanThread,(LPVOID)(LONG_PTR)(all?2:1),0,0);if(h)CloseHandle(h);else g_scanRunning=false;}
#define WM_OZTRAY (WM_APP+25)
static const GUID CLSID_TaskbarList_={0x56FDF344,0xFD6D,0x11D0,{0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90}};
static const GUID IID_ITaskbarList3_={0xEA1AFB91,0x9E28,0x4B86,{0x90,0xE9,0x9E,0x9F,0x8A,0x5E,0xE8,0x4}};
struct TaskbarV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*HrInit)(void*);HRESULT(WINAPI*AddTab)(void*,HWND);HRESULT(WINAPI*DeleteTab)(void*,HWND);HRESULT(WINAPI*ActivateTab)(void*,HWND);HRESULT(WINAPI*SetActiveAlt)(void*,HWND);HRESULT(WINAPI*MarkFullscreenWindow)(void*,HWND,BOOL);HRESULT(WINAPI*SetProgressValue)(void*,HWND,ULONGLONG,ULONGLONG);HRESULT(WINAPI*SetProgressState)(void*,HWND,int);};
static void TaskbarInit(){if(g_taskbar)return;void*p=0;if(SUCCEEDED(CoCreateInstance(&CLSID_TaskbarList_,0,CLSCTX_ALL,&IID_ITaskbarList3_,&p))&&p){TaskbarV*v=*(TaskbarV**)p;if(SUCCEEDED(v->HrInit(p)))g_taskbar=p;else v->Release(p);}}
static void TaskbarUpdate(){if(!g_taskbar||!g_main)return;TaskbarV*v=*(TaskbarV**)g_taskbar;if(g_current<0||g_length<=0)v->SetProgressState(g_taskbar,g_main,0);else{v->SetProgressState(g_taskbar,g_main,g_paused?8:(g_playing?2:0));v->SetProgressValue(g_taskbar,g_main,(ULONGLONG)Clamp(g_pos,0,g_length),(ULONGLONG)g_length);}}
static void TaskbarDone(){if(g_taskbar){TaskbarV*v=*(TaskbarV**)g_taskbar;v->SetProgressState(g_taskbar,g_main,0);v->Release(g_taskbar);g_taskbar=0;}}
static void AddTray(){if(g_trayAdded||!g_main)return;NOTIFYICONDATAW n;memset(&n,0,sizeof(n));n.cbSize=sizeof(n);n.hWnd=g_main;n.uID=1;n.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;n.uCallbackMessage=WM_OZTRAY;n.hIcon=LoadIconW(0,IDI_APPLICATION);WCopy(n.szTip,L"OzAmp 1.0.0",128);if(Shell_NotifyIconW(NIM_ADD,&n))g_trayAdded=true;}
static void RemoveTray(){if(!g_trayAdded)return;NOTIFYICONDATAW n;memset(&n,0,sizeof(n));n.cbSize=sizeof(n);n.hWnd=g_main;n.uID=1;Shell_NotifyIconW(NIM_DELETE,&n);g_trayAdded=false;}
static void NotifyTrack(){if(!g_trackNotify||g_current<0)return;if(g_trayAdded){NOTIFYICONDATAW n;memset(&n,0,sizeof(n));n.cbSize=sizeof(n);n.hWnd=g_main;n.uID=1;n.uFlags=NIF_INFO;WCopy(n.szInfoTitle,L"OzAmp — Now playing",64);WCopy(n.szInfo,g_tracks[g_current].display,256);n.dwInfoFlags=NIIF_INFO;Shell_NotifyIconW(NIM_MODIFY,&n);}ShowTrackToast();}
static void TrayMenu(){POINT p;GetCursorPos(&p);HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,1,g_playing?L"Pause":L"Play");AppendMenuW(m,MF_STRING,2,L"Previous");AppendMenuW(m,MF_STRING,3,L"Next");AppendMenuW(m,MF_STRING|(g_muted?MF_CHECKED:0),4,L"Mute");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,5,L"Show OzAmp");AppendMenuW(m,MF_STRING,6,L"Exit");SetForegroundWindow(g_main);UINT c=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,g_main,0);DestroyMenu(m);if(c==1)TogglePlay();else if(c==2)Prev();else if(c==3)Next();else if(c==4)ToggleMute();else if(c==5){ShowWindow(g_main,SW_SHOW);SetForegroundWindow(g_main);}else if(c==6)DestroyWindow(g_main);}
static void RegisterGlobals(){if(!g_globalHotkeys||!g_main)return;RegisterHotKey(g_main,1,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_SPACE);RegisterHotKey(g_main,2,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_LEFT);RegisterHotKey(g_main,3,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_RIGHT);RegisterHotKey(g_main,4,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_UP);RegisterHotKey(g_main,5,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,VK_DOWN);RegisterHotKey(g_main,6,MOD_NOREPEAT,0xB3);RegisterHotKey(g_main,7,MOD_NOREPEAT,0xB1);RegisterHotKey(g_main,8,MOD_NOREPEAT,0xB0);RegisterHotKey(g_main,9,MOD_NOREPEAT,0xB2);}
static void UnregisterGlobals(){if(!g_main)return;for(int i=1;i<=9;i++)UnregisterHotKey(g_main,i);}
static void SetGlobalHotkeys(bool on){UnregisterGlobals();g_globalHotkeys=on;if(on)RegisterGlobals();}
static OzAudioDevice g_devices[24]; static int g_deviceCount=0;
static int DeviceItemForId(const wchar_t*id){if(!id||!id[0])return 0;for(int i=0;i<g_deviceCount;i++)if(WEqI(id,g_devices[i].id))return i+1;return -1;}
static void RefreshDevices(){g_deviceCount=OzAudioEnumerate(g_devices,24);int item=DeviceItemForId(g_deviceId);if(g_settingsPendingDevice<0||g_settingsPendingDevice>g_deviceCount)g_settingsPendingDevice=item>=0?item:0;if(g_settings)InvalidateRect(g_settings,0,FALSE);if(g_about)InvalidateRect(g_about,0,FALSE);if(g_info)InvalidateRect(g_info,0,FALSE);if(g_tag)InvalidateRect(g_tag,0,FALSE);}
static void SetOutputSwitchInfo(const wchar_t*prefix,const wchar_t*stage,HRESULT hr){WCopy(g_outputSwitchInfo,prefix,192);if(stage&&stage[0]){WCat(g_outputSwitchInfo,L" // ",192);WCat(g_outputSwitchInfo,stage,192);}g_outputSwitchHr=hr;}
static bool ReinitOutput(const wchar_t*wanted,bool useDefault){
 wchar_t oldId[256];WCopy(oldId,g_deviceId,256);bool was=g_playing;int pos=g_pos,cur=g_current;if(cur>=0)CloseTrack();
 bool ok=OzAudioInit(useDefault?0:wanted);HRESULT failHr=S_OK;wchar_t failStage[96]=L"";
 if(ok&&!useDefault){const wchar_t*actual=OzAudioCurrentDeviceId();if(!actual||!actual[0]||!WEqI(actual,wanted)){ok=false;failHr=(HRESULT)0x80004005L;WCopy(failStage,L"Endpoint verification mismatch",96);}}
 if(ok){if(useDefault){g_deviceId[0]=0;g_preferredDeviceId[0]=0;}else{WCopy(g_deviceId,wanted,256);WCopy(g_preferredDeviceId,wanted,256);}WritePrivateProfileStringW(L"audio",L"device",g_deviceId,g_ini);SetOutputSwitchInfo(L"ACTIVE OUTPUT CHANGED",OzAudioCurrentDeviceName(),S_OK);SetStatus(L"OUTPUT CHANGED");}
 else{if(failStage[0]==0){failHr=OzAudioLastError();WCopy(failStage,OzAudioLastErrorStage(),96);}OzAudioShutdown();bool restored=OzAudioInit(oldId[0]?oldId:0);WCopy(g_deviceId,oldId,256);SetOutputSwitchInfo(L"OUTPUT CHANGE FAILED",failStage,failHr);SetStatus(restored?L"OUTPUT CHANGE FAILED":L"AUDIO OUTPUT ERROR");}
 if(cur>=0){if(OpenTrack(cur,false)){SeekTo(pos);if(was)Play();}}RefreshDevices();g_settingsPendingDevice=DeviceItemForId(g_deviceId);if(g_settingsPendingDevice<0)g_settingsPendingDevice=0;if(g_settings)InvalidateRect(g_settings,0,FALSE);return ok;
}
static void ApplyPendingDevice(){if(g_settingsPendingDevice<=0)ReinitOutput(0,true);else if(g_settingsPendingDevice-1<g_deviceCount)ReinitOutput(g_devices[g_settingsPendingDevice-1].id,false);}
static void SwitchDevice(int ix){if(ix<0||ix>=g_deviceCount)return;g_settingsPendingDevice=ix+1;ApplyPendingDevice();}
static void SwitchDefaultDevice(){g_settingsPendingDevice=0;ApplyPendingDevice();}
static bool AudioDeviceAvailable(const wchar_t*id){if(!id||!id[0])return true;OzAudioDevice ds[24];int n=OzAudioEnumerate(ds,24);for(int i=0;i<n;i++)if(WEqI(ds[i].id,id))return true;return false;}
static void AutoFallbackDefault(){bool was=g_playing;int pos=g_pos,cur=g_current;if(cur>=0)CloseTrack();if(OzAudioInit(0)){SetOutputSwitchInfo(L"OUTPUT LOST - USING WINDOWS DEFAULT",OzAudioCurrentDeviceName(),S_OK);SetStatus(L"OUTPUT -> WINDOWS DEFAULT");Feedback(L"AUDIO OUTPUT CHANGED TO WINDOWS DEFAULT",1800);if(cur>=0&&OpenTrack(cur,false)){SeekTo(pos);if(was)Play();}}else{SetOutputSwitchInfo(L"AUDIO OUTPUT RECOVERY FAILED",OzAudioLastErrorStage(),OzAudioLastError());ShowOzError(L"AUDIO OUTPUT ERROR",L"The active audio device disappeared and Windows default output could not be opened.",-1);}}
static void MonitorAudioDevice(){ULONGLONG now=GetTickCount64();if(now-g_lastDeviceCheck<1800)return;g_lastDeviceCheck=now;const wchar_t*active=OzAudioCurrentDeviceId();bool err=FAILED(OzAudioLastError());if(g_preferredDeviceId[0]){bool available=AudioDeviceAvailable(g_preferredDeviceId);if(available&&(!active||!active[0]||!WEqI(active,g_preferredDeviceId))){if(ReinitOutput(g_preferredDeviceId,false))Feedback(L"PREFERRED AUDIO OUTPUT RESTORED",1500);return;}if(!available&&active&&active[0]&&WEqI(active,g_preferredDeviceId)){AutoFallbackDefault();return;}}if(err&&g_nativeAudio)AutoFallbackDefault();}

static void SaveIniInt(const wchar_t*sec,const wchar_t*key,int v){wchar_t n[32];IntToW(v,n);WritePrivateProfileStringW(sec,key,n,g_ini);}
static void NamedEQCurve(int p,int&pre,int bands[10]){
 static const int rock[10]={4,3,1,-1,-2,0,2,3,4,4};static const int metal[10]={5,4,2,-1,-2,0,3,5,5,4};static const int bass[10]={7,6,5,3,1,0,-1,-1,0,1};static const int classic[10]={2,1,0,0,-1,-1,0,2,3,4};static const int vocal[10]={-2,-1,0,2,4,5,4,2,0,-1};
 pre=0;for(int i=0;i<10;i++)bands[i]=p==1?rock[i]:(p==2?metal[i]:(p==3?bass[i]:(p==4?classic[i]:(p==5?vocal[i]:0))));
}
static void SaveEQStateNow(){wchar_t n[32];WritePrivateProfileStringW(L"eq",L"enabled",g_eqEnabled?L"1":L"0",g_ini);IntToW(g_preampDb,n);WritePrivateProfileStringW(L"eq",L"preamp",n,g_ini);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);IntToW(g_eqBands[eb],n);WritePrivateProfileStringW(L"eq",key,n,g_ini);}IntToW(g_eqPreset,n);WritePrivateProfileStringW(L"eq",L"preset",n,g_ini);WritePrivateProfileStringW(L"eq",L"dirty",g_eqCustomDirty?L"1":L"0",g_ini);WritePrivateProfileStringW(L"eq_custom",L"saved",g_eqCustomSaved?L"1":L"0",g_ini);IntToW(g_eqCustomPreamp,n);WritePrivateProfileStringW(L"eq_custom",L"preamp",n,g_ini);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);IntToW(g_eqCustomBands[eb],n);WritePrivateProfileStringW(L"eq_custom",key,n,g_ini);}WritePrivateProfileStringW(0,0,0,g_ini);}
static void SaveWindowPositions(){RECT r;if(g_main&&GetWindowRect(g_main,&r)){SaveIniInt(L"window",L"main_x",r.left);SaveIniInt(L"window",L"main_y",r.top);}if(g_pl&&GetWindowRect(g_pl,&r)){SaveIniInt(L"window",L"pl_x",r.left);SaveIniInt(L"window",L"pl_y",r.top);}if(g_eq&&GetWindowRect(g_eq,&r)){SaveIniInt(L"window",L"eq_x",r.left);SaveIniInt(L"window",L"eq_y",r.top);}if(g_lib&&GetWindowRect(g_lib,&r)){SaveIniInt(L"window",L"lib_x",r.left);SaveIniInt(L"window",L"lib_y",r.top);}if(g_art&&GetWindowRect(g_art,&r)){SaveIniInt(L"window",L"art_x",r.left);SaveIniInt(L"window",L"art_y",r.top);}if(g_viz&&GetWindowRect(g_viz,&r)){SaveIniInt(L"window",L"viz_x",r.left);SaveIniInt(L"window",L"viz_y",r.top);}}
static void SaveSessionExtras(){SaveWindowPositions();SaveIniInt(L"player",L"state",g_playing?1:(g_paused?2:0));SaveIniInt(L"queue",L"count",g_queueCount);for(int i=0;i<128;i++){wchar_t k[16]=L"item000";k[4]=(wchar_t)(L'0'+(i/100)%10);k[5]=(wchar_t)(L'0'+(i/10)%10);k[6]=(wchar_t)(L'0'+i%10);k[7]=0;WritePrivateProfileStringW(L"queue",k,i<g_queueCount?g_queuePaths[i]:0,g_ini);}}
static void LoadSessionExtras(){g_restoreState=Clamp((int)GetPrivateProfileIntW(L"player",L"state",0,g_ini),0,2);g_restoreMainX=(int)GetPrivateProfileIntW(L"window",L"main_x",140,g_ini);g_restoreMainY=(int)GetPrivateProfileIntW(L"window",L"main_y",120,g_ini);g_restorePlX=(int)GetPrivateProfileIntW(L"window",L"pl_x",140,g_ini);g_restorePlY=(int)GetPrivateProfileIntW(L"window",L"pl_y",340,g_ini);g_restoreEqX=(int)GetPrivateProfileIntW(L"window",L"eq_x",650,g_ini);g_restoreEqY=(int)GetPrivateProfileIntW(L"window",L"eq_y",120,g_ini);g_restoreLibX=(int)GetPrivateProfileIntW(L"window",L"lib_x",140,g_ini);g_restoreLibY=(int)GetPrivateProfileIntW(L"window",L"lib_y",430,g_ini);g_restoreArtX=(int)GetPrivateProfileIntW(L"window",L"art_x",650,g_ini);g_restoreArtY=(int)GetPrivateProfileIntW(L"window",L"art_y",300,g_ini);g_restoreVizX=(int)GetPrivateProfileIntW(L"window",L"viz_x",220,g_ini);g_restoreVizY=(int)GetPrivateProfileIntW(L"window",L"viz_y",220,g_ini);g_queueCount=Clamp((int)GetPrivateProfileIntW(L"queue",L"count",0,g_ini),0,128);for(int i=0;i<g_queueCount;i++){wchar_t k[16]=L"item000";k[4]=(wchar_t)(L'0'+(i/100)%10);k[5]=(wchar_t)(L'0'+(i/10)%10);k[6]=(wchar_t)(L'0'+i%10);k[7]=0;GetPrivateProfileStringW(L"queue",k,L"",g_queuePaths[i],MAXP,g_ini);}WCopy(g_preferredDeviceId,g_deviceId,256);}
static void RestoreWindowPositions(){int sc=UIScale();if(g_main)MoveWindow(g_main,g_restoreMainX,g_restoreMainY,MAIN_W*sc,(g_shade?SHADE_H:MAIN_H)*sc,TRUE);if(g_pl&&!g_plDockEdge)MoveWindow(g_pl,g_restorePlX,g_restorePlY,g_plW*sc,g_plH*sc,TRUE);if(g_eq&&!g_eqDockEdge)MoveWindow(g_eq,g_restoreEqX,g_restoreEqY,EQ_W*sc,EQ_H*sc,TRUE);if(g_lib&&!g_libDockEdge)MoveWindow(g_lib,g_restoreLibX,g_restoreLibY,700*sc,420*sc,TRUE);if(g_art&&!g_artDockEdge)MoveWindow(g_art,g_restoreArtX,g_restoreArtY,260*sc,300*sc,TRUE);if(g_viz&&!g_vizDockEdge)MoveWindow(g_viz,g_restoreVizX,g_restoreVizY,800*sc,450*sc,TRUE);}

static void SaveSettings(){wchar_t n[32];IntToW(g_volume,n);WritePrivateProfileStringW(L"player",L"volume",n,g_ini);IntToW(g_balance,n);WritePrivateProfileStringW(L"player",L"balance",n,g_ini);WritePrivateProfileStringW(L"player",L"shuffle",g_shuffle?L"1":L"0",g_ini);IntToW(g_repeat,n);WritePrivateProfileStringW(L"player",L"repeat",n,g_ini);IntToW(g_current,n);WritePrivateProfileStringW(L"player",L"current",n,g_ini);IntToW(g_pos,n);WritePrivateProfileStringW(L"player",L"position",n,g_ini);WritePrivateProfileStringW(L"player",L"remaining",g_timeRemaining?L"1":L"0",g_ini);IntToW(g_crossfadeSec,n);WritePrivateProfileStringW(L"audio",L"crossfade",n,g_ini);WritePrivateProfileStringW(L"audio",L"gapless",g_gapless?L"1":L"0",g_ini);WritePrivateProfileStringW(L"audio",L"replaygain",g_replayGainEnabled?L"1":L"0",g_ini);WritePrivateProfileStringW(L"audio",L"sleep_after_current",g_sleepAfterCurrent?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"track_notify",g_trackNotify?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"global_hotkeys",g_globalHotkeys?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"skin_file",g_skinFile,g_ini);IntToW(g_visMode,n);WritePrivateProfileStringW(L"player",L"visualizer",n,g_ini);WritePrivateProfileStringW(L"ui",L"playlist",g_plVisible?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"equalizer",g_eqVisible?L"1":L"0",g_ini);WritePrivateProfileStringW(L"eq",L"enabled",g_eqEnabled?L"1":L"0",g_ini);IntToW(g_preampDb,n);WritePrivateProfileStringW(L"eq",L"preamp",n,g_ini);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);IntToW(g_eqBands[eb],n);WritePrivateProfileStringW(L"eq",key,n,g_ini);}IntToW(g_eqPreset,n);WritePrivateProfileStringW(L"eq",L"preset",n,g_ini);WritePrivateProfileStringW(L"eq",L"dirty",g_eqCustomDirty?L"1":L"0",g_ini);WritePrivateProfileStringW(L"eq_custom",L"saved",g_eqCustomSaved?L"1":L"0",g_ini);IntToW(g_eqCustomPreamp,n);WritePrivateProfileStringW(L"eq_custom",L"preamp",n,g_ini);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);IntToW(g_eqCustomBands[eb],n);WritePrivateProfileStringW(L"eq_custom",key,n,g_ini);}WritePrivateProfileStringW(L"ui",L"topmost",g_top?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"shade",g_shade?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"double_size",g_doubleSize?L"1":L"0",g_ini);IntToW(g_skin,n);WritePrivateProfileStringW(L"ui",L"skin",n,g_ini);IntToW(g_plW,n);WritePrivateProfileStringW(L"ui",L"playlist_width",n,g_ini);IntToW(g_plH,n);WritePrivateProfileStringW(L"ui",L"playlist_height",n,g_ini);IntToW(g_plDockEdge,n);WritePrivateProfileStringW(L"ui",L"playlist_dock",n,g_ini);IntToW(g_plDockTarget,n);WritePrivateProfileStringW(L"dock",L"playlist_target",n,g_ini);IntToW(g_plDockAlign,n);WritePrivateProfileStringW(L"dock",L"playlist_align",n,g_ini);IntToW(g_plDockOffset,n);WritePrivateProfileStringW(L"dock",L"playlist_offset",n,g_ini);IntToW(g_plRightAnchorTarget,n);WritePrivateProfileStringW(L"dock",L"playlist_right_anchor",n,g_ini);WritePrivateProfileStringW(L"ui",L"library",g_libVisible?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"album_art",g_artVisible?L"1":L"0",g_ini);WritePrivateProfileStringW(L"ui",L"visualizer_window",g_vizVisible?L"1":L"0",g_ini);IntToW(g_eqDockEdge,n);WritePrivateProfileStringW(L"dock",L"eq",n,g_ini);IntToW(g_eqDockTarget,n);WritePrivateProfileStringW(L"dock",L"eq_target",n,g_ini);IntToW(g_libDockEdge,n);WritePrivateProfileStringW(L"dock",L"library",n,g_ini);IntToW(g_artDockEdge,n);WritePrivateProfileStringW(L"dock",L"album_art",n,g_ini);IntToW(g_vizDockEdge,n);WritePrivateProfileStringW(L"dock",L"visualizer",n,g_ini);SaveM3UPath(g_playlistFile);SaveLibrary();SaveSessionExtras();}
static void LoadSettings(){g_volume=Clamp((int)GetPrivateProfileIntW(L"player",L"volume",78,g_ini),0,100);g_preMuteVolume=g_volume?g_volume:78;g_balance=Clamp((int)GetPrivateProfileIntW(L"player",L"balance",0,g_ini),-100,100);g_shuffle=GetPrivateProfileIntW(L"player",L"shuffle",0,g_ini)!=0;g_repeat=Clamp((int)GetPrivateProfileIntW(L"player",L"repeat",0,g_ini),0,2);g_restoreIndex=(int)GetPrivateProfileIntW(L"player",L"current",-1,g_ini);g_restorePos=(int)GetPrivateProfileIntW(L"player",L"position",0,g_ini);g_timeRemaining=GetPrivateProfileIntW(L"player",L"remaining",0,g_ini)!=0;g_visMode=Clamp((int)GetPrivateProfileIntW(L"player",L"visualizer",0,g_ini),0,3);g_plVisible=GetPrivateProfileIntW(L"ui",L"playlist",1,g_ini)!=0;g_top=GetPrivateProfileIntW(L"ui",L"topmost",0,g_ini)!=0;g_shade=GetPrivateProfileIntW(L"ui",L"shade",0,g_ini)!=0;g_doubleSize=GetPrivateProfileIntW(L"ui",L"double_size",0,g_ini)!=0;g_skin=Clamp((int)GetPrivateProfileIntW(L"ui",L"skin",0,g_ini),0,2);g_plW=Clamp((int)GetPrivateProfileIntW(L"ui",L"playlist_width",PL_DEFAULT_W,g_ini),PL_MIN_W,1400);g_plH=Clamp((int)GetPrivateProfileIntW(L"ui",L"playlist_height",PL_DEFAULT_H,g_ini),PL_MIN_H,1000);g_plDockEdge=Clamp((int)GetPrivateProfileIntW(L"ui",L"playlist_dock",1,g_ini),0,4);g_plDockTarget=Clamp((int)GetPrivateProfileIntW(L"dock",L"playlist_target",0,g_ini),0,1);g_plDockAlign=Clamp((int)GetPrivateProfileIntW(L"dock",L"playlist_align",2,g_ini),0,2);g_plDockOffset=(int)GetPrivateProfileIntW(L"dock",L"playlist_offset",0,g_ini);g_plRightAnchorTarget=Clamp((int)GetPrivateProfileIntW(L"dock",L"playlist_right_anchor",g_plDockAlign==2?g_plDockTarget:-1,g_ini),-1,1);g_crossfadeSec=Clamp((int)GetPrivateProfileIntW(L"audio",L"crossfade",4,g_ini),0,12);g_gapless=GetPrivateProfileIntW(L"audio",L"gapless",1,g_ini)!=0;g_replayGainEnabled=GetPrivateProfileIntW(L"audio",L"replaygain",1,g_ini)!=0;g_trackNotify=GetPrivateProfileIntW(L"ui",L"track_notify",1,g_ini)!=0;g_globalHotkeys=GetPrivateProfileIntW(L"ui",L"global_hotkeys",1,g_ini)!=0;GetPrivateProfileStringW(L"ui",L"skin_file",L"",g_skinFile,MAXP,g_ini);GetPrivateProfileStringW(L"audio",L"device",L"",g_deviceId,256,g_ini);g_sleepAfterCurrent=GetPrivateProfileIntW(L"audio",L"sleep_after_current",0,g_ini)!=0;g_eqVisible=GetPrivateProfileIntW(L"ui",L"equalizer",1,g_ini)!=0;g_eqEnabled=GetPrivateProfileIntW(L"eq",L"enabled",0,g_ini)!=0;g_preampDb=Clamp((int)GetPrivateProfileIntW(L"eq",L"preamp",0,g_ini),-12,12);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);g_eqBands[eb]=Clamp((int)GetPrivateProfileIntW(L"eq",key,0,g_ini),-12,12);}g_eqPreset=Clamp((int)GetPrivateProfileIntW(L"eq",L"preset",0,g_ini),0,6);g_eqCustomDirty=GetPrivateProfileIntW(L"eq",L"dirty",0,g_ini)!=0;g_eqCustomSaved=GetPrivateProfileIntW(L"eq_custom",L"saved",0,g_ini)!=0;g_eqCustomPreamp=Clamp((int)GetPrivateProfileIntW(L"eq_custom",L"preamp",0,g_ini),-12,12);for(int eb=0;eb<10;eb++){wchar_t key[8]=L"b0";key[1]=(wchar_t)(L'0'+eb);g_eqCustomBands[eb]=Clamp((int)GetPrivateProfileIntW(L"eq_custom",key,0,g_ini),-12,12);}if(g_eqPreset>=0&&g_eqPreset<=5&&!g_eqCustomDirty)NamedEQCurve(g_eqPreset,g_preampDb,g_eqBands);g_libVisible=GetPrivateProfileIntW(L"ui",L"library",0,g_ini)!=0;g_artVisible=GetPrivateProfileIntW(L"ui",L"album_art",0,g_ini)!=0;g_vizVisible=GetPrivateProfileIntW(L"ui",L"visualizer_window",0,g_ini)!=0;g_eqDockEdge=Clamp((int)GetPrivateProfileIntW(L"dock",L"eq",3,g_ini),0,4);g_eqDockTarget=Clamp((int)GetPrivateProfileIntW(L"dock",L"eq_target",0,g_ini),0,1);g_libDockEdge=Clamp((int)GetPrivateProfileIntW(L"dock",L"library",1,g_ini),0,4);g_artDockEdge=Clamp((int)GetPrivateProfileIntW(L"dock",L"album_art",3,g_ini),0,4);g_vizDockEdge=Clamp((int)GetPrivateProfileIntW(L"dock",L"visualizer",0,g_ini),0,4);LoadSessionExtras();}

static HBRUSH Brush(DWORD c){return CreateSolidBrush(c);} static HPEN Pen(DWORD c,int w=1){return CreatePen(PS_SOLID,w,c);}
static void Fill(HDC dc,int x1,int y1,int x2,int y2,DWORD c){RECT r={x1,y1,x2,y2};HBRUSH b=Brush(c);FillRect(dc,&r,b);DeleteObject(b);}
static void Box(HDC dc,int x1,int y1,int x2,int y2,DWORD fill,DWORD edge,int rad=10){HBRUSH b=Brush(fill);HPEN p=Pen(edge);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,x1,y1,x2,y2,rad,rad);SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(p);DeleteObject(b);}
static void Line(HDC dc,int x1,int y1,int x2,int y2,DWORD c,int w=1){HPEN p=Pen(c,w);HGDIOBJ o=SelectObject(dc,p);MoveToEx(dc,x1,y1,0);LineTo(dc,x2,y2);SelectObject(dc,o);DeleteObject(p);}
static void Txt(HDC dc,const wchar_t*s,int x1,int y1,int x2,int y2,DWORD c,HFONT f,UINT flags){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);HGDIOBJ o=SelectObject(dc,f);RECT r={x1,y1,x2,y2};DrawTextW(dc,s,-1,&r,flags|DT_NOPREFIX);SelectObject(dc,o);}

static void RoundWindow(HWND h,int radius=14){if(!h)return;RECT r;GetWindowRect(h,&r);int w=r.right-r.left,hh=r.bottom-r.top;if(w<=0||hh<=0)return;HANDLE rg=CreateRoundRectRgn(0,0,w+1,hh+1,radius*UIScale(),radius*UIScale());if(rg)SetWindowRgn(h,rg,TRUE);}
static void RoundCoreWindows(){RoundWindow(g_main,g_shade?10:14);RoundWindow(g_pl,14);RoundWindow(g_eq,14);if(g_lib)RoundWindow(g_lib,14);if(g_art)RoundWindow(g_art,14);if(g_viz&&!g_vizFull)RoundWindow(g_viz,14);if(g_settings)RoundWindow(g_settings,14);if(g_about)RoundWindow(g_about,14);if(g_info)RoundWindow(g_info,14);if(g_tag)RoundWindow(g_tag,14);if(g_error)RoundWindow(g_error,14);}
static void ButtonFX(HDC dc,int x1,int y1,int x2,int y2,const wchar_t*label,bool active,bool danger,bool hover,bool pressed){
 DWORD f=active?C_ACCENT:C_PANEL2,e=active?C_ACCENT:(danger?C_RED:C_EDGE),t=active?C_BLACK:(danger?C_RED:C_TEXT);
 if(hover&&!active&&!danger)e=C_LED2;
 if(pressed){f=C_ACCENT;e=C_LED;t=C_BLACK;y1++;y2++;}
 Box(dc,x1,y1,x2,y2,f,e,8);
 if(!active&&!danger&&!pressed)Line(dc,x1+8,y1+2,x2-8,y1+2,hover?C_EDGE:C_PANEL,1);
 Txt(dc,label,x1,y1,x2,y2,t,g_small,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
static void Button(HDC dc,int x1,int y1,int x2,int y2,const wchar_t*label,bool active=false,bool danger=false){ButtonFX(dc,x1,y1,x2,y2,label,active,danger,false,false);}
static void MainButton(HDC dc,int id,int x1,int y1,int x2,int y2,const wchar_t*label,bool active=false,bool danger=false){bool flash=g_actionFlashHit==id&&GetTickCount64()<g_actionFlashUntil;ButtonFX(dc,x1,y1,x2,y2,label,active,danger,g_hoverHit==id,g_pressedHit==id||flash);}
static void EQButton(HDC dc,int id,int x1,int y1,int x2,int y2,const wchar_t*label,bool active=false,bool danger=false){ButtonFX(dc,x1,y1,x2,y2,label,active,danger,g_eqHover==id,g_eqPressed==id);}
static void PLButton(HDC dc,int id,int x1,int y1,int x2,int y2,const wchar_t*label,bool active=false,bool danger=false){ButtonFX(dc,x1,y1,x2,y2,label,active,danger,g_plHover==id,g_plPressed==id);}


static void DrawTagEditor(HDC dc){
 const int W=500,H=338;
 Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,29,C_PANEL2,C_EDGE,12);
 Txt(dc,L"OZAMP // METADATA EDITOR",12,3,360,27,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(dc,W-28,4,W-4,25,L"X",false,true);
 const int lx=22,ex=122,ew=350;const wchar_t*labels[5]={L"TITLE",L"ARTIST",L"ALBUM",L"GENRE",L"YEAR"};int ys[5]={52,94,136,178,220};
 for(int i=0;i<5;i++){Txt(dc,labels[i],lx,ys[i],110,ys[i]+25,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);int ww=(i==4?120:ew);Box(dc,ex-4,ys[i]-3,ex+ww+4,ys[i]+28,C_PANEL2,C_EDGE,8);}
 Box(dc,18,263,W-18,298,C_PANEL,C_EDGE,9);Txt(dc,L"Metadata overrides stay local. MP3 files also receive an ID3v1 compatibility tag.",28,265,W-28,296,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 Button(dc,286,305,375,332,L"SAVE",true,false);Button(dc,383,305,474,332,L"CANCEL",false,false);
}
static void PaintTagEditor(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,500,338);HGDIOBJ old=SelectObject(mem,bm);DrawTagEditor(mem);BitBlt(dc,0,0,500,338,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);}

static void DrawTrackInfo(HDC dc){
 const int W=INFO_W,H=INFO_H;Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,29,C_PANEL2,C_EDGE,12);Txt(dc,L"OZAMP // TRACK INFO",12,3,360,27,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(dc,W-28,4,W-4,25,L"X",false,true);
 if(g_infoIndex<0||g_infoIndex>=g_count){Txt(dc,L"NO TRACK SELECTED",24,55,W-24,85,C_MUTED,g_bold,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return;}
 Track&t=g_tracks[g_infoIndex];
 Box(dc,14,40,W-14,253,C_PANEL,C_EDGE,11);Txt(dc,t.display,26,48,W-26,78,C_LED,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 const wchar_t*labs[5]={L"TITLE",L"ARTIST",L"ALBUM",L"GENRE",L"YEAR"};const wchar_t*vals[5]={t.title,t.artist,t.album,t.genre,t.year};int y=84;
 for(int i=0;i<5;i++){Txt(dc,labs[i],26,y,105,y+25,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,vals[i][0]?vals[i]:L"—",112,y,W-28,y+25,C_TEXT,g_font,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);y+=31;}
 wchar_t tech[220];BuildTrackTechLine(t,tech,220);Box(dc,14,261,W-14,293,C_PANEL2,C_EDGE,9);Txt(dc,tech[0]?tech:L"LOCAL AUDIO FILE",25,263,W-25,291,C_LED2,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 Box(dc,14,301,W-14,348,C_PANEL,C_EDGE,9);Txt(dc,t.path,25,305,W-25,344,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_WORDBREAK|DT_END_ELLIPSIS);
}
static void PaintTrackInfo(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,INFO_W,INFO_H);HGDIOBJ old=SelectObject(mem,bm);DrawTrackInfo(mem);BitBlt(dc,0,0,INFO_W,INFO_H,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);}
static void ShowTrackInfo(int ix){if(ix<0||ix>=g_count||!g_info)return;g_infoIndex=ix;RECT pr;GetWindowRect(g_pl?g_pl:g_main,&pr);int x=pr.left+24,y=pr.top+34;SetWindowPos(g_info,g_top?HWND_TOPMOST:(HWND)0,x,y,INFO_W,INFO_H,SWP_NOSIZE);ShowWindow(g_info,SW_SHOW);SetForegroundWindow(g_info);InvalidateRect(g_info,0,FALSE);}
static LRESULT CALLBACK InfoProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:PaintTrackInfo(h);return 0;case WM_NCHITTEST:{POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(p.y<29&&p.x<INFO_W-34)return HTCAPTION;return HTCLIENT;}case WM_LBUTTONUP:{int x=LOWORDi(l),y=HIWORDi(l);if(PtIn(x,y,INFO_W-28,4,INFO_W-4,25)){ShowWindow(h,SW_HIDE);return 0;}return 0;}case WM_KEYDOWN:if(w==VK_ESCAPE){ShowWindow(h,SW_HIDE);return 0;}return 0;case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}

static void SaveTagEditor(){if(g_tagIndex<0||g_tagIndex>=g_count)return;Track t=g_tracks[g_tagIndex];GetWindowTextW(g_tagTitle,t.title,160);GetWindowTextW(g_tagArtist,t.artist,128);GetWindowTextW(g_tagAlbum,t.album,128);GetWindowTextW(g_tagGenre,t.genre,64);GetWindowTextW(g_tagYear,t.year,16);SyncEditedTrack(t);WriteID3v1(t);ShowWindow(g_tag,SW_HIDE);SetStatus(L"METADATA SAVED");Feedback(L"METADATA SAVED");}
static LRESULT CALLBACK TagProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_CREATE:g_tagTitle=MkCtl(h,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,122,52,350,25,1001);g_tagArtist=MkCtl(h,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,122,94,350,25,1002);g_tagAlbum=MkCtl(h,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,122,136,350,25,1003);g_tagGenre=MkCtl(h,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,122,178,350,25,1004);g_tagYear=MkCtl(h,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,122,220,120,25,1005);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:PaintTagEditor(h);return 0;
 case 0x0133:{HDC dc=(HDC)w;static HBRUSH br=0;static DWORD last=0;if(!br||last!=C_PANEL2){if(br)DeleteObject(br);br=CreateSolidBrush(C_PANEL2);last=C_PANEL2;}SetTextColor(dc,C_TEXT);SetBkColor(dc,C_PANEL2);return (LRESULT)br;}
 case WM_NCHITTEST:{POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(p.y<29&&p.x<466)return HTCAPTION;return HTCLIENT;}
 case WM_LBUTTONUP:{int x=LOWORDi(l),y=HIWORDi(l);if(PtIn(x,y,472,4,496,25)){ShowWindow(h,SW_HIDE);return 0;}if(PtIn(x,y,286,305,375,332)){SaveTagEditor();return 0;}if(PtIn(x,y,383,305,474,332)){ShowWindow(h,SW_HIDE);return 0;}return 0;}
 case WM_KEYDOWN:if(w==VK_ESCAPE){KillTimer(h,2);ShowWindow(h,SW_HIDE);return 0;}return 0;
 case WM_CLOSE:KillTimer(h,2);ShowWindow(h,SW_HIDE);return 0;
 }return DefWindowProcW(h,m,w,l);}

static void ShowTrackToast(){if(!g_toast||g_current<0)return;int W=360,H=86,x=GetSystemMetrics(SM_CXSCREEN)-W-18,y=GetSystemMetrics(SM_CYSCREEN)-H-54;MoveWindow(g_toast,x,y,W,H,TRUE);g_toastUntil=GetTickCount64()+4200;ShowWindow(g_toast,SW_SHOW);InvalidateRect(g_toast,0,FALSE);}
static LRESULT CALLBACK ToastProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);Fill(dc,0,0,r.right,r.bottom,C_PANEL2);Box(dc,1,1,r.right-1,r.bottom-1,C_PANEL2,C_ACCENT,8);Txt(dc,L"OZAMP // NOW PLAYING",14,8,r.right-12,28,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);if(g_current>=0)Txt(dc,g_tracks[g_current].display,14,29,r.right-12,55,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);if(g_current>=0&&g_tracks[g_current].album[0])Txt(dc,g_tracks[g_current].album,14,55,r.right-12,76,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);EndPaint(h,&ps);return 0;}case WM_LBUTTONUP:ShowWindow(h,SW_HIDE);g_toastUntil=0;return 0;case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}

static const wchar_t* EQN[10]={L"31",L"62",L"125",L"250",L"500",L"1K",L"2K",L"4K",L"8K",L"16K"};
static void ApplyEQ(){OzAudioSetEQ(g_eqEnabled,g_preampDb,g_eqBands);if(g_eq)InvalidateRect(g_eq,0,FALSE);if(g_main)InvalidateRect(g_main,0,FALSE);}
static const wchar_t* EQPresetName(int p){static const wchar_t* n[7]={L"Flat",L"Rock",L"Metal",L"Bass Boost",L"Classical",L"Vocal",L"Custom"};return (p>=0&&p<7)?n[p]:L"Custom";}
static void EQPreset(int p){if(p==6){if(!g_eqCustomSaved)return;g_preampDb=g_eqCustomPreamp;for(int i=0;i<10;i++)g_eqBands[i]=g_eqCustomBands[i];g_eqPreset=6;g_eqCustomDirty=false;}else{NamedEQCurve(Clamp(p,0,5),g_preampDb,g_eqBands);g_eqPreset=Clamp(p,0,5);g_eqCustomDirty=false;}g_eqEnabled=true;ApplyEQ();SaveEQStateNow();wchar_t fb[80];WCopy(fb,L"EQ PRESET  ",80);WCat(fb,EQPresetName(g_eqPreset),80);Feedback(fb);if(g_eq)InvalidateRect(g_eq,0,FALSE);}
static void SaveCustomEQ(){g_eqCustomPreamp=g_preampDb;for(int i=0;i<10;i++)g_eqCustomBands[i]=g_eqBands[i];g_eqCustomSaved=true;g_eqCustomDirty=false;g_eqPreset=6;SaveEQStateNow();SetStatus(L"EQ CUSTOM SAVED");InvalidateRect(g_eq,0,FALSE);}
static void ShowEQPresetMenu(HWND h){POINT p;GetCursorPos(&p);HMENU mm=CreatePopupMenu();const wchar_t*nn[7]={L"Flat",L"Rock",L"Metal",L"Bass Boost",L"Classical",L"Vocal",L"Custom"};for(int i=0;i<7;i++){UINT fl=MF_STRING|((g_eqPreset==i&&!g_eqCustomDirty)?MF_CHECKED:0);if(i==6&&!g_eqCustomSaved)fl|=MF_GRAYED;AppendMenuW(mm,fl,300+i,nn[i]);}UINT c=TrackPopupMenu(mm,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,h,0);DestroyMenu(mm);if(c>=300&&c<=306)EQPreset((int)c-300);}
static int EQHit(int x,int y){if(y<24){if(x>=390&&x<468)return 1;if(x>=472)return 2;}if(y>=48&&y<=128){if(x<45)return 10;int i=(x-39)/42;if(i>=0&&i<10)return 20+i;}if(y>=151&&y<181){if(x>=58&&x<270)return 40;if(x>=278&&x<337)return 41;if(x>=344&&x<405)return 42;}return 0;}
static const wchar_t* EQHoverText(int id){if(id==1){if(g_dsActive)return L"EQ setting is saved, but DirectShow compatibility playback bypasses OzAmp PCM DSP";return g_eqEnabled?L"EQ ON — click to bypass all EQ + preamp DSP":L"EQ OFF — click to enable EQ + preamp DSP";}if(id==2)return L"Close Equalizer window (settings stay active)";if(id==10)return L"PREAMP — overall EQ gain, -12 to +12 dB";if(id>=20&&id<30)return L"EQ BAND — drag vertically to boost/cut this frequency";if(id==40)return L"PRESET — choose Flat, Rock, Metal, Bass Boost, Classical, Vocal or Custom";if(id==41)return L"SAVE — store current curve as Custom preset";if(id==42)return L"FLAT — reset all bands and preamp to 0 dB";return L"";}
static void DrawEQ(HDC dc){const int W=EQ_W,H=EQ_H;Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,25,C_PANEL2,C_EDGE,12);Txt(dc,L"10-BAND EQUALIZER",10,2,210,23,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);EQButton(dc,1,390,3,468,21,g_eqEnabled?L"EQ ON":L"EQ OFF",g_eqEnabled);EQButton(dc,2,472,3,496,21,L"X",false,true);
 Box(dc,8,30,492,148,C_PANEL,C_EDGE,10);Txt(dc,L"PRE",6,34,40,49,C_MUTED,g_small,DT_CENTER|DT_SINGLELINE);Box(dc,19,49,25,128,C_PANEL2,C_EDGE,5);int py=88-(g_preampDb*3);Box(dc,14,py-4,30,py+5,g_eqEnabled?C_ACCENT:C_MUTED,g_eqEnabled?C_ACCENT:C_MUTED,8);wchar_t pv[16];IntToW(g_preampDb,pv);WCat(pv,L" dB",16);Txt(dc,pv,2,130,45,146,g_eqEnabled?C_TEXT:C_MUTED,g_small,DT_CENTER|DT_SINGLELINE);
 for(int i=0;i<10;i++){int x=60+i*42;Txt(dc,EQN[i],x-15,34,x+20,49,C_MUTED,g_small,DT_CENTER|DT_SINGLELINE);Box(dc,x-3,49,x+4,128,C_PANEL2,C_EDGE,5);int yy=88-(g_eqBands[i]*3);Box(dc,x-7,yy-4,x+8,yy+5,g_eqEnabled?C_LED2:C_MUTED,g_eqEnabled?C_LED2:C_MUTED,8);wchar_t v[8];IntToW(g_eqBands[i],v);Txt(dc,v,x-16,130,x+18,146,g_eqEnabled?C_TEXT:C_MUTED,g_small,DT_CENTER|DT_SINGLELINE);}
 Txt(dc,L"PRESET",8,153,55,181,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 DWORD pe=(g_eqHover==40)?C_LED2:C_EDGE;Box(dc,58,151,270,181,(g_eqPressed==40)?C_PANEL2:C_PANEL,pe,8);
 wchar_t pn[96];WCopy(pn,EQPresetName(g_eqPreset),96);if(g_eqCustomDirty)WCat(pn,L" *",96);
 Txt(dc,pn,67,151,240,181,C_TEXT,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 Line(dc,247,163,253,169,(g_eqHover==40)?C_LED:C_MUTED,2);Line(dc,253,169,259,163,(g_eqHover==40)?C_LED:C_MUTED,2);
 EQButton(dc,41,278,151,337,181,L"SAVE");EQButton(dc,42,344,151,405,181,L"FLAT");const wchar_t*eqState=g_nativeAudio?(g_eqEnabled?L"DSP ACTIVE":L"BYPASSED"):(g_eqEnabled?L"NO PCM DSP":L"BYPASSED");Txt(dc,eqState,406,151,494,181,(g_nativeAudio&&g_eqEnabled)?C_LED2:C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);if(g_eqHover){const wchar_t*tip=EQHoverText(g_eqHover);if(tip&&tip[0]){Box(dc,95,26,495,47,C_BLACK,C_EDGE,7);Txt(dc,tip,102,26,489,47,C_LED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}}}
static void PaintEQ(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC m=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,EQ_W,EQ_H);HGDIOBJ o=SelectObject(m,b);DrawEQ(m);RECT rc;GetClientRect(h,&rc);if(UIScale()==1)BitBlt(dc,0,0,EQ_W,EQ_H,m,0,0,SRCCOPY);else StretchBlt(dc,0,0,rc.right,rc.bottom,m,0,0,EQ_W,EQ_H,SRCCOPY);SelectObject(m,o);DeleteObject(b);DeleteDC(m);EndPaint(h,&ps);}
static LRESULT CALLBACK EqProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:PaintEQ(h);return 0;case WM_MOVE:SnapEQ();if(g_plVisible&&g_plDockTarget==1&&g_plDockEdge)DockPlaylist();return 0;case WM_NCHITTEST:{LRESULT r=DefWindowProcW(h,m,w,l);if(r==HTCLIENT){POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(LY(p.y)<24&&EQHit(LX(p.x),LY(p.y))==0)return HTCAPTION;}return r;}case WM_LBUTTONDOWN:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));int hit=EQHit(x,y);if(hit==10){g_eqDrag=-1;SetCapture(h);}else if(hit>=20&&hit<30){g_eqDrag=hit-20;SetCapture(h);}else if(hit){g_eqPressed=hit;SetCapture(h);InvalidateRect(h,0,FALSE);}return 0;}case WM_MOUSEMOVE:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));int nh=EQHit(x,y);if(nh!=g_eqHover){g_eqHover=nh;InvalidateRect(h,0,FALSE);}if(g_eqDrag!=-99&&(w&1)){int db=Clamp((88-y)/3,-12,12);if(g_eqDrag==-1)g_preampDb=db;else g_eqBands[g_eqDrag]=db;g_eqPreset=6;g_eqCustomDirty=true;ApplyEQ();}return 0;}case WM_LBUTTONUP:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));if(g_eqDrag!=-99){g_eqDrag=-99;ReleaseCapture();ApplyEQ();SaveEQStateNow();Feedback(L"EQ CURVE UPDATED",850);return 0;}int hit=EQHit(x,y),pressed=g_eqPressed;g_eqPressed=0;ReleaseCapture();InvalidateRect(h,0,FALSE);if(pressed&&hit!=pressed)return 0;if(hit==1){g_eqEnabled=!g_eqEnabled;ApplyEQ();SaveEQStateNow();SetStatus(g_eqEnabled?L"EQ ENABLED":L"EQ BYPASSED");Feedback(g_eqEnabled?L"EQ  ON":L"EQ  BYPASSED");InvalidateRect(h,0,FALSE);}else if(hit==2){g_eqVisible=false;ShowWindow(h,SW_HIDE);Feedback(L"EQUALIZER HIDDEN");InvalidateRect(g_main,0,FALSE);}else if(hit==40)ShowEQPresetMenu(h);else if(hit==41){SaveCustomEQ();Feedback(L"EQ CUSTOM PRESET SAVED");}else if(hit==42)EQPreset(0);return 0;}case WM_RBUTTONUP:ShowEQPresetMenu(h);return 0;case WM_KEYDOWN:if(w=='D'&&CtrlDown())ToggleDoubleSize();else if(w==VK_ESCAPE){g_eqVisible=false;ShowWindow(h,SW_HIDE);InvalidateRect(g_main,0,FALSE);}else if(w=='E'){g_eqEnabled=!g_eqEnabled;ApplyEQ();SaveEQStateNow();InvalidateRect(h,0,FALSE);}return 0;case WM_CLOSE:g_eqVisible=false;ShowWindow(h,SW_HIDE);InvalidateRect(g_main,0,FALSE);return 0;}return DefWindowProcW(h,m,w,l);}

// ---------------- Media Library ----------------
static int FindLibraryPath(const wchar_t*p){for(int i=0;i<g_libCount;i++)if(WEqI(g_library[i].path,p))return i;return -1;}
static int AddLibraryPath(const wchar_t*p);
static int AddLibraryFolder(const wchar_t*dir,int depth){if(depth>24||g_libCount>=MAX_LIBRARY)return -1;int first=-1;wchar_t pat[MAXP];WCopy(pat,dir,MAXP);WCat(pat,L"\\*",MAXP);WIN32_FIND_DATAW fd;HANDLE h=FindFirstFileW(pat,&fd);if(h==INVALID_HANDLE_VALUE)return -1;do{if(fd.cFileName[0]=='.'&&(fd.cFileName[1]==0||(fd.cFileName[1]=='.'&&fd.cFileName[2]==0)))continue;wchar_t p[MAXP];WCopy(p,dir,MAXP);WCat(p,L"\\",MAXP);WCat(p,fd.cFileName,MAXP);int ix=(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?AddLibraryFolder(p,depth+1):AddLibraryPath(p);if(first<0&&ix>=0)first=ix;}while(FindNextFileW(h,&fd));FindClose(h);return first;}
static int AddLibraryPath(const wchar_t*p){if(!p||!p[0]||g_libCount>=MAX_LIBRARY)return -1;if(IsDir(p))return AddLibraryFolder(p,0);if(!Supported(p))return -1;int old=FindLibraryPath(p);if(old>=0)return old;Track&t=g_library[g_libCount];memset(&t,0,sizeof(t));WCopy(t.path,p,MAXP);LoadMetadata(p,t);LoadTrackStats(t);if(t.addedOrder<0){t.addedOrder=++g_libraryAddSerial;SaveTrackStats(t);}else if(t.addedOrder>g_libraryAddSerial)g_libraryAddSerial=t.addedOrder;return g_libCount++;}
static bool SaveLibrary(){HANDLE h=CreateFileW(g_libraryFile,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;const char head[]="#EXTM3U\r\n";DWORD w;WriteFile(h,head,sizeof(head)-1,&w,0);char u[4096];for(int i=0;i<g_libCount;i++){int n=WideCharToMultiByte(CP_UTF8,0,g_library[i].path,-1,u,4090,0,0);if(n>1){u[n-1]='\r';u[n]='\n';WriteFile(h,u,n+1,&w,0);}}CloseHandle(h);return true;}
static void LoadLibrary(){HANDLE h=CreateFileW(g_libraryFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return;DWORD sz=GetFileSize(h,0);if(sz>16*1024*1024){CloseHandle(h);return;}char*d=(char*)HeapAlloc(GetProcessHeap(),0,sz+2);DWORD r=0;if(!d||!ReadFile(h,d,sz,&r,0)){if(d)HeapFree(GetProcessHeap(),0,d);CloseHandle(h);return;}CloseHandle(h);d[r]=0;char*line=d;if(r>=3&&(BYTE)d[0]==0xef&&(BYTE)d[1]==0xbb&&(BYTE)d[2]==0xbf)line+=3;while(*line){char*e=line;while(*e&&*e!='\r'&&*e!='\n')e++;char sv=*e;*e=0;if(*line&&*line!='#'){wchar_t w[MAXP];if(MultiByteToWideChar(CP_UTF8,0,line,-1,w,MAXP)>0)AddLibraryPath(w);}*e=sv;while(*e=='\r'||*e=='\n')e++;line=e;}HeapFree(GetProcessHeap(),0,d);}
static void LibraryAddFolderDialog(){wchar_t display[MAX_PATH],path[MAXP];BROWSEINFOW b;memset(&b,0,sizeof(b));b.hwndOwner=g_lib;b.pszDisplayName=display;b.lpszTitle=L"Add music folder to OzAmp Library";b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_EDITBOX;void*pid=SHBrowseForFolderW(&b);if(pid){if(SHGetPathFromIDListW(pid,path)){AddLibraryFolder(path,0);if(g_libSelected>=g_libCount)g_libSelected=g_libCount-1;SaveLibrary();InvalidateRect(g_lib,0,FALSE);}CoTaskMemFree(pid);}}
static void LibraryRescan(){for(int i=g_libCount-1;i>=0;i--){if(GetFileAttributesW(g_library[i].path)==0xffffffffUL){for(int j=i;j<g_libCount-1;j++)g_library[j]=g_library[j+1];g_libCount--;continue;}LoadMetadata(g_library[i].path,g_library[i]);LoadTrackStats(g_library[i]);if(g_library[i].addedOrder<0){g_library[i].addedOrder=++g_libraryAddSerial;SaveTrackStats(g_library[i]);}else if(g_library[i].addedOrder>g_libraryAddSerial)g_libraryAddSerial=g_library[i].addedOrder;}if(g_libSelected>=g_libCount)g_libSelected=g_libCount-1;SaveLibrary();InvalidateRect(g_lib,0,FALSE);}
static bool LibMatch(const Track&t){if(g_libView==6&&!t.bookmark)return false;if(g_libView==8&&t.playCount<=0)return false;if(g_libSearch[0]&&!WContainsI(t.display,g_libSearch)&&!WContainsI(t.album,g_libSearch)&&!WContainsI(t.artist,g_libSearch)&&!WContainsI(t.genre,g_libSearch)&&!WContainsI(t.year,g_libSearch)&&!WContainsI(t.path,g_libSearch))return false;return true;}
static int LibVisibleCount(){int n=0;for(int i=0;i<g_libCount;i++)if(LibMatch(g_library[i]))n++;return n;}
static int LibraryCmp(const Track&a,const Track&b,int view){int c=0;if(view==1)c=WCompareI(a.artist,b.artist);else if(view==2)c=WCompareI(a.album,b.album);else if(view==3)c=WCompareI(a.genre,b.genre);else if(view==4)c=WCompareI(a.year,b.year);else if(view==5){wchar_t ad[MAXP],bd[MAXP];DirName(a.path,ad,MAXP);DirName(b.path,bd,MAXP);c=WCompareI(ad,bd);}else c=WCompareI(a.display,b.display);if(c==0)c=WCompareI(a.display,b.display);return c;}
static void LibraryQuickSort(int lo,int hi,int view){int i=lo,j=hi;Track pivot=g_library[(lo+hi)/2];while(i<=j){while(LibraryCmp(g_library[i],pivot,view)<0)i++;while(LibraryCmp(g_library[j],pivot,view)>0)j--;if(i<=j){if(i!=j){Track t=g_library[i];g_library[i]=g_library[j];g_library[j]=t;}i++;j--;}}if(lo<j)LibraryQuickSort(lo,j,view);if(i<hi)LibraryQuickSort(i,hi,view);}
static void SortLibraryView(int view){if(g_libCount<2)return;wchar_t sel[MAXP];sel[0]=0;if(g_libSelected>=0&&g_libSelected<g_libCount)WCopy(sel,g_library[g_libSelected].path,MAXP);if(view>=0&&view<=5)LibraryQuickSort(0,g_libCount-1,view);if(sel[0])g_libSelected=FindLibraryPath(sel);}
static int LibNth(int n){if(g_libView==7||g_libView==8){int chosen=-1;int lastScore=0x7fffffff,lastIx=-1;for(int rank=0;rank<=n;rank++){chosen=-1;int best=-0x7fffffff;for(int i=0;i<g_libCount;i++){if(!LibMatch(g_library[i]))continue;int score=g_libView==7?g_library[i].addedOrder:g_library[i].playCount;if(score<lastScore||(score==lastScore&&i>lastIx)){if(score>best){best=score;chosen=i;}}}if(chosen<0)return -1;lastScore=g_libView==7?g_library[chosen].addedOrder:g_library[chosen].playCount;lastIx=chosen;}return chosen;}for(int i=0;i<g_libCount;i++)if(LibMatch(g_library[i])){if(n--==0)return i;}return -1;}
static void DrawLibrary(HDC dc,int W,int H){Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,27,C_PANEL2,C_EDGE,12);Txt(dc,L"MEDIA LIBRARY",10,2,170,25,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(dc,W-26,3,W-4,23,L"X",false,true);const wchar_t*views[9]={L"ALL",L"ARTIST",L"ALBUM",L"GENRE",L"YEAR",L"FOLDER",L"FAV",L"RECENT",L"MOST"};int bw=72;for(int i=0;i<9;i++)Button(dc,8+i*bw,32,8+i*bw+bw-5,55,views[i],g_libView==i);wchar_t search[180];WCopy(search,L"SEARCH: ",180);WCat(search,g_libSearch[0]?g_libSearch:L"type to filter",180);Txt(dc,search,8,59,W-8,79,g_libSearch[0]?C_LED:C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);int by=H-42;Button(dc,8,by,92,by+30,L"ADD DIR");Button(dc,98,by,182,by+30,L"RESCAN");Button(dc,188,by,300,by+30,L"+ PLAYLIST");Button(dc,306,by,370,by+30,L"PLAY");Button(dc,376,by,455,by+30,L"CLEAR Q");int top=84,bottom=H-50;Box(dc,8,top,W-8,bottom,C_PANEL,C_EDGE,3);int rows=MaxI(1,(bottom-top-8)/18),vis=LibVisibleCount();g_libScroll=Clamp(g_libScroll,0,MaxI(0,vis-rows));for(int r=0;r<rows;r++){int li=LibNth(g_libScroll+r);if(li<0)break;Track&t=g_library[li];int y=top+4+r*18;if(li==g_libSelected)Fill(dc,12,y,W-12,y+18,C_ACCENT);wchar_t line[800],num[32],dir[MAXP];WCopy(line,t.bookmark?L"* ":L"  ",800);if(g_libView==1&&t.artist[0]){WCat(line,t.artist,800);WCat(line,L"  //  ",800);}else if(g_libView==2&&t.album[0]){WCat(line,t.album,800);WCat(line,L"  //  ",800);}else if(g_libView==3&&t.genre[0]){WCat(line,t.genre,800);WCat(line,L"  //  ",800);}else if(g_libView==4&&t.year[0]){WCat(line,t.year,800);WCat(line,L"  //  ",800);}else if(g_libView==5){DirName(t.path,dir,MAXP);WCat(line,BaseName(dir),800);WCat(line,L"  //  ",800);}WCat(line,t.display,800);if(t.playCount){WCat(line,L"  [",800);IntToW(t.playCount,num);WCat(line,num,800);WCat(line,L" plays]",800);}Txt(dc,line,16,y,W-20,y+18,li==g_libSelected?C_BLACK:C_TEXT,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}wchar_t st[140],n[24];WCopy(st,L"LIBRARY ",140);IntToW(g_libCount,n);WCat(st,n,140);WCat(st,L" tracks  //  visible ",140);IntToW(vis,n);WCat(st,n,140);Txt(dc,st,465,by,W-10,by+30,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);}
static void PaintLibrary(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC m=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ o=SelectObject(m,b);DrawLibrary(m,r.right,r.bottom);BitBlt(dc,0,0,r.right,r.bottom,m,0,0,SRCCOPY);SelectObject(m,o);DeleteObject(b);DeleteDC(m);EndPaint(h,&ps);}
static int LibraryHitRow(int y,int H){int top=84,bottom=H-50;if(y<top+4||y>=bottom)return -1;int n=g_libScroll+(y-(top+4))/18;return LibNth(n);}
static LRESULT CALLBACK LibProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_CREATE:DragAcceptFiles(h,TRUE);return 0;case WM_ERASEBKGND:return 1;case WM_PAINT:PaintLibrary(h);return 0;case WM_MOVE:SnapTool(h,g_libDockEdge);return 0;case WM_LBUTTONDOWN:{RECT r;GetClientRect(h,&r);int x=LOWORDi(l),y=HIWORDi(l);if(y>=32&&y<55){int v=(x-8)/72;if(v>=0&&v<9){g_libView=v;if(v<=5)SortLibraryView(v);g_libScroll=0;InvalidateRect(h,0,FALSE);return 0;}}int li=LibraryHitRow(y,r.bottom);if(li>=0){g_libSelected=li;InvalidateRect(h,0,FALSE);}return 0;}case WM_LBUTTONDBLCLK:{RECT r;GetClientRect(h,&r);int li=LibraryHitRow(HIWORDi(l),r.bottom);if(li>=0){int ix=AddPath(g_library[li].path);if(ix>=0)OpenTrack(ix,true);}return 0;}case WM_LBUTTONUP:{RECT r;GetClientRect(h,&r);int x=LOWORDi(l),y=HIWORDi(l),by=r.bottom-42;if(y<26&&x>=r.right-30){g_libVisible=false;ShowWindow(h,SW_HIDE);}else if(y>=by&&y<by+30){if(x<92)LibraryAddFolderDialog();else if(x<182)LibraryRescan();else if(x<300&&g_libSelected>=0)AddPath(g_library[g_libSelected].path);else if(x<370&&g_libSelected>=0){int ix=AddPath(g_library[g_libSelected].path);if(ix>=0)OpenTrack(ix,true);}else if(x<455){g_libSearch[0]=0;g_libScroll=0;InvalidateRect(h,0,FALSE);}}return 0;}case WM_MOUSEWHEEL:{int d=GET_WHEEL_DELTA_WPARAM(w);g_libScroll=MaxI(0,g_libScroll+(d>0?-3:3));InvalidateRect(h,0,FALSE);return 0;}case WM_CHAR:{wchar_t ch=(wchar_t)w;int n=WLen(g_libSearch);if(ch==8){if(n)g_libSearch[n-1]=0;}else if(ch>=32&&n<94){g_libSearch[n]=ch;g_libSearch[n+1]=0;}g_libScroll=0;InvalidateRect(h,0,FALSE);return 0;}case WM_DROPFILES:{HDROP d=(HDROP)w;UINT n=DragQueryFileW(d,0xffffffffUL,0,0);wchar_t p[MAXP];for(UINT i=0;i<n;i++)if(DragQueryFileW(d,i,p,MAXP))AddLibraryPath(p);DragFinish(d);SaveLibrary();InvalidateRect(h,0,FALSE);return 0;}case WM_KEYDOWN:if(w==VK_ESCAPE){g_libVisible=false;ShowWindow(h,SW_HIDE);}else if(w==VK_RETURN&&g_libSelected>=0){int ix=AddPath(g_library[g_libSelected].path);if(ix>=0)OpenTrack(ix,true);}return 0;case WM_CLOSE:g_libVisible=false;ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}


// ---------------- Album art ----------------
static bool FileExists(const wchar_t*p){DWORD a=GetFileAttributesW(p);return a!=0xffffffffUL&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
static void RelocateErrorTrack(){if(g_errorTrack<0||g_errorTrack>=g_count)return;wchar_t chosen[MAXP]=L"";OPENFILENAMEW o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=g_error;o.lpstrFile=chosen;o.nMaxFile=MAXP;o.lpstrFilter=L"Audio files\0*.mp3;*.wav;*.flac;*.aac;*.m4a;*.wma;*.mid;*.midi\0All files\0*.*\0\0";o.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST;if(!GetOpenFileNameW(&o))return;wchar_t old[MAXP];WCopy(old,g_tracks[g_errorTrack].path,MAXP);for(int i=0;i<g_queueCount;i++)if(WEqI(g_queuePaths[i],old))WCopy(g_queuePaths[i],chosen,MAXP);memset(&g_tracks[g_errorTrack],0,sizeof(Track));WCopy(g_tracks[g_errorTrack].path,chosen,MAXP);LoadMetadata(chosen,g_tracks[g_errorTrack]);LoadTrackStats(g_tracks[g_errorTrack]);SyncQueuedIndex();ShowWindow(g_error,SW_HIDE);OpenTrack(g_errorTrack,true);InvalidateRect(g_pl,0,FALSE);}
static void ShowOzError(const wchar_t*title,const wchar_t*body,int track){WCopy(g_errorTitle,title?title:L"OZAMP ERROR",96);WCopy(g_errorBody,body?body:L"An error occurred.",420);g_errorTrack=track;if(!g_error)return;RECT m;GetWindowRect(g_main,&m);SetWindowPos(g_error,g_top?HWND_TOPMOST:(HWND)0,m.left+20*UIScale(),m.top+42*UIScale(),0,0,SWP_NOSIZE);RoundWindow(g_error,14);ShowWindow(g_error,SW_SHOW);SetForegroundWindow(g_error);InvalidateRect(g_error,0,FALSE);}
static void DrawError(HDC dc){const int W=500,H=176;Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,29,C_PANEL2,C_RED,12);Txt(dc,L"OZAMP",12,3,85,27,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,g_errorTitle,88,3,W-40,27,C_RED,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);Button(dc,W-27,4,W-4,25,L"X",false,true);Box(dc,14,42,W-14,116,C_PANEL,C_EDGE,11);Txt(dc,g_errorBody,27,50,W-27,108,C_TEXT,g_small,DT_LEFT|DT_WORDBREAK);if(g_errorTrack>=0&&g_errorTrack<g_count){Button(dc,14,130,124,162,L"LOCATE...",false);Button(dc,132,130,254,162,L"REMOVE",false);}Button(dc,W-126,130,W-14,162,L"CLOSE",false);}
static LRESULT CALLBACK ErrorProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,500,176);HGDIOBJ old=SelectObject(mem,bm);DrawError(mem);BitBlt(dc,0,0,500,176,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);return 0;}case WM_NCHITTEST:{POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(p.y<29&&p.x<466)return HTCAPTION;return HTCLIENT;}case WM_LBUTTONUP:{int x=LOWORDi(l),y=HIWORDi(l);if(PtIn(x,y,473,4,496,25)||PtIn(x,y,374,130,486,162)){ShowWindow(h,SW_HIDE);return 0;}if(g_errorTrack>=0&&PtIn(x,y,14,130,124,162)){RelocateErrorTrack();return 0;}if(g_errorTrack>=0&&PtIn(x,y,132,130,254,162)){int ix=g_errorTrack;ShowWindow(h,SW_HIDE);if(ix>=0&&ix<g_count){g_selected=ix;ClearMarks();g_tracks[ix].marked=true;RemoveSelected();}return 0;}return 0;}case WM_KEYDOWN:if(w==VK_ESCAPE){ShowWindow(h,SW_HIDE);return 0;}break;case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}
static bool ExtractEmbeddedCover(const wchar_t*path,wchar_t*out){if(!WEndsI(path,L".mp3"))return false;HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;DWORD sz=GetFileSize(h,0);DWORD take=MinI((int)sz,2*1024*1024);BYTE*b=(BYTE*)HeapAlloc(GetProcessHeap(),0,take);DWORD r=0;bool ok=false;if(b&&ReadFile(h,b,take,&r,0)&&r>=10&&b[0]=='I'&&b[1]=='D'&&b[2]=='3'){int ver=b[3],tag=SyncSafe(b+6),end=MinI((int)r,10+tag),pos=10;while(pos+10<=end){BYTE*f=b+pos;if(!f[0])break;int fs=ver==4?SyncSafe(f+4):BE32(f+4);if(fs<=4||pos+10+fs>end)break;if(IdEq(f,"APIC")){BYTE*p=f+10,*lim=p+fs;p++;while(p<lim&&*p)p++;if(p>=lim)break;p++;if(p>=lim)break;p++;if(p>=lim)break;BYTE enc=f[10];if(enc==0||enc==3){while(p<lim&&*p)p++;if(p<lim)p++;}else{while(p+1<lim&&(p[0]||p[1]))p+=2;if(p+1<lim)p+=2;}if(p<lim&&lim-p>32){WCopy(out,g_dataDir,MAXP);WCat(out,L"\\embedded_cover.bin",MAXP);HANDLE o=CreateFileW(out,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(o!=INVALID_HANDLE_VALUE){DWORD wr=0;WriteFile(o,p,(DWORD)(lim-p),&wr,0);CloseHandle(o);ok=wr==(DWORD)(lim-p);}break;}}pos+=10+fs;}}if(b)HeapFree(GetProcessHeap(),0,b);CloseHandle(h);return ok;}
static void ReleaseCover(){if(g_coverBmp){DeleteObject(g_coverBmp);g_coverBmp=0;}g_coverPath[0]=0;}
static bool LoadBitmapGDI(const wchar_t*p){void*img=0;if(GdipLoadImageFromFile(p,&img)!=0||!img)return false;HBITMAP b=0;int st=GdipCreateHBITMAPFromBitmap(img,&b,RGBc(0,0,0));GdipDisposeImage(img);if(st||!b)return false;g_coverBmp=b;WCopy(g_coverPath,p,MAXP);return true;}
static void LoadCoverForCurrent(){ReleaseCover();if(g_current<0||g_current>=g_count){if(g_art)InvalidateRect(g_art,0,FALSE);return;}wchar_t dir[MAXP],p[MAXP];DirName(g_tracks[g_current].path,dir,MAXP);const wchar_t*names[]={L"folder.jpg",L"cover.jpg",L"front.jpg",L"album.jpg",L"folder.png",L"cover.png",L"front.png"};for(int i=0;i<7;i++){WCopy(p,dir,MAXP);WCat(p,L"\\",MAXP);WCat(p,names[i],MAXP);if(FileExists(p)&&LoadBitmapGDI(p)){if(g_art)InvalidateRect(g_art,0,FALSE);return;}}if(ExtractEmbeddedCover(g_tracks[g_current].path,p))LoadBitmapGDI(p);if(g_art)InvalidateRect(g_art,0,FALSE);}
static void DrawArt(HDC dc,int W,int H){Fill(dc,0,0,W,H,C_BG);Box(dc,1,1,W-1,27,C_PANEL2,C_EDGE,12);Txt(dc,L"ALBUM ART",10,2,140,25,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(dc,W-26,3,W-4,23,L"X",false,true);Box(dc,10,34,W-10,H-70,C_PANEL,C_EDGE,4);if(g_coverBmp){BITMAPW bm;memset(&bm,0,sizeof(bm));GetObjectW(g_coverBmp,sizeof(bm),&bm);HDC md=CreateCompatibleDC(dc);HGDIOBJ o=SelectObject(md,g_coverBmp);int aw=W-28,ah=H-116,iw=bm.bmWidth,ih=bm.bmHeight;if(iw>0&&ih>0){int dw=aw,dh=ih*aw/iw;if(dh>ah){dh=ah;dw=iw*ah/ih;}int x=(W-dw)/2,y=42+(ah-dh)/2;StretchBlt(dc,x,y,dw,dh,md,0,0,iw,ih,SRCCOPY);}SelectObject(md,o);DeleteDC(md);}else Txt(dc,L"NO COVER",10,34,W-10,H-70,C_MUTED,g_led,DT_CENTER|DT_VCENTER|DT_SINGLELINE);if(g_current>=0){Txt(dc,g_tracks[g_current].display,10,H-64,W-10,H-45,C_TEXT,g_bold,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);Txt(dc,g_tracks[g_current].album[0]?g_tracks[g_current].album:L"",10,H-44,W-10,H-27,C_MUTED,g_small,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}}
static void PaintArt(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC m=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ o=SelectObject(m,b);DrawArt(m,r.right,r.bottom);BitBlt(dc,0,0,r.right,r.bottom,m,0,0,SRCCOPY);SelectObject(m,o);DeleteObject(b);DeleteDC(m);EndPaint(h,&ps);}
static LRESULT CALLBACK ArtProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_ERASEBKGND:return 1;case WM_PAINT:PaintArt(h);return 0;case WM_MOVE:SnapTool(h,g_artDockEdge);return 0;case WM_NCHITTEST:{LRESULT r=DefWindowProcW(h,m,w,l);if(r==HTCLIENT){POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);RECT rc;GetClientRect(h,&rc);if(p.y<26&&p.x<rc.right-30)return HTCAPTION;}return r;}case WM_LBUTTONDOWN:{RECT r;GetClientRect(h,&r);if(HIWORDi(l)<26&&LOWORDi(l)>r.right-30){g_artVisible=false;ShowWindow(h,SW_HIDE);InvalidateRect(g_main,0,FALSE);return 0;}return 0;}case WM_LBUTTONUP:{RECT r;GetClientRect(h,&r);if(HIWORDi(l)<26&&LOWORDi(l)>r.right-30){g_artVisible=false;ShowWindow(h,SW_HIDE);InvalidateRect(g_main,0,FALSE);}return 0;}case WM_KEYDOWN:if(w==VK_ESCAPE){g_artVisible=false;ShowWindow(h,SW_HIDE);}return 0;case WM_CLOSE:g_artVisible=false;ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}


// ---------------- Full audio visualizer ----------------
static double VizClamp01(double v){return v<0?0:(v>1?1:v);}
static double VizBand(const float*sp,int a,int b){double v=0;if(b<=a)return 0;for(int i=a;i<b;i++)v+=sp[i];return v/(b-a);}
static DWORD PlasmaColor(double v,double intensity){
 static const int R[6]={5,18,31,45,116,202};
 static const int G[6]={10,35,78,178,88,72};
 static const int B[6]={22,86,150,190,196,156};
 v=VizClamp01(v);int seg=(int)(v*5.0);if(seg>4)seg=4;double f=v*5.0-seg;
 double gain=.38+.62*VizClamp01(intensity);
 int rr=(int)((R[seg]+(R[seg+1]-R[seg])*f)*gain);
 int gg=(int)((G[seg]+(G[seg+1]-G[seg])*f)*gain);
 int bb=(int)((B[seg]+(B[seg+1]-B[seg])*f)*gain);
 return RGBc((BYTE)Clamp(rr,0,255),(BYTE)Clamp(gg,0,255),(BYTE)Clamp(bb,0,255));
}
static void VizGlowLine(HDC dc,int x1,int y1,int x2,int y2,DWORD c){Line(dc,x1,y1,x2,y2,RGBc(18,31,49),4);Line(dc,x1,y1,x2,y2,c,1);}
static void DrawVizHeader(HDC dc,int W,bool full){if(full)return;Box(dc,1,1,W-1,29,C_PANEL2,C_EDGE,12);Txt(dc,L"OZAMP VISUALIZER",10,2,W-48,27,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(dc,W-28,3,W-4,24,L"X",false,true);}
static void DrawFullViz(HDC dc,int W,int H){
 Fill(dc,0,0,W,H,RGBc(3,7,14));
 float sp[64],wv[512];for(int i=0;i<64;i++)sp[i]=0;for(int i=0;i<512;i++)wv[i]=0;
 bool live=g_nativeAudio;if(live){OzAudioSpectrum(sp,64);OzAudioWaveform(wv,512);}ULONGLONG tm=GetTickCount64();
 int top=g_vizFull?0:28,HH=H-top;DrawVizHeader(dc,W,g_vizFull);
 if(!live){Txt(dc,L"Visualizer is available during native PCM playback.",18,top,W-18,H,C_MUTED,g_bold,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return;}
 double bass=VizBand(sp,0,7),mid=VizBand(sp,7,28),high=VizBand(sp,28,64),energy=VizBand(sp,0,48);
 static double smooth=.08,pulse=0;smooth=smooth*.86+energy*.14;if(bass>smooth*1.38&&bass>.09)pulse=1.0;else pulse*=.90;
 double t=tm/1000.0;
 // Dense, fixed-palette plasma field. Audio bends the field rather than cycling through unrelated scenes.
 int cell=MaxI(4,MinI(W,HH)/105);
 double cx=W*.50+sin(t*.31)*W*.07*mid,cy=top+HH*.48+cos(t*.27)*HH*.06*bass;
 for(int y=top;y<H;y+=cell){
  for(int x=0;x<W;x+=cell){
   double dx=x-cx,dy=y-cy,dist=sqrt(dx*dx+dy*dy);
   int si=Clamp((x*64)/MaxI(1,W),0,63);double a=sp[si];
   double v=sin(x*.018+t*.72+bass*2.4)+sin(y*.026-t*.58+mid*1.8)+sin((x+y)*.012+t*.36)+sin(dist*.031-t*.92+high*2.2);
   v=(v+4.0)/8.0;v=VizClamp01(v+a*.22+energy*.12*sin((x-y)*.015+t));
   DWORD c=PlasmaColor(v,.58+energy*.45+a*.22+pulse*.08);
   Fill(dc,x,y,MinI(W,x+cell+1),MinI(H,y+cell+1),c);
  }
 }
 // Secondary translucent-looking structure using dark scan lines and fixed cyan/violet accents.
 for(int y=top+2;y<H;y+=6)Line(dc,0,y,W,y,RGBc(5,11,21),1);
 int waveY=top+(int)(HH*.53),lastY=waveY;
 for(int x=0;x<W;x+=2){int ix=x*512/MaxI(1,W);int yy=waveY-(int)(wv[ix]*HH*.18*(1.0+energy*.55));if(x)VizGlowLine(dc,x-2,lastY,x,yy,RGBc(102,235,218));lastY=yy;}
 // Low, restrained spectrum at the bottom: information, not a second visual mode.
 int bars=64,bw=MaxI(2,W/bars),base=H-(g_vizFull?18:28),maxBh=MaxI(18,(int)(HH*.19));
 for(int i=0;i<bars;i++){int bh=(int)(VizClamp01(sp[i]*1.35)*maxBh);if(bh<2)bh=2;double q=i/63.0;DWORD c=q<.58?RGBc(77,205,194):RGBc(135,104,205);Fill(dc,i*bw+1,base-bh,i*bw+bw-1,base,c);}
 if(pulse>.18){DWORD pc=RGBc(108,226,210);Line(dc,1,top+1,W-2,top+1,pc,1);Line(dc,1,H-2,W-2,H-2,pc,1);}
 if(g_current>=0){
  Txt(dc,g_tracks[g_current].display,18,H-24,W-92,H-5,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  wchar_t tt[48],a[20],b[20];FormatTime(g_pos,a);FormatTime(g_length,b);WCopy(tt,a,48);WCat(tt,L" / ",48);WCat(tt,b,48);Txt(dc,tt,W-88,H-24,W-18,H-5,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 }
}
static void PaintViz(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC m=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ o=SelectObject(m,b);DrawFullViz(m,r.right,r.bottom);BitBlt(dc,0,0,r.right,r.bottom,m,0,0,SRCCOPY);SelectObject(m,o);DeleteObject(b);DeleteDC(m);EndPaint(h,&ps);}
static HWND ActiveVizWindow(){return g_vizFullWnd?g_vizFullWnd:g_viz;}
static void ToggleVizFull(){
 if(!g_viz)return;
 if(!g_vizFull){
  GetWindowRect(g_viz,&g_vizRestore);
  HMONITOR mon=MonitorFromWindow(g_viz,MONITOR_DEFAULTTONEAREST);MONITORINFO mi;mi.cbSize=sizeof(mi);RECT r={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};if(mon&&GetMonitorInfoW(mon,&mi))r=mi.rcMonitor;
  g_vizFull=true;
  ShowWindow(g_viz,SW_HIDE);
  g_vizFullWnd=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,L"OzAmpViz30",L"OzAmp Visualizer",WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,r.left,r.top,r.right-r.left,r.bottom-r.top,0,0,g_inst,0);
  if(!g_vizFullWnd){g_vizFull=false;ShowWindow(g_viz,SW_SHOW);return;}
  SetWindowRgn(g_vizFullWnd,0,TRUE);
  SetWindowPos(g_vizFullWnd,HWND_TOPMOST,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_SHOWWINDOW);
  SetForegroundWindow(g_vizFullWnd);SetFocus(g_vizFullWnd);InvalidateRect(g_vizFullWnd,0,FALSE);
 }else{
  HWND fw=g_vizFullWnd;g_vizFullWnd=0;g_vizFull=false;
  if(fw)DestroyWindow(fw);
  MoveWindow(g_viz,g_vizRestore.left,g_vizRestore.top,g_vizRestore.right-g_vizRestore.left,g_vizRestore.bottom-g_vizRestore.top,TRUE);
  RoundWindow(g_viz,14);ShowWindow(g_viz,SW_SHOW);SetForegroundWindow(g_viz);SetFocus(g_viz);InvalidateRect(g_viz,0,FALSE);
 }
}
static LRESULT CALLBACK VizProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:PaintViz(h);return 0;
 case WM_MOVE:if(h==g_viz&&!g_vizFull)SnapTool(h,g_vizDockEdge);return 0;
 case WM_NCHITTEST:{if(h==g_vizFullWnd)return HTCLIENT;LRESULT r=DefWindowProcW(h,m,w,l);if(r==HTCLIENT){POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);RECT rc;GetClientRect(h,&rc);if(p.y<28&&p.x<rc.right-32)return HTCAPTION;}return r;}
 case WM_LBUTTONDBLCLK:ToggleVizFull();return 0;
 case WM_NCLBUTTONDBLCLK:ToggleVizFull();return 0;
 case WM_LBUTTONUP:{if(h==g_viz&&!g_vizFull){RECT r;GetClientRect(h,&r);int x=LOWORDi(l),y=HIWORDi(l);if(y<28&&x>r.right-32){g_vizVisible=false;ShowWindow(h,SW_HIDE);return 0;}}return 0;}
 case WM_KEYDOWN:if(w==VK_ESCAPE){if(g_vizFull)ToggleVizFull();else{g_vizVisible=false;ShowWindow(g_viz,SW_HIDE);}}else if(w==VK_SPACE)TogglePlay();else if(w==VK_RIGHT)Next();else if(w==VK_LEFT)Prev();else if(w==VK_F11)ToggleVizFull();else if(w==VK_F12){if(g_vizFull)ToggleVizFull();g_vizVisible=false;ShowWindow(g_viz,SW_HIDE);}return 0;
 case WM_CLOSE:if(h==g_vizFullWnd){ToggleVizFull();return 0;}g_vizVisible=false;ShowWindow(g_viz,SW_HIDE);return 0;
 }return DefWindowProcW(h,m,w,l);}


static void DrawTransport(HDC dc,int id,int x,int y,int w,int h){
 bool hover=g_hoverHit==id,pressed=g_pressedHit==id||(g_actionFlashHit==id&&GetTickCount64()<g_actionFlashUntil);DWORD fill=pressed?C_ACCENT:(hover?C_PANEL:C_PANEL2),edge=pressed?C_LED:(hover?C_LED2:C_EDGE),c=pressed?C_BLACK:C_TEXT;
 int yy=pressed?y+1:y;Box(dc,x,yy,x+w,yy+h,fill,edge,8);
 int cx=x+w/2,cy=yy+h/2;HPEN p=Pen(c,2);HGDIOBJ o=SelectObject(dc,p);
 if(id==H_PLAY){MoveToEx(dc,cx-4,cy-7,0);LineTo(dc,cx+8,cy);LineTo(dc,cx-4,cy+7);LineTo(dc,cx-4,cy-7);}
 else if(id==H_PAUSE){Line(dc,cx-5,cy-7,cx-5,cy+7,c,3);Line(dc,cx+5,cy-7,cx+5,cy+7,c,3);}
 else if(id==H_STOP){HBRUSH b=Brush(c);RECT r={cx-6,cy-6,cx+6,cy+6};FillRect(dc,&r,b);DeleteObject(b);}
 else if(id==H_PREV||id==H_NEXT){int s=id==H_PREV?-1:1;Line(dc,cx-8*s,cy-7,cx+2*s,cy,c,2);Line(dc,cx+2*s,cy,cx-8*s,cy+7,c,2);Line(dc,cx+7*s,cy-7,cx+7*s,cy+7,c,2);}
 else if(id==H_EJECT){MoveToEx(dc,cx-7,cy+3,0);LineTo(dc,cx,cy-6);LineTo(dc,cx+7,cy+3);LineTo(dc,cx-7,cy+3);Line(dc,cx-7,cy+7,cx+7,cy+7,c,2);}
 SelectObject(dc,o);DeleteObject(p);
}

static void DrawVisualizer(HDC dc,int x,int y,int w,int h){
 Fill(dc,x,y,x+w,y+h,C_BLACK);
 if(g_nativeAudio){float sp[64];OzAudioSpectrum(sp,64);if(g_visMode==0){int bars=14,bw=(w-6)/bars;for(int i=0;i<bars;i++){int src=i*64/bars;int bh=(int)(sp[src]*(h-8));if(bh<2)bh=2;DWORD c=(i>10)?C_ACCENT:C_LED2;Fill(dc,x+3+i*bw,y+h-3-bh,x+3+i*bw+bw-2,y+h-3,c);}}else if(g_visMode==1){int lastY=y+h/2;for(int i=0;i<w-6;i+=3){int src=(i*64)/(w-6);int yy=y+h/2-(int)((sp[src]-.25f)*(h-8));yy=Clamp(yy,y+3,y+h-3);Line(dc,x+3+i-3,lastY,x+3+i,yy,C_LED2,1);lastY=yy;}}else{for(int i=0;i<32;i++){int ang=i;int r=(int)(sp[i]*(h/2-4));int xx=x+w/2+(i%8-4)*3;int yy=y+h/2-r;Fill(dc,xx,yy,xx+3,y+h/2,C_LED);}}return;}
 ULONGLONG t=GetTickCount64()/70;if(g_visMode==0){int bars=14,bw=(w-6)/bars;for(int i=0;i<bars;i++){int seed=(int)((t*(i+5)*13+i*i*17+(g_pos/47))&255);int bh=g_playing?(4+(seed%(h-9))):3;DWORD c=(i>10)?C_ACCENT:C_LED2;Fill(dc,x+3+i*bw,y+h-3-bh,x+3+i*bw+bw-2,y+h-3,c);}}else if(g_visMode==1){int lastY=y+h/2;for(int i=0;i<w-6;i+=3){int v=g_playing?(int)((((t+i*11+g_pos/23)*37)^(i*19))%MaxI(2,h-12)):0;int yy=y+6+(v%(h-12));Line(dc,x+3+i-3,lastY,x+3+i,yy,C_LED2,1);lastY=yy;}}else{for(int yy=y+5;yy<y+h-4;yy+=7)for(int xx=x+5;xx<x+w-4;xx+=7){int on=((xx*13+yy*7+(int)t*5+g_pos/71)%37)<(g_playing?9:2);if(on)Fill(dc,xx,yy,xx+2,yy+2,((xx+yy)&8)?C_LED:C_ACCENT);}}
}
static void DrawTitleMarquee(HDC dc,const wchar_t* title){
 const int x=108,y=37,w=195,h=21;
 Box(dc,105,34,306,61,C_BLACK,C_EDGE,3);
 HDC md=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,w,h);HGDIOBJ old=SelectObject(md,bm);Fill(md,0,0,w,h,C_BLACK);
 SIZEW ss;HGDIOBJ of=SelectObject(md,g_bold);GetTextExtentPoint32W(md,title,WLen(title),&ss);SetBkMode(md,TRANSPARENT);SetTextColor(md,C_LED);
 if(ss.cx<=w-4){RECT r={3,0,w-2,h};DrawTextW(md,title,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);}
 else{int gap=70,cycle=ss.cx+gap,off=g_marquee%cycle;RECT r={3-off,0,3-off+ss.cx+4,h};DrawTextW(md,title,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);r.left+=cycle;r.right+=cycle;DrawTextW(md,title,-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);}
 SelectObject(md,of);BitBlt(dc,x,y,w,h,md,0,0,SRCCOPY);SelectObject(md,old);DeleteObject(bm);DeleteDC(md);
}

static const wchar_t* MainHoverText(int id){switch(id){case H_PREV:return L"PREVIOUS — previous track [Z]";case H_PLAY:return L"PLAY / RESUME — start playback [X / Space]";case H_PAUSE:return L"PAUSE / RESUME — press again to continue [C]";case H_STOP:return L"STOP — stop and rewind [V]";case H_NEXT:return L"NEXT — next/queued track [B]";case H_EJECT:return L"OPEN — choose audio files and play immediately [L]";case H_SHUFFLE:return L"SHUFFLE — random playback order [S]";case H_REPEAT:return L"REPEAT — cycle Off / All / One";case H_AB:return L"A/B LOOP — set A, then B, then clear";case H_EQ:return L"EQUALIZER — show/hide EQ window [E]";case H_PL:return L"PLAYLIST — show/hide Playlist Editor [P]";case H_MUTE:return L"MUTE — mute/unmute [M]";case H_VOL:return L"VOLUME — drag or use mouse wheel / Up/Down";case H_BAL:return L"BALANCE — L / CENTER / R • drag left/right • double-click = center";case H_SEEK:return L"SEEK — drag through current track [Left/Right = 5 sec]";case H_VIS:return L"VISUALIZER — click to cycle compact visualizer mode";case H_TIME:return L"TIME — click to switch elapsed / remaining";case H_SETTINGS:return L"OZAMP — open the main menu • Settings [F10]";default:return L"";}}
static void DrawMainFull(HDC dc){
 Fill(dc,0,0,MAIN_W,MAIN_H,C_BG);
 // softly separated rounded title deck
 Box(dc,1,1,MAIN_W-1,25,C_PANEL2,C_EDGE,12);
 DWORD logoColor=(g_hoverHit==H_SETTINGS||g_pressedHit==H_SETTINGS)?C_LED:C_TEXT;Txt(dc,L"OZAMP",11,2,67,23,logoColor,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,L"1.0.0",70,3,130,22,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 MainButton(dc,H_MIN,MAIN_W-48,3,MAIN_W-27,21,L"_");MainButton(dc,H_CLOSE,MAIN_W-24,3,MAIN_W-3,21,L"X",false,true);
 // upper deck, matching the supplied rounded React concept
 Box(dc,10,31,315,97,C_PANEL,C_EDGE,11);DrawVisualizer(dc,17,38,82,52);
 wchar_t title[MAXD];if(g_current>=0)WCopy(title,g_tracks[g_current].display,MAXD);else WCopy(title,L"DROP MUSIC HERE OR PRESS EJECT",MAXD);
 DrawTitleMarquee(dc,title);
 wchar_t a[32],b[32],time[80];FormatTime(g_pos,a);FormatTime(g_length,b);if(g_timeRemaining&&g_length>0){wchar_t rem[32];FormatTime(MaxI(0,g_length-g_pos),rem);WCopy(time,L"-",80);WCat(time,rem,80);WCat(time,L" / ",80);WCat(time,b,80);}else{WCopy(time,a,80);WCat(time,L" / ",80);WCat(time,b,80);}Txt(dc,time,108,61,302,84,C_TEXT,g_led,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 Box(dc,322,31,490,97,C_PANEL,C_EDGE,11);
 Txt(dc,L"VOL",331,34,360,48,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 wchar_t vv[16];if(g_muted)WCopy(vv,L"MUTE",16);else{IntToW(g_volume,vv);WCat(vv,L"%",16);}Txt(dc,vv,438,34,480,48,g_muted?C_RED:C_LED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 Box(dc,363,47,478,53,C_PANEL2,(g_hoverHit==H_VOL||g_dragVol)?C_LED2:C_EDGE,5);int vx=363+(114*g_volume)/100;Box(dc,vx-4,43,vx+5,58,C_ACCENT,C_ACCENT,7);
 Txt(dc,L"BAL",331,58,360,72,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 wchar_t bv[24],bn[16];if(g_balance==0)WCopy(bv,L"CENTER",24);else{WCopy(bv,g_balance<0?L"L ":L"R ",24);IntToW(AbsI(g_balance),bn);WCat(bv,bn,24);WCat(bv,L"%",24);}Txt(dc,bv,420,58,480,72,g_balance==0?C_LED:C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 Box(dc,363,73,478,79,C_PANEL2,(g_hoverHit==H_BAL||g_dragBal)?C_LED2:C_EDGE,5);Line(dc,420,70,420,82,C_TEXT,1);int bx=420+(57*g_balance)/100;Box(dc,bx-4,69,bx+5,84,C_LED2,C_LED2,7);
 Txt(dc,L"L",360,82,370,95,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,L"C",414,82,426,95,g_balance==0?C_LED:C_MUTED,g_small,DT_CENTER|DT_VCENTER|DT_SINGLELINE);Txt(dc,L"R",471,82,481,95,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 // waveform seek strip: live PCM shape under the transport position
 Box(dc,10,99,490,116,C_PANEL,C_EDGE,8);Box(dc,16,102,484,113,C_BLACK,C_EDGE,4);if(g_nativeAudio&&g_current>=0){float wv[96];OzAudioWaveform(wv,96);for(int i=0;i<96;i++){int x=17+i*467/95;double av=wv[i]<0?-wv[i]:wv[i];int hh=1+(int)(av*11.0);if(hh>5)hh=5;Line(dc,x,107-hh,x,108+hh,(i%7==0)?C_LED:C_LED2,1);}}else{for(int x=18;x<483;x+=6)Line(dc,x,107,x,108,C_EDGE,1);}int sx=16;if(g_length>0)sx=16+(468*g_pos)/g_length;Fill(dc,16,112,sx,114,C_ACCENT);Box(dc,sx-3,100,sx+4,115,C_ACCENT,C_LED,6);
 int y=120;DrawTransport(dc,H_PREV,12,y,40,32);DrawTransport(dc,H_PLAY,56,y,40,32);DrawTransport(dc,H_PAUSE,100,y,40,32);DrawTransport(dc,H_STOP,144,y,40,32);DrawTransport(dc,H_NEXT,188,y,40,32);DrawTransport(dc,H_EJECT,232,y,40,32);
 MainButton(dc,H_SHUFFLE,282,y,326,y+32,L"SHF",g_shuffle);const wchar_t*rr=g_repeat==0?L"RPT":(g_repeat==1?L"R:A":L"R:1");MainButton(dc,H_REPEAT,330,y,370,y+32,rr,g_repeat!=0);const wchar_t*ab=g_abState==0?L"A/B":(g_abState==1?L"A1":L"A-B");MainButton(dc,H_AB,374,y,414,y+32,ab,g_abState!=0);MainButton(dc,H_EQ,418,y,451,y+32,L"EQ",g_eqVisible);MainButton(dc,H_PL,455,y,488,y+32,L"PL",g_plVisible);
 wchar_t st[160],num[32];WCopy(st,L"LOCAL  //  ",160);IntToW(g_count,num);WCat(st,num,160);WCat(st,L" TRACKS",160);if(g_queued>=0){WCat(st,L"  //  QUEUED ",160);IntToW(g_queued+1,num);WCat(st,num,160);}if(g_sleepMin){WCat(st,L"  //  SLEEP ",160);IntToW(g_sleepMin,num);WCat(st,num,160);WCat(st,L"M",160);}Box(dc,9,155,491,181,C_PANEL,C_EDGE,9);if(g_feedback[0]&&GetTickCount64()<g_feedbackUntil){Txt(dc,g_feedback,16,155,484,181,C_LED,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}else if(g_hoverHit!=H_NONE){const wchar_t*tip=MainHoverText(g_hoverHit);if(tip&&tip[0])Txt(dc,tip,16,155,484,181,C_LED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);else Txt(dc,st,16,155,484,181,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}else if(g_current>=0){wchar_t np[260],tech[180];np[0]=0;Track&t=g_tracks[g_current];if(t.artist[0])WCopy(np,t.artist,260);if(t.album[0]){if(np[0])WCat(np,L"  •  ",260);WCat(np,t.album,260);}if(t.year[0]){if(np[0])WCat(np,L"  •  ",260);WCat(np,t.year,260);}BuildTrackTechLine(t,tech,180);if(tech[0]){if(np[0])WCat(np,L"  //  ",260);WCat(np,tech,260);}Txt(dc,np[0]?np:st,16,155,484,181,np[0]?C_LED2:C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}else Txt(dc,st,16,155,484,181,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
}
static void DrawMainShade(HDC dc){Fill(dc,0,0,MAIN_W,SHADE_H,C_BG);Fill(dc,0,0,MAIN_W,SHADE_H,C_PANEL2);Txt(dc,L"OZAMP",9,0,62,SHADE_H,(g_hoverHit==H_SETTINGS||g_pressedHit==H_SETTINGS)?C_LED:C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);wchar_t t[32];FormatTime(g_pos,t);Txt(dc,t,62,0,112,SHADE_H,C_LED,g_led,DT_LEFT|DT_VCENTER|DT_SINGLELINE);const wchar_t*name=g_current>=0?g_tracks[g_current].display:L"NO TRACK";Txt(dc,name,114,0,309,SHADE_H,C_TEXT,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);MainButton(dc,H_PREV,312,5,342,29,L"|<");MainButton(dc,H_PLAY,346,5,378,29,g_playing?L"II":L">");MainButton(dc,H_NEXT,382,5,414,29,L">|");MainButton(dc,H_PL,418,5,450,29,L"PL",g_plVisible);MainButton(dc,H_CLOSE,456,5,496,29,L"X",false,true);int px=0;if(g_length>0)px=Clamp((MAIN_W*g_pos)/g_length,0,MAIN_W);Fill(dc,0,SHADE_H-2,px,SHADE_H,C_ACCENT);}
static void PaintMain(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT rc;GetClientRect(h,&rc);int lh=g_shade?SHADE_H:MAIN_H;HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,MAIN_W,lh);HGDIOBJ old=SelectObject(mem,bm);if(g_shade)DrawMainShade(mem);else DrawMainFull(mem);if(UIScale()==1)BitBlt(dc,0,0,MAIN_W,lh,mem,0,0,SRCCOPY);else StretchBlt(dc,0,0,rc.right,rc.bottom,mem,0,0,MAIN_W,lh,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);}

static int PlaylistKnownMs();
static const wchar_t* PlaylistHoverText(int id);
static int PlListTop(){return 58;}
static int PlListBottom(){return g_plH-54;}
static int PlButtonY(){return g_plH-45;}
static int PlaylistRows(){return MaxI(1,(PlListBottom()-(PlListTop()+4))/17);}
static int PlaylistMaxScroll(){return MaxI(0,PlaylistVisibleCount()-PlaylistRows());}
static void EnsurePlaylistSelectionVisible(){int rows=PlaylistRows(),vp=VisiblePosOfTrack(g_selected);if(vp<0){g_scroll=Clamp(g_scroll,0,PlaylistMaxScroll());return;}if(vp<g_scroll)g_scroll=vp;else if(vp>=g_scroll+rows)g_scroll=vp-rows+1;g_scroll=Clamp(g_scroll,0,PlaylistMaxScroll());}
static void SelectPlaylistIndex(int idx){if(g_count<=0){g_selected=-1;g_scroll=0;return;}g_selected=Clamp(idx,0,g_count-1);ClearMarks();g_tracks[g_selected].marked=true;g_markAnchor=g_selected;EnsurePlaylistSelectionVisible();}
static void PlaylistScrollGeom(int&top,int&bottom,int&thumbTop,int&thumbH){top=PlListTop()+4;bottom=PlListBottom()-4;int trackH=MaxI(1,bottom-top),rows=PlaylistRows(),vis=PlaylistVisibleCount();thumbH=trackH;if(vis>rows){thumbH=MaxI(26,trackH*rows/vis);if(thumbH>trackH)thumbH=trackH;int maxs=MaxI(1,vis-rows);thumbTop=top+(trackH-thumbH)*g_scroll/maxs;}else thumbTop=top;}
static void DrawResizeGrip(HDC dc){int x=g_plW-13,y=g_plH-13;Line(dc,x+1,y+10,x+10,y+1,C_EDGE);Line(dc,x+5,y+10,x+10,y+5,C_EDGE);Line(dc,x+8,y+10,x+10,y+8,C_EDGE);}
static void DrawPlaylist(HDC dc){
 int w=g_plW,h=g_plH,listBottom=PlListBottom(),by=PlButtonY(),vis=PlaylistVisibleCount();
 Fill(dc,0,0,w,h,C_BG);Box(dc,1,1,w-1,25,C_PANEL2,C_EDGE,12);Txt(dc,L"PLAYLIST",10,2,120,23,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 if(g_jump){wchar_t jt[140];WCopy(jt,L"JUMP: ",140);WCat(jt,g_jumpText,140);Txt(dc,jt,235,2,MaxI(260,w-145),23,C_LED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}
 wchar_t c[96],n[32];WCopy(c,L"TRACKS ",96);IntToW(g_count,n);WCat(c,n,96);if(g_queueCount){WCat(c,L"  //  QUEUE ",96);IntToW(g_queueCount,n);WCat(c,n,96);}Txt(dc,c,MaxI(185,w-275),2,w-65,23,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);PLButton(dc,H_PL_CLOSE,w-27,3,w-4,21,L"X",false,true);
 Box(dc,8,30,w-8,54,g_plFilterActive?C_BLACK:C_PANEL2,g_plFilterActive?C_LED2:C_EDGE,8);Txt(dc,g_plFilter[0]?L"FILTER":L"FILTER  Ctrl+F",16,31,95,53,g_plFilterActive?C_LED:C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,g_plFilter[0]?g_plFilter:L"Type artist, title, album or genre...",96,31,w-17,53,g_plFilter[0]?C_TEXT:C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 Box(dc,8,58,w-8,listBottom,C_PANEL,C_EDGE,10);int rows=PlaylistRows(),rh=17;EnsurePlaylistSelectionVisible();
 for(int r=0;r<rows;r++){int i=VisibleTrackAt(g_scroll+r);if(i<0)break;int y=62+r*rh;if(g_tracks[i].marked)Fill(dc,12,y,w-12,y+rh,C_ACCENT);else if(i==g_current)Fill(dc,12,y,w-12,y+rh,C_PANEL2);wchar_t ix[16],line[620],qn[16];IntToW(i+1,ix);int qp=QueuePositionForPath(g_tracks[i].path);if(i==g_current)WCopy(line,L"> ",620);else if(qp>0){WCopy(line,L"Q",620);IntToW(qp,qn);WCat(line,qn,620);WCat(line,L" ",620);}else if(g_tracks[i].bookmark)WCopy(line,L"* ",620);else WCopy(line,L"  ",620);WCat(line,ix,620);WCat(line,L". ",620);WCat(line,g_tracks[i].display,620);if(g_tracks[i].rating){WCat(line,L"  [",620);for(int st=0;st<g_tracks[i].rating;st++)WCat(line,L"*",620);WCat(line,L"]",620);}Txt(dc,line,16,y,w-34,y+rh,g_tracks[i].marked?C_BLACK:(i==g_current?C_LED:C_TEXT),g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}
 if(vis>rows){int st,sb,ty,th;PlaylistScrollGeom(st,sb,ty,th);Box(dc,w-25,st,w-10,sb,C_PANEL2,C_EDGE,7);Box(dc,w-23,ty+2,w-12,ty+th-2,C_ACCENT,C_ACCENT,6);}
 PLButton(dc,H_PL_ADD,8,by,68,by+32,L"OPEN");PLButton(dc,H_PL_DIR,73,by,133,by+32,L"DIR");PLButton(dc,H_PL_REMOVE,138,by,198,by+32,L"-SEL");PLButton(dc,H_PL_CLEAR,203,by,263,by+32,L"CLEAR");PLButton(dc,H_PL_LOAD,268,by,333,by+32,L"LOAD");PLButton(dc,H_PL_SAVE,338,by,403,by+32,L"SAVE");
 // Bottom-right status area only. Never draw help/hover UI over the track list.
 if(w>=500){
  const wchar_t*tip=g_plHover?PlaylistHoverText(g_plHover):L"";
  if(tip&&tip[0])Txt(dc,tip,408,by,w-18,by+32,C_LED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  else{wchar_t ps[180],tm[32];WCopy(ps,g_plFilter[0]?L"FILTERED ":L"Enter=play  Ctrl/Shift=select",180);if(g_plFilter[0]){IntToW(vis,n);WCat(ps,n,180);WCat(ps,L" / ",180);IntToW(g_count,n);WCat(ps,n,180);}else{int kt=PlaylistKnownMs();if(kt>0){WCat(ps,L"  //  ",180);FormatTime(kt,tm);WCat(ps,tm,180);WCat(ps,L" known",180);}}Txt(dc,ps,408,by,w-18,by+32,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);}
 }
 DrawResizeGrip(dc);
}
static void PaintPlaylist(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT rc;GetClientRect(h,&rc);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,g_plW,g_plH);HGDIOBJ old=SelectObject(mem,bm);DrawPlaylist(mem);if(UIScale()==1)BitBlt(dc,0,0,g_plW,g_plH,mem,0,0,SRCCOPY);else StretchBlt(dc,0,0,rc.right,rc.bottom,mem,0,0,g_plW,g_plH,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);}

static int MainHit(int x,int y){if(g_shade){if(PtIn(x,y,6,0,62,SHADE_H))return H_SETTINGS;if(PtIn(x,y,312,5,342,29))return H_PREV;if(PtIn(x,y,346,5,378,29))return H_PLAY;if(PtIn(x,y,382,5,414,29))return H_NEXT;if(PtIn(x,y,418,5,450,29))return H_PL;if(PtIn(x,y,456,5,496,29))return H_CLOSE;if(y<SHADE_H)return H_TITLE;return 0;}if(PtIn(x,y,8,2,68,23))return H_SETTINGS;if(PtIn(x,y,MAIN_W-48,3,MAIN_W-27,21))return H_MIN;if(PtIn(x,y,MAIN_W-24,3,MAIN_W-3,21))return H_CLOSE;if(y<24)return H_TITLE;if(PtIn(x,y,16,37,99,90))return H_VIS;if(PtIn(x,y,108,61,226,84))return H_TIME;if(PtIn(x,y,326,33,360,54))return H_MUTE;if(PtIn(x,y,363,35,480,54))return H_VOL;if(PtIn(x,y,363,59,480,79))return H_BAL;if(PtIn(x,y,10,99,490,116))return H_SEEK;int yy=120;if(PtIn(x,y,12,yy,52,yy+32))return H_PREV;if(PtIn(x,y,56,yy,96,yy+32))return H_PLAY;if(PtIn(x,y,100,yy,140,yy+32))return H_PAUSE;if(PtIn(x,y,144,yy,184,yy+32))return H_STOP;if(PtIn(x,y,188,yy,228,yy+32))return H_NEXT;if(PtIn(x,y,232,yy,272,yy+32))return H_EJECT;if(PtIn(x,y,282,yy,330,yy+32))return H_SHUFFLE;if(PtIn(x,y,334,yy,382,yy+32))return H_REPEAT;if(PtIn(x,y,374,yy,414,yy+32))return H_AB;if(PtIn(x,y,418,yy,451,yy+32))return H_EQ;if(PtIn(x,y,455,yy,488,yy+32))return H_PL;return 0;}
static int PlHit(int x,int y){int by=PlButtonY(),lb=PlListBottom();if(PtIn(x,y,g_plW-27,3,g_plW-4,21))return H_PL_CLOSE;if(PtIn(x,y,8,30,g_plW-8,54))return H_PL_SEARCH;if(PtIn(x,y,8,by,68,by+32))return H_PL_ADD;if(PtIn(x,y,73,by,133,by+32))return H_PL_DIR;if(PtIn(x,y,138,by,198,by+32))return H_PL_REMOVE;if(PtIn(x,y,203,by,263,by+32))return H_PL_CLEAR;if(PtIn(x,y,268,by,333,by+32))return H_PL_LOAD;if(PtIn(x,y,338,by,403,by+32))return H_PL_SAVE;if(PlaylistVisibleCount()>PlaylistRows()&&PtIn(x,y,g_plW-27,PlListTop()+4,g_plW-8,lb-4))return H_PL_SCROLL;if(PtIn(x,y,8,PlListTop(),g_plW-28,lb))return H_PL_LIST;if(y<24)return H_TITLE;return 0;}
static int PlHoverId(int x,int y){return PlHit(x,y);}
static const wchar_t* PlaylistHoverText(int id){switch(id){case H_PL_CLOSE:return L"CLOSE — hide Playlist Editor [P]";case H_PL_SEARCH:return L"FILTER — search artist, title, album or genre [Ctrl+F]";case H_PL_ADD:return L"OPEN — choose audio file(s) and add to playlist";case H_PL_DIR:return L"DIR — add a music folder recursively";case H_PL_REMOVE:return L"-SEL — remove selected/marked tracks [Delete]";case H_PL_CLEAR:return L"CLEAR — empty current playlist";case H_PL_LOAD:return L"LOAD — open M3U/M3U8 playlist";case H_PL_SAVE:return L"SAVE — write current playlist as M3U/M3U8";case H_PL_LIST:return L"TRACK LIST — Shift/Ctrl multi-select • drag marked block • right-click queue/sort/info";case H_PL_SCROLL:return L"SCROLL — drag the thumb, click the track, or use the mouse wheel";default:return L"";}}

static bool SnapRect(RECT p,RECT t,int snap,int&edge,int&nx,int&ny){bool hov=Overlap1D(p.left,p.right,t.left,t.right),vov=Overlap1D(p.top,p.bottom,t.top,t.bottom);if(AbsI(p.top-t.bottom)<=snap&&hov){edge=1;nx=t.left;ny=t.bottom;return true;}if(AbsI(p.bottom-t.top)<=snap&&hov){edge=2;nx=t.left;ny=t.top-(p.bottom-p.top);return true;}if(AbsI(p.left-t.right)<=snap&&vov){edge=3;nx=t.right;ny=t.top;return true;}if(AbsI(p.right-t.left)<=snap&&vov){edge=4;nx=t.left-(p.right-p.left);ny=t.top;return true;}return false;}
static int SnapDistance(RECT p,RECT t){return MinI(MinI(AbsI(p.top-t.bottom),AbsI(p.bottom-t.top)),MinI(AbsI(p.left-t.right),AbsI(p.right-t.left)));}

static void DockPlaylist(){
 if(!g_pl||!g_plVisible||g_plDockEdge==0)return;
 RECT t;
 if(g_plDockTarget==1&&g_eqVisible&&g_eq){GetWindowRect(g_eq,&t);}
 else{GetWindowRect(g_main,&t);if(g_plDockTarget==1&&!g_eqVisible)g_plDockTarget=0;}
 int sc=UIScale(),pw=g_plW*sc,ph=g_plH*sc,x=t.left,y=t.bottom;
 if(g_plDockEdge==1||g_plDockEdge==2){
  if(g_plDockAlign==2)x=t.right-pw;
  else if(g_plDockAlign==1)x=t.left;
  else x=t.left+g_plDockOffset*sc;
  y=g_plDockEdge==1?t.bottom:t.top-ph;
 }else if(g_plDockEdge==3){x=t.right;y=t.top;}
 else if(g_plDockEdge==4){x=t.left-pw;y=t.top;}
 g_dockMove=true;MoveWindow(g_pl,x,y,pw,ph,TRUE);g_dockMove=false;
}
static bool GetPlaylistTargetRect(int target,RECT& t){
 if(target==1){if(!g_eqVisible||!g_eq)return false;GetWindowRect(g_eq,&t);return true;}
 if(!g_main)return false;GetWindowRect(g_main,&t);return true;
}
static bool CrossedX(int previousX,int currentX,int targetX){
 int a=previousX-targetX,b=currentX-targetX;
 return (a<0&&b>0)||(a>0&&b<0)||a==0||b==0;
}
static int PlaylistResizeRightMagnet(int left,int rawRight){
 const int sc=UIScale(),acquire=22*sc,release=40*sc,outwardRelease=8*sc;
 RECT eqr={0,0,0,0},mr={0,0,0,0};
 bool haveEq=GetPlaylistTargetRect(1,eqr),haveMain=GetPlaylistTargetRect(0,mr);
 int target=-1,targetRight=rawRight,prev=g_plResizeLastRawRight;
 bool crossEq=haveEq&&prev&&((prev<eqr.right&&rawRight>=eqr.right)||(prev>eqr.right&&rawRight<=eqr.right));
 bool crossMain=haveMain&&prev&&((prev<mr.right&&rawRight>=mr.right)||(prev>mr.right&&rawRight<=mr.right));
 bool towardEq=haveEq&&(!prev||AbsI(rawRight-eqr.right)<=AbsI(prev-eqr.right));
 bool towardMain=haveMain&&(!prev||AbsI(rawRight-mr.right)<=AbsI(prev-mr.right));
 bool keepEq=haveEq&&g_plRightAnchorTarget==1&&AbsI(rawRight-eqr.right)<=release&&rawRight<=eqr.right+outwardRelease;
 bool keepMain=haveMain&&g_plRightAnchorTarget==0&&AbsI(rawRight-mr.right)<=release&&rawRight<=mr.right+outwardRelease;
 if(haveEq&&eqr.right-left>=PL_MIN_W*sc&&(keepEq||(AbsI(rawRight-eqr.right)<=acquire&&towardEq)||crossEq)){target=1;targetRight=eqr.right;}
 else if(haveMain&&mr.right-left>=PL_MIN_W*sc&&(keepMain||(AbsI(rawRight-mr.right)<=acquire&&towardMain)||crossMain)){target=0;targetRight=mr.right;}
 g_plRightAnchorTarget=target;
 g_plResizeLastRawRight=rawRight;
 return targetRight;
}
static void ApplyPlaylistDirectDragSnap(RECT& p){
 const int sc=UIScale();
 const int acquire=42*sc,release=84*sc,vertical=24*sc;
 RECT eqr={0,0,0,0},mr={0,0,0,0};
 bool haveEq=GetPlaylistTargetRect(1,eqr),haveMain=GetPlaylistTargetRect(0,mr);
 const int rawRight=p.right;
 int target=-1;

 // A classic player snap must not depend on receiving a mouse event on the exact
 // snap pixel. Detect crossing of the target edge as well as ordinary proximity.
 bool crossEq=haveEq&&g_plWindowDragLastRawRight&&CrossedX(g_plWindowDragLastRawRight,rawRight,eqr.right);
 bool crossMain=haveMain&&g_plWindowDragLastRawRight&&CrossedX(g_plWindowDragLastRawRight,rawRight,mr.right);

 // EQ wins intentionally. If its right edge is crossed or approached, capture it.
 if(haveEq&&((g_plRightAnchorTarget==1&&AbsI(rawRight-eqr.right)<=release)||AbsI(rawRight-eqr.right)<=acquire||crossEq))target=1;
 else if(haveMain&&((g_plRightAnchorTarget==0&&AbsI(rawRight-mr.right)<=release)||AbsI(rawRight-mr.right)<=acquire||crossMain))target=0;

 if(target>=0){
  RECT t=target==1?eqr:mr;
  int dx=t.right-p.right;p.left+=dx;p.right+=dx;
  g_plRightAnchorTarget=target;
 }else g_plRightAnchorTarget=-1;

 // Remember the UNSNAPPED edge, so a fast mouse move that jumps across the EQ
 // edge is caught on the next frame instead of silently skipping the magnet zone.
 if(g_plWindowDrag)g_plWindowDragLastRawRight=rawRight;

 // Vertical docking is separate from the right-edge magnet.
 int bestTarget=-1,bestEdge=0,bestGap=0x7fffffff,bestY=p.top;
 for(int cand=1;cand>=0;cand--){RECT t;if(!GetPlaylistTargetRect(cand,t))continue;if(!Overlap1D(p.left,p.right,t.left,t.right))continue;
  int dBottom=AbsI(p.top-t.bottom),dTop=AbsI(p.bottom-t.top);
  if(dBottom<=vertical&&(dBottom<bestGap||(dBottom==bestGap&&cand==1))){bestTarget=cand;bestEdge=1;bestGap=dBottom;bestY=t.bottom;}
  if(dTop<=vertical&&(dTop<bestGap||(dTop==bestGap&&cand==1))){bestTarget=cand;bestEdge=2;bestGap=dTop;bestY=t.top-(p.bottom-p.top);}
 }
 if(bestTarget>=0){int dy=bestY-p.top;p.top+=dy;p.bottom+=dy;g_plDockEdge=bestEdge;g_plDockTarget=bestTarget;
  if(g_plRightAnchorTarget==bestTarget)g_plDockAlign=2;else{RECT t;GetPlaylistTargetRect(bestTarget,t);int dl=AbsI(p.left-t.left);if(dl<=28*sc)g_plDockAlign=1;else{g_plDockAlign=0;g_plDockOffset=(p.left-t.left)/MaxI(1,sc);}}
 }else{g_plDockEdge=0;g_plDockTarget=0;g_plDockAlign=0;g_plDockOffset=0;}
}
static int RectVerticalGap(const RECT&a,const RECT&b){if(a.bottom<b.top)return b.top-a.bottom;if(b.bottom<a.top)return a.top-b.bottom;return 0;}
static int PickPlaylistRightAnchor(const RECT&p,int current,int&targetRight){
 int sc=UIScale(),best=-1,bestDist=0x7fffffff;
 for(int target=0;target<=1;target++){
  RECT t;if(!GetPlaylistTargetRect(target,t))continue;
  int d=AbsI(p.right-t.right);int lim=(current==target?72:38)*sc;
  if(d<=lim&&(d<bestDist||(d==bestDist&&target==1))){best=target;bestDist=d;targetRight=t.right;}
 }
 return best;
}
static bool PlaylistVerticalDockForTarget(RECT& p,int target,int snapY,int&edge,int&ny){RECT t;if(!GetPlaylistTargetRect(target,t))return false;if(!Overlap1D(p.left,p.right,t.left,t.right))return false;int g1=AbsI(p.top-t.bottom),g2=AbsI(p.bottom-t.top);if(g1<=snapY&&g1<=g2){edge=1;ny=t.bottom;return true;}if(g2<=snapY){edge=2;ny=t.top-(p.bottom-p.top);return true;}return false;}
static void ResolvePlaylistDockForRect(RECT& p,int preferredTarget){
 int sc=UIScale(),snapY=26*sc;g_plDockEdge=0;g_plDockTarget=preferredTarget>=0?preferredTarget:0;g_plDockAlign=0;g_plDockOffset=0;
 int bestTarget=-1,bestEdge=0,bestY=p.top;
 // If the right edge is magnetized to a window, use that same window for vertical docking whenever possible.
 if(preferredTarget>=0){int e=0,y=p.top;if(PlaylistVerticalDockForTarget(p,preferredTarget,snapY,e,y)){bestTarget=preferredTarget;bestEdge=e;bestY=y;}}
 if(bestTarget<0){int bestGap=0x7fffffff;for(int target=0;target<=1;target++){int e=0,y=p.top;if(!PlaylistVerticalDockForTarget(p,target,snapY,e,y))continue;RECT t;GetPlaylistTargetRect(target,t);int gap=e==1?AbsI(p.top-t.bottom):AbsI(p.bottom-t.top);if(gap<bestGap||(gap==bestGap&&target==1)){bestGap=gap;bestTarget=target;bestEdge=e;bestY=y;}}}
 if(bestTarget>=0){int dy=bestY-p.top;p.top+=dy;p.bottom+=dy;g_plDockEdge=bestEdge;g_plDockTarget=bestTarget;if(g_plRightAnchorTarget==bestTarget)g_plDockAlign=2;else{RECT t;GetPlaylistTargetRect(bestTarget,t);int dl=AbsI(p.left-t.left);if(dl<=30*sc)g_plDockAlign=1;else{g_plDockAlign=0;g_plDockOffset=(p.left-t.left)/MaxI(1,sc);}}}
}
static void ApplyPlaylistMoveMagnet(RECT& p){
 int right=0;int anchor=PickPlaylistRightAnchor(p,g_plRightAnchorTarget,right);
 if(anchor>=0){int dx=right-p.right;p.left+=dx;p.right+=dx;g_plRightAnchorTarget=anchor;}else g_plRightAnchorTarget=-1;
 ResolvePlaylistDockForRect(p,g_plRightAnchorTarget);
}
static void SnapPlaylistIfClose(){
 if(!g_pl||!g_main||g_dockMove)return;RECT before;GetWindowRect(g_pl,&before);RECT after=before;ApplyPlaylistMoveMagnet(after);
 if(after.left!=before.left||after.top!=before.top){g_dockMove=true;MoveWindow(g_pl,after.left,after.top,after.right-after.left,after.bottom-after.top,TRUE);g_dockMove=false;}
}
static void DockPlaylistRightAnchor(){
 if(!g_pl||!g_plVisible||g_plRightAnchorTarget<0||g_plDockEdge)return;RECT t;if(!GetPlaylistTargetRect(g_plRightAnchorTarget,t)){g_plRightAnchorTarget=-1;return;}RECT p;GetWindowRect(g_pl,&p);int w=p.right-p.left;int nx=t.right-w;if(nx==p.left)return;g_dockMove=true;MoveWindow(g_pl,nx,p.top,w,p.bottom-p.top,TRUE);g_dockMove=false;
}
static void DockTool(HWND h,int edge,int lw,int lh){if(!h||edge==0)return;RECT m;GetWindowRect(g_main,&m);int ww=lw*UIScale(),hh=lh*UIScale(),x=m.right,y=m.top;if(edge==1){x=m.left;y=m.bottom;}else if(edge==2){x=m.left;y=m.top-hh;}else if(edge==3){x=m.right;y=m.top;}else if(edge==4){x=m.left-ww;y=m.top;}g_dockMove=true;MoveWindow(h,x,y,ww,hh,TRUE);g_dockMove=false;}
static void SnapTool(HWND h,int&edge){if(!h||g_dockMove)return;RECT m,p;GetWindowRect(g_main,&m);GetWindowRect(h,&p);int snap=14*UIScale(),e=0,nx=p.left,ny=p.top;bool hov=Overlap1D(p.left,p.right,m.left,m.right),vov=Overlap1D(p.top,p.bottom,m.top,m.bottom);if(AbsI(p.top-m.bottom)<=snap&&hov){e=1;nx=m.left;ny=m.bottom;}else if(AbsI(p.bottom-m.top)<=snap&&hov){e=2;nx=m.left;ny=m.top-(p.bottom-p.top);}else if(AbsI(p.left-m.right)<=snap&&vov){e=3;nx=m.right;ny=m.top;}else if(AbsI(p.right-m.left)<=snap&&vov){e=4;nx=m.left-(p.right-p.left);ny=m.top;}edge=e;if(e){g_dockMove=true;MoveWindow(h,nx,ny,p.right-p.left,p.bottom-p.top,TRUE);g_dockMove=false;}}
static void DockEQ(){if(!g_eq||!g_eqVisible||g_eqDockEdge==0)return;RECT t;if(g_eqDockTarget==1&&g_plVisible&&g_pl)GetWindowRect(g_pl,&t);else{GetWindowRect(g_main,&t);if(g_eqDockTarget==1&&!g_plVisible)g_eqDockTarget=0;}int ww=EQ_W*UIScale(),hh=EQ_H*UIScale(),x=t.right,y=t.top;if(g_eqDockEdge==1){x=t.left;y=t.bottom;}else if(g_eqDockEdge==2){x=t.left;y=t.top-hh;}else if(g_eqDockEdge==3){x=t.right;y=t.top;}else if(g_eqDockEdge==4){x=t.left-ww;y=t.top;}g_dockMove=true;MoveWindow(g_eq,x,y,ww,hh,TRUE);g_dockMove=false;}
static void SnapEQ(){if(!g_eq||g_dockMove)return;RECT p,m;GetWindowRect(g_eq,&p);GetWindowRect(g_main,&m);int snap=14*UIScale(),edge=0,nx=p.left,ny=p.top,target=0;bool hit=SnapRect(p,m,snap,edge,nx,ny);int best=hit?SnapDistance(p,m):0x7fffffff;
 // If playlist is already a child in the docking graph (playlist -> EQ), EQ must not dock back to it.
 if(g_plVisible&&g_pl&&!(g_plDockTarget==1&&g_plDockEdge)){RECT q;GetWindowRect(g_pl,&q);int pe=0,px=p.left,py=p.top;if(SnapRect(p,q,snap,pe,px,py)){int d=SnapDistance(p,q);if(!hit||d<=best){hit=true;edge=pe;nx=px;ny=py;target=1;best=d;}}}
 g_eqDockEdge=hit?edge:0;g_eqDockTarget=hit?target:0;if(hit){g_dockMove=true;MoveWindow(g_eq,nx,ny,p.right-p.left,p.bottom-p.top,TRUE);g_dockMove=false;}}

static void DockAllTools(){
 // Resolve the two-way MAIN / PLAYLIST / EQ docking graph without creating cycles.
 if(g_eqVisible&&g_eqDockEdge&&g_eqDockTarget==0)DockEQ();
 if(g_plVisible&&g_plDockEdge&&g_plDockTarget==0)DockPlaylist();
 if(g_eqVisible&&g_eqDockEdge&&g_eqDockTarget==1)DockEQ();
 if(g_plVisible&&g_plDockEdge&&g_plDockTarget==1)DockPlaylist();
 if(g_plVisible&&!g_plDockEdge&&g_plRightAnchorTarget>=0)DockPlaylistRightAnchor();
 if(g_libVisible&&g_libDockEdge)DockTool(g_lib,g_libDockEdge,700,420);if(g_artVisible&&g_artDockEdge)DockTool(g_art,g_artDockEdge,260,300);if(g_vizVisible&&g_vizDockEdge&&!g_vizFull)DockTool(g_viz,g_vizDockEdge,800,450);
}
static void ToggleDoubleSize(){RECT mr,pr;GetWindowRect(g_main,&mr);GetWindowRect(g_pl,&pr);g_doubleSize=!g_doubleSize;int sc=UIScale();MoveWindow(g_main,mr.left,mr.top,MAIN_W*sc,(g_shade?SHADE_H:MAIN_H)*sc,TRUE);if(g_plVisible&&!g_plDockEdge)MoveWindow(g_pl,pr.left,pr.top,g_plW*sc,g_plH*sc,TRUE);if(g_eq&&g_eqVisible&&!g_eqDockEdge){RECT er;GetWindowRect(g_eq,&er);MoveWindow(g_eq,er.left,er.top,EQ_W*sc,EQ_H*sc,TRUE);}DockAllTools();if(g_lib&&g_libVisible){RECT r;GetWindowRect(g_lib,&r);if(g_libDockEdge)DockTool(g_lib,g_libDockEdge,700,420);else MoveWindow(g_lib,r.left,r.top,700*sc,420*sc,TRUE);}if(g_art&&g_artVisible){RECT r;GetWindowRect(g_art,&r);if(g_artDockEdge)DockTool(g_art,g_artDockEdge,260,300);else MoveWindow(g_art,r.left,r.top,260*sc,300*sc,TRUE);}if(g_viz&&g_vizVisible&&!g_vizFull){RECT r;GetWindowRect(g_viz,&r);if(g_vizDockEdge)DockTool(g_viz,g_vizDockEdge,800,450);else MoveWindow(g_viz,r.left,r.top,800*sc,450*sc,TRUE);}RoundCoreWindows();InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);if(g_eq)InvalidateRect(g_eq,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);if(g_art)InvalidateRect(g_art,0,FALSE);if(g_viz)InvalidateRect(g_viz,0,FALSE);}

static void TogglePlaylist(){g_plVisible=!g_plVisible;ShowWindow(g_pl,g_plVisible?SW_SHOW:SW_HIDE);if(g_plVisible){DockPlaylist();if(g_eqVisible&&g_eqDockTarget==1&&g_eqDockEdge)DockEQ();}else if(g_eqVisible&&g_eqDockTarget==1){g_eqDockTarget=0;DockEQ();}InvalidateRect(g_main,0,FALSE);}
static void ToggleShade(){g_shade=!g_shade;RECT r;GetWindowRect(g_main,&r);MoveWindow(g_main,r.left,r.top,MAIN_W*UIScale(),(g_shade?SHADE_H:MAIN_H)*UIScale(),TRUE);DockPlaylist();RoundWindow(g_main,g_shade?10:14);InvalidateRect(g_main,0,FALSE);}
static void ToggleTop(){g_top=!g_top;SetWindowPos(g_main,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);SetWindowPos(g_pl,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_eq)SetWindowPos(g_eq,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_lib)SetWindowPos(g_lib,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_art)SetWindowPos(g_art,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_viz)SetWindowPos(g_viz,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_settings)SetWindowPos(g_settings,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_about)SetWindowPos(g_about,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_info)SetWindowPos(g_info,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_tag)SetWindowPos(g_tag,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);if(g_error)SetWindowPos(g_error,g_top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);}
static void ToggleEQWindow(){g_eqVisible=!g_eqVisible;ShowWindow(g_eq,g_eqVisible?SW_SHOW:SW_HIDE);if(g_eqVisible){if(g_eqDockEdge)DockEQ();if(g_plVisible&&g_plDockTarget==1&&g_plDockEdge)DockPlaylist();}else if(g_plVisible&&g_plDockTarget==1){g_plDockTarget=0;DockPlaylist();}InvalidateRect(g_main,0,FALSE);}
static void InitFonts();
static void OpenSkinDialog();
static void ToggleLibrary(){g_libVisible=!g_libVisible;ShowWindow(g_lib,g_libVisible?SW_SHOW:SW_HIDE);if(g_libVisible&&g_libDockEdge)DockTool(g_lib,g_libDockEdge,700,420);}
static void ToggleArt(){g_artVisible=!g_artVisible;if(g_artVisible)LoadCoverForCurrent();ShowWindow(g_art,g_artVisible?SW_SHOW:SW_HIDE);if(g_artVisible&&g_artDockEdge)DockTool(g_art,g_artDockEdge,260,300);}
static void ToggleVisualizerWindow(){if(g_vizVisible){if(g_vizFull)ToggleVizFull();g_vizVisible=false;ShowWindow(g_viz,SW_HIDE);return;}g_vizVisible=true;ShowWindow(g_viz,SW_SHOW);if(g_vizDockEdge)DockTool(g_viz,g_vizDockEdge,800,450);RoundWindow(g_viz,14);SetForegroundWindow(g_viz);}
static void CycleSleep(){int vals[5]={0,15,30,60,90};int at=0;for(int i=0;i<5;i++)if(vals[i]==g_sleepMin)at=i;at=(at+1)%5;g_sleepMin=vals[at];g_sleepUntil=g_sleepMin?GetTickCount64()+(ULONGLONG)g_sleepMin*60000ULL:0;InvalidateRect(g_main,0,FALSE);}
static void CycleAB(){if(g_current<0)return;if(g_abState==0){g_a=g_pos;g_abState=1;SetStatus(L"A SET");}else if(g_abState==1){if(g_pos>g_a+500){g_b=g_pos;g_abState=2;SetStatus(L"A-B LOOP");}else{g_abState=0;}}else{g_abState=0;SetStatus(g_playing?L"PLAYING":L"READY");}InvalidateRect(g_main,0,FALSE);}

// ---------------- Settings ----------------
static void SettingsStatusLine(wchar_t*out,int cap){
 wchar_t n[32];WCopy(out,L"WASAPI SHARED // ",cap);IntToW(OzAudioOutputRate(),n);WCat(out,n,cap);WCat(out,L" Hz // ",cap);IntToW(OzAudioOutputChannels(),n);WCat(out,n,cap);WCat(out,L" CH",cap);
}
static void DrawSettingToggle(HDC dc,int y,const wchar_t*label,const wchar_t*desc,bool on,int hoverId,int id){
 Box(dc,18,y,602,y+44,(g_settingsHover==id)?C_PANEL2:C_PANEL,C_EDGE,5);Button(dc,510,y+9,586,y+34,on?L"ON":L"OFF",on,false);Txt(dc,label,30,y+5,480,y+23,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(dc,desc,30,y+22,490,y+40,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
}
static int SettingsDeviceRows(){return 7;}
static int SettingsHit(int x,int y){
 if(y<28&&x>=SETTINGS_W-34)return 1;
 if(y>=38&&y<67){if(x>=18&&x<174)return 10;if(x>=182&&x<338)return 11;if(x>=346&&x<502)return 12;}
 if(g_settingsTab==0){
  if(y>=169&&y<169+SettingsDeviceRows()*27){int row=(y-169)/27;return 1000+g_settingsDeviceScroll+row;}
  if(PtIn(x,y,18,374,112,406))return 20;
  if(PtIn(x,y,122,374,252,406))return 21;
  if(PtIn(x,y,262,374,410,406))return 22;
 }else if(g_settingsTab==1){
  if(PtIn(x,y,18,86,602,130))return 30;
  if(PtIn(x,y,18,138,602,182))return 31;
  if(PtIn(x,y,18,190,602,234))return 32;
  if(PtIn(x,y,394,254,438,286))return 33;
  if(PtIn(x,y,538,254,582,286))return 34;
 }else{
  int ys[8]={82,126,170,214,258,302,346,390};for(int i=0;i<8;i++)if(y>=ys[i]&&y<ys[i]+38)return 50+i;
 }
 return 0;
}
static void PaintSettings(HWND h){
 PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);HDC m=CreateCompatibleDC(dc);HBITMAP b=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ ob=SelectObject(m,b);
 Fill(m,0,0,r.right,r.bottom,C_BG);Box(m,1,1,r.right-1,29,C_PANEL2,C_EDGE,12);Txt(m,L"OZAMP // SETTINGS",12,0,280,28,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(m,SETTINGS_W-31,4,SETTINGS_W-7,24,L"X",false,true);
 Button(m,18,38,174,67,L"AUDIO",g_settingsTab==0);Button(m,182,38,338,67,L"PLAYBACK",g_settingsTab==1);Button(m,346,38,502,67,L"INTERFACE",g_settingsTab==2);
 if(g_settingsTab==0){
  Box(m,18,78,602,157,C_PANEL,C_EDGE,5);Txt(m,L"ACTIVE OUTPUT",30,84,180,102,C_MUTED,g_small,DT_LEFT|DT_SINGLELINE);Txt(m,OzAudioReady()?OzAudioCurrentDeviceName():L"No WASAPI output initialized",30,102,588,123,OzAudioReady()?C_LED:C_RED,g_bold,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);
  wchar_t st[160];SettingsStatusLine(st,160);Txt(m,st,30,126,290,146,C_TEXT,g_small,DT_LEFT|DT_SINGLELINE);Txt(m,g_nativeAudio?OzAudioBackend():(g_dsActive?L"DIRECTSHOW COMPAT // NO PCM DSP":L"NO ACTIVE TRACK"),292,126,588,146,g_nativeAudio?C_LED2:C_MUTED,g_small,DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS);
  Txt(m,L"OUTPUT DEVICE",18,159,300,175,C_MUTED,g_small,DT_LEFT|DT_SINGLELINE);int total=1+g_deviceCount,vis=SettingsDeviceRows();int maxs=MaxI(0,total-vis);g_settingsDeviceScroll=Clamp(g_settingsDeviceScroll,0,maxs);
  const wchar_t*active=OzAudioCurrentDeviceId();for(int row=0;row<vis;row++){int item=g_settingsDeviceScroll+row;if(item>=total)break;int y=169+row*27;bool pending=(item==g_settingsPendingDevice);bool saved=(item==0?!g_deviceId[0]:(g_deviceId[0]&&WEqI(g_deviceId,g_devices[item-1].id)));bool actual=(item==0?!g_deviceId[0]&&OzAudioReady():(item>0&&active&&active[0]&&WEqI(active,g_devices[item-1].id)));DWORD fillc=pending?C_PANEL2:C_PANEL;Box(m,18,y,602,y+24,fillc,(g_settingsHover==1000+item)?C_ACCENT:C_EDGE,4);if(pending)Fill(m,21,y+3,25,y+21,C_ACCENT);if(actual)Fill(m,30,y+8,36,y+14,C_LED2);const wchar_t*name=item==0?L"Follow Windows default output":g_devices[item-1].name;Txt(m,name,43,y,478,y+24,pending?C_TEXT:C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);if(actual)Txt(m,L"ACTIVE",500,y,588,y+24,C_LED2,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);else if(pending)Txt(m,L"PENDING",500,y,588,y+24,C_LED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);else if(saved)Txt(m,L"SAVED",500,y,588,y+24,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);}
  Button(m,18,374,112,406,L"REFRESH",false);Button(m,122,374,252,406,L"APPLY",false);Button(m,262,374,410,406,L"USE DEFAULT",g_settingsPendingDevice==0);if(g_outputSwitchInfo[0]){wchar_t msg[240];WCopy(msg,g_outputSwitchInfo,240);if(FAILED(g_outputSwitchHr)){wchar_t hx[16];Hex32((DWORD)g_outputSwitchHr,hx);WCat(msg,L" // ",240);WCat(msg,hx,240);}Txt(m,msg,420,373,598,408,FAILED(g_outputSwitchHr)?C_RED:C_MUTED,g_small,DT_LEFT|DT_WORDBREAK);}else Txt(m,L"Click a device, then APPLY. Green = endpoint actually opened by WASAPI.",420,373,598,408,C_MUTED,g_small,DT_LEFT|DT_WORDBREAK);
 }else if(g_settingsTab==1){
  DrawSettingToggle(m,86,L"Gapless playback",L"Prepares the next track for seamless album transitions.",g_gapless,g_settingsHover,30);
  DrawSettingToggle(m,138,L"ReplayGain adjustment",L"Applies stored loudness gain before EQ/output.",g_replayGainEnabled,g_settingsHover,31);
  DrawSettingToggle(m,190,L"Sleep after current track",L"Stops playback when the current song ends.",g_sleepAfterCurrent,g_settingsHover,32);
  Box(m,18,246,602,296,C_PANEL,C_EDGE,5);Txt(m,L"CROSSFADE",30,251,180,270,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Txt(m,L"Native PCM tracks only",30,270,260,289,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);Button(m,394,254,438,286,L"-",false);wchar_t cf[40],nn[16];IntToW(g_crossfadeSec,nn);WCopy(cf,g_crossfadeSec?nn:L"OFF",40);if(g_crossfadeSec)WCat(cf,L" SEC",40);Txt(m,cf,442,254,534,286,C_LED,g_bold,DT_CENTER|DT_VCENTER|DT_SINGLELINE);Button(m,538,254,582,286,L"+",false);
  Box(m,18,310,602,382,C_PANEL,C_EDGE,5);Txt(m,L"NATIVE PCM PIPELINE",30,318,230,338,C_TEXT,g_bold,DT_LEFT|DT_SINGLELINE);Txt(m,L"Media Foundation stream -> PCM -> ReplayGain -> EQ -> volume/balance -> FFT -> WASAPI",30,341,586,363,C_MUTED,g_small,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);Txt(m,g_nativeAudio?L"ACTIVE FOR CURRENT TRACK":(g_current>=0?L"CURRENT TRACK IS USING COMPATIBILITY PLAYBACK":L"READY"),30,362,586,380,g_nativeAudio?C_LED:C_MUTED,g_small,DT_LEFT|DT_SINGLELINE);
 }else{
  DrawSettingToggle(m,82,L"Playlist Editor",L"Resizable magnetic playlist window.",g_plVisible,g_settingsHover,50);
  DrawSettingToggle(m,126,L"Equalizer",L"10-band EQ window; opens automatically by default.",g_eqVisible,g_settingsHover,51);
  DrawSettingToggle(m,170,L"Media Library",L"Artist / album / genre / year / folder views.",g_libVisible,g_settingsHover,52);
  DrawSettingToggle(m,214,L"Album Art",L"Dockable cover-art window with working close button.",g_artVisible,g_settingsHover,53);
  DrawSettingToggle(m,258,L"Audio Visualizer",L"Audio-reactive visualizer with fullscreen mode.",g_vizVisible,g_settingsHover,54);
  DrawSettingToggle(m,302,L"Always on top",L"Keeps OzAmp tool windows above normal windows.",g_top,g_settingsHover,55);
  DrawSettingToggle(m,346,L"Global hotkeys",L"Play, previous, next and volume controls outside OzAmp.",g_globalHotkeys,g_settingsHover,56);
  DrawSettingToggle(m,390,L"Track notifications",L"Shows a local notification when the track changes.",g_trackNotify,g_settingsHover,57);
 }
 BitBlt(dc,0,0,r.right,r.bottom,m,0,0,SRCCOPY);SelectObject(m,ob);DeleteObject(b);DeleteDC(m);EndPaint(h,&ps);
}
static void ToggleSettings(){if(!g_settings)return;if(IsWindowVisible(g_settings)){SaveSettings();ShowWindow(g_settings,SW_HIDE);return;}RefreshDevices();g_settingsPendingDevice=DeviceItemForId(g_deviceId);if(g_settingsPendingDevice<0)g_settingsPendingDevice=0;RECT mr;GetWindowRect(g_main,&mr);int x=mr.left+(MAIN_W*UIScale()-SETTINGS_W)/2,y=mr.bottom+12;SetWindowPos(g_settings,g_top?HWND_TOPMOST:(HWND)0,x,y,SETTINGS_W,SETTINGS_H,SWP_NOSIZE);ShowWindow(g_settings,SW_SHOW);SetForegroundWindow(g_settings);InvalidateRect(g_settings,0,FALSE);}
static LRESULT CALLBACK SettingsProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_ERASEBKGND:return 1;case WM_PAINT:PaintSettings(h);return 0;
 case WM_NCHITTEST:{LRESULT z=DefWindowProcW(h,m,w,l);if(z==HTCLIENT){POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(p.y<28&&p.x<SETTINGS_W-34)return HTCAPTION;}return z;}
 case WM_MOUSEMOVE:{int hit=SettingsHit(LOWORDi(l),HIWORDi(l));if(hit!=g_settingsHover){g_settingsHover=hit;InvalidateRect(h,0,FALSE);}return 0;}
 case WM_MOUSEWHEEL:if(g_settingsTab==0){int total=1+g_deviceCount,maxs=MaxI(0,total-SettingsDeviceRows());int d=GET_WHEEL_DELTA_WPARAM(w);g_settingsDeviceScroll=Clamp(g_settingsDeviceScroll+(d>0?-2:2),0,maxs);InvalidateRect(h,0,FALSE);}return 0;
 case WM_LBUTTONUP:{int hit=SettingsHit(LOWORDi(l),HIWORDi(l));if(hit==1){SaveSettings();ShowWindow(h,SW_HIDE);return 0;}if(hit>=10&&hit<=12){g_settingsTab=hit-10;InvalidateRect(h,0,FALSE);return 0;}if(g_settingsTab==0){if(hit==20){RefreshDevices();SetStatus(L"OUTPUT LIST REFRESHED");}else if(hit==21){ApplyPendingDevice();}else if(hit==22){g_settingsPendingDevice=0;ApplyPendingDevice();}else if(hit>=1000){int item=hit-1000;if(item>=0&&item<=g_deviceCount){g_settingsPendingDevice=item;SetOutputSwitchInfo(L"DEVICE SELECTED - PRESS APPLY",item==0?L"Windows default":g_devices[item-1].name,S_OK);}}InvalidateRect(h,0,FALSE);return 0;}if(g_settingsTab==1){if(hit==30){g_gapless=!g_gapless;PrepareNextTrack();}else if(hit==31){g_replayGainEnabled=!g_replayGainEnabled;ApplyAudio();PrepareNextTrack();}else if(hit==32){g_sleepAfterCurrent=!g_sleepAfterCurrent;PrepareNextTrack();}else if(hit==33){g_crossfadeSec=Clamp(g_crossfadeSec-1,0,12);PrepareNextTrack();}else if(hit==34){g_crossfadeSec=Clamp(g_crossfadeSec+1,0,12);PrepareNextTrack();}InvalidateRect(h,0,FALSE);return 0;}if(g_settingsTab==2){if(hit==50)TogglePlaylist();else if(hit==51)ToggleEQWindow();else if(hit==52)ToggleLibrary();else if(hit==53)ToggleArt();else if(hit==54)ToggleVisualizerWindow();else if(hit==55)ToggleTop();else if(hit==56)SetGlobalHotkeys(!g_globalHotkeys);else if(hit==57)g_trackNotify=!g_trackNotify;InvalidateRect(h,0,FALSE);return 0;}return 0;}
 case WM_KEYDOWN:if(w==VK_RETURN&&g_settingsTab==0){ApplyPendingDevice();return 0;}if(w==VK_ESCAPE||w==VK_F10){SaveSettings();ShowWindow(h,SW_HIDE);return 0;}return 0;case WM_CLOSE:SaveSettings();ShowWindow(h,SW_HIDE);return 0;}return DefWindowProcW(h,m,w,l);}

static void MainContext(int sx,int sy){HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,1,L"Open file(s) and play...");AppendMenuW(m,MF_STRING,2,L"Add folder and play...");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING|(g_plVisible?MF_CHECKED:0),3,L"Playlist Editor        P");if(g_queueCount){wchar_t qm[64],qn[16];WCopy(qm,L"Now Playing Queue      ",64);IntToW(g_queueCount,qn);WCat(qm,L"(",64);WCat(qm,qn,64);WCat(qm,L")",64);AppendMenuW(m,MF_STRING,38,qm);}AppendMenuW(m,MF_STRING|(g_eqVisible?MF_CHECKED:0),13,L"Equalizer              E");AppendMenuW(m,MF_STRING|(g_libVisible?MF_CHECKED:0),14,L"Media Library          Ctrl+L");AppendMenuW(m,MF_STRING|(g_artVisible?MF_CHECKED:0),15,L"Album Art");AppendMenuW(m,MF_STRING|(g_vizVisible?MF_CHECKED:0),16,L"Visualizer             F12");AppendMenuW(m,MF_SEPARATOR,0,0);HMENU audio=CreatePopupMenu();AppendMenuW(audio,MF_STRING|(g_gapless?MF_CHECKED:0),30,L"Gapless playback");AppendMenuW(audio,MF_STRING|(g_replayGainEnabled?MF_CHECKED:0),31,L"ReplayGain gain adjustment");AppendMenuW(audio,MF_STRING|(g_sleepAfterCurrent?MF_CHECKED:0),32,L"Sleep after current track");HMENU cf=CreatePopupMenu();int cvs[7]={0,2,4,6,8,10,12};for(int i=0;i<7;i++){wchar_t z[32],n[12];IntToW(cvs[i],n);WCopy(z,cvs[i]?n:L"OFF",32);if(cvs[i])WCat(z,L" seconds",32);AppendMenuW(cf,MF_STRING|(g_crossfadeSec==cvs[i]?MF_CHECKED:0),400+i,z);}AppendMenuW(audio,MF_POPUP,(UINT_PTR)cf,L"Crossfade");AppendMenuW(m,MF_POPUP,(UINT_PTR)audio,L"Audio / DSP");RefreshDevices();HMENU dev=CreatePopupMenu();AppendMenuW(dev,MF_STRING|(g_deviceId[0]?0:MF_CHECKED),599,L"Default Windows device");for(int i=0;i<g_deviceCount;i++)AppendMenuW(dev,MF_STRING|(g_deviceId[0]&&WEqI(g_deviceId,g_devices[i].id)?MF_CHECKED:0),600+i,g_devices[i].name);AppendMenuW(m,MF_POPUP,(UINT_PTR)dev,L"WASAPI output device");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING|(g_shade?MF_CHECKED:0),4,L"Shade mode");AppendMenuW(m,MF_STRING|(g_doubleSize?MF_CHECKED:0),12,L"Double size            Ctrl+D");AppendMenuW(m,MF_STRING|(g_top?MF_CHECKED:0),5,L"Always on top");AppendMenuW(m,MF_STRING|(g_globalHotkeys?MF_CHECKED:0),33,L"Global hotkeys");AppendMenuW(m,MF_STRING|(g_trackNotify?MF_CHECKED:0),34,L"Track notifications");AppendMenuW(m,MF_STRING,6,L"Sleep timer (cycle)");AppendMenuW(m,MF_STRING,9,L"Cycle built-in skin");AppendMenuW(m,MF_STRING,35,L"Load .ozskin...");AppendMenuW(m,MF_STRING,36,L"Hide to tray");AppendMenuW(m,MF_STRING,37,L"Settings...             F10");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,7,L"About OzAmp 1.0.0");AppendMenuW(m,MF_STRING,8,L"Exit");UINT c=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,sx,sy,0,g_main,0);DestroyMenu(m);if(c==1)OpenFilesDialog();else if(c==2)OpenFolderDialog();else if(c==3)TogglePlaylist();else if(c==38){if(!g_plVisible)TogglePlaylist();int qi=QueueFirstIndex();if(qi>=0)SelectPlaylistIndex(qi);SetForegroundWindow(g_pl);InvalidateRect(g_pl,0,FALSE);}else if(c==13)ToggleEQWindow();else if(c==14)ToggleLibrary();else if(c==15)ToggleArt();else if(c==16)ToggleVisualizerWindow();else if(c==4)ToggleShade();else if(c==5)ToggleTop();else if(c==12)ToggleDoubleSize();else if(c==6)CycleSleep();else if(c==9){g_externalSkin=false;g_skinFile[0]=0;g_skin=(g_skin+1)%3;WCopy(g_fontName,L"Segoe UI",64);WCopy(g_ledFontName,L"Consolas",64);InitColors();InitFonts();InvalidateRect(g_main,0,FALSE);InvalidateRect(g_pl,0,FALSE);if(g_eq)InvalidateRect(g_eq,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);if(g_art)InvalidateRect(g_art,0,FALSE);if(g_viz)InvalidateRect(g_viz,0,FALSE);if(g_settings)InvalidateRect(g_settings,0,FALSE);if(g_about)InvalidateRect(g_about,0,FALSE);if(g_info)InvalidateRect(g_info,0,FALSE);if(g_tag)InvalidateRect(g_tag,0,FALSE);}else if(c==30){g_gapless=!g_gapless;PrepareNextTrack();}else if(c==31){g_replayGainEnabled=!g_replayGainEnabled;ApplyAudio();PrepareNextTrack();}else if(c==32){g_sleepAfterCurrent=!g_sleepAfterCurrent;PrepareNextTrack();}else if(c==33)SetGlobalHotkeys(!g_globalHotkeys);else if(c==34)g_trackNotify=!g_trackNotify;else if(c==35)OpenSkinDialog();else if(c==36)ShowWindow(g_main,SW_HIDE);else if(c==37)ToggleSettings();else if(c>=400&&c<=406){int vals[7]={0,2,4,6,8,10,12};g_crossfadeSec=vals[c-400];PrepareNextTrack();}else if(c==599)SwitchDefaultDevice();else if(c>=600&&c<600+(UINT)g_deviceCount)SwitchDevice(c-600);else if(c==7)ToggleAbout();else if(c==8)DestroyWindow(g_main);}
static int PlaylistKnownMs(){long long t=0;for(int i=0;i<g_count;i++)if(g_tracks[i].lengthMs>0)t+=g_tracks[i].lengthMs;return t>0x7fffffff?0x7fffffff:(int)t;}
static void PlaylistContext(int sx,int sy){HMENU m=CreatePopupMenu();bool have=g_selected>=0&&g_selected<g_count;if(have){bool inq=QueueHasPath(g_tracks[g_selected].path);AppendMenuW(m,MF_STRING,1,L"Play now");AppendMenuW(m,MF_STRING,5,L"Play next");AppendMenuW(m,MF_STRING|(inq?MF_CHECKED:0),17,inq?L"Remove from queue":L"Add to queue");if(g_queueCount)AppendMenuW(m,MF_STRING,18,L"Clear queue");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,6,L"Track info");AppendMenuW(m,MF_STRING,7,L"Edit metadata...");AppendMenuW(m,MF_STRING|(g_tracks[g_selected].bookmark?MF_CHECKED:0),8,L"Bookmark / Favorite");HMENU rt=CreatePopupMenu();AppendMenuW(rt,MF_STRING|(g_tracks[g_selected].rating==0?MF_CHECKED:0),200,L"No rating");for(int i=1;i<=5;i++){wchar_t z[40];WCopy(z,L"",40);for(int q=0;q<i;q++)WCat(z,L"*",40);AppendMenuW(rt,MF_STRING|(g_tracks[g_selected].rating==i?MF_CHECKED:0),200+i,z);}AppendMenuW(m,MF_POPUP,(UINT_PTR)rt,L"Rating");AppendMenuW(m,MF_STRING,9,L"ReplayGain-style scan this track");AppendMenuW(m,MF_STRING,14,L"Add to Media Library");AppendMenuW(m,MF_STRING,2,L"Open containing folder");AppendMenuW(m,MF_STRING,3,L"Copy full path");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,4,L"Remove from playlist");}if(g_undoCount)AppendMenuW(m,MF_STRING,15,L"Undo remove");AppendMenuW(m,MF_SEPARATOR,0,0);HMENU sort=CreatePopupMenu();AppendMenuW(sort,MF_STRING,301,L"Artist");AppendMenuW(sort,MF_STRING,302,L"Title");AppendMenuW(sort,MF_STRING,303,L"Album");AppendMenuW(sort,MF_STRING,304,L"Duration");AppendMenuW(sort,MF_STRING,300,L"Display name");AppendMenuW(m,MF_POPUP,(UINT_PTR)sort,L"Sort by");AppendMenuW(m,MF_STRING,11,L"Reverse playlist");AppendMenuW(m,MF_STRING,12,L"Randomize playlist");AppendMenuW(m,MF_STRING,13,L"Remove missing files");AppendMenuW(m,MF_SEPARATOR,0,0);AppendMenuW(m,MF_STRING,16,L"ReplayGain-style scan entire playlist");UINT c=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,sx,sy,0,g_pl,0);DestroyMenu(m);if(c==1)OpenTrack(g_selected,true);else if(c==2&&have)OpenContaining(g_tracks[g_selected].path);else if(c==3&&have)CopyText(g_tracks[g_selected].path);else if(c==4)RemoveSelected();else if(c==5&&have){QueueAddPath(g_tracks[g_selected].path,true);Feedback(L"PLAY NEXT");InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}else if(c==17&&have){if(QueueHasPath(g_tracks[g_selected].path))QueueRemovePath(g_tracks[g_selected].path);else QueueAddPath(g_tracks[g_selected].path,false);InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}else if(c==18){QueueClear();InvalidateRect(g_pl,0,FALSE);InvalidateRect(g_main,0,FALSE);}else if(c==6)TrackInfo(g_selected);else if(c==7)OpenTagEditor(g_selected);else if(c==8&&have){g_tracks[g_selected].bookmark=!g_tracks[g_selected].bookmark;SaveTrackStats(g_tracks[g_selected]);for(int i=0;i<g_libCount;i++)if(WEqI(g_library[i].path,g_tracks[g_selected].path)){g_library[i].bookmark=g_tracks[g_selected].bookmark;SaveTrackStats(g_library[i]);}InvalidateRect(g_pl,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);}else if(c==9)StartRGScan(false);else if(c==14&&have){AddLibraryPath(g_tracks[g_selected].path);SaveLibrary();if(g_lib)InvalidateRect(g_lib,0,FALSE);SetStatus(L"ADDED TO LIBRARY");}else if(c==15)UndoRemove();else if(c==16)StartRGScan(true);else if(c>=200&&c<=205&&have){g_tracks[g_selected].rating=(int)c-200;SaveTrackStats(g_tracks[g_selected]);for(int i=0;i<g_libCount;i++)if(WEqI(g_library[i].path,g_tracks[g_selected].path)){g_library[i].rating=g_tracks[g_selected].rating;SaveTrackStats(g_library[i]);}InvalidateRect(g_pl,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);}else if(c>=300&&c<=304)SortPlaylistBy(c==300?0:c-300);else if(c==11)ReversePlaylist();else if(c==12)RandomizePlaylist();else if(c==13)RemoveMissing();}


static void Tick(){
 if(g_current>=0){if(g_nativeAudio){OzAudioPump();if(OzAudioConsumeAdvanced()&&g_preparedNext>=0){if(g_queueCount>0&&g_preparedNext==QueueFirstIndex())QueuePopFirst();g_current=g_preparedNext;g_selected=g_current;g_preparedNext=-1;g_tracks[g_current].playCount++;SaveTrackStats(g_tracks[g_current]);g_length=OzAudioLengthMs();g_pos=OzAudioPosMs();wchar_t wt[MAXD+32];WCopy(wt,L"OzAmp - ",MAXD+32);WCat(wt,g_tracks[g_current].display,MAXD+32);SetWindowTextW(g_main,wt);LoadCoverForCurrent();SetStatus(L"CROSSFADE / GAPLESS");NotifyTrack();PrepareNextTrack();InvalidateRect(g_pl,0,FALSE);}g_pos=OzAudioPosMs();g_playing=OzAudioPlaying();g_paused=OzAudioPaused();}else if(g_dsActive){g_pos=DSPosMs();if(g_length>0&&g_pos>=g_length-50&&g_playing)g_playing=false;}else{wchar_t r[64];if(!MCI(L"status ozamp_track position",r,64))g_pos=WToInt(r);wchar_t md[32];if(!MCI(L"status ozamp_track mode",md,32)&&g_playing&&WEqI(md,L"stopped"))g_playing=false;}if(g_abState==2&&g_pos>=g_b)SeekTo(g_a);if(g_repeat==2&&g_length>0&&g_pos>=g_length-30){SeekTo(0);Play();}else if(!g_playing&&!g_paused&&g_length>0&&g_pos>=g_length-30)Next(false);}
 if(g_sleepUntil&&GetTickCount64()>=g_sleepUntil){g_sleepUntil=0;g_sleepMin=0;Stop();SetStatus(L"SLEEP STOP");}
 if(g_scanRunning){wchar_t z[32],a[12],b[12];WCopy(z,L"LOUDNESS ",32);IntToW((int)g_scanProgress,a);IntToW(g_scanTotal,b);WCat(z,a,32);WCat(z,L"/",32);WCat(z,b,32);WCopy(g_mode,z,32);}else if(AtomicExchange(&g_scanDone,0)){for(int i=0;i<g_libCount;i++){for(int j=0;j<g_count;j++)if(WEqI(g_library[i].path,g_tracks[j].path)){g_library[i].replayGainDb=g_tracks[j].replayGainDb;g_library[i].peak=g_tracks[j].peak;break;}}SaveLibrary();ApplyAudio();PrepareNextTrack();SetStatus(L"LOUDNESS SCAN DONE");InvalidateRect(g_pl,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);}
 if(g_toastUntil&&GetTickCount64()>=g_toastUntil){g_toastUntil=0;if(g_toast)ShowWindow(g_toast,SW_HIDE);}
 MonitorAudioDevice();if((++g_tickCounter&3)==0)TaskbarUpdate();g_marquee+=2;InvalidateRect(g_main,0,FALSE);if(g_vizVisible){HWND vh=ActiveVizWindow();if(vh)InvalidateRect(vh,0,FALSE);}
}

static LRESULT CALLBACK MainProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:DragAcceptFiles(h,TRUE);SetTimer(h,1,50,0);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:PaintMain(h);return 0;
 case WM_MOVE:DockAllTools();return 0;
 case WM_NCHITTEST:{int x=LOWORDi(l),y=HIWORDi(l);POINT p={(LONG)x,(LONG)y};ScreenToClient(h,&p);int hit=MainHit(LX(p.x),LY(p.y));if(hit==H_TITLE)return HTCAPTION;return HTCLIENT;}
 case WM_NCLBUTTONDBLCLK:if((int)w==HTCAPTION){ToggleShade();return 0;}break;
 case WM_LBUTTONDBLCLK:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));int hit=MainHit(x,y);if(!g_shade&&hit==H_BAL){g_balance=0;ApplyAudio();FeedbackBalance();InvalidateRect(h,0,FALSE);return 0;}if((!g_shade&&y<24)||(g_shade&&y<SHADE_H))ToggleShade();return 0;}
 case WM_LBUTTONDOWN:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l)),id=MainHit(x,y);if(id==H_VOL){g_dragVol=true;SetCapture(h);g_volume=Clamp((x-363)*100/114,0,100);ApplyAudio();wchar_t q[32],n[16];WCopy(q,L"VOLUME  ",32);IntToW(g_volume,n);WCat(q,n,32);WCat(q,L"%",32);Feedback(q,900);InvalidateRect(h,0,FALSE);}else if(id==H_BAL){g_dragBal=true;SetCapture(h);g_balance=Clamp((x-420)*100/57,-100,100);ApplyAudio();FeedbackBalance();InvalidateRect(h,0,FALSE);}else if(id==H_SEEK){g_dragSeek=true;SetCapture(h);if(g_length)SeekTo(Clamp((x-12)*g_length/476,0,g_length));}else if(id!=H_NONE&&id!=H_TITLE){g_pressedHit=id;SetCapture(h);InvalidateRect(h,0,FALSE);}return 0;}
 case WM_MOUSEMOVE:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));if(g_dragVol){g_volume=Clamp((x-363)*100/114,0,100);ApplyAudio();wchar_t q[32],n[16];WCopy(q,L"VOLUME  ",32);IntToW(g_volume,n);WCat(q,n,32);WCat(q,L"%",32);Feedback(q,700);InvalidateRect(h,0,FALSE);}if(g_dragBal){g_balance=Clamp((x-420)*100/57,-100,100);ApplyAudio();FeedbackBalance();InvalidateRect(h,0,FALSE);}if(g_dragSeek&&g_length){int ms=Clamp((x-12)*g_length/476,0,g_length);g_pos=ms;InvalidateRect(h,0,FALSE);}if(!g_dragVol&&!g_dragBal&&!g_dragSeek){int nh=MainHit(x,y);if(nh!=g_hoverHit){g_hoverHit=nh;InvalidateRect(h,0,FALSE);}}return 0;}
 case WM_LBUTTONUP:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));if(g_dragSeek&&g_length){SeekTo(Clamp((x-12)*g_length/476,0,g_length));Feedback(L"SEEK POSITION UPDATED",750);}if(g_dragSeek||g_dragVol||g_dragBal){g_dragSeek=g_dragVol=g_dragBal=false;ReleaseCapture();return 0;}int id=MainHit(x,y),pressed=g_pressedHit;g_pressedHit=H_NONE;ReleaseCapture();InvalidateRect(h,0,FALSE);if(pressed!=H_NONE&&id!=pressed)return 0;if(id==H_CLOSE)DestroyWindow(h);else if(id==H_MIN)ShowWindow(h,SW_MINIMIZE);else if(id==H_SETTINGS){RECT mr;GetWindowRect(h,&mr);int sc=UIScale();MainContext(mr.left+8*sc,mr.top+(g_shade?SHADE_H:26)*sc);}else if(id==H_PREV){Prev();Feedback(L"PREVIOUS TRACK");}else if(id==H_PLAY){TogglePlay();Feedback(g_playing?L"PLAY":L"PAUSE");}else if(id==H_PAUSE){Pause();Feedback(g_playing?L"PLAY / RESUME":L"PAUSE");}else if(id==H_STOP){Stop();Feedback(L"STOP");}else if(id==H_NEXT){Next();Feedback(L"NEXT TRACK");}else if(id==H_EJECT){OpenFilesDialog();Feedback(L"OPEN FILES");}else if(id==H_TIME){g_timeRemaining=!g_timeRemaining;Feedback(g_timeRemaining?L"TIME  REMAINING":L"TIME  ELAPSED");InvalidateRect(h,0,FALSE);}else if(id==H_VIS){g_visMode=(g_visMode+1)%3;Feedback(L"COMPACT VISUALIZER MODE CHANGED");InvalidateRect(h,0,FALSE);}else if(id==H_MUTE){ToggleMute();Feedback(g_muted?L"MUTE  ON":L"MUTE  OFF");}else if(id==H_SHUFFLE){g_shuffle=!g_shuffle;Feedback(g_shuffle?L"SHUFFLE  ON":L"SHUFFLE  OFF");InvalidateRect(h,0,FALSE);}else if(id==H_REPEAT){g_repeat=(g_repeat+1)%3;if(g_current>=0&&g_playing&&g_repeat==2&&!g_nativeAudio&&!g_dsActive){MCI(L"stop ozamp_track");MCI(L"play ozamp_track repeat");}PrepareNextTrack();Feedback(g_repeat==0?L"REPEAT  OFF":(g_repeat==1?L"REPEAT  ALL":L"REPEAT  ONE"));InvalidateRect(h,0,FALSE);}else if(id==H_AB){CycleAB();Feedback(g_abState==0?L"A/B LOOP  OFF":(g_abState==1?L"A POINT SET":L"A/B LOOP  ACTIVE"));}else if(id==H_EQ){ToggleEQWindow();Feedback(g_eqVisible?L"EQUALIZER  OPEN":L"EQUALIZER  HIDDEN");}else if(id==H_PL){TogglePlaylist();Feedback(g_plVisible?L"PLAYLIST  OPEN":L"PLAYLIST  HIDDEN");}return 0;}
 case WM_RBUTTONUP:{POINT p;GetCursorPos(&p);MainContext(p.x,p.y);return 0;}
 case WM_MOUSEWHEEL:{int d=GET_WHEEL_DELTA_WPARAM(w);g_volume=Clamp(g_volume+(d>0?3:-3),0,100);ApplyAudio();InvalidateRect(h,0,FALSE);return 0;}
 case WM_DROPFILES:{HDROP d=(HDROP)w;UINT n=DragQueryFileW(d,0xffffffffUL,0,0);wchar_t p[MAXP];int play=-1;for(UINT i=0;i<n;i++){if(DragQueryFileW(d,i,p,MAXP)){int ix=AddPath(p);if(play<0)play=ix;}}DragFinish(d);if(play>=0)OpenTrack(play,true);return 0;}
 case WM_KEYDOWN:{UINT k=(UINT)w;if(k==VK_F10){ToggleSettings();return 0;}if(k=='D'&&CtrlDown()){ToggleDoubleSize();return 0;}if(k=='L'&&CtrlDown()){ToggleLibrary();return 0;}if(k==VK_F12){ToggleVisualizerWindow();return 0;}if(k==VK_F11&&g_vizVisible){ToggleVizFull();return 0;}if(k==VK_SPACE)TogglePlay();else if(k==VK_LEFT&&g_current>=0)SeekTo(g_pos-5000);else if(k==VK_RIGHT&&g_current>=0)SeekTo(g_pos+5000);else if(k==VK_UP){g_muted=false;g_volume=Clamp(g_volume+2,0,100);ApplyAudio();}else if(k==VK_DOWN){g_volume=Clamp(g_volume-2,0,100);ApplyAudio();}else if(k=='Z')Prev();else if(k=='X')Play();else if(k=='C')Pause();else if(k=='V')Stop();else if(k=='B')Next();else if(k=='L')OpenFilesDialog();else if(k=='P')TogglePlaylist();else if(k=='E')ToggleEQWindow();else if(k=='A')ToggleArt();else if(k=='J'){if(!g_plVisible)TogglePlaylist();g_jump=true;g_jumpText[0]=0;SetFocus(g_pl);InvalidateRect(g_pl,0,FALSE);}else if(k=='S')g_shuffle=!g_shuffle;else if(k=='M')ToggleMute();else if(k==VK_F5){g_externalSkin=false;g_skin=(g_skin+1)%3;InitColors();InitFonts();InvalidateRect(g_pl,0,FALSE);}InvalidateRect(h,0,FALSE);return 0;}
 case WM_APPCOMMAND:{int c=(int)((l>>16)&0x7ff);if(c==APPCOMMAND_MEDIA_NEXTTRACK)Next();else if(c==APPCOMMAND_MEDIA_PREVIOUSTRACK)Prev();else if(c==APPCOMMAND_MEDIA_STOP)Stop();else if(c==APPCOMMAND_MEDIA_PLAY_PAUSE)TogglePlay();return 1;}
 case WM_HOTKEY:if(w==1||w==6)TogglePlay();else if(w==2||w==7)Prev();else if(w==3||w==8)Next();else if(w==9)Stop();else if(w==4){g_muted=false;g_volume=Clamp(g_volume+4,0,100);ApplyAudio();InvalidateRect(h,0,FALSE);}else if(w==5){g_volume=Clamp(g_volume-4,0,100);ApplyAudio();InvalidateRect(h,0,FALSE);}return 0;
 case WM_OZTRAY:if(l==WM_LBUTTONDBLCLK){ShowWindow(g_main,SW_SHOW);SetForegroundWindow(g_main);}else if(l==WM_RBUTTONUP)TrayMenu();return 0;
 case WM_CONTEXTMENU:{POINT p;GetCursorPos(&p);MainContext(p.x,p.y);return 0;}
 case WM_TIMER:Tick();return 0;
 case WM_CLOSE:DestroyWindow(h);return 0;
 case WM_DESTROY:SaveSettings();SaveLibrary();UnregisterGlobals();RemoveTray();TaskbarDone();ReleaseCover();CloseTrack();KillTimer(h,1);if(g_gdiplusToken){GdiplusShutdown(g_gdiplusToken);g_gdiplusToken=0;}PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,m,w,l);
}

static void PlaylistMatchEQWidth(HWND h){
 if(!h)return;
 RECT target={0,0,0,0},p={0,0,0,0};
 if(g_eq&&g_eqVisible)GetWindowRect(g_eq,&target);else if(g_main)GetWindowRect(g_main,&target);else return;
 GetWindowRect(h,&p);
 int sc=MaxI(1,UIScale());
 int targetW=target.right-target.left;
 g_plW=Clamp((targetW+sc/2)/sc,PL_MIN_W,1400);
 int actualW=g_plW*sc;
 g_plRightAnchorTarget=(g_eq&&g_eqVisible)?1:0;
 g_plDockAlign=2;
 // Keep current Y/height; only normalize width and right edge.
 g_dockMove=true;
 MoveWindow(h,target.right-actualW,p.top,actualW,p.bottom-p.top,TRUE);
 g_dockMove=false;
 RoundWindow(h,14);
 InvalidateRect(h,0,FALSE);
}

static LRESULT CALLBACK PlProc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:DragAcceptFiles(h,TRUE);return 0;
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:PaintPlaylist(h);return 0;
 case WM_WINDOWPOSCHANGING:{WINDOWPOS* wp=(WINDOWPOS*)l;if(wp&&g_plWindowDrag&&g_plRightAnchorTarget>=0&&!(wp->flags&SWP_NOMOVE)){RECT t;if(GetPlaylistTargetRect(g_plRightAnchorTarget,t))wp->x=t.right-wp->cx;}return 0;}
 case WM_MOVING:{RECT* pr=(RECT*)l;if(pr){ApplyPlaylistMoveMagnet(*pr);}return 1;}
 case WM_MOVE:if(g_eqVisible&&g_eqDockTarget==1&&g_eqDockEdge)DockEQ();return 0;
 case WM_EXITSIZEMOVE:SnapPlaylistIfClose();return 0;
 case WM_NCHITTEST:{return HTCLIENT;}
 case WM_LBUTTONDOWN:{
  int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));bool rr=x>=g_plW-9,bb=y>=g_plH-9;if(rr||bb){g_plResize=rr&&bb?3:(rr?1:2);GetCursorPos(&g_plResizeStart);GetWindowRect(h,&g_plResizeStartRect);g_plResizeStartW=g_plW;g_plResizeStartH=g_plH;g_plResizeLastRawRight=g_plResizeStartRect.right;SetCapture(h);return 0;}
  bool topClose=(y<24&&x>=g_plW-27);
  if(y<24&&!topClose){g_plWindowDrag=true;GetCursorPos(&g_plWindowDragStart);GetWindowRect(h,&g_plWindowDragRect);g_plWindowDragLastRawRight=g_plWindowDragRect.right;SetCapture(h);return 0;}
  int id=PlHit(x,y);
  if(id==H_PL_SEARCH){g_plFilterActive=true;SetFocus(h);InvalidateRect(h,0,FALSE);return 0;}
  if(id==H_PL_SCROLL){int st,sb,ty,th;PlaylistScrollGeom(st,sb,ty,th);if(y>=ty&&y<ty+th){g_plScrollDrag=true;g_plScrollGrab=y-ty;SetCapture(h);}else{g_scroll=Clamp(g_scroll+(y<ty?-PlaylistRows():PlaylistRows()),0,PlaylistMaxScroll());InvalidateRect(h,0,FALSE);}return 0;}
  if(id==H_PL_LIST){int r=(y-(PlListTop()+4))/17,idx=VisibleTrackAt(g_scroll+r);if(idx>=0&&idx<g_count){g_selected=idx;if(ShiftDown()&&g_markAnchor>=0){MarkRange(g_markAnchor,idx);}else if(CtrlDown()){g_tracks[idx].marked=!g_tracks[idx].marked;g_markAnchor=idx;}else{ClearMarks();g_tracks[idx].marked=true;g_markAnchor=idx;}if(!g_scanRunning){g_plDrag=true;g_plDragIndex=idx;g_plDragTarget=idx;SetCapture(h);}else SetStatus(L"WAIT FOR RG SCAN");InvalidateRect(h,0,FALSE);}return 0;}
  if(id&&id!=H_TITLE){g_plPressed=id;SetCapture(h);InvalidateRect(h,0,FALSE);}return 0;}
 case WM_MOUSEMOVE:{
  if(g_plWindowDrag){POINT p;GetCursorPos(&p);int dx=p.x-g_plWindowDragStart.x,dy=p.y-g_plWindowDragStart.y;RECT r=g_plWindowDragRect;r.left+=dx;r.right+=dx;r.top+=dy;r.bottom+=dy;ApplyPlaylistDirectDragSnap(r);g_dockMove=true;MoveWindow(h,r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);g_dockMove=false;return 0;}
  if(g_plScrollDrag){int y=LY(HIWORDi(l));int st,sb,ty,th;PlaylistScrollGeom(st,sb,ty,th);int travel=MaxI(1,(sb-st)-th);int pos=Clamp(y-g_plScrollGrab-st,0,travel);g_scroll=Clamp((pos*PlaylistMaxScroll()+travel/2)/travel,0,PlaylistMaxScroll());InvalidateRect(h,0,FALSE);}
  else if(g_plResize){
   POINT p;GetCursorPos(&p);int sc=UIScale();int dx=(p.x-g_plResizeStart.x)/sc,dy=(p.y-g_plResizeStart.y)/sc;
   int nw=g_plResizeStartW,nh=g_plResizeStartH;
   if(g_plResize==1||g_plResize==3)nw=Clamp(g_plResizeStartW+dx,PL_MIN_W,1400);
   if(g_plResize==2||g_plResize==3)nh=Clamp(g_plResizeStartH+dy,PL_MIN_H,1000);
   int widthPx=nw*sc;
   if(g_plResize==1||g_plResize==3){int rawRight=g_plResizeStartRect.left+widthPx;int snappedRight=PlaylistResizeRightMagnet(g_plResizeStartRect.left,rawRight);widthPx=MaxI(PL_MIN_W*sc,snappedRight-g_plResizeStartRect.left);nw=Clamp((widthPx+sc/2)/sc,PL_MIN_W,1400);widthPx=nw*sc;}
   if(nw!=g_plW||nh!=g_plH){
    g_plW=nw;g_plH=nh;
    // A bottom/right resize behaves like a normal Windows window: LEFT stays fixed.
    // The right edge may snap to EQ/Main, but it never drags the whole playlist left.
    g_dockMove=true;MoveWindow(h,g_plResizeStartRect.left,g_plResizeStartRect.top,widthPx,g_plH*sc,TRUE);g_dockMove=false;
    RoundWindow(h,14);InvalidateRect(h,0,FALSE);
   }
  }
  else if(g_plDrag){int y=LY(HIWORDi(l));if(y>=PlListTop()+4&&y<PlListBottom()){int t=VisibleTrackAt(g_scroll+(y-(PlListTop()+4))/17);if(t>=0)g_plDragTarget=t;InvalidateRect(h,0,FALSE);}}
  else{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));int nh=PlHoverId(x,y);if(nh!=g_plHover){g_plHover=nh;InvalidateRect(h,0,FALSE);}}return 0;}
 case WM_LBUTTONDBLCLK:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));bool topClose=(y<24&&x>=g_plW-27);if(y<24&&!topClose){g_plWindowDrag=false;ReleaseCapture();PlaylistMatchEQWidth(h);return 0;}if(PlHit(x,y)==H_PL_LIST){int idx=VisibleTrackAt(g_scroll+(y-(PlListTop()+4))/17);if(idx>=0&&idx<g_count){g_selected=idx;OpenTrack(idx,true);}}return 0;}
 case WM_LBUTTONUP:{
  if(g_plWindowDrag){g_plWindowDrag=false;g_plWindowDragLastRawRight=0;ReleaseCapture();RECT r;GetWindowRect(h,&r);ApplyPlaylistDirectDragSnap(r);g_dockMove=true;MoveWindow(h,r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);g_dockMove=false;return 0;}
  if(g_plScrollDrag){g_plScrollDrag=false;ReleaseCapture();return 0;}if(g_plResize){int finishedResize=g_plResize;g_plResize=0;g_plResizeLastRawRight=0;ReleaseCapture();RECT r;GetWindowRect(h,&r);
   if(g_plDockEdge){
    RECT t;if(GetPlaylistTargetRect(g_plDockTarget,t)){
     if(g_plRightAnchorTarget==g_plDockTarget&&AbsI(r.right-t.right)<=3*UIScale())g_plDockAlign=2;
     else{g_plDockAlign=0;g_plDockOffset=(r.left-t.left)/MaxI(1,UIScale());}
    }
    DockPlaylist();
   }else SnapPlaylistIfClose();
   return 0;}
  int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));if(g_plDrag){ReleaseCapture();if(g_plDragIndex>=0&&g_plDragTarget>=0&&g_plDragIndex!=g_plDragTarget){if(MarkedCount()>1)MoveMarkedBlock(g_plDragTarget);else{wchar_t c[MAXP],ss[MAXP],q[MAXP];RememberIndices(c,ss,q);Track t=g_tracks[g_plDragIndex];if(g_plDragIndex<g_plDragTarget){for(int i=g_plDragIndex;i<g_plDragTarget;i++)g_tracks[i]=g_tracks[i+1];}else{for(int i=g_plDragIndex;i>g_plDragTarget;i--)g_tracks[i]=g_tracks[i-1];}g_tracks[g_plDragTarget]=t;RestoreIndices(c,ss,q);SyncQueuedIndex();}Feedback(L"PLAYLIST ORDER UPDATED");}g_plDrag=false;g_plDragIndex=g_plDragTarget=-1;InvalidateRect(h,0,FALSE);return 0;}
  int id=PlHit(x,y);int pressed=g_plPressed;g_plPressed=0;ReleaseCapture();InvalidateRect(h,0,FALSE);if(pressed&&pressed!=id)return 0;
  if(id==H_PL_CLOSE){TogglePlaylist();Feedback(L"PLAYLIST  HIDDEN");}else if(id==H_PL_ADD){OpenFilesDialog();Feedback(L"FILES ADDED");}else if(id==H_PL_DIR){OpenFolderDialog();Feedback(L"FOLDER ADDED");}else if(id==H_PL_REMOVE){RemoveSelected();Feedback(L"SELECTION REMOVED");}else if(id==H_PL_CLEAR){ClearAll();QueueClear();Feedback(L"PLAYLIST CLEARED");}else if(id==H_PL_LOAD){LoadM3UDialog();Feedback(L"PLAYLIST LOADED");}else if(id==H_PL_SAVE){SaveM3UDialog();Feedback(L"PLAYLIST SAVED");}return 0;}
 case WM_RBUTTONUP:{int x=LX(LOWORDi(l)),y=LY(HIWORDi(l));if(PtIn(x,y,8,PlListTop(),g_plW-28,PlListBottom())){int idx=VisibleTrackAt(g_scroll+(y-(PlListTop()+4))/17);if(idx>=0&&idx<g_count)g_selected=idx;}POINT p;GetCursorPos(&p);PlaylistContext(p.x,p.y);InvalidateRect(h,0,FALSE);return 0;}
 case WM_MOUSEWHEEL:{int d=GET_WHEEL_DELTA_WPARAM(w);g_scroll=Clamp(g_scroll+(d>0?-3:3),0,PlaylistMaxScroll());InvalidateRect(h,0,FALSE);return 0;}
 case WM_KEYDOWN:{UINT k=(UINT)w;if(k=='F'&&CtrlDown()){g_plFilterActive=true;SetFocus(h);InvalidateRect(h,0,FALSE);return 0;}if(k=='D'&&CtrlDown()){ToggleDoubleSize();return 0;}if(k==VK_UP&&PlaylistVisibleCount()){int vp=VisiblePosOfTrack(g_selected);int ix=VisibleTrackAt(vp>0?vp-1:0);if(ix>=0)SelectPlaylistIndex(ix);InvalidateRect(h,0,FALSE);}else if(k==VK_DOWN&&PlaylistVisibleCount()){int vp=VisiblePosOfTrack(g_selected);int ix=VisibleTrackAt(vp<0?0:MinI(PlaylistVisibleCount()-1,vp+1));if(ix>=0)SelectPlaylistIndex(ix);InvalidateRect(h,0,FALSE);}else if(k==VK_HOME&&PlaylistVisibleCount()){int ix=VisibleTrackAt(0);if(ix>=0)SelectPlaylistIndex(ix);InvalidateRect(h,0,FALSE);}else if(k==VK_END&&PlaylistVisibleCount()){int ix=VisibleTrackAt(PlaylistVisibleCount()-1);if(ix>=0)SelectPlaylistIndex(ix);InvalidateRect(h,0,FALSE);}else if(k==VK_RETURN&&g_selected>=0){if(g_jump)g_jump=false;g_plFilterActive=false;OpenTrack(g_selected,true);}else if(k==VK_DELETE)RemoveSelected();else if(k==VK_SPACE)TogglePlay();else if(k=='Q'&&g_selected>=0){QueueAddPath(g_tracks[g_selected].path,ShiftDown());Feedback(ShiftDown()?L"PLAY NEXT":L"ADDED TO QUEUE");InvalidateRect(h,0,FALSE);InvalidateRect(g_main,0,FALSE);}else if(k==VK_ESCAPE){if(g_plFilterActive){g_plFilterActive=false;if(g_plFilter[0]){g_plFilter[0]=0;g_scroll=0;}InvalidateRect(h,0,FALSE);}else if(g_jump){g_jump=false;g_jumpText[0]=0;InvalidateRect(h,0,FALSE);}else TogglePlaylist();}return 0;}
 case WM_CHAR:{wchar_t ch=(wchar_t)w;if(g_plFilterActive){int n=WLen(g_plFilter);if(ch==8){if(n>0)g_plFilter[n-1]=0;}else if(ch==13){g_plFilterActive=false;}else if(ch>=32&&ch!=127&&n<94){g_plFilter[n]=ch;g_plFilter[n+1]=0;}g_scroll=0;BuildFilterMap();if(g_filterCount&&VisiblePosOfTrack(g_selected)<0)g_selected=g_filterMap[0];InvalidateRect(h,0,FALSE);return 0;}if(!g_jump&&(ch==L'j'||ch==L'J')){g_jump=true;g_jumpText[0]=0;InvalidateRect(h,0,FALSE);return 0;}if(g_jump){int n=WLen(g_jumpText);if(ch==8){if(n>0)g_jumpText[n-1]=0;}else if(ch==13){g_jump=false;if(g_selected>=0)OpenTrack(g_selected,true);}else if(ch>=32&&ch<127&&n<94){g_jumpText[n]=ch;g_jumpText[n+1]=0;}JumpFind();InvalidateRect(h,0,FALSE);return 0;}return 0;}
 case WM_DROPFILES:{HDROP d=(HDROP)w;UINT n=DragQueryFileW(d,0xffffffffUL,0,0);wchar_t p[MAXP];int first=-1;for(UINT i=0;i<n;i++){if(DragQueryFileW(d,i,p,MAXP)){int ix=AddPath(p);if(first<0)first=ix;}}DragFinish(d);if(first>=0){SelectPlaylistIndex(first);SetStatus(L"ADDED TO PLAYLIST");InvalidateRect(h,0,FALSE);InvalidateRect(g_main,0,FALSE);}return 0;}
 case WM_CLOSE:TogglePlaylist();return 0;
 }return DefWindowProcW(h,m,w,l);
}



static void DrawAbout(HDC dc){
 Fill(dc,0,0,ABOUT_W,ABOUT_H,C_BG);
 Box(dc,1,1,ABOUT_W-1,29,C_PANEL2,C_EDGE,12);
 Txt(dc,L"ABOUT OZAMP",12,3,330,27,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 Button(dc,ABOUT_W-27,4,ABOUT_W-4,25,L"X",false,true);

 if(!g_aboutEgg){
  // Restrained identity block: no labels or decorative slogans.
  Box(dc,14,42,ABOUT_W-14,118,C_PANEL,C_EDGE,12);
  Txt(dc,L"OZAMP",28,48,200,78,C_TEXT,g_led,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"1.0.0",29,79,110,101,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"Created & developed by Oskar Lumbojev",122,79,ABOUT_W-28,101,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

  // A subtle animated visual signature. Deliberately unlabeled.
  int px1=14,py1=130,px2=ABOUT_W-14,py2=190;
  Box(dc,px1,py1,px2,py2,RGBc(5,9,17),C_EDGE,11);
  ULONGLONG tm=GetTickCount64();int cell=10;
  for(int y=py1+5;y<py2-5;y+=cell){
   for(int x=px1+5;x<px2-5;x+=cell){
    double dx=(double)(x-(px1+px2)/2),dy=(double)(y-(py1+py2)/2);
    double dist=sqrt(dx*dx+dy*dy),t=(double)tm/1200.0;
    double v=(sin(x*.045+t)+sin(y*.061-t*.8)+sin((x+y)*.027+t*.55)+sin(dist*.060-t*1.25))/4.0;
    v=(v+1.0)*.5;
    DWORD c=PlasmaColor(VizClamp01(v),VizClamp01(.30+v*.42));
    Fill(dc,x,y,MinI(px2-5,x+cell),MinI(py2-5,y+cell),c);
   }
  }
  // Dark veil keeps the visual signature restrained.
  for(int yy=py1+6;yy<py2-6;yy+=4)Line(dc,px1+6,yy,px2-6,yy,RGBc(7,13,22),1);
  int mid=(py1+py2)/2,last=-1;
  for(int x=px1+12;x<px2-12;x+=5){double t=(double)tm/420.0;int yy=mid+(int)(sin((x-px1)*.047+t)*5+sin((x-px1)*.015-t*.8)*4);if(last>=0)Line(dc,x-5,last,x,yy,C_LED2,1);last=yy;}

  Box(dc,14,202,ABOUT_W-14,314,C_PANEL,C_EDGE,12);
  Txt(dc,L"About",28,211,ABOUT_W-28,233,C_TEXT,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"OzAmp began as a personal project to build the kind of desktop music player I wanted to use myself: fast, compact and focused on the music.",28,238,ABOUT_W-28,274,C_MUTED,g_small,DT_LEFT|DT_WORDBREAK);
  Txt(dc,L"It is designed and developed by Oskar Lumbojev, with an emphasis on direct controls, local playback and a UI that feels good to use.",28,274,ABOUT_W-28,307,C_MUTED,g_small,DT_LEFT|DT_WORDBREAK);
 }else{
  Box(dc,14,42,ABOUT_W-14,118,C_PANEL,C_EDGE,12);
  Txt(dc,L"OZAMP",28,48,190,84,C_LED,g_led,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"1.0.0  //  ORIGINAL BUILD",29,84,ABOUT_W-28,108,C_LED2,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Box(dc,14,128,ABOUT_W-14,314,C_BLACK,C_LED2,12);
  Txt(dc,L"ORIGINAL CREDITS",28,136,230,158,C_LED,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"",260,136,ABOUT_W-28,158,C_LED2,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
  Line(dc,28,164,ABOUT_W-28,164,C_EDGE,1);
  Txt(dc,L"OSKAR LUMBOJEV",28,172,ABOUT_W-28,204,C_TEXT,g_led,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"CREATOR & DEVELOPER",29,201,ABOUT_W-28,223,C_LED2,g_bold,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Txt(dc,L"OzAmp was created and developed independently in 2026.",29,228,ABOUT_W-28,249,C_MUTED,g_small,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  Line(dc,28,257,ABOUT_W-28,257,C_EDGE,1);
  ULONGLONG tm=GetTickCount64();int scan=267+(int)((tm/38)%35);
  for(int i=0;i<22;i++){int x=29+i*20;int hh=4+(int)(((tm/70)+(ULONGLONG)i*7)%13);Fill(dc,x,301-hh,x+11,301,(i%5==0)?C_LED:C_LED2);}
  Line(dc,28,scan,ABOUT_W-28,scan,C_ACCENT,1);
  Txt(dc,L"OZAMP // ORIGINAL CREDITS",28,304,ABOUT_W-28,312,C_MUTED,g_small,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 }
}
static void PaintAbout(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);HDC mem=CreateCompatibleDC(dc);HBITMAP bm=CreateCompatibleBitmap(dc,ABOUT_W,ABOUT_H);HGDIOBJ old=SelectObject(mem,bm);DrawAbout(mem);BitBlt(dc,0,0,ABOUT_W,ABOUT_H,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(h,&ps);}
static void ToggleAbout(){if(!g_about)return;if(IsWindowVisible(g_about)){KillTimer(g_about,2);ShowWindow(g_about,SW_HIDE);return;}RECT m;GetWindowRect(g_main,&m);int x=m.left+20*UIScale(),y=m.top+34*UIScale();SetWindowPos(g_about,g_top?HWND_TOPMOST:(HWND)0,x,y,0,0,SWP_NOSIZE);RoundWindow(g_about,14);ShowWindow(g_about,SW_SHOW);SetTimer(g_about,2,60,0);SetForegroundWindow(g_about);InvalidateRect(g_about,0,FALSE);}
static LRESULT CALLBACK AboutProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
 case WM_ERASEBKGND:return 1;
 case WM_PAINT:PaintAbout(h);return 0;
 case WM_NCHITTEST:{POINT p={(LONG)LOWORDi(l),(LONG)HIWORDi(l)};ScreenToClient(h,&p);if(p.y<29&&p.x<ABOUT_W-34)return HTCAPTION;return HTCLIENT;}
 case WM_LBUTTONUP:{int x=LOWORDi(l),y=HIWORDi(l);if(PtIn(x,y,ABOUT_W-27,4,ABOUT_W-4,25)){KillTimer(h,2);ShowWindow(h,SW_HIDE);return 0;}
  // Easter egg: five quick clicks on the large OZAMP wordmark. Nothing in the normal About UI advertises it.
  if(PtIn(x,y,24,44,205,112)){ULONGLONG now=GetTickCount64();if(now>g_aboutLogoClickUntil)g_aboutLogoClicks=0;g_aboutLogoClickUntil=now+3000;g_aboutLogoClicks++;if(g_aboutLogoClicks>=5){g_aboutLogoClicks=0;g_aboutEgg=!g_aboutEgg;if(g_aboutEgg){if(g_main)Feedback(L"ORIGINAL CREDITS",1100);}else{if(g_main)Feedback(L"ABOUT",700);}InvalidateRect(h,0,FALSE);}return 0;}
  return 0;}
 case WM_TIMER:if(w==2){InvalidateRect(h,0,FALSE);return 0;}break;
 case WM_KEYDOWN:if(w==VK_ESCAPE){KillTimer(h,2);ShowWindow(h,SW_HIDE);return 0;}return 0;
 case WM_CLOSE:KillTimer(h,2);ShowWindow(h,SW_HIDE);return 0;
 }return DefWindowProcW(h,m,w,l);}

static void InitColors(){if(g_skin==1){C_BG=RGBc(13,15,13);C_PANEL=RGBc(18,22,18);C_PANEL2=RGBc(27,34,27);C_EDGE=RGBc(62,78,62);C_TEXT=RGBc(222,235,215);C_MUTED=RGBc(130,151,125);C_ACCENT=RGBc(112,159,102);C_LED=RGBc(173,255,123);C_LED2=RGBc(94,207,94);C_RED=RGBc(218,91,105);C_BLACK=RGBc(3,8,3);}else if(g_skin==2){C_BG=RGBc(224,233,242);C_PANEL=RGBc(243,247,251);C_PANEL2=RGBc(218,229,240);C_EDGE=RGBc(146,165,185);C_TEXT=RGBc(29,41,54);C_MUTED=RGBc(94,116,139);C_ACCENT=RGBc(102,136,174);C_LED=RGBc(44,92,120);C_LED2=RGBc(71,122,96);C_RED=RGBc(165,82,88);C_BLACK=RGBc(236,243,248);}else{C_BG=RGBc(10,14,22);C_PANEL=RGBc(15,23,31);C_PANEL2=RGBc(22,32,43);C_EDGE=RGBc(49,69,86);C_TEXT=RGBc(226,236,244);C_MUTED=RGBc(125,147,164);C_ACCENT=RGBc(105,148,188);C_LED=RGBc(150,246,212);C_LED2=RGBc(74,190,164);C_RED=RGBc(218,91,105);C_BLACK=RGBc(4,10,13);}}
static void InitFonts(){if(g_font)DeleteObject(g_font);if(g_small)DeleteObject(g_small);if(g_bold)DeleteObject(g_bold);if(g_led)DeleteObject(g_led);g_font=CreateFontW(-14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,g_fontName);g_small=CreateFontW(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,g_fontName);g_bold=CreateFontW(-15,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,g_fontName);g_led=CreateFontW(-17,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,g_ledFontName);}
static void LoadExternalSkin(const wchar_t*path){if(!path||!path[0])return;wchar_t v[128];struct KV{const wchar_t*k;DWORD*c;}kv[]={{L"bg",&C_BG},{L"panel",&C_PANEL},{L"panel2",&C_PANEL2},{L"edge",&C_EDGE},{L"text",&C_TEXT},{L"muted",&C_MUTED},{L"accent",&C_ACCENT},{L"led",&C_LED},{L"led2",&C_LED2},{L"danger",&C_RED},{L"black",&C_BLACK}};for(int i=0;i<11;i++){GetPrivateProfileStringW(L"colors",kv[i].k,L"",v,128,path);DWORD c;if(ParseColor(v,c))*kv[i].c=c;}GetPrivateProfileStringW(L"fonts",L"ui",g_fontName,g_fontName,64,path);GetPrivateProfileStringW(L"fonts",L"led",g_ledFontName,g_ledFontName,64,path);WCopy(g_skinFile,path,MAXP);g_externalSkin=true;InitFonts();if(g_main)InvalidateRect(g_main,0,FALSE);if(g_pl)InvalidateRect(g_pl,0,FALSE);if(g_eq)InvalidateRect(g_eq,0,FALSE);if(g_lib)InvalidateRect(g_lib,0,FALSE);if(g_art)InvalidateRect(g_art,0,FALSE);if(g_viz)InvalidateRect(g_viz,0,FALSE);if(g_settings)InvalidateRect(g_settings,0,FALSE);if(g_about)InvalidateRect(g_about,0,FALSE);if(g_info)InvalidateRect(g_info,0,FALSE);if(g_tag)InvalidateRect(g_tag,0,FALSE);}
static void OpenSkinDialog(){static wchar_t p[MAXP];p[0]=0;OPENFILENAMEW o;memset(&o,0,sizeof(o));o.lStructSize=sizeof(o);o.hwndOwner=g_main;o.lpstrFile=p;o.nMaxFile=MAXP;o.lpstrFilter=L"OzAmp skins\0*.ozskin\0All files\0*.*\0\0";o.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST;if(GetOpenFileNameW(&o))LoadExternalSkin(p);}


static bool LoadCommandLine(){int argc=0;LPWSTR*argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(!argv||argc<=1){if(argv)LocalFree(argv);return false;}ClearAll();int play=-1;for(int i=1;i<argc;i++){if(WEndsI(argv[i],L".m3u")||WEndsI(argv[i],L".m3u8")){LoadM3UPath(argv[i],true);if(g_count)play=0;}else{int ix=AddPath(argv[i]);if(play<0)play=ix;}}LocalFree(argv);if(play>=0){OpenTrack(play,true);return true;}return false;}

extern "C" void WINAPI WinMainCRTStartup(){
 SetProcessDPIAware();g_inst=GetModuleHandleW(0);InitPaths();LoadSettings();InitColors();InitFonts();if(g_skinFile[0]&&FileExists(g_skinFile))LoadExternalSkin(g_skinFile);GDIPLUS_STARTUP_INPUT gi;memset(&gi,0,sizeof(gi));gi.GdiplusVersion=1;GdiplusStartup(&g_gdiplusToken,&gi,0);if(!OzAudioInit(g_deviceId)){HRESULT ehr=OzAudioLastError();wchar_t estage[96];WCopy(estage,OzAudioLastErrorStage(),96);if(g_deviceId[0]){bool defok=OzAudioInit(0);SetOutputSwitchInfo(defok?L"SAVED OUTPUT UNAVAILABLE - TEMPORARILY USING WINDOWS DEFAULT":L"AUDIO OUTPUT INITIALIZATION FAILED",estage,ehr);}else SetOutputSwitchInfo(L"WINDOWS DEFAULT OUTPUT FAILED",estage,ehr);}
 if(OzAudioReady())OzAudioSetEQ(g_eqEnabled,g_preampDb,g_eqBands);
 WNDCLASSEXW wc;memset(&wc,0,sizeof(wc));wc.cbSize=sizeof(wc);wc.style=CS_DBLCLKS;wc.lpfnWndProc=MainProc;wc.hInstance=g_inst;wc.hCursor=LoadCursorW(0,IDC_ARROW);wc.hIcon=LoadIconW(0,IDI_APPLICATION);wc.lpszClassName=L"OzAmpMain30";RegisterClassExW(&wc);
 WNDCLASSEXW pc=wc;pc.lpfnWndProc=PlProc;pc.lpszClassName=L"OzAmpPlaylist30";RegisterClassExW(&pc);WNDCLASSEXW ec=wc;ec.lpfnWndProc=EqProc;ec.lpszClassName=L"OzAmpEQ30";RegisterClassExW(&ec);WNDCLASSEXW lc=wc;lc.lpfnWndProc=LibProc;lc.lpszClassName=L"OzAmpLibrary30";RegisterClassExW(&lc);WNDCLASSEXW ac=wc;ac.lpfnWndProc=ArtProc;ac.lpszClassName=L"OzAmpArt30";RegisterClassExW(&ac);WNDCLASSEXW vc=wc;vc.lpfnWndProc=VizProc;vc.lpszClassName=L"OzAmpViz30";RegisterClassExW(&vc);WNDCLASSEXW tc=wc;tc.lpfnWndProc=TagProc;tc.lpszClassName=L"OzAmpTag30";RegisterClassExW(&tc);WNDCLASSEXW nc=wc;nc.lpfnWndProc=ToastProc;nc.lpszClassName=L"OzAmpToast30";RegisterClassExW(&nc);WNDCLASSEXW sc=wc;sc.lpfnWndProc=SettingsProc;sc.lpszClassName=L"OzAmpSettings30";RegisterClassExW(&sc);WNDCLASSEXW bc=wc;bc.lpfnWndProc=AboutProc;bc.lpszClassName=L"OzAmpAbout30";RegisterClassExW(&bc);WNDCLASSEXW ic=wc;ic.lpfnWndProc=InfoProc;ic.lpszClassName=L"OzAmpInfo30";RegisterClassExW(&ic);WNDCLASSEXW xc=wc;xc.lpfnWndProc=ErrorProc;xc.lpszClassName=L"OzAmpError35";RegisterClassExW(&xc);
 DWORD ex=WS_EX_ACCEPTFILES|(g_top?WS_EX_TOPMOST:0);g_main=CreateWindowExW(ex,L"OzAmpMain30",L"OzAmp",WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,140,120,MAIN_W*UIScale(),(g_shade?SHADE_H:MAIN_H)*UIScale(),0,0,g_inst,0);
 g_pl=CreateWindowExW(WS_EX_ACCEPTFILES|WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpPlaylist30",L"OzAmp Playlist",WS_POPUP|WS_CLIPCHILDREN,140,120+(g_shade?SHADE_H:MAIN_H)*UIScale(),g_plW*UIScale(),g_plH*UIScale(),g_main,0,g_inst,0);
 g_eq=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpEQ30",L"OzAmp Equalizer",WS_POPUP|WS_CLIPCHILDREN,650,120,EQ_W*UIScale(),EQ_H*UIScale(),g_main,0,g_inst,0);
 g_lib=CreateWindowExW(WS_EX_ACCEPTFILES|WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpLibrary30",L"OzAmp Media Library",WS_POPUP|WS_CLIPCHILDREN,140,430,700,420,g_main,0,g_inst,0);
 g_art=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpArt30",L"OzAmp Album Art",WS_POPUP|WS_CLIPCHILDREN,650,300,260,300,g_main,0,g_inst,0);
 g_viz=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpViz30",L"OzAmp Visualizer",WS_POPUP|WS_CLIPCHILDREN,220,220,800,450,g_main,0,g_inst,0);
 g_toast=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,L"OzAmpToast30",L"OzAmp",WS_POPUP|WS_CLIPCHILDREN,0,0,360,86,g_main,0,g_inst,0);
 g_settings=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpSettings30",L"OzAmp Settings",WS_POPUP|WS_CLIPCHILDREN,260,180,SETTINGS_W,SETTINGS_H,g_main,0,g_inst,0);
 g_about=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpAbout30",L"About OzAmp",WS_POPUP|WS_CLIPCHILDREN,280,200,ABOUT_W,ABOUT_H,g_main,0,g_inst,0);
 g_info=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpInfo30",L"OzAmp Track Info",WS_POPUP|WS_CLIPCHILDREN,300,220,INFO_W,INFO_H,g_main,0,g_inst,0);
 g_error=CreateWindowExW(WS_EX_TOOLWINDOW|(g_top?WS_EX_TOPMOST:0),L"OzAmpError35",L"OzAmp",WS_POPUP|WS_CLIPCHILDREN,300,220,500,176,g_main,0,g_inst,0);
 RoundCoreWindows();RestoreWindowPositions();
 LoadLibrary();bool loaded=LoadM3UPath(g_playlistFile,false);if(!loaded)LoadM3UPath(g_session,false);SyncQueuedIndex();if(g_restoreIndex>=0&&g_restoreIndex<g_count){if(OpenTrack(g_restoreIndex,false)){if(g_restorePos>0)SeekTo(MinI(g_restorePos,g_length));if(g_restoreState==1)Play();else if(g_restoreState==2){g_paused=true;g_playing=false;SetStatus(L"SESSION RESTORED / PAUSED");}}}LoadCommandLine();
 AddTray();TaskbarInit();RegisterGlobals();ShowWindow(g_main,SW_SHOW);UpdateWindow(g_main);ShowWindow(g_eq,g_eqVisible?SW_SHOW:SW_HIDE);ShowWindow(g_lib,g_libVisible?SW_SHOW:SW_HIDE);ShowWindow(g_art,g_artVisible?SW_SHOW:SW_HIDE);ShowWindow(g_viz,g_vizVisible?SW_SHOW:SW_HIDE);ShowWindow(g_toast,SW_HIDE);ShowWindow(g_settings,SW_HIDE);ShowWindow(g_about,SW_HIDE);ShowWindow(g_info,SW_HIDE);ShowWindow(g_error,SW_HIDE);if(g_plVisible){ShowWindow(g_pl,SW_SHOW);DockPlaylist();}else ShowWindow(g_pl,SW_HIDE);DockAllTools();
 MSG msg;while(GetMessageW(&msg,0,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}OzAudioShutdown();DeleteObject(g_font);DeleteObject(g_small);DeleteObject(g_bold);DeleteObject(g_led);ExitProcess(0);
}
