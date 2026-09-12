#!/usr/bin/env python3
"""Compile production browser functions against API fixtures; no live Spotify account needed."""
import pathlib,re,subprocess,tempfile,os
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'main.cpp').read_text()
def function(name):
    m=re.search(r'^static [^\n]*?\b'+name+r'\([^;\n]*\)\s*\{',s,re.M)
    if not m:raise RuntimeError('Missing function '+name)
    start=m.start();i=s.index('{',m.start());depth=0;quote=None
    while i<len(s):
        c=s[i]
        if quote:
            if c=='\\':i+=2;continue
            if c==quote:quote=None
        elif c in '\"\'':quote=c
        elif s[i:i+2]=='//':i=s.index('\n',i);continue
        elif s[i:i+2]=='/*':i=s.index('*/',i)+2;continue
        elif c=='{':depth+=1
        elif c=='}':
            depth-=1
            if depth==0:return s[start:i+1]+'\n'
        i+=1
    raise RuntimeError(name)
code=r'''
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <iostream>
using LONG=int;using DWORD=unsigned;using ULONGLONG=unsigned long long;
using LPVOID=void*;using LPARAM=long long;using UINT_PTR=unsigned long long;
#define WINAPI
#define HEAP_ZERO_MEMORY 1
#define FALSE 0
void* GetProcessHeap(){return nullptr;}
void* HeapAlloc(void*,unsigned,size_t n){return calloc(1,n);}
void HeapFree(void*,unsigned,void* p){free(p);}
void InvalidateRect(void*,void*,int){}
LONG AtomicExchange(volatile LONG* p,LONG n){LONG old=*p;*p=n;return old;}
LONG generation=1;
LONG SpGeneration(){return generation;}
void* g_main=nullptr;void* g_spotify=nullptr;
int g_spotifyTab=0,g_spotifyScroll=0,g_spRecentCount=0,g_spPlaylistCount=0,g_spQueueCount=0,g_spDeviceCount=0;
bool g_spotifyConnected=true,g_spotifyBrowseBusy=false;
volatile LONG g_spotifyBrowseSerial=0;
wchar_t g_spSearch[160]={},g_spBrowseStatus[4][192]={},g_spNext[4][1024]={};
int g_spFiltered[1000],g_spFilteredCount,g_spPageRows=7;
constexpr int WM_OZSPOTIFYBROWSE=1;
'''
for name in ['SpotifyBrowseItem','SpotifyBrowseResult','SpBrowseJob']:
    code+=re.search(r'^struct '+name+r' \{[^\n]+',s,re.M).group()+'\n'
code+='static SpotifyBrowseItem g_spRecent[1000],g_spPlaylists[1000],g_spQueue[1000],g_spDevices[1000];\n'
for name in ['WLen','WCopy','WCat','WEqI','Clamp','MaxI','MinI','LowerW','WContainsI','HexNib','JsonTopValue','JsonValueStringPtr','JsonValueIntPtr','JsonValueBoolPtr','JsonFirstArrayObject','JsonArrayStart','JsonNextArrayObject','SpotifyTrackFromObject','SpotifyTabCount','SpotifyTabItems','SpFilter','SpBrowseError','SpNextPath','SpParseBrowse','ApplySpotifyBrowse']:
    code+=function(name)
