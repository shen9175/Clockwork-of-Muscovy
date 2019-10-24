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
class Sound {
public:
	Sound(IXAudio2* pXAudio2, std::string filename);//infinite looping background flat music without any effect
	Sound(IXAudio2* pXAudio2, std::string filename, int loop);//no looping or specify looping times flat wave file play (door sound)
	Sound(IXAudio2* pXAudio2, std::string filename, int loop, IXAudio2MasteringVoice* pMasterVoice, IXAudio2SubmixVoice* pSubmixVoice);//for 3D positional sound with LPF Direct path and LPF Reverb Path
	~Sound();
	void Play();
	void LoopPlay();
	void Stop();
	void setVolume(float volume) { pSourceVoice->SetVolume(volume); }
	float getVolume() { float v; pSourceVoice->GetVolume(&v); return v; }
	void initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ) { lisener->Position = { listenerX, lisenerY, listenerZ }; emitter->Position = {emitterX, emitterY, emitterZ}; }
	void X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime);

private:
	IXAudio2* pXAudio2;
	IXAudio2SourceVoice* pSourceVoice;
	IXAudio2SubmixVoice* pSubmixVoice;
	IXAudio2MasteringVoice* pMasterVoice;
	char* wfx = nullptr;
	XAUDIO2_BUFFER buffer = { 0 };
	HRESULT hr;
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	XAUDIO2_VOICE_DETAILS MasterVoiceDetails;
#else
	XAUDIO2_DEVICE_DETAILS DeviceDetails;
#endif
	X3DAUDIO_HANDLE X3DInstance;
	X3DAUDIO_LISTENER* lisener;
	X3DAUDIO_EMITTER* emitter;
	X3DAUDIO_CONE* Listener_DirectionalCone;
	X3DAUDIO_CONE* emitterCone;
	X3DAUDIO_DISTANCE_CURVE_POINT* Emitter_LFE_CurvePoints;
	X3DAUDIO_DISTANCE_CURVE*       Emitter_LFE_Curve;
	X3DAUDIO_DISTANCE_CURVE_POINT* Emitter_Reverb_CurvePoints;
	X3DAUDIO_DISTANCE_CURVE*       Emitter_Reverb_Curve;
	X3DAUDIO_DSP_SETTINGS* dsp_setting;

	float* matrixCoefficients;
	float* emitterAzimuths;
	bool UseRedirectToLFE;
	bool LoadFile(std::string filename, char*& wfx);
	void initial3DSoundCone();
	void initial3D_LFE_Curve();
	void initial3D_Reverb_Curve();
	void X3Dcleanup();

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
