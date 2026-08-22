#include "audio_engine.h"

extern "C" void* memset(void*,int,SIZE_T);
extern "C" void* memcpy(void*,const void*,SIZE_T);
extern "C" int memcmp(const void*,const void*,SIZE_T);

static const GUID CLSID_MMDeviceEnumerator={0xBCDE0395,0xE52F,0x467C,{0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E}};
static const GUID IID_IMMDeviceEnumerator={0xA95664D2,0x9614,0x4F35,{0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6}};
static const GUID IID_IAudioClient={0x1CB9AD4C,0xDBFA,0x4C32,{0xB1,0x78,0xC2,0xF5,0x68,0xA7,0x03,0xB2}};
// Windows SDK: F294ACFC-3146-4483-A7BF-ADDCA7C260E2.
// NOTE: 3.2.0 accidentally used 0x0E as the final byte, which makes
// IAudioClient::GetService(IAudioRenderClient) fail on every endpoint.
static const GUID IID_IAudioRenderClient={0xF294ACFC,0x3146,0x4483,{0xA7,0xBF,0xAD,0xDC,0xA7,0xC2,0x60,0xE2}};
static const PROPERTYKEY PKEY_Device_FriendlyName={{0xA45C254E,0xDF1C,0x4EFD,{0x80,0x20,0x67,0xD1,0x46,0xA8,0x50,0xE0}},14};
// Media Foundation GUIDs used for codec-backed PCM decoding.
static const GUID G_MF_MT_MAJOR_TYPE={0x48eba18e,0xf8c9,0x4687,{0xbf,0x11,0x0a,0x74,0xc9,0xf9,0x6a,0x8f}};
static const GUID G_MF_MT_SUBTYPE={0xf7e34c9a,0x42e8,0x4714,{0xb7,0x4b,0xcb,0x29,0xd7,0x2c,0x35,0xe5}};
static const GUID G_MF_MT_AUDIO_NUM_CHANNELS={0x37e48bf5,0x645e,0x4c5b,{0x89,0xde,0xad,0xa9,0xe2,0x9b,0x69,0x6a}};
static const GUID G_MF_MT_AUDIO_SAMPLES_PER_SECOND={0x5faeeae7,0x0290,0x4c31,{0x9e,0x8a,0xc5,0x34,0xf6,0x8d,0x9d,0xba}};
static const GUID G_MF_MT_AUDIO_BITS_PER_SAMPLE={0xf2deb57f,0x40fa,0x4764,{0xaa,0x33,0xed,0x4f,0x2d,0x1f,0xf6,0x69}};
static const GUID G_MF_MT_AUDIO_AVG_BYTES_PER_SECOND={0x1aab75c8,0xcfef,0x451c,{0xab,0x95,0xac,0x03,0x4b,0x8e,0x17,0x31}};
static const GUID G_MF_MT_AUDIO_BLOCK_ALIGNMENT={0x322de230,0x9eeb,0x43bd,{0xab,0x7a,0xff,0x41,0x22,0x51,0x54,0x1d}};
static const GUID G_MFMediaType_Audio={0x73647561,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
static const GUID G_MFAudioFormat_Float={0x00000003,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
static const GUID G_KSDATAFORMAT_SUBTYPE_PCM={0x00000001,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
static const GUID G_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT={0x00000003,0x0000,0x0010,{0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71}};
static const GUID G_MF_PD_DURATION={0x6c990d33,0xbb8e,0x477a,{0x85,0x98,0x0d,0x5d,0x96,0xfc,0xd8,0x8a}};
static const GUID G_GUID_NULL={0,0,0,{0,0,0,0,0,0,0,0}};
static const DWORD MF_SOURCE_READER_MEDIASOURCE_=0xffffffffUL;
static const WORD VT_I8_=20, VT_UI8_=21;
struct WAVEFORMATEXTENSIBLE_MIN { WAVEFORMATEX Format; union {WORD wValidBitsPerSample;WORD wSamplesPerBlock;WORD wReserved;} Samples; DWORD dwChannelMask; GUID SubFormat; };
static_assert(sizeof(WAVEFORMATEXTENSIBLE_MIN)==40, "WAVEFORMATEXTENSIBLE ABI must match Windows mmreg/ksmedia layout");


struct IUnknownV { HRESULT (WINAPI*QueryInterface)(void*,const GUID*,void**); ULONG (WINAPI*AddRef)(void*); ULONG (WINAPI*Release)(void*); };
struct IMMDeviceEnumeratorV {
 HRESULT (WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);
 HRESULT(WINAPI*EnumAudioEndpoints)(void*,int,DWORD,void**);HRESULT(WINAPI*GetDefaultAudioEndpoint)(void*,int,int,void**);HRESULT(WINAPI*GetDevice)(void*,LPCWSTR,void**);
 HRESULT(WINAPI*RegisterEndpointNotificationCallback)(void*,void*);HRESULT(WINAPI*UnregisterEndpointNotificationCallback)(void*,void*);
};
struct IMMDeviceCollectionV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*GetCount)(void*,UINT*);HRESULT(WINAPI*Item)(void*,UINT,void**);};
struct IMMDeviceV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*Activate)(void*,const GUID*,DWORD,PROPVARIANT*,void**);HRESULT(WINAPI*OpenPropertyStore)(void*,DWORD,void**);HRESULT(WINAPI*GetId)(void*,LPWSTR*);HRESULT(WINAPI*GetState)(void*,DWORD*);};
struct IPropertyStoreV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*GetCount)(void*,DWORD*);HRESULT(WINAPI*GetAt)(void*,DWORD,PROPERTYKEY*);HRESULT(WINAPI*GetValue)(void*,const PROPERTYKEY*,PROPVARIANT*);HRESULT(WINAPI*SetValue)(void*,const PROPERTYKEY*,const PROPVARIANT*);HRESULT(WINAPI*Commit)(void*);};
struct IAudioClientV {
 HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);
 HRESULT(WINAPI*Initialize)(void*,int,DWORD,REFERENCE_TIME,REFERENCE_TIME,const WAVEFORMATEX*,const GUID*);
 HRESULT(WINAPI*GetBufferSize)(void*,UINT*);HRESULT(WINAPI*GetStreamLatency)(void*,REFERENCE_TIME*);HRESULT(WINAPI*GetCurrentPadding)(void*,UINT*);
 HRESULT(WINAPI*IsFormatSupported)(void*,int,const WAVEFORMATEX*,WAVEFORMATEX**);HRESULT(WINAPI*GetMixFormat)(void*,WAVEFORMATEX**);HRESULT(WINAPI*GetDevicePeriod)(void*,REFERENCE_TIME*,REFERENCE_TIME*);
 HRESULT(WINAPI*Start)(void*);HRESULT(WINAPI*Stop)(void*);HRESULT(WINAPI*Reset)(void*);HRESULT(WINAPI*SetEventHandle)(void*,HANDLE);HRESULT(WINAPI*GetService)(void*,const GUID*,void**);
};
struct IAudioRenderClientV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*GetBuffer)(void*,UINT,BYTE**);HRESULT(WINAPI*ReleaseBuffer)(void*,UINT,DWORD);};
// Minimal Media Foundation COM vtables. The IMFAttributes portion is kept in
// exact ABI order so media types and samples can be used without the Windows SDK.
struct IMFAttributesV {
 HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**); ULONG(WINAPI*AddRef)(void*); ULONG(WINAPI*Release)(void*);
 HRESULT(WINAPI*GetItem)(void*,const GUID*,PROPVARIANT*); HRESULT(WINAPI*GetItemType)(void*,const GUID*,int*); HRESULT(WINAPI*CompareItem)(void*,const GUID*,const PROPVARIANT*,BOOL*); HRESULT(WINAPI*Compare)(void*,void*,int,BOOL*);
 HRESULT(WINAPI*GetUINT32)(void*,const GUID*,UINT*); HRESULT(WINAPI*GetUINT64)(void*,const GUID*,ULONGLONG*); HRESULT(WINAPI*GetDouble)(void*,const GUID*,double*); HRESULT(WINAPI*GetGUID)(void*,const GUID*,GUID*);
 HRESULT(WINAPI*GetStringLength)(void*,const GUID*,UINT*); HRESULT(WINAPI*GetString)(void*,const GUID*,LPWSTR,UINT,UINT*); HRESULT(WINAPI*GetAllocatedString)(void*,const GUID*,LPWSTR*,UINT*);
 HRESULT(WINAPI*GetBlobSize)(void*,const GUID*,UINT*); HRESULT(WINAPI*GetBlob)(void*,const GUID*,BYTE*,UINT,UINT*); HRESULT(WINAPI*GetAllocatedBlob)(void*,const GUID*,BYTE**,UINT*); HRESULT(WINAPI*GetUnknown)(void*,const GUID*,const GUID*,void**);
 HRESULT(WINAPI*SetItem)(void*,const GUID*,const PROPVARIANT*); HRESULT(WINAPI*DeleteItem)(void*,const GUID*); HRESULT(WINAPI*DeleteAllItems)(void*);
 HRESULT(WINAPI*SetUINT32)(void*,const GUID*,UINT); HRESULT(WINAPI*SetUINT64)(void*,const GUID*,ULONGLONG); HRESULT(WINAPI*SetDouble)(void*,const GUID*,double); HRESULT(WINAPI*SetGUID)(void*,const GUID*,const GUID*);
 HRESULT(WINAPI*SetString)(void*,const GUID*,LPCWSTR); HRESULT(WINAPI*SetBlob)(void*,const GUID*,const BYTE*,UINT); HRESULT(WINAPI*SetUnknown)(void*,const GUID*,void*);
 HRESULT(WINAPI*LockStore)(void*); HRESULT(WINAPI*UnlockStore)(void*); HRESULT(WINAPI*GetCount)(void*,UINT*); HRESULT(WINAPI*GetItemByIndex)(void*,UINT,GUID*,PROPVARIANT*); HRESULT(WINAPI*CopyAllItems)(void*,void*);
};
struct IMFSourceReaderV {
 HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**); ULONG(WINAPI*AddRef)(void*); ULONG(WINAPI*Release)(void*);
 HRESULT(WINAPI*GetStreamSelection)(void*,DWORD,BOOL*); HRESULT(WINAPI*SetStreamSelection)(void*,DWORD,BOOL); HRESULT(WINAPI*GetNativeMediaType)(void*,DWORD,DWORD,void**);
 HRESULT(WINAPI*GetCurrentMediaType)(void*,DWORD,void**); HRESULT(WINAPI*SetCurrentMediaType)(void*,DWORD,DWORD*,void*); HRESULT(WINAPI*SetCurrentPosition)(void*,const GUID*,const PROPVARIANT*);
 HRESULT(WINAPI*ReadSample)(void*,DWORD,DWORD,DWORD*,DWORD*,long long*,void**); HRESULT(WINAPI*Flush)(void*,DWORD); HRESULT(WINAPI*GetServiceForStream)(void*,DWORD,const GUID*,const GUID*,void**); HRESULT(WINAPI*GetPresentationAttribute)(void*,DWORD,const GUID*,PROPVARIANT*);
};
struct IMFSampleV {
 // IMFAttributes
 IMFAttributesV a;
 // IMFSample
 HRESULT(WINAPI*GetSampleFlags)(void*,DWORD*); HRESULT(WINAPI*SetSampleFlags)(void*,DWORD); HRESULT(WINAPI*GetSampleTime)(void*,long long*); HRESULT(WINAPI*SetSampleTime)(void*,long long);
 HRESULT(WINAPI*GetSampleDuration)(void*,long long*); HRESULT(WINAPI*SetSampleDuration)(void*,long long); HRESULT(WINAPI*GetBufferCount)(void*,DWORD*); HRESULT(WINAPI*GetBufferByIndex)(void*,DWORD,void**); HRESULT(WINAPI*ConvertToContiguousBuffer)(void*,void**);
 HRESULT(WINAPI*AddBuffer)(void*,void*); HRESULT(WINAPI*RemoveBufferByIndex)(void*,DWORD); HRESULT(WINAPI*RemoveAllBuffers)(void*); HRESULT(WINAPI*GetTotalLength)(void*,DWORD*); HRESULT(WINAPI*CopyToBuffer)(void*,void*);
};
struct IMFMediaBufferV {HRESULT(WINAPI*QueryInterface)(void*,const GUID*,void**);ULONG(WINAPI*AddRef)(void*);ULONG(WINAPI*Release)(void*);HRESULT(WINAPI*Lock)(void*,BYTE**,DWORD*,DWORD*);HRESULT(WINAPI*Unlock)(void*);HRESULT(WINAPI*GetCurrentLength)(void*,DWORD*);HRESULT(WINAPI*SetCurrentLength)(void*,DWORD);HRESULT(WINAPI*GetMaxLength)(void*,DWORD*);};
struct Obj {void* v;};

