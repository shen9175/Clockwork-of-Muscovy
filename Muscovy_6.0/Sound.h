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
	Sound(int sound_api, std::string filename);//infinite looping background flat music without any effect
	Sound(int sound_api, std::string filename, int loop);//no looping or specify looping times flat wave file play (door sound)
	Sound(int sound_api, std::string filename, int loop, int I3DL2);//for 3D positional sound with LPF Direct path and LPF Reverb Path
	~Sound();
	void Play();
	void LoopPlay();
	void Stop();
	void setVolume(float volume) { pSourceVoice->SetVolume(volume); }
	float getVolume() { float v; pSourceVoice->GetVolume(&v); return v; }
	void initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ) { lisener->Position = { listenerX, lisenerY, listenerZ }; emitter->Position = {emitterX, emitterY, emitterZ}; }
	void X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime);
	void SetEffectParameters(int I3DL2);
	const char* GetI3DL2_Name(int index) { return I3DL2_Name[index]; }
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
	void common_init();

	XAUDIO2FX_REVERB_I3DL2_PARAMETERS I3DL2_Reverb[30] =
	{
		XAUDIO2FX_I3DL2_PRESET_DEFAULT,
		XAUDIO2FX_I3DL2_PRESET_GENERIC,
		XAUDIO2FX_I3DL2_PRESET_FOREST,
		XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
		XAUDIO2FX_I3DL2_PRESET_ROOM,
		XAUDIO2FX_I3DL2_PRESET_BATHROOM,
		XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
		XAUDIO2FX_I3DL2_PRESET_STONEROOM,
		XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
		XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
		XAUDIO2FX_I3DL2_PRESET_CAVE,
		XAUDIO2FX_I3DL2_PRESET_ARENA,
		XAUDIO2FX_I3DL2_PRESET_HANGAR,
		XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
		XAUDIO2FX_I3DL2_PRESET_HALLWAY,
		XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
		XAUDIO2FX_I3DL2_PRESET_ALLEY,
		XAUDIO2FX_I3DL2_PRESET_CITY,
		XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
		XAUDIO2FX_I3DL2_PRESET_QUARRY,
		XAUDIO2FX_I3DL2_PRESET_PLAIN,
		XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
		XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
		XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
		XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
		XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
		XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
		XAUDIO2FX_I3DL2_PRESET_PLATE,
	};

	const char* I3DL2_Name[30] =
	{
		"Default",
		"Generic",
		"Forest",
		"Padded cell",
		"Room",
		"Bathroom",
		"Living room",
		"Stone room",
		"Auditorium",
		"Concert hall",
		"Cave",
		"Arena",
		"Hangar",
		"Carpeted hallway",
		"Hallway",
		"Stone Corridor",
		"Alley",
		"City",
		"Mountains",
		"Quarry",
		"Plain",
		"Parking lot",
		"Sewer pipe",
		"Underwater",
		"Small room",
		"Medium room",
		"Large room",
		"Medium hall",
		"Large hall",
		"Plate",
	};


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
