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
/* Handles Music Files */

#include "C4Include.h"
#include "platform/C4MusicFile.h"

#include "game/C4Application.h"
#include "lib/C4Log.h"

#if AUDIO_TK == AUDIO_TK_OPENAL
#if defined(__APPLE__)
#import <CoreFoundation/CoreFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#else
#ifdef _WIN32
// This is an ugly hack to make FreeALUT not dllimport everything.
#define _XBOX
#endif
#include <alut.h>
#undef _XBOX
#endif
#define alErrorCheck(X) do { X; { ALenum err = alGetError(); if (err) LogF("al error: %s (%x)", #X, err); } } while (0)
#endif

/* helpers */

void C4MusicFile::Announce()
{
	LogF(LoadResStr("IDS_PRC_PLAYMUSIC"), GetFilename(FileName));
	announced = true;
}

bool C4MusicFile::ExtractFile()
{
	// safety
	if (SongExtracted) return true;
	// extract entry
	if (!C4Group_CopyItem(FileName, Config.AtTempPath(C4CFN_TempMusic2))) return false;
	// ok
	SongExtracted = true;
	return true;
}

bool C4MusicFile::RemTempFile()
{
	if (!SongExtracted) return true;
	// delete it
	EraseFile(Config.AtTempPath(C4CFN_TempMusic2));
	SongExtracted = false;
	return true;
}

bool C4MusicFile::Init(const char *szFile)
{
	SCopy(szFile, FileName);
	return true;
}

#if AUDIO_TK == AUDIO_TK_SDL_MIXER
C4MusicFileSDL::C4MusicFileSDL():
		Data(nullptr),
		Music(nullptr)
{
}

C4MusicFileSDL::~C4MusicFileSDL()
{
	Stop();
}

bool C4MusicFileSDL::Play(bool loop, double max_resume_time)
{
	const SDL_version * link_version = Mix_Linked_Version();
	if (link_version->major < 1
	    || (link_version->major == 1 && link_version->minor < 2)
	    || (link_version->major == 1 && link_version->minor == 2 && link_version->patch < 7))
	{
		// Check existance and try extracting it
		if (!FileExists(FileName)) if (!ExtractFile())
				// Doesn't exist - or file is corrupt
			{
				LogF("Error reading %s", FileName);
				return false;
			}
		// Load
		Music = Mix_LoadMUS(SongExtracted ? Config.AtTempPath(C4CFN_TempMusic2) : FileName);
		// Load failed
		if (!Music)
		{
			LogF("SDL_mixer: %s", SDL_GetError());
			return false;
		}
		// Play Song
		if (Mix_PlayMusic(Music, loop? -1 : 1) == -1)
		{
			LogF("SDL_mixer: %s", SDL_GetError());
			return false;
		}
	}
	else
	{
		// Load Song
		// Fixme: Try loading this from the group incrementally for less lag
		size_t filesize;
		if (!C4Group_ReadFile(FileName, &Data, &filesize))
		{
			LogF("Error reading %s", FileName);
			return false;
		}
		// Mix_FreeMusic frees the RWop
		Music = Mix_LoadMUS_RW(SDL_RWFromConstMem(Data, filesize), 1);
		if (!Music)
		{
			LogF("SDL_mixer: %s", SDL_GetError());
			return false;
		}
		if (Mix_PlayMusic(Music, loop? -1 : 1) == -1)
		{
			LogF("SDL_mixer: %s", SDL_GetError());
			return false;
		}
	}
	return true;
}

void C4MusicFileSDL::Stop(int fadeout_ms)
{
	if (fadeout_ms && Music)
	{
		// Don't really stop yet
		Mix_FadeOutMusic(fadeout_ms);
		return;
	}
	if (Music)
	{
		Mix_FreeMusic(Music);
		Music = nullptr;
	}
	RemTempFile();
	if (Data)
	{
		delete[] Data;
		Data = nullptr;
	}
}

void C4MusicFileSDL::CheckIfPlaying()
{
	if (!Mix_PlayingMusic())
		Application.MusicSystem.NotifySuccess();
}

void C4MusicFileSDL::SetVolume(int iLevel)
{
	Mix_VolumeMusic((int) ((iLevel * MIX_MAX_VOLUME) / 100));
}