template<class V> static V* VT(void* p){return p?*(V**)p:0;}
static void Rel(void*&p){if(p){VT<IUnknownV>(p)->Release(p);p=0;}}
static void WCopyA(wchar_t*d,const wchar_t*s,int cap){int i=0;if(!d||cap<1)return;if(s)while(s[i]&&i<cap-1){d[i]=s[i];i++;}d[i]=0;}
static int WLenA(const wchar_t*s){int n=0;if(s)while(s[n])n++;return n;}
static bool EndsI(const wchar_t*s,const wchar_t*e){int a=WLenA(s),b=WLenA(e);if(b>a)return false;for(int i=0;i<b;i++){wchar_t x=s[a-b+i],y=e[i];if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}return true;}
static DWORD U32(const BYTE*p){return (DWORD)p[0]|((DWORD)p[1]<<8)|((DWORD)p[2]<<16)|((DWORD)p[3]<<24);} static WORD U16(const BYTE*p){return (WORD)(p[0]|(p[1]<<8));}
static void* Alloc(SIZE_T n){return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,n);}static void Free(void*p){if(p)HeapFree(GetProcessHeap(),0,p);}

struct PCM {float* s; unsigned long long frames;};
static void* g_enum=0,*g_dev=0,*g_client=0,*g_render=0;static bool g_platformStarted=false;static bool g_comOwned=false;static bool g_mfStarted=false;static UINT g_bufFrames=0;static int g_rate=48000;static bool g_fmtFloat=true;static int g_outBits=32;static int g_outChannels=2;static int g_outBlockAlign=8;static int g_outValidBits=32;static bool g_fmtSigned32=false,g_fmtPCM24=false;
static wchar_t g_activeDeviceId[256]=L"",g_activeDeviceName[192]=L"Default Windows device";
static HRESULT g_lastError=S_OK; static wchar_t g_lastErrorStage[96]=L"OK";
static void SetAudioError(const wchar_t*stage,HRESULT hr){g_lastError=hr;WCopyA(g_lastErrorStage,stage?stage:L"Unknown",96);}
static PCM g_pcm={0,0},g_next={0,0};static unsigned long long g_frame=0,g_nextFrame=0;static bool g_play=false,g_pause=false,g_native=false;static int g_crossfadeMs=0;static bool g_advanced=false;static double g_nextRgDb=0.0;static wchar_t g_backend[96]=L"Compatibility";
// Streaming Source Reader state. Compressed audio is decoded incrementally so long files
// never require hundreds of MB / >1 GB of float PCM in RAM.
static void* g_streamReader=0;static BYTE* g_streamData=0;static SIZE_T g_streamCap=0,g_streamBytes=0,g_streamAt=0;
static int g_streamBits=16,g_streamChannels=2,g_streamRate=48000;static bool g_streamFloat=false,g_streamPCM32=false,g_streamEos=false;
static bool g_rsInit=false,g_rsDone=false; static double g_rsFrac=0.0,g_rsStep=1.0; static float g_rsAL=0,g_rsAR=0,g_rsBL=0,g_rsBR=0;
static int g_streamLengthMs=0;
static int g_vol=78,g_bal=0;static bool g_mute=false,g_eqOn=false;static int g_preamp=0,g_band[10]={0};static double g_rgDb=0.0;
static float g_fft[512];static int g_fftPos=0;

struct BQ {double b0,b1,b2,a1,a2,z1,z2;}; static BQ g_bq[2][10];
static const double PI=3.14159265358979323846;static const double FREQS[10]={31.0,62.0,125.0,250.0,500.0,1000.0,2000.0,4000.0,8000.0,16000.0};
static void ResetEQ(){for(int c=0;c<2;c++)for(int b=0;b<10;b++)g_bq[c][b].z1=g_bq[c][b].z2=0;}
static void CalcEQ(){for(int b=0;b<10;b++){double A=pow(10.0,(double)g_band[b]/40.0),w=2.0*PI*FREQS[b]/(double)g_rate,al=sin(w)/2.0,c=cos(w);double b0=1+al*A,b1=-2*c,b2=1-al*A,a0=1+al/A,a1=-2*c,a2=1-al/A;for(int ch=0;ch<2;ch++){g_bq[ch][b].b0=b0/a0;g_bq[ch][b].b1=b1/a0;g_bq[ch][b].b2=b2/a0;g_bq[ch][b].a1=a1/a0;g_bq[ch][b].a2=a2/a0;}}ResetEQ();}
static float RunEQ(float x,int ch){if(!g_eqOn)return x;for(int b=0;b<10;b++){BQ&q=g_bq[ch][b];double y=q.b0*x+q.z1;q.z1=q.b1*x-q.a1*y+q.z2;q.z2=q.b2*x-q.a2*y;x=(float)y;}return x;}
static float ClampF(float x){return x<-1.f?-1.f:(x>1.f?1.f:x);}

