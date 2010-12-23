/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Mortimer
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

#ifdef USE_OPEN_AL
extern "C"
{
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
}
#endif

#include <vector>

namespace C4SoundLoaders
{
	struct SoundInfo
	{
	public:
		double sample_length;
		uint32_t sample_rate;
		std::vector<BYTE> sound_data;
#ifdef USE_OPEN_AL
		ALenum format;
#endif
		C4SoundHandle final_handle;

		SoundInfo(): sound_data(), final_handle(NULL) {}
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
		virtual bool ReadInfo(SoundInfo* info, BYTE* data, size_t data_length, uint32_t options = 0) = 0;
	};

#if defined(USE_OPEN_AL) && defined(__APPLE__)
	class AppleSoundLoader: public SoundLoader
	{
	public:
		AppleSoundLoader(): SoundLoader() {}
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	protected:
		static AppleSoundLoader singleton;
	};
#endif

#ifdef USE_OPEN_AL
	class VorbisLoader: public SoundLoader
	{
	private:
		struct CompressedData
		{
		public:
			BYTE* data;
			size_t data_length;
			size_t data_pos;
			CompressedData(BYTE* data, size_t data_length): data(data), data_length(data_length), data_pos(0)
			{}
		};
		static size_t read_func(void* ptr, size_t byte_size, size_t size_to_read, void* datasource);
		static int seek_func(void* datasource, ogg_int64_t offset, int whence);
		static int close_func(void* datasource);
		static long tell_func(void* datasource);
	public:
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	protected:
		static VorbisLoader singleton;
	};
#endif

#ifdef HAVE_LIBSDL_MIXER
	class SDLMixerSoundLoader: public SoundLoader
	{
	public:
		static SDLMixerSoundLoader singleton;
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t);
	};
#endif

#ifdef HAVE_FMOD
	class FMODSoundLoader: public SoundLoader
	{
	public:
		static FMODSoundLoader singleton;
		virtual bool ReadInfo(SoundInfo* result, BYTE* data, size_t data_length, uint32_t options);
	};
#endif
}
