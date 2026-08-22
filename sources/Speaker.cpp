#include <Speaker.hpp>

#include <SDL3/SDL.h>
#include <GameLinker.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#define MA_NO_ENGINE
#include "miniaudio/miniaudio.h"

// Size of the ring buffer, in samples
#define SOUND_BUFFER_SIZE 16384

// Size of a chunk that the TC side needs to fill with audio data, in samples
#define BUFFER_CHUNK 512
// Offset (in bytes) between the flags byte and the start of the buffer
#define BUFFER_OFFSET 16

Speaker::Speaker()
{
	soundBuffer = new u32[SOUND_BUFFER_SIZE];
	copyBuffer = new u32[BUFFER_CHUNK];

#ifdef USE_MINIAUDIO
	device = new ma_device();
#endif
}

Speaker::~Speaker()
{
#ifdef USE_MINIAUDIO
	if (ma_device_is_started(device))
	{
		ma_device_uninit(device);
	}
	delete device;
#else
	if (boundStream)
	{
		SDL_PauseAudioStreamDevice(boundStream);
		SDL_UnbindAudioStream(boundStream);
		boundStream = nullptr;
	}
#endif

	delete[] soundBuffer;
	delete[] copyBuffer;
}

void Speaker::Update()
{
	if (!HasAudioStream())
		return;
	int dif = soundPosA - soundPosB;
	if (dif < 0)
		dif += SOUND_BUFFER_SIZE;
	if (dif > BUFFER_CHUNK * 4)
		return;

	u16 flags;
	if (!GameLinker::ReadMemory(address - sizeof(flags), &flags, sizeof(flags)))
		return;

	if (flags == 1)
		return;
	if (flags == 2)
	{
		int pos = soundPosA;

		switch (dataType)
		{
		case Speaker::Byte:
		{
			u8 *tmpBuffer = reinterpret_cast<u8*>(copyBuffer);
			if (!GameLinker::ReadMemory(address - BUFFER_CHUNK * sizeof(u8) - BUFFER_OFFSET, tmpBuffer, BUFFER_CHUNK * sizeof(u8)))
				return;
			for (int i = 0; i < BUFFER_CHUNK; i++)
				reinterpret_cast<u8*>(soundBuffer)[(pos + i) % SOUND_BUFFER_SIZE] = tmpBuffer[BUFFER_CHUNK - i - 1];
		}
		break;
		case Speaker::Short:
		{
			u16 *tmpBuffer = reinterpret_cast<u16*>(copyBuffer);
			if (!GameLinker::ReadMemory(address - BUFFER_CHUNK * sizeof(u16) - BUFFER_OFFSET, tmpBuffer, BUFFER_CHUNK * sizeof(u16)))
				return;
			for (int i = 0; i < BUFFER_CHUNK; i++)
				reinterpret_cast<u16*>(soundBuffer)[(pos + i) % SOUND_BUFFER_SIZE] = tmpBuffer[BUFFER_CHUNK - i - 1];
		}
		break;
		case Speaker::Word:
		case Speaker::Float:
		{
			u32 *tmpBuffer = reinterpret_cast<u32*>(copyBuffer);
			if (!GameLinker::ReadMemory(address - BUFFER_CHUNK * sizeof(u32) - BUFFER_OFFSET, tmpBuffer, BUFFER_CHUNK * sizeof(u32)))
				return;
			for (int i = 0; i < BUFFER_CHUNK; i++)
				reinterpret_cast<u32*>(soundBuffer)[(pos + i) % SOUND_BUFFER_SIZE] = tmpBuffer[BUFFER_CHUNK - i - 1];
		}
		break;
		}

		int tmp = soundPosA + BUFFER_CHUNK;
		if (tmp >= SOUND_BUFFER_SIZE)
			tmp -= SOUND_BUFFER_SIZE;
		soundPosA = tmp;
	}

	flags = 1;
	GameLinker::WriteMemory(address - sizeof(flags), &flags, sizeof(flags));
}

