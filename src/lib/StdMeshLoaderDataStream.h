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

#ifndef INC_StdMeshLoaderDataStream
#define INC_StdMeshLoaderDataStream

#include "StdMeshLoader.h"

namespace Ogre
{
	class DataStream
	{
		const char *begin, *cursor, *end;
	public:
		DataStream(const char *src, size_t length)
		{
			begin = cursor = src;
			end = cursor + length;
		}

		// non-copyable
		DataStream(const DataStream&) = delete;
		DataStream& operator=(const DataStream&) = delete;

		bool AtEof() const { return cursor == end; }
		size_t GetRemainingBytes() const { return end - cursor; }
		void Rewind() { cursor = begin; }
		void Seek(ptrdiff_t offset)
		{
			if (offset > 0 && GetRemainingBytes() < static_cast<size_t>(offset))
				throw InsufficientData();
			if (cursor - begin < -offset)
				throw InsufficientData();
			cursor += offset;
		}

		// Only read directly into T when T is trivially copyable (i.e., allows bit-wise copy)
		template<class T>
		typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value, T>::type
		Peek() const
		{
			if (GetRemainingBytes() < sizeof(T))
				throw InsufficientData();
			T temp;
			std::memcpy(reinterpret_cast<char*>(&temp), cursor, sizeof(T));
			return temp;
		}

		// declaration for non-trivially copyable types
		template<class T> typename std::enable_if<!std::is_pod<T>::value, T>::type
		Peek() const;

		template<class T> T Read()
		{
			T temp = Peek<T>();
			Seek(sizeof(T));
			return temp;
		}
		void Peek(void *dest, size_t size) const
		{
			if (GetRemainingBytes() < size)
				throw InsufficientData();
			std::memcpy(dest, cursor, size);
		}
		void Read(void *dest, size_t size)
		{
			Peek(dest, size);
			cursor += size;
		}
	};

	template<> inline bool DataStream::Peek<bool>() const
	{
		if (GetRemainingBytes() < 1)
			throw InsufficientData();
		return *cursor != '\0';
	}

	template<> inline std::string DataStream::Peek<std::string>() const
	{
		// Ogre terminates strings with \n
		const char *terminator = static_cast<const char*>(std::memchr(cursor, '\n', GetRemainingBytes()));
		if (!terminator)
			throw InsufficientData("Unterminated string");
		return std::string(cursor, terminator);
	}

	template<> inline std::string DataStream::Read<std::string>()
	{
		std::string temp = Peek<std::string>();
		Seek(temp.size() + 1);
		return temp;
	}

}

#endif
