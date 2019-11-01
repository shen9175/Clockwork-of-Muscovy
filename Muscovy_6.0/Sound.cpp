#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <DirectXMath.h>
#else
#include <XNAMath.h>
#endif

#include <windows.h>
#include <iostream>
#include <fstream>
#include <memory>



#if defined(_MSC_VER)
#include <al.h>
#include <alc.h>
#include <efx.h>
#include <alut.h>
#elif defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/efx.h>
#include <OpenAL/alut.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/alut.h>
#endif

#include "Sound.h"

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	/*•The DirectX SDK version of XAudio2 used CoCreateInstance and was registered with COM, and required an explicit Initialize method to be called. 
	The DirectX SDK  'xaudio2.h' header contained a wrapper inline function XAudio2Create that did this to simplify portability with the Xbox 360 version. 
	For the Windows 8 version, you don't use COM creation and instead call the XAudio2Create function and link with "xaudio2.lib".*/
	#pragma comment(lib,"xaudio2.lib")
#else
	#pragma comment(lib,"x3daudio.lib")
#endif

#pragma comment(lib, "openal32.lib")
#pragma comment(lib, "alut.lib")

#pragma comment(lib, "Winmm.lib")
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
using namespace DirectX;
#endif

SoundDevice::SoundDevice(int sound_api) : sound_api(sound_api) {
	if (sound_api == 1) {
	CoInitializeEx(nullptr, 0);
	IXAudio2* pXAudio2;
	if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		MessageBox(nullptr, "Creating XAudio2 Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, nullptr, nullptr))) {
		MessageBox(nullptr, "Creating MasteringVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#else
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)/*, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, nullptr)*/)) {
		char buffer[256];
		sprintf_s(buffer, "Creating MasteringVoice Failed! %x, %d", hr, hr);
		MessageBox(nullptr, buffer, "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#endif
		pAudio = pXAudio2;

	} else if (sound_api == 2) {
		ALCdevice* pDevice = nullptr;
		if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT") != AL_FALSE) {
			const ALchar* defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
			pContext = nullptr;
			attribs[4] = { 0 };

			pDevice = alcOpenDevice(defaultDeviceName);
			if (pDevice) {
				attribs[0] = ALC_MAX_AUXILIARY_SENDS;
				attribs[1] = 4;
				pContext = alcCreateContext(pDevice, attribs);
				if (pContext) {
					alcMakeContextCurrent(pContext);
				} else {
					MessageBox(nullptr, "Context creation failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
				}
			} else {
				MessageBox(nullptr, "OpenAL Device not found!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			}
		} else {
			MessageBox(nullptr, "OpenAL Device not found!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		}
		pAudio = pDevice;
	}
}
SoundDevice::~SoundDevice() {
	if (sound_api == 1) {
		pMasterVoice->DestroyVoice();
		reinterpret_cast<IXAudio2*>(pAudio)->Release();
		CoUninitialize();
	} else if (sound_api == 2) {
		alcDestroyContext(pContext);
		alcCloseDevice(reinterpret_cast<ALCdevice*>(pAudio));
	}
}
OpenAL::OpenAL() {
	alutInit(nullptr, nullptr);
	alGetError(); // clear error code 
	listener = new OpenALObject;
	emitter = new OpenALObject;
}


ALvoid OpenAL::DisplayALError(std::string text, ALint errorcode) {
	text += " : ";
	text += reinterpret_cast<const char*>(alGetString(errorcode));
	MessageBox(nullptr, text.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
}
ALvoid OpenAL::DisplayALUTError(std::string text, ALint errorcode) {
	text += " : ";
	text += reinterpret_cast<const char*>(alutGetErrorString(errorcode));
	MessageBox(nullptr, text.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
}


OpenAL::~OpenAL() {
	if (listener) {
		delete listener;
		listener = nullptr;
	}
	if (emitter) {
		delete emitter;
		emitter = nullptr;
	}
	alDeleteSources(1, sources);
	alDeleteBuffers(1, buffers);
	alutExit();

}
void OpenAL::CreateSource(std::string filename) {
	
	if (alIsExtensionPresent("EAX-RAM") == AL_TRUE) {
		EAXSetBufferMode g_eaxSetMode;

		g_eaxSetMode = reinterpret_cast<EAXSetBufferMode>(alGetProcAddress("EAXSetBufferMode"));

		if (g_eaxSetMode) {
			g_eaxSetMode(1, buffers, alGetEnumValue("AL_STORAGE_HARDWARE"));
		}
	}

		alGenBuffers(1, buffers);
		ALenum error;
		ALenum format;
		ALvoid* data;
		ALsizei size = 0;
		//ALsizei freq = 0;
		ALfloat freq = 0.0f;
		if ((error = alGetError()) == AL_NO_ERROR) {

		} else {
			DisplayALError("alGenBuffers :", error);
		}
		alGetError(); // clear error code 


		if (!LoadFile(filename, data, format, size, freq)) {
			// Copy test.wav data into AL Buffer 0 
			alBufferData(buffers[0], format, data, size, static_cast<ALsizei>(freq));
			if ((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("alBufferData buffer 0 : ", error);
			}
		} else {
			std::string display = "Load " + filename + " error!";
			MessageBox(nullptr, display.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
			alDeleteBuffers(1, buffers);
		}



		delete[]data;
		data = nullptr;
		
		alGenSources(1, sources);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alGenSources 1 : ", error);
		}

		// Attach buffer 0 to source - 10 - 
		alSourcei(sources[0], AL_BUFFER, buffers[0]);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alSourcei AL_BUFFER 0 : ", error);
		}

	
}

void OpenAL::CreateSource(std::string filename, int loop) {
	
	if (alIsExtensionPresent("EAX-RAM") == AL_TRUE) {
		EAXSetBufferMode g_eaxSetMode;

		g_eaxSetMode = reinterpret_cast<EAXSetBufferMode>(alGetProcAddress("EAXSetBufferMode"));

		if (g_eaxSetMode) {
			g_eaxSetMode(1, buffers, alGetEnumValue("AL_STORAGE_HARDWARE"));
		}

	}
		alGenBuffers(1, buffers);
		ALenum error;
		ALenum format;
		ALvoid* data;
		ALsizei size = 0;
		//ALsizei freq = 0;
		ALfloat freq = 0.0f;
		if ((error = alGetError()) == AL_NO_ERROR) {

		} else {
			DisplayALError("alGenBuffers :", error);
		}
		alGetError(); // clear error code 

		if (!LoadFile(filename, data, format, size, freq)) {
			// Copy test.wav data into AL Buffer 0 
			alBufferData(buffers[0], format, data, size, static_cast<ALsizei>(freq));
			if ((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("alBufferData buffer 0 : ", error);
			}
		} else {
			std::string display = "Load " + filename + " error!";
			MessageBox(nullptr, display.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
			alDeleteBuffers(1, buffers);
		}



		delete[]data;
		data = nullptr;

		alGenSources(1, sources);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alGenSources 1 : ", error);
		}

		// Attach buffer 0 to source - 10 - 
		alSourcei(sources[0], AL_BUFFER, buffers[0]);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alSourcei AL_BUFFER 0 : ", error);
		}

		alSourcei(sources[0], AL_LOOPING, AL_TRUE);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alSourcei AL_BUFFER 0 : ", error);
		}
	
}
void OpenAL::Create3DPositionalSource(std::string filename, int loop, void* p) {
	
	if (alIsExtensionPresent("EAX-RAM") == AL_TRUE) {
		EAXSetBufferMode g_eaxSetMode;

		g_eaxSetMode = reinterpret_cast<EAXSetBufferMode>(alGetProcAddress("EAXSetBufferMode"));

		if (g_eaxSetMode) {
			g_eaxSetMode(1, buffers, alGetEnumValue("AL_STORAGE_HARDWARE"));
		}
	}

		alGenBuffers(1, buffers);
		ALenum error;
		ALenum format;
		ALvoid* data;
		ALsizei size = 0;
		//ALsizei freq = 0;
		ALfloat freq = 0.0f;
		if ((error = alGetError()) == AL_NO_ERROR) {

		} else {
			DisplayALError("alGenBuffers :", error);
		}
		alGetError(); // clear error code 
		

		if (!LoadFile(filename, data, format, size, freq)) {
				// Copy test.wav data into AL Buffer 0 
			alBufferData(buffers[0], format, data, size, static_cast<ALsizei>(freq));
			if ((error = alGetError()) != AL_NO_ERROR) {
				DisplayALError("alBufferData buffer 0 : ", error);
			}
		} else {
			std::string display = "Load " + filename + " error!";
			MessageBox(nullptr, display.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
			alDeleteBuffers(1, buffers);
		}


		delete []data;
		data = nullptr;

		alGenSources(1, sources);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alGenSources 1 : ", error);
		}

		// Attach buffer 0 to source - 10 - 
		alSourcei(sources[0], AL_BUFFER, buffers[0]);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alSourcei AL_BUFFER 0 : ", error);
		}

		alSourcei(sources[0], AL_LOOPING, AL_TRUE);
		if ((error = alGetError()) != AL_NO_ERROR) {
			DisplayALError("alSourcei AL_BUFFER 0 : ", error);
		}
	
}
void OpenAL::Play() {
//	ALint playstate = 0;
	alSourcePlay(sources[0]);
//	alGetSourcei(sources[0], AL_SOURCE_STATE, &playstate);

//	while (playstate == AL_PLAYING) {
//		alutSleep(2);
//		alGetSourcei(sources[0], AL_SOURCE_STATE, &playstate);
//	}


}
void OpenAL::LoopPlay() {
	alSourcePlay(sources[0]);
}
void OpenAL::Stop() {
}
void OpenAL::setVolume(float volume) {
	alSourcef(sources[0], AL_GAIN, volume);
}
float OpenAL::getVolume() {
	float ret;
	alGetSourcef(sources[0], AL_GAIN, &ret);
	return ret;
}
void OpenAL::initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ) {
	alListener3f(AL_POSITION, listenerX, lisenerY, listenerZ);
	alSource3f(sources[0], AL_POSITION, emitterX, emitterY, emitterZ);
}
void OpenAL::X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime) {
	XMVECTOR v1 = XMVectorSet(listenerX, lisenerY, listenerZ, 0.0f);//new listener positions
	XMVECTOR v2 = XMVectorSet(listener->Position.x, listener->Position.y, listener->Position.z, 0);//old listener positions
	XMVECTOR velocity = (v1 - v2) / elaspedtime;
	XMFLOAT3 temp;
	XMStoreFloat3(&temp, velocity);
	listener->Position = { listenerX, lisenerY, listenerZ };//update new listener positions
	listener->Velocity.x = temp.x;
	listener->Velocity.y = temp.y;
	listener->Velocity.z = temp.z;

	v1 = XMVectorSet(emitterX, emitterY, emitterZ, 0.0f);//new emitter positions
	v2 = XMVectorSet(emitter->Position.x, emitter->Position.y, emitter->Position.z, 0);//old emitter positions
	velocity = (v1 - v2) / elaspedtime;
	XMStoreFloat3(&temp, velocity);
	emitter->Position = { emitterX, emitterY, emitterZ };//update new emitter positions
	emitter->Velocity.x = temp.x;
	emitter->Velocity.y = temp.y;
	emitter->Velocity.z = temp.z;
	alListener3f(AL_POSITION, listenerX, lisenerY, listenerZ);
	alListener3f(AL_VELOCITY, listener->Velocity.x, listener->Velocity.y, listener->Velocity.z);
	ALfloat listenerOri[] = { listener->At.x, listener->At.y, listener->At.z, listener->Up.x, listener->Up.y, listener->Up.z };
	alListenerfv(AL_ORIENTATION, listenerOri);
	alSource3f(sources[0], AL_POSITION, emitterX, emitterY, emitterZ);
	alSource3f(sources[0], AL_VELOCITY, emitterX, emitterY, emitterZ);
	ALfloat SourceOri[] = { emitter->At.x, emitter->At.y, emitter->At.z, emitter->Up.x, emitter->Up.y, emitter->Up.z };
	alSourcefv(sources[0], AL_ORIENTATION, SourceOri);
}

bool OpenAL::LoadFile(std::string szFile, ALvoid*& data, ALenum& format, ALsizei& size, ALfloat& freq) {
	if (szFile.empty())
		return false;

	std::ifstream inFile(szFile, std::ios::binary | std::ios::in);
	if (inFile.fail())
		return false;

	DWORD dwChunkId = 0, dwFileSize = 0, dwChunkSize = 0, dwExtra = 0;

	//look for 'RIFF' chunk identifier
	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
	if (dwChunkId != 'FFIR') {
		inFile.close();
		return false;
	}
	inFile.seekg(4, std::ios::beg); //get file size
	inFile.read(reinterpret_cast<char*>(&dwFileSize), sizeof(dwFileSize));
	if (dwFileSize <= 16) {
		inFile.close();
		return false;
	}
	inFile.seekg(8, std::ios::beg); //get file format
	inFile.read(reinterpret_cast<char*>(&dwExtra), sizeof(dwExtra));
	if (dwExtra != 'EVAW') {
		inFile.close();
		return false;
	}
	char* wfx = nullptr;
	//look for 'fmt ' chunk id
	bool bFilledFormat = false;
	for (unsigned int i = 12; i < dwFileSize; ) {
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + 4, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkSize), sizeof(dwChunkSize));
		if (dwChunkId == ' tmf') {
			//wfx could be any type of wave, depend on the dwChunkSize: 16 bytes->PCMWAVEFORMAT or 18bytes->WAVEFORMATEX/PCMWAVEFORMAT
			//or 40bytes-> WAVEFORMATEXTENSIBLE  or 50bytes->ADPCMWAVEFORMAT 
			//we just put it in the bytes count memory area and don't need to know what ever it is.
			// every xxWAVEFORAMxx first 16 bytes contains the same thing.
			// Microsoft CreateSourceVoice need WAVEFORMEX*, just reintepret_cast to it. But if you only use WAVEFORAMEX or WAVEFORAMEXTENSIBLE to load this information,
			// Some of wave files will fail either in CreateSourceVoice or SubmitSourceBuffer method
			//only in this way could read and worked on 4-bit mono ADPCM wav file. Microsoft's 2 fancy sample loading codes file don't work at all at this kind of file.

			wfx = new char[dwChunkSize];
			inFile.seekg(i + 8, std::ios::beg);
			inFile.read(wfx, dwChunkSize);
			bFilledFormat = true;
			break;
		}
		dwChunkSize += 8; //add offsets of the chunk id, and chunk size data entries
		dwChunkSize += 1;
		dwChunkSize &= 0xfffffffe; //guarantees WORD padding alignment
		i += dwChunkSize;
	}
	if (!bFilledFormat) {
		inFile.close();
		return false;
	}
	

	/*
	struct FMTCHUNK {
		short format;
		short channels;
		DWORD srate;
		DWORD bps;
		short balign;
		short samp;
	};

	bitRate = fmt.samp;
	freqRate = (float)fmt.srate;
	channels = fmt.channels;
	*/
	typedef struct {
		WORD        wFormatTag;         // format type
		WORD        nChannels;          // number of channels (i.e. mono, stereo...)
		DWORD       nSamplesPerSec;     // frequency
		DWORD       nAvgBytesPerSec;    // for buffer estimation
		WORD        nBlockAlign;        // block size of data
		WORD        wBitsPerSample;     // number of bits per sample of mono data
		WORD        cbSize;             // the count in bytes of the size of
										// extra information (after cbSize)
	} waveformat;
	

	if (reinterpret_cast<waveformat*>(wfx)->wBitsPerSample == 16) {
		format = reinterpret_cast<waveformat*>(wfx)->nChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	} else if (reinterpret_cast<waveformat*>(wfx)->wBitsPerSample == 8) {
		format = reinterpret_cast<waveformat*>(wfx)->nChannels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	} else {
		format = AL_FORMAT_MONO8;
	}

	//format = static_cast<ALenum>(reinterpret_cast<waveformat*>(wfx)->wFormatTag);
	freq = static_cast<ALfloat>(reinterpret_cast<waveformat*>(wfx)->nSamplesPerSec);
	//format = static_cast<ALenum>(reinterpret_cast<FMTCHUNK*>(wfx)->format);
	//freq = static_cast<ALfloat>(reinterpret_cast<FMTCHUNK*>(wfx)->srate);
	//format = static_cast<ALenum>(wfx.format);
	//freq = static_cast<ALfloat>(wfx.srate);

	delete []wfx;
	wfx = nullptr;
	char* pDataBuffer;
	//look for 'data' chunk id
	bool bFilledData = false;
	for (unsigned int i = 12; i < dwFileSize; ) {
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + 4, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkSize), sizeof(dwChunkSize));
		if (dwChunkId == 'atad') {
			//WAVEFORMATEX* temp = reinterpret_cast<WAVEFORMATEX*>(wfx);//to calculate the lengtho of the time, need the nAvgBytesPerSec in WAVEFORMATEX
			//length = dwChunkSize / static_cast<float>(temp->nAvgBytesPerSec);
			
			pDataBuffer = new char[dwChunkSize];
			inFile.seekg(i + 8, std::ios::beg);
			inFile.read(pDataBuffer, dwChunkSize);
			size = dwChunkSize;
			data = reinterpret_cast<ALvoid*>(pDataBuffer);
			break;
		}
		dwChunkSize += 8; //add offsets of the chunk id, and chunk size data entries
		dwChunkSize += 1;
		dwChunkSize &= 0xfffffffe; //guarantees WORD padding alignment
		i += dwChunkSize;
	}
	if (!bFilledData) {
		inFile.close();
		return false;
	}

	inFile.close();
	return true;
}

