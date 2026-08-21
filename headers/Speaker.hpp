#pragma once

#include <atomic>
#include <Peripheral.hpp>

struct SDL_AudioStream;

class Speaker : public Peripheral
{
public:
	Speaker();

	virtual ~Speaker() override;

	enum DataType : u32
	{
		Byte = 0,
		Short = 1,
		Word = 2,
		Float = 3
	};

	void Update() override;
	bool DrawGui() override;
	u64 GetSize() override;
	bool CreateAudioStream(DataType type, u32 freq, bool stereo);
	bool CreateAudioStream();
	bool HasAudioStream() const { return boundStream != nullptr; }
	void StopAudioStream();

	// Internal use only
	void _SoundUpdate(SDL_AudioStream *stream, s32 frame_count);

private:
	SDL_AudioStream *boundStream = nullptr;
	float volume = 0.5f;

	DataType dataType = Short;
	u32 frequency = 48000;
	bool stereo = false;

	s32 dataTypeGui = Short;
	s32 frequencyGui = 48000;
	bool stereoGui = false;

	void *soundBuffer;
	void *copyBuffer;
	std::atomic_int soundPosA = 0;
	std::atomic_int soundPosB = 0;
};
