#ifndef SOUND_H
#define SOUND_H

#define INPUTCHANNELS 1
#define OUTPUTCHANNELS 8
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p) = nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

typedef ALboolean(*EAXSetBufferMode)(ALsizei, ALuint*, ALint);

typedef ALenum(*EAXGetBufferMode)(ALuint, ALint*);


class Xaudio2 {
public:
	Xaudio2(IXAudio2* p);
	~Xaudio2();
	void CreateSource(std::string filename);
	void CreateSource(std::string filename, int loop);
	void Create3DPositionalSource(std::string filename, int loop, IXAudio2MasteringVoice* pMasterVoice);
	void Play();
	void LoopPlay();
	void Stop();
	void setVolume(float volume) { pSourceVoice->SetVolume(volume); }
	float getVolume() { float v; pSourceVoice->GetVolume(&v); return v; }
	void initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ) { lisener->Position = { listenerX, lisenerY, listenerZ }; emitter->Position = { emitterX, emitterY, emitterZ }; }
	void X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime);

	const char* GetI3DL2_Name(int index) { return I3DL2_Name[index]; }
	//IXAudio2* GetIXAudio2Ptr() { return pXAudio2; }
	//IXAudio2SubmixVoice* GetSubmixVoicePtr() { return pSubmixVoice; }
	
	void SetEffectParameters(int I3DL2);
private:
	//pointers that received from others, don't need release in destructor
	IXAudio2* pXAudio2;
	IXAudio2MasteringVoice* pMasterVoice;

	//pointers that created in this class, need release in destructor
	IXAudio2SubmixVoice* pSubmixVoice;
	IXAudio2SourceVoice* pSourceVoice;
	
	char* wfx = nullptr;
	XAUDIO2_BUFFER buffer = { 0 };
	HRESULT hr;

	X3DAUDIO_HANDLE X3DInstance;
	X3DAUDIO_LISTENER* lisener;
	X3DAUDIO_EMITTER* emitter;
	X3DAUDIO_CONE* Listener_DirectionalCone;
	X3DAUDIO_CONE* emitterCone;
	X3DAUDIO_DISTANCE_CURVE_POINT* Emitter_LFE_CurvePoints;
	X3DAUDIO_DISTANCE_CURVE* Emitter_LFE_Curve;
	X3DAUDIO_DISTANCE_CURVE_POINT* Emitter_Reverb_CurvePoints;
	X3DAUDIO_DISTANCE_CURVE* Emitter_Reverb_Curve;
	X3DAUDIO_DSP_SETTINGS* dsp_setting;

	float* matrixCoefficients;
	float* emitterAzimuths;
	bool UseRedirectToLFE;
	bool LoadFile(std::string filename, char*& wfx);
	void initial3DSoundCone();
	void initial3D_LFE_Curve();
	void initial3D_Reverb_Curve();
	void X3Dcleanup();
	static XAUDIO2FX_REVERB_I3DL2_PARAMETERS I3DL2_Reverb[30];
	static const char* I3DL2_Name[30];
};
class OpenAL {
public:
	OpenAL();
	~OpenAL();
	void CreateSource(std::string filename);
	void CreateSource(std::string filename, int loop);
	void Create3DPositionalSource(std::string filename, int loop, void* p);
	void Play();
	void LoopPlay();
	void Stop();
	void setVolume(float volume);
	float getVolume();
	void initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ);
	void X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime);
	bool LoadFile(std::string szFile, ALvoid*& data, ALenum& format, ALsizei& size, ALfloat& freq);
private:
	typedef struct  {
		DirectX::XMFLOAT3 Velocity;
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 At;
		DirectX::XMFLOAT3 Up;
	} OpenALObject;

	OpenALObject* listener;
	OpenALObject* emitter;
	ALenum error;
	ALenum format;
	ALsizei size = 0;
	ALfloat freq = 0.0f;
	ALvoid* data;
	ALuint buffers[1];
	ALuint sources[1];
	ALvoid DisplayALError(std::string text, ALint errorcode);
	ALvoid DisplayALUTError(std::string text, ALint errorcode);
};

class SoundDevice {
public:
	SoundDevice(int sound_api);
	~SoundDevice();
	int GetSoundAPI() { return sound_api; }
	void* GetDevicePtr() { return pAudio; }
	IXAudio2MasteringVoice* GetMasterVoice() { return pMasterVoice; }
private:
	void* pAudio;
	int sound_api;

	IXAudio2MasteringVoice* pMasterVoice;
	HRESULT hr;
	ALCcontext* pContext;
	ALint attribs[4];
	ALCint iSends;


};

//Sound class now is just a wrapper that keep old interface that wraps both xaudio2 and openal Source(Emitter)Voice class
class Sound {
public:
	Sound(SoundDevice* pDevice, std::string filename);//infinite looping background flat music without any effect
	Sound(SoundDevice* pDevice, std::string filename, int loop);//no looping or specify looping times flat wave file play (door sound)
	Sound(SoundDevice* pDevice, std::string filename, int loop, int dummy);//for 3D positional sound with LPF Direct path and LPF Reverb Path
	~Sound();
	void Play();
	void LoopPlay();
	void Stop();

	void setVolume(float volume);
	float getVolume();
	void initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ);
	void PositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime);
	
private:
	void* pAudio;
	void* pSource;
	int sound_api;

};


class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	VoiceCallback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
	VoiceCallback(bool* playing) { control = playing; };
	~VoiceCallback() { CloseHandle(hBufferEndEvent); }

	//Called when the voice has just finished playing a contiguous audio stream.
	void OnStreamEnd() { *control = false; SetEvent(hBufferEndEvent); }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
	void OnBufferEnd(void * pBufferContext) { }
	void OnBufferStart(void * pBufferContext) {    }
	void OnLoopEnd(void * pBufferContext) {    }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { }
private:
	bool* control;
};

#endif
