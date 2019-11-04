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
#include <unordered_map>
#include <algorithm>


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
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
using namespace DirectX;
#endif
#include "OALSound.h"

OALSound::OALSound() {
	filename = "";
	streaming = false;
	bitRate = 0;
	freq = 0.0f;
	length = 0.0f;
	data = nullptr;
}
OALSound::OALSound(std::string filename) {
	filename = filename;
	streaming = false;
	bitRate = 0;
	freq = 0.0f;
	length = 0.0f;
	data = nullptr;
}
bool OALSound::LoadSoundtoMemory() {
	if (filename.empty())
		return false;
	
	std::ifstream inFile(filename, std::ios::binary | std::ios::in);
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
	for (int i = 12; i < dwFileSize; ) {
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + static_cast<int>(4), std::ios::beg);
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
			inFile.seekg(i + static_cast<int>(8), std::ios::beg);
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

	freq = static_cast<ALfloat>(reinterpret_cast<waveformat*>(wfx)->nSamplesPerSec);
	channels = reinterpret_cast<waveformat*>(wfx)->nChannels;
	bitRate = reinterpret_cast<waveformat*>(wfx)->wBitsPerSample;

	delete[]wfx;
	wfx = nullptr;
	char* pDataBuffer;
	//look for 'data' chunk id
	bool bFilledData = false;
	for (int i = 12; i < dwFileSize; ) {
		inFile.seekg(i, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkId), sizeof(dwChunkId));
		inFile.seekg(i + 4, std::ios::beg);
		inFile.read(reinterpret_cast<char*>(&dwChunkSize), sizeof(dwChunkSize));
		if (dwChunkId == 'atad') {
			//WAVEFORMATEX* temp = reinterpret_cast<WAVEFORMATEX*>(wfx);//to calculate the lengtho of the time, need the nAvgBytesPerSec in WAVEFORMATEX
			//length = dwChunkSize / static_cast<float>(temp->nAvgBytesPerSec);

			pDataBuffer = new char[dwChunkSize];
			inFile.seekg(i + static_cast<int>(8), std::ios::beg);
			inFile.read(pDataBuffer, dwChunkSize);
			size = dwChunkSize;
			data = reinterpret_cast<char*>(pDataBuffer);
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
	length = static_cast<float>(size) / (channels * freq * (bitRate / 8.0f)) * 1000.0f;
	inFile.close();
	return true;
}



bool OALSound::ReleaseSoundMemory() {
	delete[]data;
	data = nullptr;
}



OALSoundManager::OALSoundManager() {
}
OALSoundManager::~OALSoundManager() {
}
bool OALSoundManager::AddSound(std::string filename) {
	OALSound* s = GetSound(filename);
	if (!s) {
		SoundList.insert(make_pair(filename, s));
	}
}
OALSound* OALSoundManager::GetSound(std::string name) {
	std::unordered_map<std::string, OALSound*>::iterator s = SoundList.find(name);
	return (s != SoundList.end() ? s->second : nullptr);
}
bool OALSoundManager::DeleteSound() {
	for (std::unordered_map<std::string, OALSound*>::iterator i = SoundList.begin(); i != SoundList.end(); ++i) {
		delete i->second;
	}
}




SoundEmitter::SoundEmitter(OALContext* pContext) : pContext(pContext) {
	Reset();
}

SoundEmitter::SoundEmitter(OALContext* pContext, OALSound* s) : pContext(pContext) {
	Reset();
	SetSound(s);
}

void	SoundEmitter::Reset() {
	emitter = new Emitter;
	emitter->x = 0.0f;
	emitter->y = 0.0f;
	emitter->z = 0.0f;
	priority = SOUNDPRIORTY_LOW;
	pitch = 1.0f;
	volume = 1.0f;
	radius = 500.0f;
	timeLeft = 0.0f;
	isLooping = true;
	oalSource = nullptr;
	sound = nullptr;
	//Part 2
	streamPos = 0;
	isGlobal = false;
	for (unsigned int i = 0; i < NUM_STREAM_BUFFERS; ++i) {
		streamBuffers[i] = 0;
	}
}

SoundEmitter::~SoundEmitter(void) {
	if (emitter) {
		delete emitter;
		emitter = nullptr;
	}
	DetachSource();
}
void SoundEmitter::SetEmitter(Emitter e) {
	if (!emitter) {
		emitter = new Emitter;
	}
	emitter->x = e.x;
	emitter->y = e.y;
	emitter->z = e.z;
}
bool SoundEmitter::CompareNodesByPriority(SoundEmitter* a, SoundEmitter* b) {
	return (a->priority > b->priority) ? true : false;
}

void SoundEmitter::SetSound(OALSound* s) {
	sound = s;
	DetachSource();
	if (sound) {
		timeLeft = sound->GetLength();

		if (sound->IsStreaming()) {
			alGenBuffers(NUM_STREAM_BUFFERS, streamBuffers);
		} else {
			alDeleteBuffers(NUM_STREAM_BUFFERS, streamBuffers);
		}
	}
}

void SoundEmitter::AttachSource(OALSource* s) {
	oalSource = s;

	if (!oalSource) {
		return;
	}

	oalSource->inUse = true;

	alSourceStop(oalSource->source);
	alSourcef(oalSource->source, AL_MAX_DISTANCE, radius);
	alSourcef(oalSource->source, AL_REFERENCE_DISTANCE, radius * 0.2f);

	//if(timeLeft > 0) {
	if (sound->IsStreaming()) {	//Part 2
		streamPos = timeLeft;
		int numBuffered = 0;
		while (numBuffered < NUM_STREAM_BUFFERS) {
			double streamed = sound->StreamData(streamBuffers[numBuffered], streamPos);

			if (streamed) {
				streamPos -= streamed;
				++numBuffered;
			} else {
				break;
			}
		}
		alSourceQueueBuffers(oalSource->source, numBuffered, &streamBuffers[0]);
	} else {
		alSourcei(oalSource->source, AL_BUFFER, sound->GetBuffer());
		alSourcef(oalSource->source, AL_SEC_OFFSET, (sound->GetLength() / 1000.0) - (timeLeft / 1000.0));

		//std::cout << "Attaching! Timeleft: " << (sound->GetLength() / 1000.0) - (timeLeft / 1000.0) << std::endl;
		alSourcePlay(oalSource->source);
	}
	alSourcePlay(oalSource->source);
	//}
}

void SoundEmitter::DetachSource() {
	if (!oalSource) {
		return;
	}

	oalSource->inUse = false;

	alSourcef(oalSource->source, AL_GAIN, 0.0f);
	alSourceStop(oalSource->source);
	alSourcei(oalSource->source, AL_BUFFER, 0);

	if (sound && sound->IsStreaming()) {	//Part 2
		int numProcessed = 0;
		ALuint tempBuffer;
		alGetSourcei(oalSource->source, AL_BUFFERS_PROCESSED, &numProcessed);
		while (numProcessed--) {
			alSourceUnqueueBuffers(oalSource->source, 1, &tempBuffer);
		}
	}

	oalSource = nullptr;
}

void SoundEmitter::UpdateSoundState(float msec) {
	if (sound) {
		timeLeft -= (msec * pitch);

		if (isLooping) {
			while (timeLeft < 0) {
				timeLeft += sound->GetLength();
				//streamPos += sound->GetLength();
			}
		}
		if (oalSource) {
			alSourcef(oalSource->source, AL_GAIN, volume);
			alSourcef(oalSource->source, AL_PITCH, pitch);
			alSourcef(oalSource->source, AL_MAX_DISTANCE, radius);
			alSourcef(oalSource->source, AL_REFERENCE_DISTANCE, radius * 0.2f);

			
			
			float position[3];
			if (isGlobal) {
				position[0] = pContext->GetListener()->x;
				position[1] = pContext->GetListener()->y;
				position[2] = pContext->GetListener()->z;
			} else {
				position[0] = emitter->x;
				position[1] = emitter->y;
				position[2] = emitter->z;
			}

			alSource3f(oalSource->source, AL_POSITION, position[0], position[1], position[2]);

			if (sound->IsStreaming()) {
				int numProcessed;
				alGetSourcei(oalSource->source, AL_BUFFERS_PROCESSED, &numProcessed);
				alSourcei(oalSource->source, AL_LOOPING, 0);

				while (numProcessed--/* && streamPos > 0*/) {	//The && prevents clipping at the end of sounds!
					ALuint freeBuffer;

					alSourceUnqueueBuffers(oalSource->source, 1, &freeBuffer);

					streamPos -= sound->StreamData(freeBuffer, streamPos);
					alSourceQueueBuffers(oalSource->source, 1, &freeBuffer);

					if (streamPos < 0 && isLooping) {
						streamPos += sound->GetLength();
					}
				}
			} else {
				alSourcei(oalSource->source, AL_LOOPING, isLooping ? 1 : 0);
			}
		}
	}
}

void SoundEmitter::Update(float msec) {
	//OALContext::GetOALContext()->AddSoundEmitter(this);
	//SceneNode::Update(msec);
}



OALContext::OALContext(OALDevice* pDevice, unsigned int channels) {
	listener = nullptr;
	masterVolume = 1.0f;



	pContext = alcCreateContext(pDevice->GetOALDevice(), nullptr);
	if (pContext) {
		alcMakeContextCurrent(pContext);
	} else {
		MessageBox(nullptr, "Context creation failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}



	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	for (unsigned int i = 0; i < channels; ++i) {
		ALuint source;

		alGenSources(1, &source);
		ALenum error = alGetError();

		if (error == AL_NO_ERROR) {
			sources.push_back(new OALSource(source));
		} else {
			break;
		}
	}

}
OALContext::OALContext(OALDevice* pDevice) {
	listener = nullptr;
	masterVolume = 1.0f;




	pContext = alcCreateContext(pDevice->GetOALDevice(), nullptr);
	if (pContext) {
		alcMakeContextCurrent(pContext);
	} else {
		MessageBox(nullptr, "Context creation failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}



	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	ALenum error;
	alGetError(); // clear error code 
	while (error == AL_NO_ERROR){
		ALuint source;
		alGenSources(1, &source);
		error = alGetError();
		if (error == AL_NO_ERROR) {
			sources.push_back(new OALSource(source));
		}
	}

}
OALContext::~OALContext(void) {
	for (std::vector<SoundEmitter*>::iterator i = temporaryEmitters.begin(); i != temporaryEmitters.end(); ++i) {
		delete (*i);
	}

	for (std::vector<OALSource*>::iterator i = sources.begin(); i != sources.end(); ++i) {
		alDeleteSources(1, &(*i)->source);
		delete (*i);
	}
	if (listener) {
		delete listener;
		listener = nullptr;
	}

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(pContext);
}

void OALContext::Update(float msec) {
	UpdateListener();
	UpdateTemporaryEmitters(msec);

	//Update values for every node, whether in range or not
	for (std::vector<SoundEmitter*>::iterator i = emitters.begin(); i != emitters.end(); ++i) {
		(*i)->UpdateSoundState(msec);
	}

	CullNodes();	//First off, remove nodes that are too far away

	if (emitters.size() > sources.size()) {
		std::sort(emitters.begin(), emitters.end(), SoundEmitter::CompareNodesByPriority);	//Then sort by priority

		DetachSources(emitters.begin() + (sources.size() + 1), emitters.end());		//Detach sources from nodes that won't be covered this frame
		AttachSources(emitters.begin(), emitters.begin() + (sources.size()));	//And attach sources to nodes that WILL be covered this frame
	} else {
		AttachSources(emitters.begin(), emitters.end());//And attach sources to nodes that WILL be covered this frame
	}

	emitters.clear();	//We're done for the frame! empty the emitters list
}

void OALContext::CullNodes() {
	for (std::vector<SoundEmitter*>::iterator i = emitters.begin(); i != emitters.end();) {

		float length;

		if ((*i)->GetIsGlobal()) {
			length = 0.0f;
		} else {
			XMFLOAT3 v1 = XMFLOAT3(listener->x, listener->y, listener->z);
			XMFLOAT3 v2 = XMFLOAT3((*i)->GetEmitter()->x, (*i)->GetEmitter()->y, (*i)->GetEmitter()->z);
			XMVECTOR vector1 = XMLoadFloat3(&v1);
			XMVECTOR vector2 = XMLoadFloat3(&v2);
			XMVECTOR vectorSub = XMVectorSubtract(vector1, vector2);
			XMVECTOR distance = XMVector3Length(vectorSub);
			XMStoreFloat(&length, distance);
		}

		if (length > (*i)->GetRadius() || !(*i)->GetSound() || (*i)->GetTimeLeft() < 0) {
			(*i)->DetachSource();	//Important!
			i = emitters.erase(i);
		} else {
			++i;
		}
	}
	
	for (std::vector<OALBuffer*>::iterator i = pDevice->GetBuffers()->begin(); i != pDevice->GetBuffers()->end();) {
	}
}

void OALContext::DetachSources(std::vector<SoundEmitter*>::iterator from, std::vector<SoundEmitter*>::iterator to) {
	for (std::vector<SoundEmitter*>::iterator i = from; i != to; ++i) {
		(*i)->DetachSource();
	}
}

void OALContext::AttachSources(std::vector<SoundEmitter*>::iterator from, std::vector<SoundEmitter*>::iterator to) {
	for (std::vector<SoundEmitter*>::iterator i = from; i != to; ++i) {
		if (!(*i)->GetSource()) {	//Don't attach a new source if we already have one!
			(*i)->AttachSource(GetSource());
		}
	}
}

OALSource* OALContext::GetSource() {
	for (std::vector<OALSource*>::iterator i = sources.begin(); i != sources.end(); ++i) {
		OALSource* s = *i;
		if (!s->inUse) {
			return s;
		}
	}
	return nullptr;
}

void OALContext::SetMasterVolume(float value) {
	value = max(0.0f, value);
	value = min(1.0f, value);
	masterVolume = value;
	alListenerf(AL_GAIN, masterVolume);
}
void OALContext::SetListener(Listener l) {
	listener = new Listener;
	listener->dir = l.dir;
	listener->x = l.x;
	listener->y = l.y;
	listener->z = l.z;
}
void OALContext::UpdateListener() {
	if (listener) {
		float direction[6] = {0};
		if (listener->dir == 0) {
			direction[0] = listener->x;
			direction[1] = max (0, listener->y - 1);
			direction[2] = listener->z;
		} else if (listener->dir == 1) {
			direction[0] = listener->x;
			direction[1] = listener->y + 1;
			direction[2] = listener->z;
		} else if (listener->dir == 2) {
			direction[0] = max (0, listener->x - 1);
			direction[1] = listener->y;
			direction[2] = listener->z;
		} else if (listener->dir == 3) {
			direction[0] = listener->x + 1;
			direction[1] = listener->y;
			direction[2] = listener->z;
		} else {
		}
		direction[3] = listener->x;
		direction[4] = listener->y;
		direction[5] = listener->z;

		alListener3f(AL_POSITION, listener->x, listener->y, listener->z);
		alListenerfv(AL_ORIENTATION, direction);
	}
}

void OALContext::PlaySound(OALSound* s, float x, float y, float z) {
	SoundEmitter* n = new SoundEmitter(this);
	n->SetLooping(false);
	n->SetEmitter(Emitter{ x,y,z });
	n->SetSound(s);
	temporaryEmitters.push_back(n);
}

void OALContext::PlaySound(OALSound* s, SoundPriority p) {
	SoundEmitter* n = new SoundEmitter(this);
	n->SetLooping(false);
	n->SetSound(s);
	n->SetIsGlobal(true);
	n->SetPriority(p);
	temporaryEmitters.push_back(n);
}

void OALContext::UpdateTemporaryEmitters(float msec) {
	for (std::vector<SoundEmitter*>::iterator i = temporaryEmitters.begin(); i != temporaryEmitters.end(); ) {
		if ((*i)->GetTimeLeft() < 0.0f && !(*i)->GetLooping()) {
			delete (*i);
			i = temporaryEmitters.erase(i);
		} else {
			(*i)->Update(msec);
			++i;
		}
	}
}

OALDevice::OALDevice() {
}

OALDevice::~OALDevice() {
}