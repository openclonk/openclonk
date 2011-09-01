/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2004  Peter Wortmann
 * Copyright (c) 2005-2006, 2008  Sven Eberhardt
 * Copyright (c) 2005-2006  Günther Brammer
 * Copyright (c) 2010  Martin Plicht
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include "C4SoundLoaders.h"

#include <C4Application.h>

#if defined(USE_OPEN_AL) && defined(__APPLE__)
#import <CoreFoundation/CoreFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#endif

using namespace C4SoundLoaders;

SoundLoader* SoundLoader::first_loader(NULL);

#if defined(USE_OPEN_AL) && defined(__APPLE__)
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
		NULL,
		AudioToolBoxGetSizeProc,
		NULL,
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

#ifdef USE_OPEN_AL

size_t VorbisLoader::read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource)
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

int VorbisLoader::seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	size_t spaceToEOF;
	ogg_int64_t actualOffset; // How much we can actually offset it by
	CompressedData* data = (CompressedData*)datasource;
	
	switch (whence)
	{
	case SEEK_SET:
		data->data_pos = offset < data->data_length ? offset : data->data_length;
		break;
	case SEEK_CUR:
		data->data_pos += offset < data->data_length - data->data_pos ? offset : data->data_length - data->data_pos;
		break;
	case SEEK_END:
		data->data_pos = data->data_length+1;
		break;
	}
	return 0;
}

int VorbisLoader::close_func(void* datasource)
{
	return 1;
}

long VorbisLoader::tell_func(void* datasource)
{
	return ((CompressedData*)datasource)->data_pos;
}

bool VorbisLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	CompressedData compressed(data, data_length);

	int endian = 0;

	vorbis_info* info;
	OggVorbis_File ogg_file;
	memset(&ogg_file, 0, sizeof(ogg_file));
	ov_callbacks callbacks;
	callbacks.read_func  = &read_func;
	callbacks.seek_func  = &seek_func;
	callbacks.close_func = &close_func;
	callbacks.tell_func  = &tell_func;
	
	// open using callbacks
	if (ov_open_callbacks(&compressed, &ogg_file, NULL, 0, callbacks) != 0)
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

	// read
	unsigned long buffer_size = 0;
	long bytes_read;
	do {
		const int chunk_size = 1024*8;
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
#endif

#ifdef HAVE_LIBSDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>

bool SDLMixerSoundLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t)
{
	// Be paranoid about SDL_Mixer initialisation
	if (!Application.MusicSystem.IsMODInitialized())
		{ return false; }
	if (!(result->final_handle = Mix_LoadWAV_RW(SDL_RWFromConstMem(data, data_length), 1)))
		{ return false; }
	//FIXME: Is this actually correct?
	result->sample_length = result->final_handle->alen / (44100 * 2);
	result->sample_rate = 0;
	return true;
}

SDLMixerSoundLoader SDLMixerSoundLoader::singleton;
#endif

#ifdef HAVE_FMOD
bool FMODSoundLoader::ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t options)
{
	int32_t iOptions = FSOUND_NORMAL | FSOUND_2D | FSOUND_LOADMEMORY;
	if (options & OPTION_Raw)
		iOptions |= FSOUND_LOADRAW;
	C4SoundHandle pSample;
	if (!(pSample = FSOUND_Sample_Load(FSOUND_UNMANAGED, (const char *)data, iOptions, 0, data_length)))
		{ return false; }
	// get length
	int32_t iSamples = FSOUND_Sample_GetLength(pSample);
	int iSampleRate = 0;
	if (!iSamples || !FSOUND_Sample_GetDefaults(pSample, &iSampleRate, 0, 0, 0))
	{
		FSOUND_Sample_Free(pSample);
		return false;
	}
	result->sample_rate = iSampleRate;
	result->sample_length = static_cast<double>(iSamples) / iSampleRate;
	result->final_handle = pSample;
	assert(result->sample_length > 0);
	return true;
}

FMODSoundLoader FMODSoundLoader::singleton;
#endif