static void ReleaseAudio(){if(g_client)VT<IAudioClientV>(g_client)->Stop(g_client);Rel(g_render);Rel(g_client);Rel(g_dev);Rel(g_enum);g_bufFrames=0;}
static void CaptureActiveDevice(){
 g_activeDeviceId[0]=0;WCopyA(g_activeDeviceName,L"Windows audio device",192);if(!g_dev)return;
 LPWSTR id=0;if(SUCCEEDED(VT<IMMDeviceV>(g_dev)->GetId(g_dev,&id))&&id){WCopyA(g_activeDeviceId,id,256);CoTaskMemFree(id);}
 void* ps=0;if(SUCCEEDED(VT<IMMDeviceV>(g_dev)->OpenPropertyStore(g_dev,STGM_READ,&ps))&&ps){PROPVARIANT v;memset(&v,0,sizeof(v));if(SUCCEEDED(VT<IPropertyStoreV>(ps)->GetValue(ps,&PKEY_Device_FriendlyName,&v))&&v.vt==VT_LPWSTR&&v.pwszVal)WCopyA(g_activeDeviceName,v.pwszVal,192);PropVariantClear(&v);Rel(ps);}
}

int OzAudioEnumerate(OzAudioDevice*out,int cap){if(!out||cap<=0)return 0;HRESULT ci=CoInitializeEx(0,COINIT_MULTITHREADED);bool uninit=SUCCEEDED(ci);void* en=0;if(FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator,0,CLSCTX_ALL,&IID_IMMDeviceEnumerator,&en))||!en){if(uninit)CoUninitialize();return 0;}void* col=0;int count=0;if(SUCCEEDED(VT<IMMDeviceEnumeratorV>(en)->EnumAudioEndpoints(en,0,DEVICE_STATE_ACTIVE,&col))&&col){UINT n=0;VT<IMMDeviceCollectionV>(col)->GetCount(col,&n);for(UINT i=0;i<n&&count<cap;i++){void*d=0;if(FAILED(VT<IMMDeviceCollectionV>(col)->Item(col,i,&d))||!d)continue;LPWSTR id=0;if(SUCCEEDED(VT<IMMDeviceV>(d)->GetId(d,&id))&&id){WCopyA(out[count].id,id,256);CoTaskMemFree(id);}void* ps=0;if(SUCCEEDED(VT<IMMDeviceV>(d)->OpenPropertyStore(d,STGM_READ,&ps))&&ps){PROPVARIANT v;memset(&v,0,sizeof(v));if(SUCCEEDED(VT<IPropertyStoreV>(ps)->GetValue(ps,&PKEY_Device_FriendlyName,&v))&&v.vt==VT_LPWSTR&&v.pwszVal)WCopyA(out[count].name,v.pwszVal,192);else WCopyA(out[count].name,L"Audio device",192);PropVariantClear(&v);Rel(ps);}else WCopyA(out[count].name,L"Audio device",192);count++;Rel(d);}Rel(col);}Rel(en);if(uninit)CoUninitialize();return count;}

