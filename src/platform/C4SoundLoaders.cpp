/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010 Mortimer 
 * Copyright (c) 2003-2004  Peter Wortmann
 * Copyright (c) 2005-2006, 2008  Sven Eberhardt
 * Copyright (c) 2005-2006  Günther Brammer
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

#if defined(USE_OPEN_AL) && defined(__APPLE__)
#import <CoreFoundation/CoreFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#endif

#ifdef USE_OPEN_AL
extern "C"
{
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
}
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

bool AppleSoundLoader::ReadInfo(SoundInfo& info, BYTE* data, size_t data_length, uint32_t)
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
	
		info.sound_data_size = 0;
		property_size = sizeof(info.sound_data_size);
		AudioFileGetProperty(sound_file, kAudioFilePropertyAudioDataByteCount, &property_size, &info.sound_data_size);
		
		info.sample_length = -1;
		property_size = sizeof(info.sample_length);
		AudioFileGetProperty(sound_file, kAudioFilePropertyEstimatedDuration, &property_size, &info.sample_length);
		
		UInt32 sound_data_size_32 = info.sound_data_size;
		info.sound_data = malloc(info.sound_data_size);
		AudioFileReadBytes(sound_file, false, 0, &sound_data_size_32, info.sound_data);
		
		AudioStreamBasicDescription desc;
		property_size = sizeof(desc);
		AudioFileGetProperty(sound_file, kAudioFilePropertyDataFormat, &property_size, &desc);
		info.sample_rate = desc.mSampleRate;
		
		switch (desc.mChannelsPerFrame)
		{
		case 1:
			info.format = desc.mBitsPerChannel == 16 ? AL_FORMAT_MONO16 : AL_FORMAT_MONO8;
			break;
		case 2:
			info.format = desc.mBitsPerChannel == 16 ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO8;
			break;
		default:
			info.format = 0;
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
	VorbisLoader* loader = (VorbisLoader*)datasource;
	
	spaceToEOF = loader->data_length - loader->data_pos;
	if (size_to_read*byte_size < spaceToEOF)
		actualSizeToRead = size_to_read*byte_size;
	else
		actualSizeToRead = spaceToEOF;
	
	if (actualSizeToRead)
	{
		memcpy(ptr, (char*)loader->data + loader->data_pos, actualSizeToRead);
		loader->data_pos += actualSizeToRead;
	}
	
	return actualSizeToRead;
}

int VorbisLoader::seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	size_t spaceToEOF;
	ogg_int64_t actualOffset;	// How much we can actually offset it by
	VorbisLoader* loader = (VorbisLoader*)datasource;
	
	switch (whence)
	{
	case SEEK_SET:
		loader->data_pos = offset < loader->data_length ? offset : loader->data_length;
		break;
	case SEEK_CUR:
		loader->data_pos += offset < loader->data_length - loader->data_pos ? offset : loader->data_length - loader->data_pos;
		break;
	case SEEK_END:
		loader->data_pos = loader->data_length+1;
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
	return ((VorbisLoader*)datasource)->data_pos;
}

bool VorbisLoader::ReadInfo(SoundInfo& result, BYTE* data, size_t data_length, uint32_t)
{
	this->data = data;
	this->data_length = data_length;
	this->data_pos = 0;

	int endian = 0;
	int bitStream;
	long bytes;
	char array[32768];
	
	vorbis_info* info;
	OggVorbis_File ogg_file;
	memset(&ogg_file, 0, sizeof(ogg_file));
	ov_callbacks callbacks;
	callbacks.read_func  = &read_func;
	callbacks.seek_func  = &seek_func;
	callbacks.close_func = &close_func;
	callbacks.tell_func  = &tell_func;
	
	// open using callbacks
	if (ov_open_callbacks(this, &ogg_file, NULL, 0, callbacks) != 0)
	{
		ov_clear(&ogg_file);
		return false;
	}

	info = ov_info(&ogg_file, -1);
	if (info->channels == 1)
		result.format = AL_FORMAT_MONO16;
	else
		result.format = AL_FORMAT_STEREO16;
	result.sample_rate = info->rate;
	
	std::vector<BYTE> buffer;
	do {
		bytes = ov_read(&ogg_file, array, sizeof(array)/sizeof(array[0]), endian, 2, 1, &bitStream);
		for (int i = 0; i < bytes; i++)
			buffer.push_back(array[i]);
	} while (bytes > 0);
	
	result.sound_data = malloc(buffer.size());
	result.sound_data_size = buffer.size();
	for (int i = 0; i < buffer.size(); i++)
		((BYTE*)result.sound_data)[i] = buffer[i];
	result.sample_length = 5;

	ov_clear(&ogg_file);
	return true;
}

VorbisLoader VorbisLoader::singleton;
#endif

#ifdef HAVE_LIBSDL_MIXER
bool SDLMixerSoundLoader::ReadInfo(SoundInfo& result, BYTE* data, size_t data_length, uint32_t)
{
	// Be paranoid about SDL_Mixer initialisation
	if (!Application.MusicSystem.IsMODInitialized())
		{ return false; }
	if (!(result.final_handle = Mix_LoadWAV_RW(SDL_RWFromConstMem(data, data_length), 1)))
		{ return false; }
	//FIXME: Is this actually correct?
	result.sample_length = result.final_handle->alen / (44100 * 2);
	result.sample_rate = 0;
	return true;
}

SDLMixerSoundLoader SDLMixerSoundLoader::singleton;
#endif

#ifdef HAVE_FMOD
bool FMODSoundLoader::ReadInfo(SoundInfo& result, BYTE* data, size_t data_length, uint32_t options)
{
	int32_t iOptions = FSOUND_NORMAL | FSOUND_2D | FSOUND_LOADMEMORY;
	if (options & OPTION_Raw)
		iOptions |= FSOUND_LOADRAW;
	SoundHandle pSample;
	if (!(pSample = FSOUND_Sample_Load(FSOUND_UNMANAGED, (const char *)data, iOptions, 0, data_length)))
		{ Clear(); return false; }
	// get length
	int32_t iSamples = FSOUND_Sample_GetLength(pSample);
	int iSampleRate = SampleRate;
	if (!iSamples || !FSOUND_Sample_GetDefaults(pSample, &iSampleRate, 0, 0, 0))
		return false;
	result.sample_rate = iSampleRate;
	result.sample_length = iSamples / iSampleRate;
	result.final_handle = pSample;
	return true;
}

FMODSoundLoader FMODSoundLoader::singleton;
#endif