#elif AUDIO_TK == AUDIO_TK_OPENAL

/* Ogg Vobis */

C4MusicFileOgg::C4MusicFileOgg() :
	playing(false), streaming_done(false), loaded(false), channel(0), current_section(0), byte_pos_total(0), volume(1.0f),
	is_loading_from_file(false), last_source_file_pos(0), last_playback_pos_sec(0), last_interruption_time()
{
	for (size_t i=0; i<num_buffers; ++i)
		buffers[i] = 0;
}

C4MusicFileOgg::~C4MusicFileOgg()
{
	Clear();
	Stop();
}

void C4MusicFileOgg::Clear()
{
	// clear ogg file
	if (loaded)
	{
		ov_clear(&ogg_file);
		loaded = false;
	}
	categories.clear();
	is_loading_from_file = false;
	source_file.Close();
	last_source_file_pos = 0;
	last_playback_pos_sec = 0;
	last_interruption_time = C4TimeMilliseconds();
}

bool C4MusicFileOgg::Init(const char *strFile)
{
	// Clear previous
	Clear();
	// Base init file
	if (!C4MusicFile::Init(strFile)) return false;
	// Prepare ogg reader
	vorbis_info* info;
	memset(&ogg_file, 0, sizeof(ogg_file));
	ov_callbacks callbacks;
	// Initial file loading
	// For packed groups, the whole compressed file is kept in memory because reading/seeking inside C4Group is problematic. Uncompress while playing.
	// This increases startup time a bit.
	// Later, this could be replaced with proper random access in c4group. Either replacing the file format or e.g. storing the current zlib state here
	//  and then updating callbacks.read/seek/close/tell_func to read data from the group directly as needed
	bool is_loading_from_file = FileExists(strFile);
	void *data_source;
	if (!is_loading_from_file)
	{
		char *file_contents;
		size_t file_size;
		if (!C4Group_ReadFile(FileName, &file_contents, &file_size))
			return false;
		data.SetOwnedData((BYTE *)file_contents, file_size);
		// C4Group preloaded ogg reader
		callbacks.read_func = &::C4SoundLoaders::VorbisLoader::mem_read_func;
		callbacks.seek_func = &::C4SoundLoaders::VorbisLoader::mem_seek_func;
		callbacks.close_func = &::C4SoundLoaders::VorbisLoader::mem_close_func;
		callbacks.tell_func = &::C4SoundLoaders::VorbisLoader::mem_tell_func;
		data_source = &data;
	}
	else
	{
		// Load directly from file
		if (!source_file.Open(FileName))
			return false;
		// Uncompressed file ogg reader
		callbacks.read_func = &::C4SoundLoaders::VorbisLoader::file_read_func;
		callbacks.seek_func = &::C4SoundLoaders::VorbisLoader::file_seek_func;
		callbacks.close_func = &::C4SoundLoaders::VorbisLoader::file_close_func;
		callbacks.tell_func = &::C4SoundLoaders::VorbisLoader::file_tell_func;
		data_source = this;
	}

	// open using callbacks either to memory or to file loader
	if (ov_open_callbacks(data_source, &ogg_file, nullptr, 0, callbacks) != 0)
	{
		ov_clear(&ogg_file);
		return false;
	}

	// get information about music
	info = ov_info(&ogg_file, -1);
	if (info->channels == 1)
		ogg_info.format = AL_FORMAT_MONO16;
	else
		ogg_info.format = AL_FORMAT_STEREO16;
	ogg_info.sample_rate = info->rate;
	ogg_info.sample_length = ov_time_total(&ogg_file, -1) / 1000.0;
	
	// Get categories from ogg comment header
	vorbis_comment *comment = ov_comment(&ogg_file, -1);
	const char *comment_id = "COMMENT=";
	int comment_id_len = strlen(comment_id);
	for (int i = 0; i < comment->comments; ++i)
	{
		if (comment->comment_lengths[i] > comment_id_len)
		{
			if (SEqual2NoCase(comment->user_comments[i], comment_id, comment_id_len))
			{
				// Add all categories delimeted by ';'
				const char *categories_string = comment->user_comments[i] + comment_id_len;
				for (;;)
				{
					int delimeter = SCharPos(';', categories_string);
					StdCopyStrBuf category;
					category.Copy(categories_string, delimeter >= 0 ? delimeter : SLen(categories_string));
					categories.push_back(category);
					if (delimeter < 0) break;
					categories_string += delimeter+1;
				}
			}
		}
	}

	// File not needed for now
	UnprepareSourceFileReading();

	// mark successfully loaded
	return loaded = true;
}