bool Speaker::DrawGui()
{
	const char *dataTypes[] = {"byte", "short", "word", "float"};
	bool result = DrawGuiBase("Speaker");
	ImGui::Text("Sound stream state: %s", HasAudioStream() ? "OPEN" : "CLOSED");
	if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.02f", ImGuiSliderFlags_AlwaysClamp) && HasAudioStream())
	{
#ifdef USE_MINIAUDIO
		volume_atom = volume;
#else
		SDL_SetAudioStreamGain(boundStream, volume);
#endif
	}

	ImGui::Text("Current frequency: %d", frequency);
	ImGui::Text("Current data type: %s", dataTypes[dataType]);
	ImGui::Text("Current channel count: %d", stereo ? 2 : 1);

	ImGui::InputInt("Frequency", &frequencyGui);
	ImGui::ListBox("Data type", &dataTypeGui, dataTypes, sizeof(dataTypes) / sizeof(dataTypes[0]));
	ImGui::Checkbox("Stereo", &stereoGui);
	
	if (ImGui::Button("Create stream"))
		result |= CreateAudioStream(static_cast<DataType>(dataTypeGui), frequencyGui, stereoGui);
	if (ImGui::Button("Stop stream"))
		StopAudioStream();
	ImGui::EndChild();

	return result;
}

u64 Speaker::GetSize()
{
	u32 dataSize = dataType == Byte ? 1 : (dataType == Short ? 2 : 4);
	return BUFFER_OFFSET + BUFFER_CHUNK * dataSize * (stereo ? 2 : 1);
}

void SDLCALL data_callback_sdl(void *userdata, SDL_AudioStream *astream, int additional_amount, int total_amount)
{
	Speaker *self = reinterpret_cast<Speaker*>(userdata);
	self->_SoundUpdateSDL(astream, additional_amount);
}

void data_callback_ma(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	Speaker *self = reinterpret_cast<Speaker*>(pDevice->pUserData);
	self->_SoundUpdateMA(pOutput, frameCount);
}

void Speaker::_SoundUpdateSDL(SDL_AudioStream *stream, s32 frame_count)
{
	s32 dif = (soundPosA - soundPosB);
	if (dif == 0)
		return;

	u32 mult = 1;
	if (dataType == Short)
	{
		mult = 2;
		frame_count >>= 1;
	}
	else if (dataType != Byte)
	{
		mult = 4;
		frame_count >>= 2;
	}


	if (dif < 0)
		dif += SOUND_BUFFER_SIZE;
	if (dif > frame_count)
		dif = frame_count;
	if (soundPosB + dif >= SOUND_BUFFER_SIZE)
	{
		u32 dt = soundPosB + dif - SOUND_BUFFER_SIZE;
		SDL_PutAudioStreamData(stream, reinterpret_cast<u8*>(soundBuffer) + soundPosB * mult, (dif-dt) * mult);
		SDL_PutAudioStreamData(stream, reinterpret_cast<u8*>(soundBuffer), dt * mult);
		soundPosB += dif - SOUND_BUFFER_SIZE;
	}
	else
	{
		SDL_PutAudioStreamData(stream, reinterpret_cast<u8*>(soundBuffer) + soundPosB * mult, dif * mult);
		soundPosB += dif;
	}	
	SDL_FlushAudioStream(stream);
}

void Speaker::_SoundUpdateMA(void *stream, u32 frame_count)
{
	s32 dif = (soundPosA - soundPosB);
	if (dif == 0)
		return;
	float vol = volume_atom;

	switch (dataType)
	{
	case Speaker::Byte:
	{
		u8 *buffer = reinterpret_cast<u8*>(stream);
		s8 *bufferIn = reinterpret_cast<s8*>(soundBuffer);
		for (u32 i = 0; i < frame_count; i++)
		{
			if (soundPosA == soundPosB)
				*buffer = 0;
			else
			{
				*buffer = static_cast<s8>(bufferIn[soundPosB++] * vol) ^ 0x80;
				if (soundPosB >= SOUND_BUFFER_SIZE)
					soundPosB = 0;
			}
			buffer++;
		}
	}
	break;
	case Speaker::Short:
	{
		s16 *buffer = reinterpret_cast<s16*>(stream);
		s16 *bufferIn = reinterpret_cast<s16*>(soundBuffer);
		for (u32 i = 0; i < frame_count; i++)
		{
			if (soundPosA == soundPosB)
				*buffer = 0;
			else
			{
				*buffer = static_cast<s16>(bufferIn[soundPosB++] * vol);
				if (soundPosB >= SOUND_BUFFER_SIZE)
					soundPosB = 0;
			}
			buffer++;
		}
	}
	break;
	case Speaker::Word:
	{
		s32 *buffer = reinterpret_cast<s32*>(stream);
		s32 *bufferIn = reinterpret_cast<s32*>(soundBuffer);
		for (u32 i = 0; i < frame_count; i++)
		{
			if (soundPosA == soundPosB)
				*buffer = 0;
			else
			{
				*buffer = static_cast<s32>(bufferIn[soundPosB++] * vol);
				if (soundPosB >= SOUND_BUFFER_SIZE)
					soundPosB = 0;
			}
			buffer++;
		}
	}
	break;
	case Speaker::Float:
	{
		float *buffer = reinterpret_cast<float*>(stream);
		float *bufferIn = reinterpret_cast<float*>(soundBuffer);
		for (u32 i = 0; i < frame_count; i++)
		{
			if (soundPosA == soundPosB)
				*buffer = 0;
			else
			{
				*buffer = bufferIn[soundPosB++] * vol;
				if (soundPosB >= SOUND_BUFFER_SIZE)
					soundPosB = 0;
			}
			buffer++;
		}
	}
	break;
	}
}

