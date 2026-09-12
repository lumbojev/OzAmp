#!/usr/bin/env python3
"""Exercise production layout and pointer mapping at common monitor dimensions."""
from pathlib import Path
import tempfile, subprocess
s=(Path(__file__).resolve().parents[1]/'spotify_workspace.h').read_text()
layout=s[s.index('struct SpLayout'):s.index('static SpLayout SpGetLayout')]
seek=s[s.index('static int SpSeekAt'):s.index('static void SpSetKaraoke')]
karaoke=s[s.index('struct SpKaraokeLayout'):s.index('static void DrawSpotifyKaraoke')]
code='''#include <cassert>
#include <cstdio>
#include <initializer_list>
int MaxI(int a,int b){return a>b?a:b;}
int MinI(int a,int b){return a<b?a:b;}
int Clamp(int x,int a,int b){return x<a?a:x>b?b:x;}
int g_spotifyDuration=240000,g_spotifyScroll=0,g_spFilteredCount=1000;
double g_spDpiScale=1.0;
bool g_spKaraoke=false;
'''+layout+seek+karaoke+'''
int main(){
 int sizes[][2]={{1280,720},{1366,768},{1920,1080},{2560,1440},{3840,2160},{3440,1440},{1024,768},{1080,1920}};
 for(double dpi : {1.0,1.25,1.5,2.0}) for(auto& size:sizes){g_spDpiScale=dpi;auto a=SpLayoutFor(size[0],size[1]);
 assert(a.contentR-a.contentX>=460);assert(a.listTop+a.rows*52<=a.listBottom);
 assert(a.listBottom<a.H-124);assert(a.seekR>a.seekL+400);
 assert(SpSeekAt(a,a.seekL-100)==0);assert(SpSeekAt(a,a.seekR+100)==240000);
 assert(SpRowAt(a,a.contentX+20,a.listTop)==0);
 g_spKaraoke=true;assert(SpRowAt(a,a.contentX+20,a.listTop)==-1);g_spKaraoke=false;
 auto k=SpKaraokeLayoutFor(a);assert(k.visible==3||k.visible==5);
 assert(k.center-k.rowHeight*(k.visible/2)-k.rowHeight/2>=k.top);
 assert(k.center+k.rowHeight*(k.visible/2)+k.rowHeight/2<=k.bottom);
 assert(k.bottom<a.H-160);
 assert(SpRowAt(a,a.contentX+20,a.listTop+a.rows*52)==-1);
 g_spotifyScroll=997;assert(SpRowAt(a,a.contentX+20,a.listTop+2*52)==999);
 assert(SpRowAt(a,a.contentX+20,a.listTop+3*52)==-1);g_spotifyScroll=0;
 printf("PASS %dx%d DPI %.2f: %d rows, scale %.3f\\n",size[0],size[1],dpi,a.rows,a.scale);
 }
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code);exe=Path(d)/'test'
 subprocess.run(['g++','-std=c++17','-fsanitize=undefined','-o',str(exe),str(p)],check=True)
 subprocess.run([str(exe)],check=True)