Xaudio2::Xaudio2(IXAudio2* p) {
	pXAudio2 = p;
}

XAUDIO2FX_REVERB_I3DL2_PARAMETERS Xaudio2::I3DL2_Reverb[30] =
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
const char* Xaudio2::I3DL2_Name[30] =
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
void Xaudio2::SetEffectParameters(int I3DL2) {
	XAUDIO2FX_REVERB_PARAMETERS reverbParameters;
	ReverbConvertI3DL2ToNative(&I3DL2_Reverb[(I3DL2 % 30 + 30) % 30], &reverbParameters);//(I3DL2 % 30 + 30) % 30 positive modulo
	pSubmixVoice->SetEffectParameters(0, &reverbParameters, sizeof(reverbParameters));
}



Xaudio2::~Xaudio2() {

	if (pSourceVoice) {
		pSourceVoice->DestroyVoice();//must destroy voice or it will crash when quit
	}

	if (wfx) {
		delete[]wfx;
		wfx = nullptr;
	}
	if (buffer.pAudioData) {
		delete[]buffer.pAudioData;
		buffer.pAudioData = nullptr;
	}
	X3Dcleanup();
	if (pSubmixVoice) {
		pSubmixVoice->DestroyVoice();
	}
}
bool Xaudio2::LoadFile(std::string szFile, char* &wfx) {
	if(szFile.empty())
		return false;

	std::ifstream inFile(szFile, std::ios::binary | std::ios::in);
	if(inFile.fail())
		return false;

	DWORD dwChunkId = 0, dwFileSize = 0, dwChunkSize = 0, dwExtra = 0;

	//look for 'RIFF' chunk identifier
	inFile.seekg(0, std::ios::beg);
	inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
	if(dwChunkId != 'FFIR')
	{
		inFile.close();
		return false;
	}
	inFile.seekg(4, std::ios::beg); //get file size
	inFile.read(reinterpret_cast<char*>(&dwFileSize), sizeof(dwFileSize));
	if(dwFileSize <= 16)
	{
		inFile.close();
		return false;
	}
	inFile.seekg(8, std::ios::beg); //get file format
	inFile.read(reinterpret_cast<char*>(&dwExtra), sizeof(dwExtra));
	if(dwExtra != 'EVAW')
	{
		inFile.close();
		return false;
	}

	//look for 'fmt ' chunk id
	bool bFilledFormat = false;
	for(unsigned int i = 12; i < dwFileSize; )
	{
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + 4, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkSize), sizeof(dwChunkSize));
		if(dwChunkId == ' tmf')
		{
			//wfx could be any type of wave, depend on the dwChunkSize: 16 bytes->PCMWAVEFORMAT or 18bytes->WAVEFORMATEX/PCMWAVEFORMAT
			//or 40bytes-> WAVEFORMATEXTENSIBLE  or 50bytes->ADPCMWAVEFORMAT 
			//we just put it in the bytes count memory area and don't need to know what ever it is.
			// every xxWAVEFORAMxx first 16 bytes contains the same thing.
			// Microsoft CreateSourceVoice need WAVEFORMEX*, just reintepret_cast to it. But if you only use WAVEFORAMEX or WAVEFORAMEXTENSIBLE to load this information,
			// Some of wave files will fail either in CreateSourceVoice or SubmitSourceBuffer method
			//only in this way could read and worked on 4-bit mono ADPCM wav file. Microsoft's 2 fancy sample loading codes file don't work at all at this kind of file.
			wfx = new char[dwChunkSize];
			inFile.seekg(i + 8, std::ios::beg);
			inFile.read(wfx, dwChunkSize);
			bFilledFormat = true;
			break;
		}
		dwChunkSize += 8; //add offsets of the chunk id, and chunk size data entries
		dwChunkSize += 1;
		dwChunkSize &= 0xfffffffe; //guarantees WORD padding alignment
		i += dwChunkSize;
	}
	if(!bFilledFormat)
	{
		inFile.close();
		return false;
	}
	char* pDataBuffer;
	//look for 'data' chunk id
	bool bFilledData = false;
	for(unsigned int i = 12; i < dwFileSize; )
	{
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + 4, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkSize), sizeof(dwChunkSize));
		if(dwChunkId == 'atad')
		{
			//WAVEFORMATEX* temp = reinterpret_cast<WAVEFORMATEX*>(wfx);//to calculate the lengtho of the time, need the nAvgBytesPerSec in WAVEFORMATEX
			//length = dwChunkSize / static_cast<float>(temp->nAvgBytesPerSec);
			pDataBuffer = new char[dwChunkSize];
			inFile.seekg(i + 8, std::ios::beg);
			inFile.read(pDataBuffer, dwChunkSize);
			buffer.AudioBytes = dwChunkSize;
			buffer.pAudioData = reinterpret_cast<unsigned char*>(pDataBuffer);
			buffer.PlayBegin = 0;
			buffer.PlayLength = 0;
			bFilledData = true;
			break;
		}
		dwChunkSize += 8; //add offsets of the chunk id, and chunk size data entries
		dwChunkSize += 1;
		dwChunkSize &= 0xfffffffe; //guarantees WORD padding alignment
		i += dwChunkSize;
	}
	if(!bFilledData)
	{
		inFile.close();
		return false;
	}

	inFile.close();
	return true;
}