bool OzAudioInit(const wchar_t* deviceId){
 ReleaseAudio();SetAudioError(L"Audio init",S_OK);
 if(!g_platformStarted){
  HRESULT ci=CoInitializeEx(0,COINIT_MULTITHREADED);g_comOwned=SUCCEEDED(ci);
  HRESULT mh=MFStartup(MF_VERSION,MFSTARTUP_FULL);g_mfStarted=SUCCEEDED(mh);
  g_platformStarted=true;
 }
 HRESULT ehr=CoCreateInstance(&CLSID_MMDeviceEnumerator,0,CLSCTX_ALL,&IID_IMMDeviceEnumerator,&g_enum);if(FAILED(ehr)||!g_enum){SetAudioError(L"Create MMDeviceEnumerator",ehr);return false;}
 HRESULT hr;if(deviceId&&deviceId[0])hr=VT<IMMDeviceEnumeratorV>(g_enum)->GetDevice(g_enum,deviceId,&g_dev);else hr=VT<IMMDeviceEnumeratorV>(g_enum)->GetDefaultAudioEndpoint(g_enum,0,0,&g_dev);
 if(FAILED(hr)||!g_dev){SetAudioError(deviceId&&deviceId[0]?L"Open selected endpoint":L"Open default endpoint",hr);ReleaseAudio();return false;}
 hr=VT<IMMDeviceV>(g_dev)->Activate(g_dev,&IID_IAudioClient,CLSCTX_ALL,0,&g_client);if(FAILED(hr)||!g_client){SetAudioError(L"Activate IAudioClient",hr);ReleaseAudio();return false;}
 // Shared-mode WASAPI must follow the selected endpoint's real mix format. The old
 // 3.0.x code effectively required stereo/48 kHz and therefore rejected many HDMI,
 // USB and multichannel endpoints and silently fell back to Default.
 WAVEFORMATEX* mix=0;hr=VT<IAudioClientV>(g_client)->GetMixFormat(g_client,&mix);if(FAILED(hr)||!mix){SetAudioError(L"Get endpoint mix format",hr);ReleaseAudio();return false;}
 g_rate=(int)mix->nSamplesPerSec;g_outBits=(int)mix->wBitsPerSample;g_outChannels=(int)mix->nChannels;g_outBlockAlign=(int)mix->nBlockAlign;g_outValidBits=g_outBits;g_fmtFloat=false;g_fmtSigned32=false;g_fmtPCM24=false;bool outOK=g_rate>=8000&&g_rate<=384000&&g_outChannels>=1&&g_outChannels<=16;
 if(outOK&&mix->wFormatTag==WAVE_FORMAT_IEEE_FLOAT&&mix->wBitsPerSample==32)g_fmtFloat=true;
 else if(outOK&&mix->wFormatTag==WAVE_FORMAT_PCM&&mix->wBitsPerSample==16){}
 else if(outOK&&mix->wFormatTag==WAVE_FORMAT_PCM&&mix->wBitsPerSample==24)g_fmtPCM24=true;
 else if(outOK&&mix->wFormatTag==WAVE_FORMAT_PCM&&mix->wBitsPerSample==32)g_fmtSigned32=true;
 else if(outOK&&mix->wFormatTag==0xfffe&&mix->cbSize>=22){WAVEFORMATEXTENSIBLE_MIN*ex=(WAVEFORMATEXTENSIBLE_MIN*)mix;g_outValidBits=ex->Samples.wValidBitsPerSample?ex->Samples.wValidBitsPerSample:g_outBits;if(memcmp(&ex->SubFormat,&G_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,sizeof(GUID))==0&&mix->wBitsPerSample==32)g_fmtFloat=true;else if(memcmp(&ex->SubFormat,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0&&mix->wBitsPerSample==16){}else if(memcmp(&ex->SubFormat,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0&&mix->wBitsPerSample==24)g_fmtPCM24=true;else if(memcmp(&ex->SubFormat,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0&&mix->wBitsPerSample==32)g_fmtSigned32=true;else outOK=false;}
 else outOK=false;
 if(!outOK){CoTaskMemFree(mix);SetAudioError(L"Unsupported endpoint mix format",(HRESULT)0x88890008L);ReleaseAudio();return false;}
 hr=VT<IAudioClientV>(g_client)->Initialize(g_client,AUDCLNT_SHAREMODE_SHARED,0,2000000,0,mix,0);CoTaskMemFree(mix);
 if(FAILED(hr)){SetAudioError(L"Initialize WASAPI shared stream",hr);ReleaseAudio();return false;}
 hr=VT<IAudioClientV>(g_client)->GetBufferSize(g_client,&g_bufFrames);if(FAILED(hr)){SetAudioError(L"Get WASAPI buffer size",hr);ReleaseAudio();return false;}
 hr=VT<IAudioClientV>(g_client)->GetService(g_client,&IID_IAudioRenderClient,&g_render);if(FAILED(hr)||!g_render){SetAudioError(L"Get IAudioRenderClient",hr);ReleaseAudio();return false;}
 CaptureActiveDevice();CalcEQ();SetAudioError(L"OK",S_OK);return true;
}

static void Resample16(const short*src,unsigned long long frames,int sr,int ch,PCM&out){if(!src||!frames||sr<=0||(ch!=1&&ch!=2))return;unsigned long long of=(frames*(unsigned long long)g_rate)/(unsigned)sr+2;float*d=(float*)Alloc((SIZE_T)of*2*sizeof(float));if(!d)return;double step=(double)sr/(double)g_rate,pos=0;for(unsigned long long i=0;i<of;i++,pos+=step){unsigned long long a=(unsigned long long)pos;if(a>=frames)a=frames-1;unsigned long long b=a+1<frames?a+1:a;double t=pos-(double)a;for(int c=0;c<2;c++){int sc=ch==1?0:c;double x=src[a*ch+sc]/32768.0,y=src[b*ch+sc]/32768.0;d[i*2+c]=(float)(x+(y-x)*t);}}out.s=d;out.frames=of;}
static void ResampleFloat(const float*src,unsigned long long frames,int sr,int ch,PCM&out){if(!src||!frames||sr<=0||(ch!=1&&ch!=2))return;unsigned long long of=(frames*(unsigned long long)g_rate)/(unsigned)sr+2;float*d=(float*)Alloc((SIZE_T)of*2*sizeof(float));if(!d)return;double step=(double)sr/(double)g_rate,pos=0;for(unsigned long long i=0;i<of;i++,pos+=step){unsigned long long a=(unsigned long long)pos;if(a>=frames)a=frames-1;unsigned long long b=a+1<frames?a+1:a;double t=pos-(double)a;for(int c=0;c<2;c++){int sc=ch==1?0:c;double x=src[a*ch+sc],y=src[b*ch+sc];d[i*2+c]=(float)(x+(y-x)*t);}}out.s=d;out.frames=of;}
static void ResampleS32(const int*src,unsigned long long frames,int sr,int ch,PCM&out){if(!src||!frames||sr<=0||(ch!=1&&ch!=2))return;unsigned long long of=(frames*(unsigned long long)g_rate)/(unsigned)sr+2;float*d=(float*)Alloc((SIZE_T)of*2*sizeof(float));if(!d)return;double step=(double)sr/(double)g_rate,pos=0;for(unsigned long long i=0;i<of;i++,pos+=step){unsigned long long a=(unsigned long long)pos;if(a>=frames)a=frames-1;unsigned long long b=a+1<frames?a+1:a;double t=pos-(double)a;for(int c=0;c<2;c++){int sc=ch==1?0:c;double x=src[a*ch+sc]/2147483648.0,y=src[b*ch+sc]/2147483648.0;d[i*2+c]=(float)(x+(y-x)*t);}}out.s=d;out.frames=of;}

static bool DecodeWav(const wchar_t*path,PCM&out){HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;DWORD sz=GetFileSize(h,0);if(sz<44||sz>0x7fffffffUL){CloseHandle(h);return false;}BYTE*b=(BYTE*)Alloc(sz);DWORD rd=0;if(!b||!ReadFile(h,b,sz,&rd,0)||rd!=sz){Free(b);CloseHandle(h);return false;}CloseHandle(h);if(U32(b)!=0x46464952||U32(b+8)!=0x45564157){Free(b);return false;}WORD fmt=0,ch=0,bits=0;DWORD sr=0;BYTE*data=0;DWORD dataSz=0;DWORD p=12;while(p+8<=sz){DWORD id=U32(b+p),cs=U32(b+p+4);p+=8;if(p+cs>sz)break;if(id==0x20746d66&&cs>=16){fmt=U16(b+p);ch=U16(b+p+2);sr=U32(b+p+4);bits=U16(b+p+14);}else if(id==0x61746164){data=b+p;dataSz=cs;}p+=cs+(cs&1);}if(!data||!sr||(ch!=1&&ch!=2)){Free(b);return false;}unsigned long long frames=0;if(fmt==1&&bits==16){frames=dataSz/(2*ch);Resample16((short*)data,frames,(int)sr,ch,out);}else if(fmt==3&&bits==32){frames=dataSz/(4*ch);ResampleFloat((float*)data,frames,(int)sr,ch,out);}else{Free(b);return false;}Free(b);return out.s!=0;}

static bool ParseMP3Header(const BYTE*h,int&sr,int&ch,int&br,int&frame){if(h[0]!=0xff||(h[1]&0xe0)!=0xe0)return false;int ver=(h[1]>>3)&3,layer=(h[1]>>1)&3,bi=(h[2]>>4)&15,si=(h[2]>>2)&3,pad=(h[2]>>1)&1;if(ver==1||layer!=1||bi==0||bi==15||si==3)return false;static const int srT[3]={44100,48000,32000};sr=srT[si];if(ver==2)sr/=2;else if(ver==0)sr/=4;static const int br1[16]={0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};static const int br2[16]={0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};br=(ver==3?br1[bi]:br2[bi]);ch=((h[3]>>6)==3)?1:2;frame=((ver==3?144:72)*br*1000/sr)+pad;return true;}
static bool DecodeMP3(const wchar_t*path,PCM&out){HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return false;DWORD sz=GetFileSize(h,0);if(sz<128||sz>0x7fffffffUL){CloseHandle(h);return false;}BYTE*b=(BYTE*)Alloc(sz);DWORD rd=0;if(!b||!ReadFile(h,b,sz,&rd,0)||rd!=sz){Free(b);CloseHandle(h);return false;}CloseHandle(h);DWORD off=0;if(sz>10&&b[0]=='I'&&b[1]=='D'&&b[2]=='3'){DWORD tag=((b[6]&0x7f)<<21)|((b[7]&0x7f)<<14)|((b[8]&0x7f)<<7)|(b[9]&0x7f);off=10+tag;}int sr=0,ch=0,br=0,fb=0;while(off+4<sz&&!ParseMP3Header(b+off,sr,ch,br,fb))off++;if(off+4>=sz){Free(b);return false;}DWORD end=sz;if(end>=128&&b[end-128]=='T'&&b[end-127]=='A'&&b[end-126]=='G')end-=128;DWORD srcLen=end-off;MPEGLAYER3WAVEFORMAT in;memset(&in,0,sizeof(in));in.wfx.wFormatTag=WAVE_FORMAT_MPEGLAYER3;in.wfx.nChannels=(WORD)ch;in.wfx.nSamplesPerSec=sr;in.wfx.nAvgBytesPerSec=br*1000/8;in.wfx.nBlockAlign=1;in.wfx.wBitsPerSample=0;in.wfx.cbSize=12;in.wID=1;in.fdwFlags=0;/* VBR-safe decoder description: Windows decoders do not require a fixed MP3 frame size here. */in.nBlockSize=1;in.nFramesPerBlock=1;in.nCodecDelay=0;WAVEFORMATEX dst;memset(&dst,0,sizeof(dst));dst.wFormatTag=WAVE_FORMAT_PCM;dst.nChannels=(WORD)ch;dst.nSamplesPerSec=sr;dst.wBitsPerSample=16;dst.nBlockAlign=(WORD)(ch*2);dst.nAvgBytesPerSec=sr*dst.nBlockAlign;void* as=0;UINT mm=acmStreamOpen(&as,0,&in.wfx,&dst,0,0,0,ACM_STREAMOPENF_NONREALTIME);if(mm||!as){Free(b);return false;}DWORD dstLen=0;if(acmStreamSize(as,srcLen,&dstLen,ACM_STREAMSIZEF_SOURCE)||!dstLen||dstLen>700*1024*1024UL){acmStreamClose(as,0);Free(b);return false;}BYTE*d=(BYTE*)Alloc(dstLen);if(!d){acmStreamClose(as,0);Free(b);return false;}ACMSTREAMHEADER sh;memset(&sh,0,sizeof(sh));sh.cbStruct=sizeof(sh);sh.pbSrc=b+off;sh.cbSrcLength=srcLen;sh.pbDst=d;sh.cbDstLength=dstLen;if(acmStreamPrepareHeader(as,&sh,0)){Free(d);acmStreamClose(as,0);Free(b);return false;}mm=acmStreamConvert(as,&sh,ACM_STREAMCONVERTF_START|ACM_STREAMCONVERTF_END);acmStreamUnprepareHeader(as,&sh,0);acmStreamClose(as,0);Free(b);if(mm||!sh.cbDstLengthUsed){Free(d);return false;}unsigned long long frames=sh.cbDstLengthUsed/(dst.nBlockAlign);Resample16((short*)d,frames,sr,ch,out);Free(d);return out.s!=0;}

static bool DecodeMF(const wchar_t*path,PCM&out){
 if(!g_mfStarted)return false;
 void* reader=0;if(FAILED(MFCreateSourceReaderFromURL(path,0,&reader))||!reader)return false;
 IMFSourceReaderV* rv=VT<IMFSourceReaderV>(reader);
 rv->SetStreamSelection(reader,MF_SOURCE_READER_ALL_STREAMS,FALSE);
 rv->SetStreamSelection(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE);
 void* mt=0;if(FAILED(MFCreateMediaType(&mt))||!mt){Rel(reader);return false;}
 IMFAttributesV* av=VT<IMFAttributesV>(mt);
 av->SetGUID(mt,&G_MF_MT_MAJOR_TYPE,&G_MFMediaType_Audio);
 // Ask Media Foundation for ordinary signed 16-bit PCM. This is supported by
 // more Windows audio decoder/MFT combinations than requiring float output.
 av->SetGUID(mt,&G_MF_MT_SUBTYPE,&G_KSDATAFORMAT_SUBTYPE_PCM);
 av->SetUINT32(mt,&G_MF_MT_AUDIO_BITS_PER_SAMPLE,16);
 HRESULT sethr=rv->SetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mt);
 if(FAILED(sethr)){/* Some transforms reject the explicit bit-depth request. Retry with plain PCM. */Rel(mt);mt=0;if(FAILED(MFCreateMediaType(&mt))||!mt){Rel(reader);return false;}av=VT<IMFAttributesV>(mt);av->SetGUID(mt,&G_MF_MT_MAJOR_TYPE,&G_MFMediaType_Audio);av->SetGUID(mt,&G_MF_MT_SUBTYPE,&G_KSDATAFORMAT_SUBTYPE_PCM);if(FAILED(rv->SetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mt))){Rel(mt);Rel(reader);return false;}}
 Rel(mt);mt=0;
 if(FAILED(rv->GetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,&mt))||!mt){Rel(reader);return false;}
 av=VT<IMFAttributesV>(mt);UINT ch=0,sr=0,bits=0;GUID subtype;memset(&subtype,0,sizeof(subtype));av->GetUINT32(mt,&G_MF_MT_AUDIO_NUM_CHANNELS,&ch);av->GetUINT32(mt,&G_MF_MT_AUDIO_SAMPLES_PER_SECOND,&sr);av->GetUINT32(mt,&G_MF_MT_AUDIO_BITS_PER_SAMPLE,&bits);av->GetGUID(mt,&G_MF_MT_SUBTYPE,&subtype);bool mfFloat=memcmp(&subtype,&G_MFAudioFormat_Float,sizeof(GUID))==0||memcmp(&subtype,&G_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,sizeof(GUID))==0;bool mfPCM=memcmp(&subtype,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0;Rel(mt);
 if(bits==0)bits=mfFloat?32:16;if((ch!=1&&ch!=2)||sr<8000||sr>384000||(!mfPCM&&!mfFloat)||(bits!=16&&bits!=32)){Rel(reader);return false;}
 BYTE* all=0;SIZE_T used=0,cap=0;bool ok=true,eos=false;unsigned guard=0;
 while(!eos&&guard++<2000000u){
  DWORD actual=0,flags=0;long long ts=0;void* sample=0;HRESULT hr=rv->ReadSample(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,&actual,&flags,&ts,&sample);
  if(FAILED(hr)||(flags&MF_SOURCE_READERF_ERROR)){ok=false;if(sample)Rel(sample);break;}
  if(flags&MF_SOURCE_READERF_ENDOFSTREAM)eos=true;
  if(sample){void* mb=0;IMFSampleV* sv=VT<IMFSampleV>(sample);if(SUCCEEDED(sv->ConvertToContiguousBuffer(sample,&mb))&&mb){BYTE*p=0;DWORD max=0,cur=0;IMFMediaBufferV*bv=VT<IMFMediaBufferV>(mb);if(SUCCEEDED(bv->Lock(mb,&p,&max,&cur))&&p&&cur){if(used+cur>cap){SIZE_T nc=cap?cap*2:1024*1024;while(nc<used+cur)nc*=2;if(nc>1024ULL*1024ULL*1024ULL){ok=false;bv->Unlock(mb);Rel(mb);Rel(sample);break;}void*np=all?HeapReAlloc(GetProcessHeap(),0,all,nc):HeapAlloc(GetProcessHeap(),0,nc);if(!np){ok=false;bv->Unlock(mb);Rel(mb);Rel(sample);break;}all=(BYTE*)np;cap=nc;}memcpy(all+used,p,cur);used+=cur;bv->Unlock(mb);}Rel(mb);}Rel(sample);}
 }
 Rel(reader);if(!ok||!all){if(all)HeapFree(GetProcessHeap(),0,all);return false;}
 bool decoded=false;
 if(bits==16&&mfPCM&&used>=sizeof(short)*ch){unsigned long long frames=used/(sizeof(short)*ch);Resample16((short*)all,frames,(int)sr,(int)ch,out);decoded=out.s!=0;}
 else if(bits==32&&mfFloat&&used>=sizeof(float)*ch){unsigned long long frames=used/(sizeof(float)*ch);ResampleFloat((float*)all,frames,(int)sr,(int)ch,out);decoded=out.s!=0;}
 else if(bits==32&&mfPCM&&used>=sizeof(int)*ch){unsigned long long frames=used/(sizeof(int)*ch);ResampleS32((int*)all,frames,(int)sr,(int)ch,out);decoded=out.s!=0;}
 HeapFree(GetProcessHeap(),0,all);return decoded;
}


static void ResetStreamResampler(){g_rsInit=false;g_rsDone=false;g_rsFrac=0.0;g_rsStep=(g_rate>0&&g_streamRate>0)?((double)g_streamRate/(double)g_rate):1.0;g_rsAL=g_rsAR=g_rsBL=g_rsBR=0.0f;}
static void CloseMFStream(){if(g_streamReader)Rel(g_streamReader);g_streamReader=0;if(g_streamData)Free(g_streamData);g_streamData=0;g_streamCap=g_streamBytes=g_streamAt=0;g_streamEos=false;g_streamLengthMs=0;ResetStreamResampler();}
static bool ReadReaderFormat(void* reader,int&ch,int&sr,int&bits,bool&flt,bool&pcm32){
 if(!reader)return false;IMFSourceReaderV*rv=VT<IMFSourceReaderV>(reader);void*mt=0;HRESULT hr=rv->GetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,&mt);if(FAILED(hr)||!mt)return false;IMFAttributesV*av=VT<IMFAttributesV>(mt);UINT uch=0,usr=0,ubits=0;GUID sub;memset(&sub,0,sizeof(sub));av->GetUINT32(mt,&G_MF_MT_AUDIO_NUM_CHANNELS,&uch);av->GetUINT32(mt,&G_MF_MT_AUDIO_SAMPLES_PER_SECOND,&usr);av->GetUINT32(mt,&G_MF_MT_AUDIO_BITS_PER_SAMPLE,&ubits);av->GetGUID(mt,&G_MF_MT_SUBTYPE,&sub);bool isPcm=memcmp(&sub,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0;bool isFloat=memcmp(&sub,&G_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,sizeof(GUID))==0||memcmp(&sub,&G_MFAudioFormat_Float,sizeof(GUID))==0;Rel(mt);if(uch<1||uch>2||usr<8000||usr>384000||(!isPcm&&!isFloat))return false;if(!ubits)ubits=isFloat?32:16;if(ubits!=16&&ubits!=32)return false;ch=(int)uch;sr=(int)usr;bits=(int)ubits;flt=isFloat&&ubits==32;pcm32=isPcm&&ubits==32;return true;
}
static bool ConfigureReaderPCM(void*reader){
 IMFSourceReaderV*rv=VT<IMFSourceReaderV>(reader);HRESULT hr=rv->SetStreamSelection(reader,MF_SOURCE_READER_ALL_STREAMS,FALSE);if(FAILED(hr)){SetAudioError(L"MF deselect streams",hr);return false;}hr=rv->SetStreamSelection(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE);if(FAILED(hr)){SetAudioError(L"MF select audio stream",hr);return false;}
 // Microsoft recommends a partial output type: major type + PCM subtype only. The
 // Source Reader then loads the decoder and returns the file's natural PCM rate/channels.
 void*mt=0;hr=MFCreateMediaType(&mt);if(FAILED(hr)||!mt){SetAudioError(L"MFCreateMediaType",hr);return false;}IMFAttributesV*av=VT<IMFAttributesV>(mt);hr=av->SetGUID(mt,&G_MF_MT_MAJOR_TYPE,&G_MFMediaType_Audio);if(SUCCEEDED(hr))hr=av->SetGUID(mt,&G_MF_MT_SUBTYPE,&G_KSDATAFORMAT_SUBTYPE_PCM);if(SUCCEEDED(hr))hr=rv->SetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mt);Rel(mt);if(FAILED(hr)){SetAudioError(L"MF set PCM output",hr);return false;}return true;
}
static bool OpenMFStream(const wchar_t*path){
 if(!g_mfStarted||!path||!path[0]){SetAudioError(L"Media Foundation unavailable",(HRESULT)0x80004005L);return false;}CloseMFStream();void*reader=0;HRESULT hr=MFCreateSourceReaderFromURL(path,0,&reader);if(FAILED(hr)||!reader){SetAudioError(L"MF open source",hr);return false;}if(!ConfigureReaderPCM(reader)){Rel(reader);return false;}int ch=0,sr=0,bits=0;bool flt=false,pcm32=false;if(!ReadReaderFormat(reader,ch,sr,bits,flt,pcm32)){SetAudioError(L"MF read PCM format",(HRESULT)0x80004005L);Rel(reader);return false;}
 g_streamReader=reader;g_streamChannels=ch;g_streamRate=sr;g_streamBits=bits;g_streamFloat=flt;g_streamPCM32=pcm32;g_streamEos=false;g_streamBytes=g_streamAt=0;g_streamLengthMs=0;PROPVARIANT pv;memset(&pv,0,sizeof(pv));if(SUCCEEDED(VT<IMFSourceReaderV>(reader)->GetPresentationAttribute(reader,MF_SOURCE_READER_MEDIASOURCE_,&G_MF_PD_DURATION,&pv))&&(pv.vt==VT_UI8_||pv.vt==VT_I8_))g_streamLengthMs=(int)(pv.uhVal/10000ULL);PropVariantClear(&pv);ResetStreamResampler();SetAudioError(L"OK",S_OK);return true;
}
static bool RefillMFStream(){
 if(!g_streamReader||g_streamEos)return false;IMFSourceReaderV*rv=VT<IMFSourceReaderV>(g_streamReader);for(int guard=0;guard<64;guard++){DWORD actual=0,flags=0;long long ts=0;void*sample=0;HRESULT hr=rv->ReadSample(g_streamReader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,&actual,&flags,&ts,&sample);if(FAILED(hr)||(flags&MF_SOURCE_READERF_ERROR)){SetAudioError(L"MF ReadSample",FAILED(hr)?hr:(HRESULT)0x80004005L);if(sample)Rel(sample);g_streamEos=true;return false;}if(flags&MF_SOURCE_READERF_ENDOFSTREAM)g_streamEos=true;if(flags&0x20u){int ch=0,sr=0,bits=0;bool flt=false,p32=false;if(ReadReaderFormat(g_streamReader,ch,sr,bits,flt,p32)){g_streamChannels=ch;g_streamRate=sr;g_streamBits=bits;g_streamFloat=flt;g_streamPCM32=p32;ResetStreamResampler();}}
  if(sample){void*mb=0;IMFSampleV*sv=VT<IMFSampleV>(sample);if(SUCCEEDED(sv->ConvertToContiguousBuffer(sample,&mb))&&mb){BYTE*src=0;DWORD mx=0,cur=0;IMFMediaBufferV*bv=VT<IMFMediaBufferV>(mb);if(SUCCEEDED(bv->Lock(mb,&src,&mx,&cur))&&src&&cur){if(cur>g_streamCap){SIZE_T nc=cur+4096;void*np=g_streamData?HeapReAlloc(GetProcessHeap(),0,g_streamData,nc):HeapAlloc(GetProcessHeap(),0,nc);if(!np){bv->Unlock(mb);Rel(mb);Rel(sample);g_streamEos=true;SetAudioError(L"MF PCM buffer allocation",(HRESULT)0x8007000EL);return false;}g_streamData=(BYTE*)np;g_streamCap=nc;}memcpy(g_streamData,src,cur);g_streamBytes=cur;g_streamAt=0;bv->Unlock(mb);Rel(mb);Rel(sample);return cur>0;}if(src)bv->Unlock(mb);Rel(mb);}Rel(sample);}if(g_streamEos)return false;}return false;
}
static bool ReadMFSourceFrame(float&l,float&r){
 int bytesPer=(g_streamBits/8)*g_streamChannels;if(bytesPer<=0)return false;if(g_streamAt+(SIZE_T)bytesPer>g_streamBytes&&!RefillMFStream())return false;BYTE*p=g_streamData+g_streamAt;g_streamAt+=bytesPer;if(g_streamBits==16){short*q=(short*)p;l=q[0]/32768.0f;r=(g_streamChannels==1?q[0]:q[1])/32768.0f;}else if(g_streamFloat){float*q=(float*)p;l=q[0];r=(g_streamChannels==1?q[0]:q[1]);}else if(g_streamPCM32){int*q=(int*)p;l=(float)(q[0]/2147483648.0);r=(float)((g_streamChannels==1?q[0]:q[1])/2147483648.0);}else return false;return true;
}
static bool StreamFrame(float&l,float&r){
 if(g_streamRate==g_rate)return ReadMFSourceFrame(l,r);if(g_rsDone)return false;if(!g_rsInit){if(!ReadMFSourceFrame(g_rsAL,g_rsAR))return false;if(!ReadMFSourceFrame(g_rsBL,g_rsBR)){g_rsBL=g_rsAL;g_rsBR=g_rsAR;g_rsDone=true;}g_rsInit=true;g_rsFrac=0.0;g_rsStep=(double)g_streamRate/(double)g_rate;}
 l=(float)(g_rsAL+(g_rsBL-g_rsAL)*g_rsFrac);r=(float)(g_rsAR+(g_rsBR-g_rsAR)*g_rsFrac);g_rsFrac+=g_rsStep;while(g_rsFrac>=1.0&&!g_rsDone){g_rsFrac-=1.0;g_rsAL=g_rsBL;g_rsAR=g_rsBR;if(!ReadMFSourceFrame(g_rsBL,g_rsBR)){g_rsBL=g_rsAL;g_rsBR=g_rsAR;g_rsDone=true;}}return true;
}
static bool SeekMFStream(int ms){if(!g_streamReader)return false;if(ms<0)ms=0;if(g_streamLengthMs>0&&ms>g_streamLengthMs)ms=g_streamLengthMs;PROPVARIANT pv;memset(&pv,0,sizeof(pv));pv.vt=VT_I8_;pv.uhVal=(ULONGLONG)ms*10000ULL;HRESULT hr=VT<IMFSourceReaderV>(g_streamReader)->SetCurrentPosition(g_streamReader,&G_GUID_NULL,&pv);if(FAILED(hr)){SetAudioError(L"MF seek",hr);return false;}g_streamBytes=g_streamAt=0;g_streamEos=false;ResetStreamResampler();g_frame=(unsigned long long)ms*(unsigned)g_rate/1000ULL;return true;}


