/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "platform/C4SoundLoaders.h"
#include "platform/C4MusicFile.h"

#include "game/C4Application.h"


using namespace C4SoundLoaders;

SoundLoader* SoundLoader::first_loader(nullptr);

#if AUDIO_TK == AUDIO_TK_OPENAL && defined(__APPLE__)
namespace
{
	static OSStatus AudioToolBoxReadCallback(void* context, SInt64 inPosition, UInt32 requestCount, void* buffer, UInt32* actualCount)
	{
		CFDataRef audio = (CFDataRef)context;
		CFIndex audio_length = CFDataGetLength(audio);
		int requestOffset = inPosition + requestCount;
		int availableBytes = audio_length - inPosition;
		if (requestOffset > audio_length) {
			*actualCount = availableBytes;
		} else {
			*actualCount = requestCount;
		}
		CFRange range;
		range.location = inPosition;
		range.length = *actualCount;
		CFDataGetBytes(audio, range, (UInt8*)buffer);
		return noErr;
	}
	static SInt64 AudioToolBoxGetSizeProc(void* context)
	{
		return CFDataGetLength((CFDataRef)context);
	}
}

bool AppleSoundLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	CFDataRef data_container = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, data, data_length, kCFAllocatorNull);
	AudioFileID sound_file;
	OSStatus err = AudioFileOpenWithCallbacks((void*)data_container,
		AudioToolBoxReadCallback,
		nullptr,
		AudioToolBoxGetSizeProc,
		nullptr,
		0,
		&sound_file
	);
	if (err == noErr)
	{
		UInt32 property_size;
	
		uint64_t sound_data_size = 0;
		property_size = sizeof(sound_data_size);
		AudioFileGetProperty(sound_file, kAudioFilePropertyAudioDataByteCount, &property_size, &sound_data_size);
		
		result->sample_length = -1;
		property_size = sizeof(result->sample_length);
		AudioFileGetProperty(sound_file, kAudioFilePropertyEstimatedDuration, &property_size, &result->sample_length);

		UInt32 sound_data_size_32 = sound_data_size;
		result->sound_data.resize(sound_data_size);
		AudioFileReadBytes(sound_file, false, 0, &sound_data_size_32, &result->sound_data[0]);
		
		AudioStreamBasicDescription desc;
		property_size = sizeof(desc);
		AudioFileGetProperty(sound_file, kAudioFilePropertyDataFormat, &property_size, &desc);
		result->sample_rate = desc.mSampleRate;
		
		switch (desc.mChannelsPerFrame)
		{
		case 1:
			result->format = desc.mBitsPerChannel == 16 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
			break;
		case 2:
			result->format = desc.mBitsPerChannel == 16 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
			break;
		default:
			result->format = 0;
		}
	}
	CFRelease(data_container);
	return err == noErr;
}

AppleSoundLoader AppleSoundLoader::singleton;
#endif

#if AUDIO_TK == AUDIO_TK_OPENAL
size_t VorbisLoader::mem_read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource)
{
	size_t spaceToEOF;
	size_t actualSizeToRead;
	CompressedData* data = (CompressedData*)datasource;
	
	spaceToEOF = data->data_length - data->data_pos;
	if (size_to_read*byte_size < spaceToEOF)
		actualSizeToRead = size_to_read*byte_size;
	else
		actualSizeToRead = spaceToEOF;
	
	if (actualSizeToRead)
	{
		memcpy(ptr, (char*)data->data + data->data_pos, actualSizeToRead);
		data->data_pos += actualSizeToRead;
	}
	
	return actualSizeToRead;
}

int VorbisLoader::mem_seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	CompressedData* data = (CompressedData*)datasource;
	
	switch (whence)
	{
	case SEEK_SET:
		data->data_pos = static_cast<size_t>(offset) < data->data_length ? static_cast<size_t>(offset) : data->data_length;
		break;
	case SEEK_CUR:
		data->data_pos += static_cast<size_t>(offset) < data->data_length - data->data_pos ? static_cast<size_t>(offset) : data->data_length - data->data_pos;
		break;
	case SEEK_END:
		data->data_pos = data->data_length+1;
		break;
	}
	return 0;
}

int VorbisLoader::mem_close_func(void* datasource)
{
	return 1;
}

long VorbisLoader::mem_tell_func(void* datasource)
{
	return ((CompressedData*)datasource)->data_pos;
}

size_t VorbisLoader::file_read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource)
{
	C4MusicFileOgg *ogg = static_cast<C4MusicFileOgg *>(datasource);
	size_t bytes_to_read = size_to_read*byte_size;
	size_t bytes_read = 0u;
	ogg->source_file.Read(ptr, bytes_to_read, &bytes_read);
	return bytes_read / byte_size;
}

