// Unified Spotify presentation. Transitions change windows only, never playback.
static double g_spDpiScale=1.0;
static HFONT g_spBodyFont=0,g_spCaptionFont=0,g_spStrongFont=0;
static int g_spFontDpi=0;
static bool g_spKaraoke=false,g_spKaraokeSavedFollow=true;
static int g_spKaraokeSavedScroll=0,g_spKaraokeSavedLyricOffset=0,g_spKaraokeLyricSerial=0;
static HFONT g_spKaraokeFont=0,g_spKaraokeSmallFont=0;
typedef UINT (WINAPI *SpGetDpiFn)(HWND);
typedef void* (WINAPI *SpThreadDpiFn)(void*);
static SpThreadDpiFn SpThreadDpi(){return (SpThreadDpiFn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetThreadDpiAwarenessContext");}
static void SpReadDpi(){SpGetDpiFn fn=(SpGetDpiFn)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");UINT dpi=fn?fn(g_spotify):96;g_spDpiScale=(double)(dpi?dpi:96)/96.0;}
struct SpLayout {int W,H,contentX,contentR,lyricsX,listTop,listBottom,rows,seekL,seekR;double scale;};
static SpLayout SpLayoutFor(int width,int height){
 SpLayout a={};a.scale=g_spDpiScale;if(width/a.scale<1000)a.scale=(double)width/1000;if(height/a.scale<680)a.scale=(double)height/680;if(a.scale<0.1)a.scale=0.1;
 a.W=(int)(width/a.scale);a.H=(int)(height/a.scale);a.contentX=182;a.lyricsX=a.W-Clamp(a.W/4,260,400)-18;a.contentR=a.lyricsX-18;a.listTop=346;a.listBottom=a.H-168;a.rows=MaxI(1,(a.listBottom-a.listTop)/52);a.seekL=294;a.seekR=a.W-260;return a;
}
static SpLayout SpGetLayout(){RECT r;GetClientRect(g_spotify,&r);SpReadDpi();SpLayout a=SpLayoutFor(MaxI(1,r.right),MaxI(1,r.bottom));g_spDpiScale=a.scale;return a;}
static int SpSeekAt(const SpLayout&a,int x){return (int)((long long)Clamp(x-a.seekL,0,a.seekR-a.seekL)*g_spotifyDuration/MaxI(1,a.seekR-a.seekL));}
static int SpRowAt(const SpLayout&a,int x,int y){if(g_spKaraoke||x<a.contentX+4||x>=a.contentR-16||y<a.listTop||y>=a.listTop+a.rows*52)return -1;int i=g_spotifyScroll+(y-a.listTop)/52;return i<g_spFilteredCount?i:-1;}
static void SpSetKaraoke(bool on){
 if(!g_spFull||on==g_spKaraoke)return;
 if(on){g_spKaraokeSavedScroll=g_spotifyScroll;g_spKaraokeSavedFollow=g_spLyricFollow;g_spKaraokeSavedLyricOffset=g_spLyricOffset;g_spKaraokeLyricSerial=(int)g_lyricsSerial;g_spLyricFollow=true;}
 else{g_spotifyScroll=g_spKaraokeSavedScroll;SpFilter();if(g_spKaraokeLyricSerial==(int)g_lyricsSerial){g_spLyricFollow=g_spKaraokeSavedFollow;g_spLyricOffset=g_spKaraokeSavedLyricOffset;}else{g_spLyricFollow=true;g_spLyricOffset=0;}}
 g_spKaraoke=on;g_spHoverRow=-1;ShowWindow(g_spSearchEdit,on?SW_HIDE:SW_SHOW);SetFocus(g_spotify);InvalidateRect(g_spotify,0,FALSE);
}
static void SpScrollKaraoke(int delta){if(g_spLyricFollow)g_spLyricOffset=MaxI(0,LyricsActiveLine());g_spLyricFollow=false;g_spLyricOffset=Clamp(g_spLyricOffset+delta,0,MaxI(0,g_lyricsLineCount-1));}
static HWND SpAux(int i){return i==0?g_eq:i==1?g_pl:i==2?g_lib:i==3?g_art:i==4?g_lyrics:g_viz;}
static bool* SpAuxFlag(int i){return i==0?&g_eqVisible:i==1?&g_plVisible:i==2?&g_libVisible:i==3?&g_artVisible:i==4?&g_lyricsVisible:&g_vizVisible;}
static void SpHideAux(bool* snapshot){for(int i=0;i<6;i++){HWND h=SpAux(i);snapshot[i]=h&&IsWindowVisible(h);if(h)ShowWindow(h,SW_HIDE);}}
static void SpShowAux(bool* snapshot){for(int i=0;i<6;i++){HWND h=SpAux(i);if(h)ShowWindow(h,snapshot[i]?SW_SHOW:SW_HIDE);}}
// Connection never changes window visibility or the saved normal-player layout.
static void SpCompactSession(){if(g_main)InvalidateRect(g_main,0,FALSE);}
static void SpRestoreLocalLayout(){}
static void SpRestorePanels(){
 SpExitFull();g_eqVisible=g_artVisible=g_lyricsVisible=true;g_artDismissed=g_lyricsDismissed=false;
 ShowWindow(g_main,SW_SHOW);ShowWindow(g_eq,SW_SHOW);ShowWindow(g_art,SW_SHOW);ShowWindow(g_lyrics,SW_SHOW);
 DockAllTools();SaveSettings();SetForegroundWindow(g_main);
}
static void SpPlaceFull(){
 MONITORINFO mi;mi.cbSize=sizeof(mi);HMONITOR mon=MonitorFromWindow(g_main,MONITOR_DEFAULTTONEAREST);
 RECT r={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};if(mon&&GetMonitorInfoW(mon,&mi))r=mi.rcWork;
 SetWindowRgn(g_spotify,0,TRUE);
 ShowWindow(g_spotify,SW_RESTORE);
 int width=MinI(r.right-r.left,1400),height=MinI(r.bottom-r.top,900);
 SetWindowPos(g_spotify,g_top?HWND_TOPMOST:(HWND)0,r.left+(r.right-r.left-width)/2,r.top+(r.bottom-r.top-height)/2,width,height,0);
 ShowWindow(g_spotify,3); // SW_MAXIMIZE: retains native caption and respects taskbar.
 SpReadDpi();SpWorkspaceLayout();
}
static void SpWorkspaceLayout(){
 if(!g_spFull||!g_spSearchEdit)return;SpLayout a=SpGetLayout();g_spPageRows=a.rows;
 ShowWindow(g_spSearchEdit,g_spKaraoke?SW_HIDE:SW_SHOW);
 MoveWindow(g_spSearchEdit,(int)((a.contentX+18)*a.scale),(int)(83*a.scale),(int)((a.contentR-a.contentX-166)*a.scale),(int)(28*a.scale),TRUE);
 HFONT oldSearch=g_spSearchFont;g_spSearchFont=CreateFontW(-(int)(16*a.scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");SendMessageW(g_spSearchEdit,WM_SETFONT,(WPARAM)g_spSearchFont,TRUE);if(oldSearch)DeleteObject(oldSearch);
 InvalidateRect(g_spotify,0,TRUE);
}
static void SpEnterFull(){
 if(!g_spotify||!g_spotifyConnected)return;if(g_spFull){SetForegroundWindow(g_spotify);return;}
 SpHideAux(g_spBeforeFull);g_spFull=true;g_spotifyVisible=true;
 ShowWindow(g_settings,SW_HIDE);ShowWindow(g_main,SW_HIDE);SpPlaceFull();ShowWindow(g_spSearchEdit,SW_SHOW);SetForegroundWindow(g_spotify);SetFocus(g_spotify);
 g_spLyricFollow=true;if(g_spotifyTrackId[0])RequestLyricsForSpotify();SpotifyPollNow();if(!g_spLastBrowse[g_spotifyTab]||GetTickCount64()-g_spLastBrowse[g_spotifyTab]>60000)SpotifyBrowseRefresh();
}
static void SpExitFull(){
 if(g_spKaraoke)SpSetKaraoke(false);
 if(!g_spFull)return;g_spFull=false;g_spotifyVisible=false;g_spSeekDrag=g_spotifyDragVolume=false;g_spSeekPreview=-1;ReleaseCapture();
 ShowWindow(g_spSearchEdit,SW_HIDE);ShowWindow(g_spotify,SW_HIDE);g_spPageRows=7;ShowWindow(g_main,SW_SHOW);SpShowAux(g_spBeforeFull);SetForegroundWindow(g_main);SpWriteProfile(L"ui",L"spotify",L"0",g_ini);
}
static void SpTab(int i){if(g_spKaraoke)SpSetKaraoke(false);g_spSavedScroll[g_spotifyTab]=g_spotifyScroll;g_spotifyTab=i;g_spotifyScroll=g_spSavedScroll[i];g_spHoverRow=-1;SpFilter();if(!g_spLastBrowse[i]||GetTickCount64()-g_spLastBrowse[i]>=60000)SpotifyBrowseRefresh();InvalidateRect(g_spotify,0,FALSE);}
static void SpWorkspaceClick(const SpLayout&a,int x,int y){
 if(PtIn(x,y,a.W-188,14,a.W-18,50)){SpExitFull();return;}
 if(PtIn(x,y,a.W-360,14,a.W-202,50)){SpRestorePanels();return;}
 for(int i=0;i<4;i++)if(PtIn(x,y,16,156+i*56,170,202+i*56)){SpTab(i);return;}
 if(PtIn(x,y,16,a.H-172,166,a.H-134)){SpotifyDisconnect();return;}
 if(PtIn(x,y,a.W-174,82,a.W-88,112)){SpSetKaraoke(!g_spKaraoke);return;}
 if(PtIn(x,y,a.W-80,82,a.W-32,112)){g_spLyricFollow=true;if(!g_spKaraoke||(!g_lyricsLoading&&!g_lyricsLineCount))RequestLyricsForSpotify();InvalidateRect(g_spotify,0,FALSE);return;}
 if(!g_spKaraoke){
 if(PtIn(x,y,a.contentR-132,78,a.contentR,116)){SpotifyBrowseRefresh();return;}
 if(PtIn(x,y,a.contentR-136,a.H-160,a.contentR,a.H-124)){SpotifyBrowseLoad(true);return;}
 if(PtIn(x,y,a.contentR-12,a.listTop,a.contentR,a.listBottom)){g_spotifyScroll=(int)((long long)Clamp(y-a.listTop,0,a.listBottom-a.listTop)*MaxI(0,g_spFilteredCount-a.rows)/MaxI(1,a.listBottom-a.listTop));InvalidateRect(g_spotify,0,FALSE);return;}
 int row=SpRowAt(a,x,y);if(row>=0){SpFilter();if(row>=g_spFilteredCount)return;SpotifyBrowseItem* items=SpotifyTabItems(g_spotifyTab);SpotifyBrowseItem& item=items[g_spFiltered[row]];SpotifySendUriCommand(g_spotifyTab==1?8:g_spotifyTab==3?9:7,g_spotifyTab==3?item.id:item.uri);return;}
 }
 int c=(a.seekL+a.seekR)/2;
 if(PtIn(x,y,c-98,a.H-106,c-46,a.H-62)){SpotifySendCommand(4);return;}
 if(PtIn(x,y,c-38,a.H-111,c+38,a.H-57)){SpotifySendCommand(g_spotifyPlaying?2:1);return;}
 if(PtIn(x,y,c+46,a.H-106,c+98,a.H-62)){SpotifySendCommand(3);return;}
 if(PtIn(x,y,a.W-224,a.H-56,a.W-24,a.H-20)){SpTab(3);return;}
}
// Rasterize primitives and fonts at physical-pixel size; never stretch a UI bitmap.
static int SpPx(int n){return (int)(n*g_spDpiScale+0.5);}
static HFONT SpFont(int height,int weight){return CreateFontW(-SpPx(height),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,g_fontName);}
static void SpFonts(){
 int dpi=(int)(g_spDpiScale*96);if(dpi==g_spFontDpi)return;g_spFontDpi=dpi;
 if(g_spBodyFont)DeleteObject(g_spBodyFont);if(g_spCaptionFont)DeleteObject(g_spCaptionFont);if(g_spStrongFont)DeleteObject(g_spStrongFont);
 if(g_spHeadingFont)DeleteObject(g_spHeadingFont);if(g_spHeroFont)DeleteObject(g_spHeroFont);
 if(g_spKaraokeFont)DeleteObject(g_spKaraokeFont);if(g_spKaraokeSmallFont)DeleteObject(g_spKaraokeSmallFont);
 g_spBodyFont=SpFont(16,FW_NORMAL);g_spCaptionFont=SpFont(13,FW_NORMAL);g_spStrongFont=SpFont(16,FW_BOLD);
 g_spHeadingFont=SpFont(19,FW_BOLD);g_spHeroFont=SpFont(25,FW_BOLD);
 g_spKaraokeFont=SpFont(36,FW_BOLD);g_spKaraokeSmallFont=SpFont(24,FW_NORMAL);
}
static void SpFill(HDC d,int x,int y,int r,int b,DWORD c){Fill(d,SpPx(x),SpPx(y),SpPx(r),SpPx(b),c);}
static void SpBox(HDC d,int x,int y,int r,int b,DWORD c,DWORD edge,int radius){Box(d,SpPx(x),SpPx(y),SpPx(r),SpPx(b),c,edge,SpPx(radius));}
static void SpText(HDC d,const wchar_t*t,int x,int y,int r,int b,DWORD c,HFONT f,UINT flags){Txt(d,t,SpPx(x),SpPx(y),SpPx(r),SpPx(b),c,f,flags);}
static void SpCover(HDC d,int x,int y,int w,int h){DrawCoverImage(d,SpPx(x),SpPx(y),SpPx(w),SpPx(h));}
static void SpButton(HDC d,int x,int y,int r,int b,const wchar_t*t,bool active,bool danger){
 SpBox(d,x,y,r,b,active?C_ACCENT:C_PANEL2,danger?C_RED:C_EDGE,4);
 SpText(d,t,x+6,y+1,r-6,b-1,active?C_BLACK:danger?C_RED:C_TEXT,g_spCaptionFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
}
struct SpKaraokeLayout {int top,bottom,rowHeight,visible,center;};
static SpKaraokeLayout SpKaraokeLayoutFor(const SpLayout&a){SpKaraokeLayout k={};k.top=210;k.bottom=a.H-182;int available=MaxI(1,k.bottom-k.top);k.visible=available>=420?5:3;k.rowHeight=MinI(110,available/k.visible);k.center=(k.top+k.bottom)/2;return k;}
static void DrawSpotifyKaraoke(HDC m,const SpLayout&a){
 int L=a.contentX,R=a.W-24;SpBox(m,L,78,R,a.H-124,C_PANEL,C_EDGE,12);
 SpText(m,L"LYRICS // KARAOKE",L+20,86,R-180,112,C_LED2,g_spStrongFont,DT_LEFT|DT_SINGLELINE);
 SpButton(m,a.W-174,82,a.W-88,112,L"KARAOKE",true,false);SpButton(m,a.W-80,82,a.W-32,112,L"SYNC",false,false);
 SpText(m,g_spotifyTitle,L+32,132,R-32,163,C_TEXT,g_spHeadingFont,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 SpText(m,g_spotifyArtist,L+32,172,R-32,197,C_MUTED,g_spBodyFont,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 SpKaraokeLayout k=SpKaraokeLayoutFor(a);bool synced=g_lyricsPreferSynced&&g_lyricsSynced[0];int active=synced?LyricsActiveLine():-1;
 if(g_lyricsLoading||!g_lyricsLineCount){SpText(m,g_lyricsLoading?L"Finding lyrics...":g_lyricsStatus,L+50,k.center-32,R-50,k.center+48,C_MUTED,g_spHeadingFont,DT_CENTER|DT_WORDBREAK);}
 else{
  int focus=g_spLyricFollow?MaxI(0,active):Clamp(g_spLyricOffset,0,g_lyricsLineCount-1);
  for(int slot=-k.visible/2;slot<=k.visible/2;slot++){int li=focus+slot;if(li<0||li>=g_lyricsLineCount)continue;int y=k.center+slot*k.rowHeight-k.rowHeight/2;bool current=li==active;bool centered=slot==0;
   if(current)SpBox(m,L+28,y+2,R-28,y+k.rowHeight-2,C_PANEL2,C_EDGE,8);
   HFONT font=centered?g_spKaraokeFont:g_spKaraokeSmallFont;int size=centered?36:24;
   if(WLen(g_lyricsLines[li].text)*size/2>(R-L-112)*2){font=g_spKaraokeSmallFont;size=24;}
   // Measure wrapped text at native DPI before vertical centering within the row.
   RECT measure={0,0,SpPx(R-L-112),0};HGDIOBJ old=SelectObject(m,font);DrawTextW(m,g_lyricsLines[li].text,-1,&measure,DT_WORDBREAK|DT_CALCRECT|DT_NOPREFIX);SelectObject(m,old);
   int th=MinI(k.rowHeight-12,(int)(measure.bottom/g_spDpiScale));int ty=y+(k.rowHeight-th)/2;
   SpText(m,g_lyricsLines[li].text,L+56,ty,R-56,y+k.rowHeight-6,current?C_LED2:centered?C_TEXT:C_MUTED,font,DT_CENTER|DT_WORDBREAK|DT_END_ELLIPSIS);
  }
 }
 const wchar_t*hint=!synced?L"Plain lyrics / scroll to browse / no timed highlighting":g_spLyricFollow?L"Following the song / scroll to browse / KARAOKE returns to library":L"Manual scroll / SYNC resumes following / KARAOKE returns to library";
 SpText(m,hint,L+20,a.H-160,R-20,a.H-134,C_MUTED,g_spCaptionFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
}
static void DrawSpotifyWorkspace(HWND h){
 PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT actual;GetClientRect(h,&actual);if(actual.right<=0||actual.bottom<=0){EndPaint(h,&ps);return;}SpLayout a=SpGetLayout();int W=a.W,H=a.H;
 HDC m=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,MaxI(1,actual.right),MaxI(1,actual.bottom));HGDIOBJ old=SelectObject(m,bitmap);
 const DWORD bg=C_BG,panel=C_PANEL,edge=C_EDGE,muted=C_MUTED,text=C_TEXT,accent=C_LED2,blue=C_ACCENT;
 SpFonts();
 SpFill(m,0,0,W,H,bg);SpFill(m,0,0,170,H-118,C_BG);SpFill(m,0,0,W,68,C_PANEL2);
 SpText(m,L"OZAMP",18,14,164,50,text,g_spHeadingFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 SpText(m,L"SPOTIFY // BROWSER",a.contentX,14,a.contentX+214,50,accent,g_spStrongFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 SpText(m,g_spotifyDevice[0]?g_spotifyDevice:L"Choose a Spotify device",a.contentX+228,14,W-380,50,muted,g_spBodyFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 SpButton(m,W-360,14,W-202,50,L"RESTORE PANELS",false,false);SpButton(m,W-188,14,W-18,50,L"NORMAL PLAYER / ESC",false,false);
 SpText(m,L"YOUR LIBRARY",24,104,174,131,muted,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 const wchar_t* tabs[]={L"RECENT",L"PLAYLISTS",L"QUEUE",L"DEVICES"};
 for(int i=0;i<4;i++){int y=156+i*56;bool active=i==g_spotifyTab;SpBox(m,16,y,170,y+46,active?C_PANEL2:C_BG,active?C_ACCENT:C_BG,9);if(active)SpFill(m,18,y+12,21,y+34,accent);SpText(m,tabs[i],34,y+2,162,y+44,active?text:muted,g_spStrongFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);}
 SpText(m,L"CONNECTED",24,H-234,164,H-210,accent,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);SpText(m,L"F11 switches view",24,H-208,164,H-184,muted,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);SpButton(m,16,H-172,166,H-134,L"DISCONNECT",false,false);
 if(g_spKaraoke)DrawSpotifyKaraoke(m,a);else{
 SpBox(m,a.contentX,78,a.contentR-148,116,C_PANEL2,edge,9);SpButton(m,a.contentR-132,78,a.contentR,116,g_spotifyBrowseBusy?L"LOADING...":L"REFRESH",false,false);
 SpBox(m,a.contentX,132,a.contentR,302,panel,edge,12);
 if(g_coverImage)SpCover(m,a.contentX+12,144,146,146);else{SpBox(m,a.contentX+12,144,a.contentX+158,290,C_PANEL2,edge,9);SpText(m,L"OZ",a.contentX+20,176,a.contentX+148,248,accent,g_spHeroFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 int tx=a.contentX+178;SpText(m,g_spotifyHasPlayback?(g_spotifyPlaying?L"NOW PLAYING":L"PAUSED"):L"LAST TRACK / NO ACTIVE DEVICE",tx,148,a.contentR-16,171,accent,g_spCaptionFont,DT_LEFT|DT_SINGLELINE);
 SpText(m,g_spotifyTitle[0]?g_spotifyTitle:L"Your music starts here",tx,181,a.contentR-20,221,text,g_spHeroFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 SpText(m,g_spotifyArtist,tx,227,a.contentR-20,251,text,g_spBodyFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);SpText(m,g_spotifyAlbum,tx,257,a.contentR-20,281,muted,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 SpText(m,tabs[g_spotifyTab],a.contentX,308,a.contentR-110,340,text,g_spHeadingFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);SpFilter();wchar_t count[48],total[24];IntToW(g_spFilteredCount,count);IntToW(SpotifyTabCount(g_spotifyTab),total);WCat(count,L" / ",48);WCat(count,total,48);SpText(m,count,a.contentR-105,308,a.contentR,340,muted,g_spCaptionFont,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 SpotifyBrowseItem* items=SpotifyTabItems(g_spotifyTab);g_spPageRows=a.rows;
 for(int row=0;row<a.rows;row++){int view=g_spotifyScroll+row;if(view>=g_spFilteredCount)break;SpotifyBrowseItem& it=items[g_spFiltered[view]];int y=a.listTop+row*52;bool hot=view==g_spHoverRow;
  SpBox(m,a.contentX,y,a.contentR-16,y+48,hot?C_PANEL2:panel,hot?C_ACCENT:edge,4);wchar_t number[16];IntToW(view+1,number);SpText(m,number,a.contentX+10,y+5,a.contentX+43,y+41,muted,g_spCaptionFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  SpText(m,it.title,a.contentX+52,y+5,a.contentR-92,y+26,it.active?accent:text,g_spStrongFont,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);SpText(m,it.subtitle,a.contentX+52,y+27,a.contentR-92,y+44,muted,g_spCaptionFont,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);SpText(m,g_spotifyTab==3?(it.active?L"ACTIVE":L"USE"):L"PLAY",a.contentR-85,y+5,a.contentR-26,y+42,accent,g_spCaptionFont,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 }
 if(!g_spFilteredCount)SpText(m,g_spotifyBrowseBusy?L"Loading your library...":g_spSearch[0]?L"No matches in loaded items":L"No items to show",a.contentX+10,a.listTop+20,a.contentR-20,a.listBottom-10,muted,g_spBodyFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 if(g_spFilteredCount>a.rows){int track=a.listBottom-a.listTop,thumb=MaxI(22,track*a.rows/g_spFilteredCount);int y=a.listTop+(track-thumb)*g_spotifyScroll/MaxI(1,g_spFilteredCount-a.rows);SpBox(m,a.contentR-7,a.listTop,a.contentR-3,a.listBottom,edge,edge,2);SpBox(m,a.contentR-8,y,a.contentR-2,y+thumb,muted,muted,3);}
 SpText(m,g_spStorageStatus[0]?g_spStorageStatus:g_spBrowseStatus[g_spotifyTab],a.contentX,H-154,a.contentR-144,H-126,muted,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);SpButton(m,a.contentR-136,H-160,a.contentR,H-124,g_spNext[g_spotifyTab][0]?L"LOAD MORE":L"ALL LOADED",false,false);
 // Lyrics share the existing synchronized data and request lifecycle.
 SpBox(m,a.lyricsX,78,W-24,H-124,panel,edge,12);SpText(m,L"LYRICS",a.lyricsX+14,86,W-184,111,text,g_spStrongFont,DT_LEFT|DT_SINGLELINE);SpButton(m,W-174,82,W-88,112,L"KARAOKE",false,false);SpButton(m,W-80,82,W-32,112,L"SYNC",false,false);
 int active=LyricsActiveLine(),available=MaxI(1,(H-300)/58);int first=g_spLyricFollow?MaxI(0,active-2):g_spLyricOffset;
 if(g_lyricsLoading)SpText(m,L"Finding lyrics...",a.lyricsX+18,152,W-42,216,muted,g_spBodyFont,DT_CENTER|DT_WORDBREAK);
 else if(!g_lyricsLineCount)SpText(m,g_lyricsStatus,a.lyricsX+18,152,W-42,260,muted,g_spBodyFont,DT_CENTER|DT_WORDBREAK);
 else for(int row=0;row<available;row++){int li=first+row;if(li>=g_lyricsLineCount)break;int y=134+row*58;bool on=li==active;if(on)SpBox(m,a.lyricsX+10,y-4,W-34,y+50,C_PANEL2,C_PANEL2,8);SpText(m,g_lyricsLines[li].text,a.lyricsX+22,y,W-46,y+48,on?accent:text,on?g_spStrongFont:g_spBodyFont,DT_LEFT|DT_WORDBREAK|DT_END_ELLIPSIS);}
 SpText(m,g_spLyricFollow?L"Following the song  /  scroll to browse":L"Manual scroll  /  click SYNC to follow",a.lyricsX+16,H-159,W-36,H-134,muted,g_spCaptionFont,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
 // One transport bar, independent of the window mode.
 SpFill(m,0,H-118,W,H,C_PANEL2);SpFill(m,0,H-118,W,H-117,edge);if(g_coverImage)SpCover(m,24,H-96,64,64);
 SpText(m,g_spotifyTitle,104,H-96,274,H-71,text,g_spStrongFont,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);SpText(m,g_spotifyArtist,104,H-66,274,H-43,muted,g_spCaptionFont,DT_LEFT|DT_SINGLELINE|DT_END_ELLIPSIS);
 int c=(a.seekL+a.seekR)/2;SpButton(m,c-98,H-106,c-46,H-62,L"|<",false,false);SpButton(m,c-38,H-111,c+38,H-57,g_spotifyPlaying?L"PAUSE":L"PLAY",g_spotifyPlaying,false);SpButton(m,c+46,H-106,c+98,H-62,L">|",false,false);
 int pos=g_spSeekPreview>=0?g_spSeekPreview:g_spotifyProgress;wchar_t elapsed[24],duration[24];FormatTime(pos,elapsed);FormatTime(g_spotifyDuration,duration);SpText(m,elapsed,a.seekL-54,H-43,a.seekL-8,H-13,muted,g_spCaptionFont,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);SpText(m,duration,a.seekR+8,H-43,a.seekR+58,H-13,muted,g_spCaptionFont,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 SpBox(m,a.seekL,H-30,a.seekR,H-24,edge,edge,3);int played=a.seekL+(int)((long long)(a.seekR-a.seekL)*Clamp(pos,0,MaxI(1,g_spotifyDuration))/MaxI(1,g_spotifyDuration));if(played>a.seekL)SpBox(m,a.seekL,H-30,played,H-24,accent,accent,3);
 SpText(m,L"VOLUME",W-206,H-105,W-36,H-82,muted,g_spCaptionFont,DT_LEFT|DT_SINGLELINE);SpBox(m,W-206,H-77,W-36,H-71,edge,edge,3);int vol=W-206+170*g_spotifyVolume/100;if(vol>W-206)SpBox(m,W-206,H-77,vol,H-71,g_spotifySupportsVolume?blue:muted,g_spotifySupportsVolume?blue:muted,3);
 SpText(m,g_spotifyDevice[0]?g_spotifyDevice:L"Choose device",W-190,H-53,W-24,H-24,accent,g_spCaptionFont,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 BitBlt(dc,0,0,actual.right,actual.bottom,m,0,0,SRCCOPY);SelectObject(m,old);DeleteObject(bitmap);DeleteDC(m);EndPaint(h,&ps);
}