bool OzAudioLoad(const wchar_t*path){
 OzAudioUnload();if(!g_client||!g_render)return false;PCM p={0,0};bool ok=false,usedMF=false;
 // Compressed/codec-backed files are streamed through Media Foundation directly at
 // the active WASAPI sample rate. This is retained as a fallback; 3.2.0 uses the streaming path first for compressed audio.
 // MP3 no longer expands into >1 GB of float PCM before playback can start.
 if(!EndsI(path,L".wav")&&OpenMFStream(path)){g_frame=0;g_native=true;g_play=false;g_pause=false;WCopyA(g_backend,L"WASAPI + MEDIA FOUNDATION STREAM",96);ResetEQ();return true;}
 if(EndsI(path,L".wav"))ok=DecodeWav(path,p);else if(EndsI(path,L".mp3")){ok=DecodeMF(path,p);usedMF=ok;if(!ok)ok=DecodeMP3(path,p);}else{ok=DecodeMF(path,p);usedMF=ok;}
 if(!ok)return false;g_pcm=p;g_frame=0;g_native=true;g_play=false;g_pause=false;if(EndsI(path,L".mp3"))WCopyA(g_backend,usedMF?L"WASAPI + MEDIA FOUNDATION PCM":L"WASAPI + ACM MP3",96);else if(EndsI(path,L".wav"))WCopyA(g_backend,L"WASAPI + PCM WAV",96);else WCopyA(g_backend,L"WASAPI + Media Foundation PCM",96);ResetEQ();return true;
}
void OzAudioUnload(){if(g_client)VT<IAudioClientV>(g_client)->Stop(g_client);CloseMFStream();Free(g_pcm.s);Free(g_next.s);g_pcm.s=0;g_pcm.frames=0;g_next.s=0;g_next.frames=0;g_frame=g_nextFrame=0;g_native=false;g_play=false;g_pause=false;g_advanced=false;WCopyA(g_backend,L"Compatibility",96);}
void OzAudioPlay(){if(!g_native||!g_client||!g_render||(!g_pcm.s&&!g_streamReader))return;if(g_pcm.s&&g_frame>=g_pcm.frames)g_frame=0;if(g_streamReader&&g_streamLengthMs>0&&OzAudioPosMs()>=g_streamLengthMs)SeekMFStream(0);g_play=true;g_pause=false;OzAudioPump();HRESULT hr=VT<IAudioClientV>(g_client)->Start(g_client);if(FAILED(hr)){g_play=false;g_pause=false;SetAudioError(L"Start WASAPI",hr);}}
void OzAudioPause(){if(g_native&&g_client){VT<IAudioClientV>(g_client)->Stop(g_client);g_play=false;g_pause=true;}}
void OzAudioStop(){if(g_native&&g_client){VT<IAudioClientV>(g_client)->Stop(g_client);VT<IAudioClientV>(g_client)->Reset(g_client);if(g_streamReader)SeekMFStream(0);else g_frame=0;g_play=false;g_pause=false;ResetEQ();}}
void OzAudioSeekMs(int ms){if(!g_native)return;if(g_streamReader){if(!SeekMFStream(ms))return;}else{unsigned long long f=(unsigned long long)(ms<0?0:ms)*(unsigned)g_rate/1000ULL;if(f>g_pcm.frames)f=g_pcm.frames;g_frame=f;}ResetEQ();if(g_client){VT<IAudioClientV>(g_client)->Stop(g_client);VT<IAudioClientV>(g_client)->Reset(g_client);if(g_play){OzAudioPump();HRESULT hr=VT<IAudioClientV>(g_client)->Start(g_client);if(FAILED(hr)){g_play=false;SetAudioError(L"Restart WASAPI after seek",hr);}}}}
int OzAudioPosMs(){return g_native?(int)(g_frame*1000ULL/(unsigned)g_rate):0;}
int OzAudioLengthMs(){if(!g_native)return 0;if(g_streamReader)return g_streamLengthMs;return (int)(g_pcm.frames*1000ULL/(unsigned)g_rate);}
bool OzAudioPlaying(){return g_native&&g_play;}bool OzAudioPaused(){return g_native&&g_pause;}bool OzAudioNative(){return g_native;}const wchar_t* OzAudioBackend(){return g_backend;}
const wchar_t* OzAudioCurrentDeviceName(){return g_activeDeviceName;}const wchar_t* OzAudioCurrentDeviceId(){return g_activeDeviceId;}int OzAudioOutputRate(){return g_rate;}int OzAudioOutputChannels(){return g_outChannels;}bool OzAudioReady(){return g_client&&g_render;}HRESULT OzAudioLastError(){return g_lastError;}const wchar_t* OzAudioLastErrorStage(){return g_lastErrorStage;}
void OzAudioSetVolume(int p,int bal,bool m){g_vol=p<0?0:(p>100?100:p);g_bal=bal<-100?-100:(bal>100?100:bal);g_mute=m;}
void OzAudioSetEQ(bool en,int pre,const int bands[10]){g_eqOn=en;g_preamp=pre;for(int i=0;i<10;i++)g_band[i]=bands[i];CalcEQ();}
void OzAudioSetReplayGainDb(double db){g_rgDb=db;}
void OzAudioSetCrossfadeMs(int ms){g_crossfadeMs=ms<0?0:(ms>15000?15000:ms);}
void OzAudioClearNext(){Free(g_next.s);g_next.s=0;g_next.frames=0;g_nextFrame=0;g_nextRgDb=0;}
bool OzAudioPrepareNext(const wchar_t*path,double rg){OzAudioClearNext();if(!path||!path[0])return false;PCM p={0,0};bool ok=false;if(EndsI(path,L".wav"))ok=DecodeWav(path,p);else if(EndsI(path,L".mp3")){ok=DecodeMF(path,p);if(!ok)ok=DecodeMP3(path,p);}else ok=DecodeMF(path,p);if(!ok)return false;g_next=p;g_nextFrame=0;g_nextRgDb=rg;return true;}
bool OzAudioConsumeAdvanced(){bool v=g_advanced;g_advanced=false;return v;}

