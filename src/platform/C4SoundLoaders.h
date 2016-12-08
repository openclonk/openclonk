/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#ifndef INC_C4SoundLoaders
#define INC_C4SoundLoaders

#include <vector>
#include "platform/C4SoundIncludes.h"
#include "platform/C4SoundSystem.h"

namespace C4SoundLoaders
{
	struct SoundInfo
	{
	public:
		double sample_length;
		uint32_t sample_rate;
		std::vector<BYTE> sound_data;
#if AUDIO_TK == AUDIO_TK_OPENAL
		ALenum format;
#endif
		C4SoundHandle final_handle;

		SoundInfo(): sound_data(), final_handle(0) {}
	};
	
	class SoundLoader
	{
	public:
		static const int OPTION_Raw = 1;
	public:
		static SoundLoader* first_loader;
		SoundLoader* next;
		SoundLoader()
		{
			next = first_loader;
			first_loader = this;
		}
		virtual ~SoundLoader() {}
		virtual bool ReadInfo(SoundInfo* info, BYTE* data, size_t data_length, uint32_t options = 0) = 0;
	};

#if AUDIO_TK == AUDIO_TK_OPENAL && defined(__APPLE__)
	class AppleSoundLoader: public SoundLoader
	{
	public:
		AppleSoundLoader(): SoundLoader() {}
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	protected:
		static AppleSoundLoader singleton;
	};
#endif

#if AUDIO_TK == AUDIO_TK_OPENAL
	class VorbisLoader: public SoundLoader
	{
	public: // needed by C4MusicFileOgg
		struct CompressedData
		{
		public:
			BYTE* data;
			size_t data_length;
			size_t data_pos;
			bool is_data_owned; // if true, dtor will delete data
			CompressedData(BYTE* data, size_t data_length): data(data), data_length(data_length), data_pos(0), is_data_owned(false) {}
			CompressedData() : data(nullptr), data_length(0), data_pos(0), is_data_owned(false) {}
			void SetOwnedData(BYTE* data, size_t data_length)
			{ clear(); this->data=data; this->data_length=data_length; this->data_pos=0; is_data_owned=true; }

			~CompressedData() { clear(); }
			void clear()  { if (is_data_owned) delete [] data; data=nullptr; }
		};
		// load from compressed data held in memory buffer
		static size_t mem_read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource);
		static int mem_seek_func(void* datasource, ogg_int64_t offset, int whence);
		static int mem_close_func(void* datasource);
		static long mem_tell_func(void* datasource);
		// load directly from file
		static size_t file_read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource);
		static int file_seek_func(void* datasource, ogg_int64_t offset, int whence);
		static int file_close_func(void* datasource);
		static long file_tell_func(void* datasource);
	public:
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	protected:
		static VorbisLoader singleton;
	};
#ifndef __APPLE__
	// non-apple wav loader: using ALUT
	class WavLoader: public SoundLoader
	{
	public:
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	protected:
		static WavLoader singleton;
	};
#endif // apple

#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
	class SDLMixerSoundLoader: public SoundLoader
	{
	public:
		static SDLMixerSoundLoader singleton;
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	};
#endif
}

#endif
