#pragma once

#include <atomic>
#include <peripherals/Peripheral.hpp>
#include <miniaudio/miniaudio.h>

//#define USE_MINIAUDIO

struct SDL_AudioStream;

class Speaker : public Peripheral
{
public:
	Speaker();

	virtual ~Speaker() override;

	enum AudioDataType : u32
	{
		Byte = 0,
		Short = 1,
		Word = 2,
		Float = 3
	};

	void Update() override;
	bool DrawGui(bool &visible) override;
	u64 GetSize() override;

	bool CreateAudioStream(AudioDataType type, u32 freq, bool stereo);
	bool CreateAudioStream();
	bool HasAudioStream() const;
	void StopAudioStream();

	// Internal use only
	void _SoundUpdateSDL(SDL_AudioStream *stream, s32 frame_count);
	void _SoundUpdateMA(void *stream, u32 frame_count);

private:
#ifdef USE_MINIAUDIO
	ma_device *device = nullptr;
#else
	SDL_AudioStream *boundStream = nullptr;
#endif

	float volume = 0.5f;

	AudioDataType dataType = Short;
	u32 frequency = 48000;
	bool stereo = false;

	s32 dataTypeGui = Short;
	s32 frequencyGui = 48000;
	bool stereoGui = false;

	void *soundBuffer;
	void *copyBuffer;
	std::atomic_int soundPosA = 0;
	std::atomic_int soundPosB = 0;

	std::atomic<float> volume_atom = 0.5f;
};