void OzAudioPump(){
 if(!g_native||!g_play||!g_client||!g_render||(!g_pcm.s&&!g_streamReader))return;UINT pad=0;HRESULT phr=VT<IAudioClientV>(g_client)->GetCurrentPadding(g_client,&pad);if(FAILED(phr)){SetAudioError(L"WASAPI device unavailable",phr);return;}if(pad>=g_bufFrames)return;UINT n=g_bufFrames-pad;BYTE*out=0;HRESULT bhr=VT<IAudioRenderClientV>(g_render)->GetBuffer(g_render,n,&out);if(FAILED(bhr)||!out){SetAudioError(L"WASAPI render buffer unavailable",FAILED(bhr)?bhr:(HRESULT)0x80004005L);return;}
 double base=(g_mute?0.0:(double)g_vol/100.0)*pow(10.0,(g_eqOn?g_preamp:0)/20.0);double gl=base,gr=base;if(g_bal<0)gr*=((100.0+g_bal)/100.0);else if(g_bal>0)gl*=((100.0-g_bal)/100.0);bool noMore=false;
 for(UINT i=0;i<n;i++){
  float l=0,r=0;bool haveCur=false;
  if(g_streamReader){
   haveCur=StreamFrame(l,r);unsigned long long total=g_streamLengthMs>0?(unsigned long long)g_streamLengthMs*(unsigned)g_rate/1000ULL:0;unsigned long long remain=(total>g_frame)?(total-g_frame):0;unsigned long long cf=(unsigned long long)g_crossfadeMs*(unsigned)g_rate/1000ULL;bool mixing=(haveCur&&g_next.s&&cf>0&&remain<=cf&&remain>0);double rg1=pow(10.0,g_rgDb/20.0),rg2=pow(10.0,g_nextRgDb/20.0);if(haveCur)g_frame++;
   if(mixing&&g_nextFrame<g_next.frames){double t=1.0-(double)remain/(double)cf;if(t<0)t=0;if(t>1)t=1;float nl=g_next.s[g_nextFrame*2],nr=g_next.s[g_nextFrame*2+1];g_nextFrame++;l=(float)(l*rg1*(1.0-t)+nl*rg2*t);r=(float)(r*rg1*(1.0-t)+nr*rg2*t);}else{l=(float)(l*rg1);r=(float)(r*rg1);}
   if(!haveCur){if(g_next.s){CloseMFStream();g_pcm=g_next;g_next.s=0;g_next.frames=0;g_frame=g_nextFrame;g_nextFrame=0;g_rgDb=g_nextRgDb;g_nextRgDb=0;g_advanced=true;if(g_frame<g_pcm.frames){l=g_pcm.s[g_frame*2];r=g_pcm.s[g_frame*2+1];g_frame++;haveCur=true;noMore=false;}}else noMore=true;}
  }
  else{
   haveCur=g_frame<g_pcm.frames;unsigned long long remain=haveCur?(g_pcm.frames-g_frame):0;unsigned long long cf=(unsigned long long)g_crossfadeMs*(unsigned)g_rate/1000ULL;bool mixing=(g_next.s&&cf>0&&remain<=cf&&remain>0);double rg1=pow(10.0,g_rgDb/20.0),rg2=pow(10.0,g_nextRgDb/20.0);
   if(haveCur){l=g_pcm.s[g_frame*2];r=g_pcm.s[g_frame*2+1];g_frame++;}
   if(mixing&&g_nextFrame<g_next.frames){double t=1.0-(double)remain/(double)cf;if(t<0)t=0;if(t>1)t=1;float nl=g_next.s[g_nextFrame*2],nr=g_next.s[g_nextFrame*2+1];g_nextFrame++;l=(float)(l*rg1*(1.0-t)+nl*rg2*t);r=(float)(r*rg1*(1.0-t)+nr*rg2*t);}else{l=(float)(l*rg1);r=(float)(r*rg1);}
   if(!haveCur){if(g_next.s){Free(g_pcm.s);g_pcm=g_next;g_next.s=0;g_next.frames=0;g_frame=g_nextFrame;g_nextFrame=0;g_rgDb=g_nextRgDb;g_nextRgDb=0;g_advanced=true;if(g_frame<g_pcm.frames){l=g_pcm.s[g_frame*2];r=g_pcm.s[g_frame*2+1];g_frame++;haveCur=true;}}else noMore=true;}
  }
  l=ClampF((float)(RunEQ(l,0)*gl));r=ClampF((float)(RunEQ(r,1)*gr));g_fft[g_fftPos]=(l+r)*0.5f;g_fftPos=(g_fftPos+1)&511;
  if(g_fmtFloat&&g_outBits==32){float*fo=(float*)out;SIZE_T b=(SIZE_T)i*g_outChannels;if(g_outChannels==1)fo[b]=(l+r)*0.5f;else{fo[b]=l;fo[b+1]=r;for(int c=2;c<g_outChannels;c++)fo[b+c]=0.0f;}}
  else if(g_outBits==16){short*so=(short*)out;SIZE_T b=(SIZE_T)i*g_outChannels;if(g_outChannels==1)so[b]=(short)(((l+r)*0.5f)*32767.0f);else{so[b]=(short)(l*32767.0f);so[b+1]=(short)(r*32767.0f);for(int c=2;c<g_outChannels;c++)so[b+c]=0;}}
  else if(g_fmtPCM24&&g_outBits==24){BYTE*fr=out+(SIZE_T)i*g_outBlockAlign;for(int c=0;c<g_outChannels;c++){float x=(c==0?l:(c==1?r:0.0f));if(g_outChannels==1)x=(l+r)*0.5f;int v=(int)(x*8388607.0f);BYTE*q=fr+c*3;q[0]=(BYTE)(v&255);q[1]=(BYTE)((v>>8)&255);q[2]=(BYTE)((v>>16)&255);}}
  else if(g_fmtSigned32&&g_outBits==32){int*io=(int*)out;SIZE_T b=(SIZE_T)i*g_outChannels;double scale=(g_outValidBits==24?8388607.0*256.0:2147483647.0);if(g_outChannels==1)io[b]=(int)(((l+r)*0.5f)*scale);else{io[b]=(int)(l*scale);io[b+1]=(int)(r*scale);for(int c=2;c<g_outChannels;c++)io[b+c]=0;}}
 }
 HRESULT rhr=VT<IAudioRenderClientV>(g_render)->ReleaseBuffer(g_render,n,0);if(FAILED(rhr)){SetAudioError(L"WASAPI release buffer failed",rhr);g_play=false;return;}SetAudioError(L"OK",S_OK);if(noMore){if(g_streamReader&&g_streamLengthMs>0)g_frame=(unsigned long long)g_streamLengthMs*(unsigned)g_rate/1000ULL;VT<IAudioClientV>(g_client)->Stop(g_client);g_play=false;}
}

