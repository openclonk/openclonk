/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

#ifdef USE_OPEN_AL
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#include <vector>
#include <C4SoundSystem.h>

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
#endif // openal

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