code+=r'''
std::wstring responses[4];bool responseOK[4];DWORD codes[4];int requests;
SpotifyBrowseResult* delivered;
bool SpotifyApi(const wchar_t*,const wchar_t* path,wchar_t* out,int cap,DWORD* status,LONG expected){assert(expected==generation);assert(path[0]==L'/');int i=requests++;assert(i<4);WCopy(out,responses[i].c_str(),cap);*status=codes[i];return responseOK[i];}
bool PostMessageW(void*,int,int,LPARAM p){delivered=(SpotifyBrowseResult*)p;return true;}
'''
code+=function('SpotifyBrowseThread')
code+=r'''
SpotifyBrowseResult* result(int tab=1,bool append=false){auto* r=(SpotifyBrowseResult*)calloc(1,sizeof(SpotifyBrowseResult));r->tab=tab;r->generation=generation;r->append=append;return r;}
void reset(){g_spRecentCount=g_spPlaylistCount=g_spQueueCount=g_spDeviceCount=0;g_spotifyTab=1;g_spotifyScroll=0;g_spSearch[0]=0;g_spotifyConnected=true;requests=0;delivered=nullptr;memset(g_spNext,0,sizeof(g_spNext));}
int main(){
 reset();wchar_t next[1024];
 assert(SpNextPath(LR"({"next":"https://api.spotify.com/v1/me/playlists?offset=20&limit=20"})",next));assert(!wcscmp(next,L"/v1/me/playlists?offset=20&limit=20"));
 assert(!SpNextPath(LR"({"next":"https://api.spotify.com.evil/v1/x"})",next));assert(!SpNextPath(LR"({"next":null})",next));assert(!SpNextPath(LR"({"next":""})",next));
 auto* r=result();SpParseBrowse(LR"({"items":[{"id":"a","name":"Rock","uri":"spotify:playlist:a","owner":{"display_name":"Oskar"}},{"id":"b","name":"Pop","uri":"spotify:playlist:b"}],"next":"https://api.spotify.com/v1/me/playlists?offset=2"})",r);
 assert(r->ok&&r->count==2);ApplySpotifyBrowse(r);assert(g_spPlaylistCount==2&&g_spNext[1][0]);
 // successful empty result replaces old contents
 r=result();SpParseBrowse(LR"({"items":[],"next":null})",r);ApplySpotifyBrowse(r);assert(g_spPlaylistCount==0&&!g_spNext[1][0]);
 // API error object must not be treated as a successful empty list
 g_spPlaylistCount=1;WCopy(g_spPlaylists[0].title,L"Keep me",220);
 r=result();SpParseBrowse(LR"({"error":{"status":403,"message":"Forbidden"}})",r);assert(!r->ok);ApplySpotifyBrowse(r);assert(g_spPlaylistCount==1&&!wcscmp(g_spPlaylists[0].title,L"Keep me"));
 // discard old session responses
 r=result();r->generation=0;r->ok=true;ApplySpotifyBrowse(r);assert(g_spPlaylistCount==1);
 // dedup appended pages by playlist id
 WCopy(g_spPlaylists[0].id,L"a",128);r=result(1,true);SpParseBrowse(LR"({"items":[{"id":"a","name":"Rock"},{"id":"b","name":"Pop"}],"next":null})",r);ApplySpotifyBrowse(r);assert(g_spPlaylistCount==2);
 // filtering returns original indices, including second row playback target
 WCopy(g_spSearch,L"pop",160);SpFilter();assert(g_spFilteredCount==1&&g_spFiltered[0]==1);
 // recent plays of one song at different timestamps are not duplicates
 r=result(0);SpParseBrowse(LR"({"items":[{"played_at":"2026-09-12T10:00:00Z","track":{"id":"t","name":"Song","artists":[{"name":"Band"}]}},{"played_at":"2026-09-12T11:00:00Z","track":{"id":"t","name":"Song"}}],"next":null})",r);assert(r->count==2);ApplySpotifyBrowse(r);assert(g_spRecentCount==2);
 r=result(0,true);SpParseBrowse(LR"({"items":[{"played_at":"2026-09-12T10:00:00Z","track":{"id":"t","name":"Song"}}],"next":null})",r);ApplySpotifyBrowse(r);assert(g_spRecentCount==2);
 // queue and device fixtures
 r=result(2);SpParseBrowse(LR"({"queue":[{"id":"q","name":"Queued"}]})",r);assert(r->ok&&r->count==1);ApplySpotifyBrowse(r);
 r=result(3);SpParseBrowse(LR"({"devices":[{"id":"pc","name":"Desktop","type":"Computer","is_active":true,"supports_volume":false,"volume_percent":45}]})",r);assert(r->ok&&r->items[0].active&&!r->items[0].supportsVolume&&r->items[0].volume==45);ApplySpotifyBrowse(r);
 // refresh traverses enough pages to retain the loaded range, rather than dropping to 20
 reset();responses[0]=LR"({"items":[{"id":"a","name":"A"}],"next":"https://api.spotify.com/v1/me/playlists?offset=1"})";responses[1]=LR"({"items":[{"id":"b","name":"B"}],"next":null})";responseOK[0]=responseOK[1]=true;codes[0]=codes[1]=200;
 auto* j=(SpBrowseJob*)calloc(1,sizeof(SpBrowseJob));j->generation=1;j->tab=1;j->targetCount=2;WCopy(j->path,L"/v1/me/playlists?limit=20",1024);SpotifyBrowseThread(j);assert(requests==2&&delivered->ok&&delivered->count==2);ApplySpotifyBrowse(delivered);assert(g_spPlaylistCount==2);
 // later page failure retains the entire previous cache
 requests=0;responseOK[1]=false;codes[1]=403;j=(SpBrowseJob*)calloc(1,sizeof(SpBrowseJob));j->generation=1;j->tab=1;j->targetCount=2;WCopy(j->path,L"/v1/me/playlists?limit=20",1024);SpotifyBrowseThread(j);assert(!delivered->ok);ApplySpotifyBrowse(delivered);assert(g_spPlaylistCount==2);
 assert(wcsstr(SpBrowseError(403),L"ACCESS DENIED"));assert(wcsstr(SpBrowseError(429),L"RATE LIMIT"));assert(wcsstr(SpBrowseError(0),L"NETWORK"));
 std::cout<<"PASS: pagination, URL validation, empty/error distinction, cache retention, stale sessions, deduplication, search mapping, recent/queue/devices, multi-page refresh, HTTP errors\n";
}
'''
with tempfile.TemporaryDirectory(prefix='ozamp-tests-') as tmp:
    f=pathlib.Path(tmp)/'test.cpp';f.write_text(code)
    exe=pathlib.Path(tmp)/'test'
    subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined',str(f),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,env={**os.environ,'ASAN_OPTIONS':os.environ.get('ASAN_OPTIONS','detect_leaks=0')})