static bool ScanMFIncremental(const wchar_t*path,double&ss,double&pk,unsigned long long&count){
 if(!g_mfStarted||!path||!path[0])return false;void*reader=0;if(FAILED(MFCreateSourceReaderFromURL(path,0,&reader))||!reader)return false;IMFSourceReaderV*rv=VT<IMFSourceReaderV>(reader);rv->SetStreamSelection(reader,MF_SOURCE_READER_ALL_STREAMS,FALSE);rv->SetStreamSelection(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE);
 void*mt=0;if(FAILED(MFCreateMediaType(&mt))||!mt){Rel(reader);return false;}IMFAttributesV*av=VT<IMFAttributesV>(mt);av->SetGUID(mt,&G_MF_MT_MAJOR_TYPE,&G_MFMediaType_Audio);av->SetGUID(mt,&G_MF_MT_SUBTYPE,&G_KSDATAFORMAT_SUBTYPE_PCM);av->SetUINT32(mt,&G_MF_MT_AUDIO_BITS_PER_SAMPLE,16);HRESULT hr=rv->SetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mt);if(FAILED(hr)){Rel(mt);mt=0;if(FAILED(MFCreateMediaType(&mt))||!mt){Rel(reader);return false;}av=VT<IMFAttributesV>(mt);av->SetGUID(mt,&G_MF_MT_MAJOR_TYPE,&G_MFMediaType_Audio);av->SetGUID(mt,&G_MF_MT_SUBTYPE,&G_KSDATAFORMAT_SUBTYPE_PCM);hr=rv->SetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,mt);}Rel(mt);if(FAILED(hr)){Rel(reader);return false;}
 mt=0;if(FAILED(rv->GetCurrentMediaType(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,&mt))||!mt){Rel(reader);return false;}av=VT<IMFAttributesV>(mt);UINT ch=0,bits=0;GUID sub;memset(&sub,0,sizeof(sub));av->GetUINT32(mt,&G_MF_MT_AUDIO_NUM_CHANNELS,&ch);av->GetUINT32(mt,&G_MF_MT_AUDIO_BITS_PER_SAMPLE,&bits);av->GetGUID(mt,&G_MF_MT_SUBTYPE,&sub);bool pcm=memcmp(&sub,&G_KSDATAFORMAT_SUBTYPE_PCM,sizeof(GUID))==0;bool flt=memcmp(&sub,&G_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT,sizeof(GUID))==0||memcmp(&sub,&G_MFAudioFormat_Float,sizeof(GUID))==0;Rel(mt);if(ch<1||ch>16||(!pcm&&!flt)||(bits!=16&&bits!=32)){Rel(reader);return false;}
 bool eos=false,ok=true;unsigned guard=0;while(!eos&&guard++<4000000u){DWORD actual=0,flags=0;long long ts=0;void*sample=0;hr=rv->ReadSample(reader,MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,&actual,&flags,&ts,&sample);if(FAILED(hr)||(flags&MF_SOURCE_READERF_ERROR)){ok=false;if(sample)Rel(sample);break;}if(flags&MF_SOURCE_READERF_ENDOFSTREAM)eos=true;if(sample){void*mb=0;IMFSampleV*sv=VT<IMFSampleV>(sample);if(SUCCEEDED(sv->ConvertToContiguousBuffer(sample,&mb))&&mb){BYTE*src=0;DWORD mx=0,cur=0;IMFMediaBufferV*bv=VT<IMFMediaBufferV>(mb);if(SUCCEEDED(bv->Lock(mb,&src,&mx,&cur))&&src&&cur){if(bits==16){unsigned long long n=cur/2;short*q=(short*)src;for(unsigned long long i=0;i<n;i++){double x=q[i]/32768.0;ss+=x*x;double a=fabs(x);if(a>pk)pk=a;}count+=n;}else if(flt){unsigned long long n=cur/4;float*q=(float*)src;for(unsigned long long i=0;i<n;i++){double x=q[i];ss+=x*x;double a=fabs(x);if(a>pk)pk=a;}count+=n;}else{unsigned long long n=cur/4;int*q=(int*)src;for(unsigned long long i=0;i<n;i++){double x=q[i]/2147483648.0;ss+=x*x;double a=fabs(x);if(a>pk)pk=a;}count+=n;}bv->Unlock(mb);}Rel(mb);}Rel(sample);}}
 Rel(reader);return ok&&count>0;
}
double OzAudioScanReplayGain(const wchar_t*path,double*peakOut){double ss=0,pk=0;unsigned long long count=0;if(ScanMFIncremental(path,ss,pk,count)){double rms=sqrt(ss/(double)count);if(peakOut)*peakOut=pk;if(rms<1e-9)return 0;double db=20.0*log10(rms),gain=-18.0-db;if(gain>12)gain=12;if(gain<-12)gain=-12;return gain;}PCM p={0,0};bool ok=EndsI(path,L".wav")?DecodeWav(path,p):(EndsI(path,L".mp3")?DecodeMP3(path,p):false);if(!ok||!p.s){if(peakOut)*peakOut=0;return 0;}count=p.frames*2;for(unsigned long long i=0;i<count;i++){double x=p.s[i];ss+=x*x;double a=fabs(x);if(a>pk)pk=a;}double rms=count?sqrt(ss/(double)count):0;Free(p.s);if(peakOut)*peakOut=pk;if(rms<1e-9)return 0;double db=20.0*log10(rms),gain=-18.0-db;if(gain>12)gain=12;if(gain<-12)gain=-12;return gain;}

