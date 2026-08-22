#pragma once
#include "winlite.h"

struct OzAudioDevice { wchar_t id[256]; wchar_t name[192]; };

bool OzAudioInit(const wchar_t* deviceId);
void OzAudioShutdown();
int  OzAudioEnumerate(OzAudioDevice* out, int cap);
bool OzAudioLoad(const wchar_t* path);
void OzAudioUnload();
void OzAudioPlay();
void OzAudioPause();
void OzAudioStop();
void OzAudioSeekMs(int ms);
void OzAudioPump();
int  OzAudioPosMs();
int  OzAudioLengthMs();
bool OzAudioPlaying();
bool OzAudioPaused();
bool OzAudioNative();
const wchar_t* OzAudioBackend();
const wchar_t* OzAudioCurrentDeviceName();
const wchar_t* OzAudioCurrentDeviceId();
int  OzAudioOutputRate();
int  OzAudioOutputChannels();
bool OzAudioReady();
HRESULT OzAudioLastError();
const wchar_t* OzAudioLastErrorStage();
void OzAudioSetVolume(int percent, int balance, bool muted);
void OzAudioSetEQ(bool enabled, int preampDb, const int bandsDb[10]);
void OzAudioSetReplayGainDb(double db);
bool OzAudioPrepareNext(const wchar_t* path,double replayGainDb);
void OzAudioClearNext();
void OzAudioSetCrossfadeMs(int ms);
bool OzAudioConsumeAdvanced();
double OzAudioScanReplayGain(const wchar_t* path, double* peakOut);
void OzAudioSpectrum(float* out, int bins);
void OzAudioWaveform(float* out, int count);
