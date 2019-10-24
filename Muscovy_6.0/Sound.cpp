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
#include "Sound.h"
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	/*•The DirectX SDK version of XAudio2 used CoCreateInstance and was registered with COM, and required an explicit Initialize method to be called. 
	The DirectX SDK  'xaudio2.h' header contained a wrapper inline function XAudio2Create that did this to simplify portability with the Xbox 360 version. 
	For the Windows 8 version, you don't use COM creation and instead call the XAudio2Create function and link with "xaudio2.lib".*/
	#pragma comment(lib,"xaudio2.lib")
#else
	#pragma comment(lib,"x3daudio.lib")
#endif

#pragma comment(lib, "Winmm.lib")
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
using namespace DirectX;
#endif

SoundDevice::SoundDevice(int sound_api) : sound_api(sound_api) {
	if (sound_api == 1) {
		pXaudio2 = new Xaudio2();
	} else if (sound_api == 2) {
		pOpenAL = new OpenAL();
	}
}
SoundDevice::~SoundDevice() {
	if (sound_api == 1) {
		SAFE_DELETE(pXaudio2);
	} else if (sound_api == 2) {
		SAFE_DELETE(pOpenAL);
	}
}

Xaudio2::Xaudio2() {
	CoInitializeEx(nullptr, 0);

	if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		MessageBox(nullptr, "Creating XAudio2 Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, nullptr, nullptr))) {
		MessageBox(nullptr, "Creating MasteringVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	XAUDIO2_VOICE_DETAILS MasterVoiceDetails;

	pMasterVoice->GetVoiceDetails(&MasterVoiceDetails);
	if (MasterVoiceDetails.InputChannels > 8) {
		MessageBox(nullptr, "Input Channel more than 8!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}

#else
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)/*, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, nullptr)*/)) {
		char buffer[256];
		sprintf_s(buffer, "Creating MasteringVoice Failed! %x, %d", hr, hr);
		MessageBox(nullptr, buffer, "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	XAUDIO2_DEVICE_DETAILS DeviceDetails;
	if (FAILED(hr = pXAudio2->GetDeviceDetails(0, &DeviceDetails))) {
		MessageBox(nullptr, "Getting Channel Mask Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
#endif
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
}


void Xaudio2::SetEffectParameters(int I3DL2) {
	XAUDIO2FX_REVERB_PARAMETERS reverbParameters;
	ReverbConvertI3DL2ToNative(&I3DL2_Reverb[(I3DL2 % 30 + 30) % 30], &reverbParameters);//(I3DL2 % 30 + 30) % 30 positive modulo
	pSubmixVoice->SetEffectParameters(0, &reverbParameters, sizeof(reverbParameters));
}



Xaudio2::~Xaudio2() {
	pMasterVoice->DestroyVoice();
	pSubmixVoice->DestroyVoice();
	pXAudio2->Release();
	CoUninitialize();
}
bool Sound::LoadFile(std::string szFile, char* &wfx) {
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


Sound::Sound(SoundDevice* pDevice, std::string filename) {
	LoadFile(filename, wfx);
	if (pDevice->GetSoundAPI() == 1) {

		pXAudio2 = pDevice->GetXaudio2Ptr()->GetIXAudio2Ptr();

		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

		if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, reinterpret_cast<const WAVEFORMATEX*>(wfx), 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr))) {
			MessageBox(nullptr, "Creating SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer))) {
			MessageBox(nullptr, "Submiting SourceBuffer Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			return;
		}
	} else if (pDevice->GetSoundAPI() == 2) {
		;
	}

}

Sound::Sound(SoundDevice* pDevice, std::string filename, int loop) {
	LoadFile(filename, wfx);

	if (pDevice->GetSoundAPI() == 1){


		pXAudio2 = pDevice->GetXaudio2Ptr()->GetIXAudio2Ptr();

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
	} else if (pDevice->GetSoundAPI() == 2) {
		;
	}

}

Sound::Sound(SoundDevice* pDevice, std::string filename, int loop, int I3DL2) {
	if (pDevice->GetSoundAPI() == 1) {

		pXAudio2 = pDevice->GetXaudio2Ptr()->GetIXAudio2Ptr();
		pMasterVoice = pDevice->GetXaudio2Ptr()->GetMasterVoice();
		pSubmixVoice = pDevice->GetXaudio2Ptr()->GetSubmixVoicePtr();

		initial3DSoundCone();
		initial3D_LFE_Curve();
		initial3D_Reverb_Curve();
		DWORD ChannelMask;
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
		pDevice->GetXaudio2Ptr()->SetEffectParameters(I3DL2);
		
	} else if (pDevice->GetSoundAPI() == 2) {
		;
	}


}


Sound::~Sound() {
	if (pSourceVoice) {
		pSourceVoice->DestroyVoice();//must destroy voice or it will crash when quit
	}

	if (wfx) {
		delete []wfx;
		wfx = nullptr;
	}
	if (buffer.pAudioData) {
		delete[]buffer.pAudioData;
		buffer.pAudioData = nullptr;
	}
	X3Dcleanup();

}




void Sound::X3DPositionalSoundCalculation(float listenerX, float lisenerY, float listenerZ, float emitterX, float emitterY, float emitterZ, float elaspedtime) {

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
void Sound::X3Dcleanup() {

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

void Sound::LoopPlay() {
	if (FAILED(hr = pSourceVoice->Start(0))) {
		MessageBox(nullptr, "Play SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}

void Sound::Play() {
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

void Sound::Stop() {
	if (FAILED(hr = pSourceVoice->Stop(0))) {
		MessageBox(nullptr, "Stop SourceVoice Failed!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}


void Sound::initial3DSoundCone() {
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

void Sound::initial3D_LFE_Curve() {

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

void Sound::initial3D_Reverb_Curve() {
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