void OzAudioSpectrum(float*out,int bins){if(!out||bins<=0)return;int N=256;if(bins>64)bins=64;for(int k=0;k<bins;k++){double re=0,im=0;double step=2.0*PI*(double)(k+1)/(double)N;double cs=cos(step),sn=sin(step),cr=1.0,ci=0.0;for(int j=0;j<N;j++){int ix=(g_fftPos-N+j)&511;double w=0.5-0.5*cos(2.0*PI*j/(N-1));double x=g_fft[ix]*w;re+=x*cr;im-=x*ci;double nr=cr*cs-ci*sn;ci=ci*cs+cr*sn;cr=nr;}double mag=sqrt(re*re+im*im)/(N*0.5);double db=20.0*log10(mag+1e-7);double v=(db+60.0)/60.0;if(v<0)v=0;if(v>1)v=1;out[k]=(float)v;}for(int k=bins;k<64;k++)out[k]=0;}

void OzAudioShutdown(){OzAudioUnload();ReleaseAudio();if(g_platformStarted){if(g_mfStarted)MFShutdown();if(g_comOwned)CoUninitialize();g_mfStarted=false;g_comOwned=false;g_platformStarted=false;}}

void OzAudioWaveform(float*out,int count){if(!out||count<=0)return;if(count>512)count=512;int step=512/count;if(step<1)step=1;for(int i=0;i<count;i++){int ix=(g_fftPos-512+i*step)&511;out[i]=g_fft[ix];}}
