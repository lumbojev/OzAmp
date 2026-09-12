// Offline integration test of the production DPAPI/file code. No Spotify credentials.
// The application entry point is not executed. All files live in a temporary folder.
#include "../main.cpp"
extern "C" void SpotifyStorageTests(){
 int failure=0;wchar_t folder[MAXP],stamp[32],path[MAXP];
 if(!GetEnvironmentVariableW(L"TEMP",folder,MAXP))ExitProcess(1);
 WCat(folder,L"\\OzAmp-storage-test-",MAXP);U64ToW(GetTickCount64(),stamp);WCat(folder,stamp,MAXP);
 if(!CreateDirectoryW(folder,0))ExitProcess(2);
 WCopy(g_dataDir,folder,MAXP);WCopy(g_ini,folder,MAXP);WCat(g_ini,L"\\ozamp.ini",MAXP);
 WCopy(g_spotifyClientId,L"offline-test-client-1234567890",160);
 WCopy(g_spotifyRefresh,L"test-refresh-token-one-not-a-real-credential",2048);
 if(!SpSaveSession())failure=3;
 SpWipe(g_spotifyRefresh,sizeof(g_spotifyRefresh));
 if(!failure&&(!SpLoadSession()||!WEqI(g_spotifyRefresh,L"test-refresh-token-one-not-a-real-credential")))failure=4;
 // DPAPI must reject different client binding.
 WCopy(g_spotifyClientId,L"different-client",160);SpWipe(g_spotifyRefresh,sizeof(g_spotifyRefresh));
 if(!failure&&SpLoadSession())failure=5;
 WCopy(g_spotifyClientId,L"offline-test-client-1234567890",160);
 SpTokenPath(path);
 // A locked destination makes atomic replacement fail; the previous session survives.
 HANDLE locked=CreateFileW(path,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
 if(locked==INVALID_HANDLE_VALUE)failure=6;
 else {WCopy(g_spotifyRefresh,L"test-refresh-token-two",2048);if(!failure&&SpSaveSession())failure=7;CloseHandle(locked);}
 SpWipe(g_spotifyRefresh,sizeof(g_spotifyRefresh));
 if(!failure&&(!SpLoadSession()||!WEqI(g_spotifyRefresh,L"test-refresh-token-one-not-a-real-credential")))failure=8;
 // The next successful rotation replaces it.
 WCopy(g_spotifyRefresh,L"test-refresh-token-two",2048);if(!failure&&!SpSaveSession())failure=9;
 SpWipe(g_spotifyRefresh,sizeof(g_spotifyRefresh));if(!failure&&(!SpLoadSession()||!WEqI(g_spotifyRefresh,L"test-refresh-token-two")))failure=10;
 if(!SpDeleteSession()&&!failure)failure=11;
 if(!failure&&SpLoadSession())failure=12;
 // Empty/corrupted ciphertext is rejected.
 HANDLE bad=CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
 if(bad!=INVALID_HANDLE_VALUE){DWORD n=0;const char bytes[]="invalid";WriteFile(bad,bytes,sizeof(bytes),&n,0);CloseHandle(bad);if(!failure&&SpLoadSession())failure=13;}
 SpDeleteSession();DeleteFileW(g_ini);SpWipe(g_spotifyRefresh,sizeof(g_spotifyRefresh));
 // No actual login, player, network, or user configuration was used.
 ExitProcess((UINT)failure);
}