int VorbisLoader::file_seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	C4MusicFileOgg *ogg = static_cast<C4MusicFileOgg *>(datasource);
	return ogg->source_file.Seek(static_cast<long int>(offset), whence);
}

int VorbisLoader::file_close_func(void* datasource)
{
	C4MusicFileOgg *ogg = static_cast<C4MusicFileOgg *>(datasource);
	ogg->source_file.Close();
	return 1;
}

long VorbisLoader::file_tell_func(void* datasource)
{
	C4MusicFileOgg *ogg = static_cast<C4MusicFileOgg *>(datasource);
	return ogg->source_file.Tell();
}

bool VorbisLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	CompressedData compressed(data, data_length);

	int endian = 0;

	vorbis_info* info;
	OggVorbis_File ogg_file;
	memset(&ogg_file, 0, sizeof(ogg_file));
	ov_callbacks callbacks;
	callbacks.read_func  = &mem_read_func;
	callbacks.seek_func = &mem_seek_func;
	callbacks.close_func = &mem_close_func;
	callbacks.tell_func = &mem_tell_func;
	
	// open using callbacks
	if (ov_open_callbacks(&compressed, &ogg_file, nullptr, 0, callbacks) != 0)
	{
		ov_clear(&ogg_file);
		return false;
	}

	// get information about sound
	info = ov_info(&ogg_file, -1);
	if (info->channels == 1)
		result->format = AL_FORMAT_MONO16;
	else
		result->format = AL_FORMAT_STEREO16;
	result->sample_rate = info->rate;
	result->sample_length = ov_time_total(&ogg_file, -1)/1000.0;

	// Compute the total buffer size
	const unsigned long total_size = static_cast<unsigned int>(result->sample_rate * result->sample_length * 1000.0 * info->channels * 2 + 0.5);
	const unsigned long extra_size = 1024 * 8;

	// read: try to read the whole track in one go, and if there is more data
	// than we predicted in total_size, read the additional data in 8K blocks.
	unsigned long buffer_size = 0;
	long bytes_read;
	do {
		const int chunk_size = total_size + extra_size - std::min(total_size, buffer_size);
		int bitStream;
		if (buffer_size+chunk_size > result->sound_data.size())
			result->sound_data.resize(buffer_size+chunk_size);
		bytes_read = ov_read(&ogg_file, (char*)&result->sound_data[buffer_size], chunk_size*sizeof(BYTE), endian, 2, 1, &bitStream);
		buffer_size += bytes_read;
	} while (bytes_read > 0);
	result->sound_data.resize(buffer_size);
	
	// clear ogg file
	ov_clear(&ogg_file);
	return true;
}

VorbisLoader VorbisLoader::singleton;

#ifndef __APPLE__
bool WavLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	// load WAV resource
	Application.MusicSystem.SelectContext();
	ALuint wav = alutCreateBufferFromFileImage((const ALvoid *)data, data_length);
	if (wav == AL_NONE)
	{
		return false;
	}

	// get information about sound
	ALint freq, chans, bits, size;
	alGetBufferi(wav, AL_FREQUENCY, &freq);
	result->sample_rate = freq;
	alGetBufferi(wav, AL_CHANNELS, &chans);
	alGetBufferi(wav, AL_BITS, &bits);
	alGetBufferi(wav, AL_SIZE, &size);
	if (chans == 1)
		if (bits == 8)
			result->format = AL_FORMAT_MONO8;
		else
			result->format = AL_FORMAT_MONO16;
	else
		if (bits == 8)
			result->format = AL_FORMAT_STEREO8;
		else
			result->format = AL_FORMAT_STEREO16;
	// can't find any function to determine sample length
	// just calc ourselves
	result->sample_length = double(size) / double(bits*chans*freq/8);
	// buffer loaded
	result->final_handle = wav;
	return true;
}

WavLoader WavLoader::singleton;
#endif

#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>

bool SDLMixerSoundLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	// Be paranoid about SDL_Mixer initialisation
	if (!Application.MusicSystem.IsMODInitialized())
		{ return false; }
	SDL_RWops * rwops = SDL_RWFromConstMem(data, data_length);
	// work around double free in SDL_Mixer by passing 0 here
	result->final_handle = Mix_LoadWAV_RW(rwops, 0);
	SDL_RWclose(rwops);
	if (!result->final_handle)
		{ return false; }
	//FIXME: Is this actually correct?
	result->sample_length = result->final_handle->alen / (44100 * 2);
	result->sample_rate = 0;
	return true;
}

SDLMixerSoundLoader SDLMixerSoundLoader::singleton;

#endif