bool Speaker::CreateAudioStream(DataType type, u32 freq, bool stereoIn)
{
	if (HasAudioStream())
		StopAudioStream();

	dataType = type;
	frequency = freq;
	stereo = stereoIn;
	return CreateAudioStream();
}

bool Speaker::CreateAudioStream()
{
	if (HasAudioStream())
	{
		StopAudioStream();
	}

	soundPosA = 0;
	soundPosB = 0;
	
#ifdef USE_MINIAUDIO
	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	switch (dataType)
	{
	case Speaker::Byte:
		config.playback.format = ma_format_u8;
		break;
	case Speaker::Short:
		config.playback.format = ma_format_s16;
		break;
	case Speaker::Word:
		config.playback.format = ma_format_s32;
		break;
	case Speaker::Float:
		config.playback.format = ma_format_f32;
		break;
	default:
		config.playback.format = ma_format_f32;
		break;
	}

	config.playback.channels = stereo ? 2: 1;
	config.sampleRate        = frequency;
	config.dataCallback      = data_callback_ma;
	config.pUserData         = this;

	ma_result res = ma_device_init(NULL, &config, device);
	if (res != MA_SUCCESS)
	{
		SDL_LogWarn(0, "Could not create audio stream: %d", res);
		return false;
	}
	
	res = ma_device_start(device);
	if (res != MA_SUCCESS)
	{
		SDL_LogWarn(0, "Couldn't resume audio stream: %d", res);
		return false;
	}

#else
	SDL_AudioSpec spec;
	spec.freq = frequency;
	spec.channels = stereo ? 2 : 1;
	switch (dataType)
	{
	case Speaker::Byte:
		spec.format = SDL_AUDIO_S8;
		break;
	case Speaker::Short:
		spec.format = SDL_AUDIO_S16;
		break;
	case Speaker::Word:
		spec.format = SDL_AUDIO_S32;
		break;
	case Speaker::Float:
		spec.format = SDL_AUDIO_F32;
		break;
	default:
		spec.format = SDL_AUDIO_F32;
		break;
	}

	boundStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, data_callback_sdl, this);
	if (!HasAudioStream())
	{
		SDL_LogWarn(0, "Couldn't create audio stream: %s", SDL_GetError());
		return false;
	}
	SDL_SetAudioStreamGain(boundStream, volume);

	if (!SDL_ResumeAudioStreamDevice(boundStream))
	{
		SDL_LogWarn(0, "Couldn't resume audio stream: %s", SDL_GetError());
		return false;
	}
#endif

	return true;
}

bool Speaker::HasAudioStream() const
{
#ifdef USE_MINIAUDIO
	return ma_device_is_started(device);
#else
	return boundStream != nullptr;
#endif
}

void Speaker::StopAudioStream()
{
	if (!HasAudioStream())
		return;
#ifdef USE_MINIAUDIO
	ma_device_uninit(device);
#else
	SDL_PauseAudioStreamDevice(boundStream);
	SDL_UnbindAudioStream(boundStream);
	boundStream = nullptr;
#endif
}