StdStrBuf C4MusicFileOgg::GetDebugInfo() const
{
	StdStrBuf result;
	result.Append(FileName);
	result.AppendFormat("[%.0lf]", last_playback_pos_sec);
	result.AppendChar('[');
	bool sec = false;
	for (auto i = categories.cbegin(); i != categories.cend(); ++i)
	{
		if (sec) result.AppendChar(',');
		result.Append(i->getData());
		sec = true;
	}
	result.AppendChar(']');
	return result;
}

void C4MusicFileOgg::UnprepareSourceFileReading()
{
	// The file loader could just keep all files open. But if someone symlinks
	// Music.ocg into their music folder with a million files in it, we would
	// crash with too many open file handles. So close it for now and reopen
	// when that piece is actually requested.
	if (is_loading_from_file && source_file.IsOpen())
	{
		last_source_file_pos = source_file.Tell();
		source_file.Close();
	}
}

bool C4MusicFileOgg::PrepareSourceFileReading()
{
	// mem loading always OK
	if (!is_loading_from_file) return true;
	// ensure file is open
	if (!source_file.IsOpen())
	{
		if (!source_file.Open(FileName)) return false;
		if (last_source_file_pos) if (source_file.Seek(last_source_file_pos, SEEK_SET) < 0) return false;
	}
	return true;
}

bool C4MusicFileOgg::Play(bool loop, double max_resume_time)
{
	// Valid file?
	if (!loaded) return false;
	// stop previous
	if (playing)
	{
		if (max_resume_time > 0.0) return true; // no-op
		Stop();
	}
	// Ensure data reading is ready
	PrepareSourceFileReading();
	// Get channel to use
	alGenSources(1, (ALuint*)&channel);
	if (!channel) return false;

	playing = true;
	streaming_done = false;
	this->loop = loop;
	byte_pos_total = 0;

	// Resume setting
	if (max_resume_time > 0)
	{
		// Only resume if significant amount of data is left to be played
		double time_remaining_sec = GetRemainingTime();
		if (time_remaining_sec < max_resume_time) last_playback_pos_sec = 0.0;
	}
	else
	{
		last_playback_pos_sec = 0;
	}

	// initial volume setting
	SetVolume(float(::Config.Sound.MusicVolume) / 100.0f);

	// prepare read
	ogg_info.sound_data.resize(num_buffers * buffer_size);
	alGenBuffers(num_buffers, buffers);
	ov_time_seek(&ogg_file, last_playback_pos_sec);

	// Fill initial buffers
	for (size_t i=0; i<num_buffers; ++i)
		if (!FillBuffer(i)) break; // if this fails, the piece is shorter than the initial buffers

	// play!
	alErrorCheck(alSourcePlay(channel));
	
	return true;
}

double C4MusicFileOgg::GetRemainingTime()
{
	// Note: Only valid after piece has been stopped
	return ov_time_total(&ogg_file, -1) - last_playback_pos_sec;
}

void C4MusicFileOgg::Stop(int fadeout_ms)
{
	if (playing)
	{
		// remember position for eventual later resume
		ALfloat playback_pos_in_buffer = 0;
		alErrorCheck(alGetSourcef(channel, AL_SEC_OFFSET, &playback_pos_in_buffer));
		last_playback_pos_sec += playback_pos_in_buffer;
		last_interruption_time = C4TimeMilliseconds::Now();
		// stop!
		alSourceStop(channel);
		// clear queue
		ALint num_queued=0;
		alErrorCheck(alGetSourcei(channel, AL_BUFFERS_QUEUED, &num_queued));
		ALuint buffer;
		for (size_t i = 0; i < (size_t)num_queued; ++i)
			alErrorCheck(alSourceUnqueueBuffers(channel, 1, &buffer));
	}
	// clear buffers
	if (channel) 
	{
		alSourcei(channel, AL_BUFFER, 0);
		alDeleteBuffers(num_buffers, buffers);
		alDeleteSources(1, &channel);
	}
	playing = false;
	channel = 0;
	// close file
	UnprepareSourceFileReading();
}

