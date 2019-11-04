#ifndef OALSOUND_H
#define OALSOUND_H

#define NUM_STREAM_BUFFERS 3
class OALSound {
public:
					OALSound();
					OALSound(std::string filename);
					~OALSound() { ReleaseSoundMemory(); }
	bool			LoadSoundtoMemory();
	bool			ReleaseSoundMemory();
	char*			GetData() { return data; }
	unsigned int	GetBitRate() { return bitRate; }
	float			GetFrequency() { return freq; }
	unsigned int	GetChannels() { return channels; }
	unsigned int	GetSize() { return size; }
	double			GetLength() { return length; }
	ALenum			GetFormat() { return format; }

	bool			IsStreaming() { return streaming; }
	virtual double	StreamData(ALuint	buffer, double timeLeft) { return 0.0f; }
private:
	std::string filename;
	char* data;
	bool			streaming;
	float			freq;
	double			length;
	unsigned int	bitRate;
	unsigned int	size;
	unsigned int	channels;
	ALenum format;
};

class OALSoundManager {
public:
	bool AddSound(std::string filename);
	OALSound* GetSound(std::string name);
	bool DeleteSound();
private:
	std::unordered_map<std::string, OALSound*> SoundList;
};

enum SoundPriority {
	SOUNDPRIORTY_LOW,
	SOUNDPRIORITY_MEDIUM,
	SOUNDPRIORITY_HIGH,
	SOUNDPRIORITY_ALWAYS
};


struct OALSource {
	ALuint	source;
	bool	inUse;

	OALSource(ALuint src) {
		source = src;
		inUse = false;
	}
};
struct Emitter {
	float x;
	float y;
	float z;
};
class SoundEmitter {
public:
	SoundEmitter(OALContext* pContext);
	SoundEmitter(OALContext* pContext, OALSound* s);
	~SoundEmitter(void);

	void			Reset();
	void			SetEmitter(Emitter e);
	Emitter*		GetEmitter() { return emitter; }
	void			SetSound(OALSound* s);
	OALSound*		GetSound() { return sound; }

	void			SetPriority(SoundPriority p) { priority = p; }
	SoundPriority	GetPriority() { return priority; }

	void			SetVolume(float volume) { volume = min(1.0f, max(0.0f, volume)); }
	float			GetVolume() { return volume; }

	void			SetLooping(bool state) { isLooping = state; }
	bool			GetLooping() { return isLooping; }

	void			SetRadius(float value) { radius = max(0.0f, value); }
	float			GetRadius() { return radius; }

	float			GetPitch() { return pitch; }
	void			SetPitch(float value) { pitch = value; }

	bool			GetIsGlobal() { return isGlobal; }
	void			SetIsGlobal(bool value) { isGlobal = value; }

	double			GetTimeLeft() { return timeLeft; }

	OALSource*		GetSource() { return oalSource; }

	void			UpdateSoundState(float msec);

	static bool		CompareNodesByPriority(SoundEmitter* a, SoundEmitter* b);

	void			AttachSource(OALSource* s);
	void			DetachSource();

	virtual void	Update(float msec);

protected:
	OALSound*		sound;
	OALSource*		oalSource;
	OALContext*		pContext;
	SoundPriority	priority;
	Emitter*		emitter;
	float			volume;
	float			radius;
	float			pitch;
	bool			isLooping;
	bool			isGlobal;
	double			timeLeft;

	double			streamPos;							//Part 2
	ALuint			streamBuffers[NUM_STREAM_BUFFERS];	//Part 2
};

struct Listener {
	float x;
	float y;
	float z;
	int dir;
};
class OALBuffer {
public:
	OALBuffer() {};
	~OALBuffer() {};
private:
	ALuint	buffer;
};
class OALDevice {
	public:
		OALDevice();
		~OALDevice();
		ALCdevice* GetOALDevice() { return pDevice; }
		std::vector<OALBuffer*>* GetBuffers() { return &buffers; }
	private:
		ALCdevice* pDevice;
		std::vector<OALBuffer*>	buffers;
};
class OALContext {
public:

	void		SetListener(Listener l);
	Listener*	GetListener() { return listener; }

	void		AddSoundEmitter(SoundEmitter* s) { emitters.push_back(s); }

	void		Update(float msec);

	void		SetMasterVolume(float value);

	void		PlaySound(OALSound* s, float x, float y, float z);
	void		PlaySound(OALSound* s, SoundPriority p);

protected:
	OALContext(OALDevice* pDevice);
	OALContext(OALDevice* pDevice, unsigned int channels = 128);
	~OALContext(void);

	void		UpdateListener();

	void		DetachSources(std::vector<SoundEmitter*>::iterator from, std::vector<SoundEmitter*>::iterator to);
	void		AttachSources(std::vector<SoundEmitter*>::iterator from, std::vector<SoundEmitter*>::iterator to);

	void		CullNodes();

	OALSource* GetSource();

	void		UpdateTemporaryEmitters(float msec);

	std::vector<OALSource*>	sources;

	std::vector<SoundEmitter*>	emitters;
	std::vector<SoundEmitter*>	temporaryEmitters;

	Listener* listener;

	ALCcontext* pContext;
	OALDevice* pDevice;

	float masterVolume;
};

#endif