void Xaudio2::CreateSource(std::string filename) {
	LoadFile(filename, wfx);
	buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
	//WAVEFORMATEX abc = *(reinterpret_cast<const WAVEFORMATEX*>(wfx));
	//std::string sss;
	//sss = abc.wFormatTag;
	//MessageBox(nullptr, sss.c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
	if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, reinterpret_cast<const WAVEFORMATEX*>(wfx), 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr))) {
		MessageBox(nullptr, "Creating SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
		MessageBox(nullptr, "Submiting SourceBuffer Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}


void Xaudio2::CreateSource(std::string filename, int loop) {
		LoadFile(filename, wfx);
		buffer.LoopCount = loop;
		buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer


		if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, reinterpret_cast<const WAVEFORMATEX*>(wfx), 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr))) {
			MessageBox(nullptr, "Creating SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
			MessageBox(nullptr, "Submiting SourceBuffer Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
}
void Xaudio2::Create3DPositionalSource(std::string filename, int loop, IXAudio2MasteringVoice* pMaster) {
	pMasterVoice = pMaster;

	DWORD ChannelMask;
	XAUDIO2_VOICE_DETAILS MasterVoiceDetails;
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	pMasterVoice->GetVoiceDetails(&MasterVoiceDetails);
	if (MasterVoiceDetails.InputChannels > 8) {
		MessageBox(nullptr, "Input Channel more than 8!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	if (FAILED(hr = pMasterVoice->GetChannelMask(&ChannelMask))) {
		MessageBox(nullptr, "Getting Channel Mask Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#else
	if (FAILED(hr = pXAudio2->GetDeviceDetails(0, &DeviceDetails))) {
		MessageBox(nullptr, "Getting Channel Mask Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	if (DeviceDetails.OutputFormat.Format.nChannels > 8) {
		MessageBox(nullptr, "Input Channel more than 8!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	ChannelMask = DeviceDetails.OutputFormat.dwChannelMask;
#endif

	//Create Effect->need mastervoice


	IUnknown* pXAPO;
	if (FAILED(hr = XAudio2CreateReverb(&pXAPO))) {
		MessageBox(nullptr, "Create Reverb Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	XAUDIO2_EFFECT_DESCRIPTOR descriptor;
	descriptor.InitialState = true;
	descriptor.OutputChannels = 1;
	descriptor.pEffect = pXAPO;
	XAUDIO2_EFFECT_CHAIN chain;
	chain.EffectCount = 1;
	chain.pEffectDescriptors = &descriptor;
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	if (FAILED(hr = pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1, MasterVoiceDetails.InputSampleRate, 0, 0, nullptr, &chain))) {
		MessageBox(nullptr, "Creating SubmixVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#else
	if (FAILED(hr = pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1, DeviceDetails.OutputFormat.Format.nSamplesPerSec, 0, 0, nullptr, &chain))) {
		MessageBox(nullptr, "Creating SubmixVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#endif
	pXAPO->Release();

	XAUDIO2FX_REVERB_PARAMETERS reverbParameters;
	ReverbConvertI3DL2ToNative(&I3DL2_Reverb[2], &reverbParameters);
	pSubmixVoice->SetEffectParameters(0, &reverbParameters, sizeof(reverbParameters));
	SetEffectParameters(2);



	//create 3D sourcevoice need submixvoice with effect object


	initial3DSoundCone();
	initial3D_LFE_Curve();
	initial3D_Reverb_Curve();
	X3DAudioInitialize(ChannelMask, X3DAUDIO_SPEED_OF_SOUND, X3DInstance);
	UseRedirectToLFE = ((ChannelMask & SPEAKER_LOW_FREQUENCY) != 0); //Check if there is subwoofer setup
	lisener = new X3DAUDIO_LISTENER;
	lisener->Position.x = 0;
	lisener->Position.y = 0;
	lisener->Position.z = 0;
	lisener->OrientFront.x = 0;
	lisener->OrientFront.y = -1.0f;
	lisener->OrientFront.z = -1.0f;
	lisener->OrientTop.x = 0.0f;
	lisener->OrientTop.y = -1.0f;
	lisener->OrientTop.z = 1.0f;
	lisener->pCone = Listener_DirectionalCone;

	emitter = new X3DAUDIO_EMITTER;
	emitter->pCone = emitterCone;
	emitter->OrientFront.x = 1.0f;
	emitter->OrientFront.y = 0.0f;
	emitter->OrientFront.z = 0.0f;

	emitter->OrientTop.x = 0.0f;
	emitter->OrientTop.y = 0.0f;
	emitter->OrientTop.z = 1.0f;

	emitter->ChannelCount = INPUTCHANNELS;// 1
	emitter->ChannelRadius = 1.0f;

	emitterAzimuths = new float[INPUTCHANNELS];
	emitter->pChannelAzimuths = emitterAzimuths;//?

	emitter->InnerRadius = 100.0f;
	emitter->InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
	emitter->pVolumeCurve = nullptr;//const_cast<X3DAUDIO_DISTANCE_CURVE*>(&X3DAudioDefault_LinearCurve);
	emitter->pLFECurve = Emitter_LFE_Curve;
	emitter->pLPFDirectCurve = nullptr; //use default curve
	emitter->pLPFReverbCurve = nullptr; //use default curve
	emitter->pReverbCurve = Emitter_Reverb_Curve;
	emitter->CurveDistanceScaler = 100.0f;
	emitter->DopplerScaler = 1.0f;

	dsp_setting = new X3DAUDIO_DSP_SETTINGS;
	dsp_setting->SrcChannelCount = INPUTCHANNELS;

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	dsp_setting->DstChannelCount = MasterVoiceDetails.InputChannels;
#else
	dsp_setting->DstChannelCount = DeviceDetails.OutputFormat.Format.nChannels;
#endif

	matrixCoefficients = new float[INPUTCHANNELS * OUTPUTCHANNELS];
	dsp_setting->pMatrixCoefficients = matrixCoefficients;

	LoadFile(filename, wfx);
	buffer.LoopCount = loop;
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	//Create two sound filter pathes:
	//One is for LPF direct path
	//One is for LPF reverb path

	//LFE: Low Frequency Effect -- always omnidirectional.
	//LPF: Low Pass Filter, divided into two classifications:
	//Direct -- Applied to the direct signal path, used for obstruction/occlusion effects.
	//Reverb -- Applied to the reverb signal path, used for occlusion effects only.

	XAUDIO2_SEND_DESCRIPTOR sendDescriptor[2];
	sendDescriptor[0].Flags = XAUDIO2_SEND_USEFILTER;
	sendDescriptor[0].pOutputVoice = pMasterVoice;
	sendDescriptor[1].Flags = XAUDIO2_SEND_USEFILTER;
	sendDescriptor[1].pOutputVoice = pSubmixVoice;
	XAUDIO2_VOICE_SENDS sendlist;
	sendlist.SendCount = 2;
	sendlist.pSends = sendDescriptor;

	if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, reinterpret_cast<const WAVEFORMATEX*>(wfx), 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sendlist, nullptr))) {
		MessageBox(nullptr, "Creating SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
		MessageBox(nullptr, "Submiting SourceBuffer Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

}




void Xaudio2::X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime) {

	XMVECTOR v1 = XMVectorSet( listenerX, lisenerY, listenerZ, 0.0f);//new listener positions
	XMVECTOR v2 = XMVectorSet(lisener->Position.x, lisener->Position.y, lisener->Position.z, 0);//old listener positions
	XMVECTOR velocity = (v1 - v2) / elaspedtime;
	XMFLOAT3 temp;
	XMStoreFloat3(&temp, velocity);
	lisener->Position = { listenerX, lisenerY, listenerZ};//update new listener positions
	lisener->Velocity.x = temp.x;
	lisener->Velocity.y = temp.y;
	lisener->Velocity.z = temp.z;

	v1 = XMVectorSet(emitterX, emitterY, emitterZ, 0.0f);//new emitter positions
	v2 = XMVectorSet(emitter->Position.x, emitter->Position.y, emitter->Position.z, 0);//old emitter positions
	velocity = (v1 - v2) / elaspedtime;
	XMStoreFloat3(&temp, velocity);
	emitter->Position = {emitterX, emitterY, emitterZ};//update new emitter positions
	emitter->Velocity.x = temp.x;
	emitter->Velocity.y = temp.y;
	emitter->Velocity.z = temp.z;

	DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB | X3DAUDIO_CALCULATE_REVERB;
	if (UseRedirectToLFE) {
		dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
	}

	X3DAudioCalculate(X3DInstance, lisener, emitter, dwCalcFlags, dsp_setting);

	pSourceVoice->SetFrequencyRatio(dsp_setting->DopplerFactor);//set new calculated doppler factor based on new calculated velocity

	pSourceVoice->SetOutputMatrix(pMasterVoice, 1, dsp_setting->DstChannelCount, dsp_setting->pMatrixCoefficients);//set new calculated direct path sound volumes to speaker matrix
	pSourceVoice->SetOutputMatrix(pSubmixVoice, 1, 1, &(dsp_setting->ReverbLevel));//set new calculated reverb path sound volumes to submix 1 channel output and then send it to master voice

	XAUDIO2_FILTER_PARAMETERS LPFDirectPath = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI/6.0f * dsp_setting->LPFDirectCoefficient), 1.0f };//set new calculated low pass filter of direct path
	pSourceVoice->SetOutputFilterParameters(pMasterVoice, &LPFDirectPath);//give it to mastering voice

	XAUDIO2_FILTER_PARAMETERS LPFReverbPath = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dsp_setting->LPFReverbCoefficient), 1.0f };//set new calculated low pass filter of reverb path
	pSourceVoice->SetOutputFilterParameters(pSubmixVoice, &LPFReverbPath);//give it to submix voice and mix with pre-selected reverb effect then pass to mastering voice for output to speakers

}
void Xaudio2::X3Dcleanup() {

	SAFE_DELETE(lisener);
	SAFE_DELETE(emitter);
	SAFE_DELETE(Listener_DirectionalCone);
	SAFE_DELETE(emitterCone);
	SAFE_DELETE_ARRAY(Emitter_LFE_CurvePoints);
	SAFE_DELETE(Emitter_LFE_Curve);
	SAFE_DELETE_ARRAY(Emitter_Reverb_CurvePoints);
	SAFE_DELETE(Emitter_Reverb_Curve);
	SAFE_DELETE(dsp_setting);
	SAFE_DELETE_ARRAY(matrixCoefficients);
	SAFE_DELETE_ARRAY(emitterAzimuths);
}

void Xaudio2::LoopPlay() {
	if (FAILED(hr = pSourceVoice->Start(0))) {
		MessageBox(nullptr, "Play SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}

void Xaudio2::Play() {
	if (FAILED(hr = pSourceVoice->Stop(0))) {
		MessageBox(nullptr, "Stop SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}	
	if (FAILED(hr = pSourceVoice->FlushSourceBuffers())) {
		MessageBox(nullptr, "Flush SourceBuffers Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
		MessageBox(nullptr, "reSubmiting SourceBuffer Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	if (FAILED(hr = pSourceVoice->Start(0))) {
		MessageBox(nullptr, "Play SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}

void Xaudio2::Stop() {
	if (FAILED(hr = pSourceVoice->Stop(0))) {
		MessageBox(nullptr, "Stop SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}


void Xaudio2::initial3DSoundCone() {
	// Specify sound cone to add directionality to listener for artistic effect:
	// Emitters behind the listener are defined here to be more attenuated,
	// have a lower LPF cutoff frequency,
	// yet have a slightly higher reverb send level.
	Listener_DirectionalCone = new X3DAUDIO_CONE;
	Listener_DirectionalCone->InnerAngle = X3DAUDIO_PI*5.0f / 6.0f;
	Listener_DirectionalCone->OuterAngle = X3DAUDIO_PI*11.0f / 6.0f;
	Listener_DirectionalCone->InnerVolume = 1.0f;
	Listener_DirectionalCone->OuterVolume = 0.75f;
	Listener_DirectionalCone->InnerLPF = 0.0f;
	Listener_DirectionalCone->OuterLPF = 0.25f;
	Listener_DirectionalCone->InnerReverb = 0.708f;
	Listener_DirectionalCone->OuterReverb = 1.0f;

	emitterCone = new X3DAUDIO_CONE;
	emitterCone->InnerAngle = 0.0f;
	emitterCone->OuterAngle = 0.0f;
	emitterCone->InnerVolume = 0.0f;
	emitterCone->OuterVolume = 1.0f;
	emitterCone->InnerLPF = 0.0f;
	emitterCone->OuterLPF = 1.0f;
	emitterCone->InnerReverb = 0.0f;
	emitterCone->OuterReverb = 1.0f;
}

void Xaudio2::initial3D_LFE_Curve() {

	// Specify LFE level distance curve such that it rolls off much sooner than
	// all non-LFE channels, making use of the subwoofer more dramatic.
	Emitter_LFE_CurvePoints = new X3DAUDIO_DISTANCE_CURVE_POINT[3];
	Emitter_LFE_CurvePoints[0].Distance = 0.0f;
	Emitter_LFE_CurvePoints[0].DSPSetting = 1.0f;
	Emitter_LFE_CurvePoints[1].Distance = 0.25f;
	Emitter_LFE_CurvePoints[1].DSPSetting = 0.0f;
	Emitter_LFE_CurvePoints[2].Distance = 1.0f;
	Emitter_LFE_CurvePoints[2].DSPSetting = 0.0f;

	Emitter_LFE_Curve = new X3DAUDIO_DISTANCE_CURVE;
	Emitter_LFE_Curve->pPoints = Emitter_LFE_CurvePoints;
	Emitter_LFE_Curve->PointCount = 3;
}

void Xaudio2::initial3D_Reverb_Curve() {
	// Specify reverb send level distance curve such that reverb send increases
	// slightly with distance before rolling off to silence.
	// With the direct channels being increasingly attenuated with distance,
	// this has the effect of increasing the reverb-to-direct sound ratio,
	// reinforcing the perception of distance.
	Emitter_Reverb_CurvePoints = new X3DAUDIO_DISTANCE_CURVE_POINT[3];
	Emitter_Reverb_CurvePoints[0].Distance = 0.0f;
	Emitter_Reverb_CurvePoints[0].DSPSetting = 0.5f;
	Emitter_Reverb_CurvePoints[1].Distance = 0.75f;
	Emitter_Reverb_CurvePoints[1].DSPSetting = 1.0f;
	Emitter_Reverb_CurvePoints[2].Distance = 1.0f;
	Emitter_Reverb_CurvePoints[2].DSPSetting = 0.0f;

	Emitter_Reverb_Curve = new X3DAUDIO_DISTANCE_CURVE;
	Emitter_Reverb_Curve->pPoints = Emitter_Reverb_CurvePoints;
	Emitter_Reverb_Curve->PointCount = 3;
}


Sound::Sound(SoundDevice* pDevice, std::string filename, int loop) {
	sound_api = pDevice->GetSoundAPI();
	pAudio = pDevice->GetDevicePtr();
	
	if (sound_api == 1) {
		pSource = new Xaudio2(reinterpret_cast<IXAudio2*>(pAudio));
		reinterpret_cast<Xaudio2*>(pSource)->CreateSource(filename, loop);
	} else if (sound_api == 2) {
		pSource = new OpenAL;
		reinterpret_cast<OpenAL*>(pSource)->CreateSource(filename, loop);
	}
	
}

Sound::Sound(SoundDevice* pDevice, std::string filename) {
	sound_api = pDevice->GetSoundAPI();
	pAudio = pDevice->GetDevicePtr();
	if (sound_api == 1) {
		pSource = new Xaudio2(reinterpret_cast<IXAudio2*>(pAudio));
		reinterpret_cast<Xaudio2*>(pSource)->CreateSource(filename);
	} else if (sound_api == 2) {
		pSource = new OpenAL;
		reinterpret_cast<OpenAL*>(pSource)->CreateSource(filename);
	}
}

Sound::Sound(SoundDevice* pDevice, std::string filename, int loop, int dummy) {
	sound_api = pDevice->GetSoundAPI();
	pAudio = pDevice->GetDevicePtr();
	if (sound_api == 1) {
		pSource = new Xaudio2(reinterpret_cast<IXAudio2*>(pAudio));
		reinterpret_cast<Xaudio2*>(pSource)->Create3DPositionalSource(filename, loop, pDevice->GetMasterVoice());
	} else if (sound_api == 2) {
		pSource = new OpenAL;
		reinterpret_cast<OpenAL*>(pSource)->Create3DPositionalSource(filename, loop, pDevice->GetMasterVoice());
	}
}


Sound::~Sound() {
	if (sound_api == 1) {
		delete reinterpret_cast<Xaudio2*>(pSource);
	} else if (sound_api == 2) {
		delete reinterpret_cast<OpenAL*>(pSource);
	}
	pSource = nullptr;
}


void Sound::Play() {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->Play();
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->Play();
	}
}
void Sound::LoopPlay() {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->LoopPlay();
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->LoopPlay();
	}
}
void Sound::Stop() {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->Stop();
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->Stop();
	}
}

void Sound::setVolume(float volume) {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->setVolume(volume);
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->setVolume(volume);
	}
}
float Sound::getVolume() {
	if (sound_api == 1) {
		return reinterpret_cast<Xaudio2*>(pSource)->getVolume();
	//} else if (sound_api == 2) {
	} else {
		return reinterpret_cast<OpenAL*>(pSource)->getVolume();
	}
}
void Sound::initialListenerEmitter(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ) {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->initialListenerEmitter(listenerX, lisenerY, listenerZ, emitterX, emitterY, emitterZ);
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->initialListenerEmitter(listenerX, lisenerY, listenerZ, emitterX, emitterY, emitterZ);
	}
}
void Sound::PositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime) {
	if (sound_api == 1) {
		reinterpret_cast<Xaudio2*>(pSource)->X3DPositionalSoundCalculation(listenerX, lisenerY, listenerZ, emitterX, emitterY, emitterZ, elaspedtime);
	} else if (sound_api == 2) {
		reinterpret_cast<OpenAL*>(pSource)->X3DPositionalSoundCalculation(listenerX, lisenerY, listenerZ, emitterX, emitterY, emitterZ, elaspedtime);
	}
}