void C4MusicFileOgg::CheckIfPlaying()
{
	Execute();
	if (!playing) Application.MusicSystem.NotifySuccess();
}

void C4MusicFileOgg::SetVolume(int iLevel)
{
	volume = ((float) iLevel) / 100.0f;
	if (channel) alSourcef(channel, AL_GAIN, volume);
}

bool C4MusicFileOgg::HasCategory(const char *szcat) const
{
	if (!szcat) return false;
	// check all stored categories
	for (auto i = categories.cbegin(); i != categories.cend(); ++i)
		if (WildcardMatch(szcat, i->getData()))
			return true;
	return false;
}

bool C4MusicFileOgg::FillBuffer(size_t idx)
{
	// uncompress from ogg data
	int endian = 0;
	long bytes_read_total = 0, bytes_read;
	char uncompressed_data[buffer_size];
	do {
		bytes_read = ov_read(&ogg_file, uncompressed_data+bytes_read_total, (buffer_size-bytes_read_total)*sizeof(BYTE), endian, 2, 1, &current_section);
		bytes_read_total += bytes_read;
	} while (bytes_read > 0 && bytes_read_total < buffer_size);
	// buffer data
	if (bytes_read_total)
	{
		byte_pos_total += bytes_read_total;
		ALuint buffer = buffers[idx];
		alErrorCheck(alBufferData(buffer, ogg_info.format, uncompressed_data, bytes_read_total, ogg_info.sample_rate));
		// queue buffer
		alErrorCheck(alSourceQueueBuffers(channel, 1, &buffer));
	}
	// streaming done?
	if (bytes_read_total < buffer_size)
	{
		// streaming done. loop or done.
		if (loop)
		{
			// reset pos in ogg file
			ov_raw_seek(&ogg_file, 0);
			// if looping and nothing has been committed to this buffer yet, try again
			// except if byte_pos_total==0, i.e. if the piece is completely empty
			size_t prev_bytes_total = byte_pos_total;
			byte_pos_total = 0;
			if (!bytes_read_total && prev_bytes_total) return FillBuffer(idx);
			return true;
		}
		else
		{
			// non-looping: we're done.
			return false;
		}
	}
	else
	{
		// might have more data to stream
		return true;
	}
}

void C4MusicFileOgg::Execute()
{
	if (playing)
	{
		// get processed buffer count
		ALint num_processed = 0;
		alErrorCheck(alGetSourcei(channel, AL_BUFFERS_PROCESSED, &num_processed));
		bool done = false;
		while (num_processed--)
		{
			// release processed buffer
			ALuint buffer; 
			alErrorCheck(alSourceUnqueueBuffers(channel, 1, &buffer));
			// add playback time of processed buffer to total playback time
			ALint buf_bits = 16, buf_chans = 2, buf_freq = 44100;
			alErrorCheck(alGetBufferi(buffer, AL_BITS, &buf_bits));
			alErrorCheck(alGetBufferi(buffer, AL_CHANNELS, &buf_chans));
			alErrorCheck(alGetBufferi(buffer, AL_FREQUENCY, &buf_freq));
			double buffer_secs = double(buffer_size) / buf_bits / buf_chans / buf_freq * 8;
			last_playback_pos_sec += buffer_secs;
			// refill processed buffer
			size_t buffer_idx;
			for (buffer_idx=0; buffer_idx<num_buffers; ++buffer_idx)
				if (buffers[buffer_idx] == buffer) break;
			if (!done) done = !FillBuffer(buffer_idx);
		}
		if (done) streaming_done = true;
		// check if done
		ALint state = 0;
		alErrorCheck(alGetSourcei(channel, AL_SOURCE_STATE, &state));
		if (state != AL_PLAYING && streaming_done)
		{
			Stop();
			// reset playback to beginning for next time this piece is playing
			last_playback_pos_sec = 0.0;
		}
		else if (state == AL_STOPPED)
		{
			alErrorCheck(alSourcePlay(channel));
		}
	}
}

#endif
