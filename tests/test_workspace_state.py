#!/usr/bin/env python3
"""Run production visibility transitions with deterministic Win32 window stubs."""
from pathlib import Path
import subprocess,tempfile,re
s=(Path(__file__).resolve().parents[1]/'spotify_workspace.h').read_text()
def function(name):
    m=re.search(r'^static [^\n]*?\b'+name+r'\([^;\n]*\)\s*\{',s,re.M)
    assert m,name
    start=m.start();a=s.index('{',start);depth=1;b=a+1
    while depth:
        depth+=(s[b]=='{')-(s[b]=='}');b+=1
    return s[start:b]+'\n'
code=r'''
#include <cassert>
#include <cstdio>
using HWND=int;using HFONT=int;
#define SW_HIDE 0
#define SW_SHOW 1
#define FALSE 0
int g_main=1,g_eq=2,g_pl=3,g_lib=4,g_art=5,g_lyrics=6,g_viz=7,g_spotify=8,g_spSearchEdit=9,g_settings=10;
bool windows[11]={},g_eqVisible=true,g_plVisible=false,g_libVisible=false,g_artVisible=true,g_lyricsVisible=true,g_vizVisible=false;
bool g_spBeforeFull[6]={},g_spFull=false,g_spotifyVisible=false,g_spSeekDrag=false,g_spotifyDragVolume=false;
bool g_spotifyConnected=true,g_spLyricFollow=false;int g_spSeekPreview=-1,g_spPageRows=7,g_spotifyTab=0;
bool g_spKaraoke=false,g_spKaraokeSavedFollow=true;
int g_spKaraokeSavedScroll=0,g_spKaraokeSavedLyricOffset=0,g_spKaraokeLyricSerial=0;
int g_spotifyScroll=17,g_spLyricOffset=3,g_lyricsSerial=1,g_spHoverRow=-1;
void SpFilter(){}
wchar_t g_spotifyTrackId[2]={0},g_ini[1]={0};unsigned long long g_spLastBrowse[4]={0};
int writes=0,polls=0;
bool IsWindowVisible(HWND h){return windows[h];}
void ShowWindow(HWND h,int state){windows[h]=state;}
void InvalidateRect(HWND,void*,int){}
void SetForegroundWindow(HWND){}
void SetFocus(HWND){}
void ReleaseCapture(){}
void SpWriteProfile(const wchar_t*,const wchar_t*,const wchar_t*,const wchar_t*){writes++;}
void SpPlaceFull(){ShowWindow(g_spotify,SW_SHOW);}
void RequestLyricsForSpotify(){}
void SpotifyPollNow(){polls++;}
void SpotifyBrowseRefresh(){}
unsigned long long GetTickCount64(){return 100;}
'''
for f in ['SpSetKaraoke','SpAux','SpHideAux','SpShowAux','SpCompactSession','SpEnterFull','SpExitFull']:
    code+=function(f)
code+=r'''
int main(){
 windows[g_main]=windows[g_eq]=windows[g_art]=windows[g_lyrics]=true;
 for(int n=0;n<3;n++)SpCompactSession();
 assert(windows[g_main]&&windows[g_eq]&&windows[g_art]&&windows[g_lyrics]);assert(writes==0);
 for(int n=0;n<10;n++){
  SpEnterFull();assert(g_spFull&&windows[g_spotify]);assert(!windows[g_main]&&!windows[g_eq]&&!windows[g_art]&&!windows[g_lyrics]);
  assert(g_eqVisible&&g_artVisible&&g_lyricsVisible); // persisted preferences must not change
  g_spLyricFollow=false;g_spLyricOffset=3;g_spotifyScroll=17;int p=polls;
  SpSetKaraoke(true);assert(g_spKaraoke&&!windows[g_spSearchEdit]&&g_spLyricFollow);
  SpSetKaraoke(true); // idempotent: do not overwrite the saved layout
  g_spLyricOffset=8;SpSetKaraoke(false);
  assert(!g_spKaraoke&&windows[g_spSearchEdit]&&!g_spLyricFollow&&g_spLyricOffset==3&&g_spotifyScroll==17&&polls==p);
  SpSetKaraoke(true);g_lyricsSerial++;SpSetKaraoke(false);
  assert(g_spLyricFollow&&g_spLyricOffset==0); // old lyrics position must not leak into new track
  SpCompactSession();assert(g_spFull&&windows[g_spotify]);
  SpExitFull();assert(!g_spFull&&!windows[g_spotify]);
  assert(windows[g_main]&&windows[g_eq]&&windows[g_art]&&windows[g_lyrics]);assert(!windows[g_pl]);
 }
 puts("PASS: connection preserves windows/preferences; 10 large/normal transitions restore exact visibility");
 SpSetKaraoke(true);assert(!g_spKaraoke); // no change outside Spotify workspace
 puts("PASS: karaoke round trips, search visibility, scroll restoration, new-track guards and no extra polling");
}
'''
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'state.cpp';p.write_text(code);exe=Path(d)/'state'
    subprocess.run(['g++','-std=c++17','-fsanitize=undefined',str(p),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
# Native caption/default non-client processing and final-pixel rasterization guards.
main=(Path(__file__).resolve().parents[1]/'main.cpp').read_text()
assert 'WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN' in main
assert 'case WM_NCHITTEST:return DefWindowProcW(h,m,w,l);' in main
assert 'StretchBlt' not in s
assert 'CreateCompatibleBitmap(dc,MaxI(1,actual.right),MaxI(1,actual.bottom))' in s
print('PASS: native window frame and no stretched UI bitmap')
