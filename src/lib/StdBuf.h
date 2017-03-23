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
// Standard buffer classes

#ifndef STDBUF_H
#define STDBUF_H

#include "platform/PlatformAbstraction.h"

#include <zlib.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdarg>
#include <algorithm>

// debug memory management
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

// Base buffer class. Either references or holds data.
class StdBuf
{
public:

	// *** Construction
	// Standard constructor
	StdBuf() = default;

	// Constructor from other buffer (copy construction):
	// Will take over buffer ownership. Copies data if specified.
	// Note: Construct with Buf2.getRef() to construct a reference (This will work for a constant Buf2, too)
	StdBuf(StdBuf & Buf2, bool fCopy = false)
			: fRef(true), pData(nullptr), iSize(0)
	{
		if (fCopy)
			Copy(Buf2);
		else if (!Buf2.isRef())
			Take(std::move(Buf2));
		else
			Ref(Buf2);
	}
	StdBuf(const StdBuf & Buf2, bool fCopy = true)
			: fRef(true), pData(nullptr), iSize(0)
	{
		if (fCopy)
			Copy(Buf2);
		else
			Ref(Buf2);
	}
	StdBuf(StdBuf && Buf2) noexcept
			: fRef(true), pData(nullptr), iSize(0)
	{
		if (!Buf2.isRef())
			Take(std::move(Buf2));
		else
			Ref(Buf2);
	}

	// Set by constant data. Copies data if desired.
	StdBuf(const void *pData, size_t iSize, bool fCopy = false)
			: fRef(true), pData(pData), iSize(iSize)
	{
		if (fCopy) Copy();
	}

	~StdBuf()
	{
		Clear();
	}

protected:

	// Reference? Otherwise, this object holds the data.
	bool fRef = true;
	// Data
	union
	{
		const void *pData = 0;
		void *pMData;
#if defined(_DEBUG)
		char *szString; // for debugger preview
#endif
	};
	unsigned int iSize = 0;

public:

	// *** Getters

	bool        isNull()  const { return ! getData(); }
	const void *getData() const { return fRef ? pData : pMData; }
	void       *getMData()      { assert(!fRef); return pMData; }
	size_t      getSize() const { return iSize; }
	bool        isRef()   const { return fRef; }

	const void *getPtr(size_t i) const { return reinterpret_cast<const char*>(getData()) + i; }
	void       *getMPtr(size_t i)      { return reinterpret_cast<char*>(getMData()) + i; }

	StdBuf getPart(size_t iStart, size_t inSize) const
	{
		assert(iStart + inSize <= iSize);
		return StdBuf(getPtr(iStart), inSize);
	}

	// *** Setters

	// * Direct setters

	// Reference given data
	void Ref(const void *pnData, size_t inSize)
	{
		Clear();
		fRef = true; pData = pnData; iSize = inSize;
	}
	// Take over data (hold it)
	void Take(void *pnData, size_t inSize)
	{
		Clear();
		if (pnData)
		{
			fRef = false; pMData = pnData; iSize = inSize;
		}
	}
	// Transfer puffer ownership to the caller
	void *GrabPointer()
	{
		if (isNull()) return nullptr;
		// Do not give out a buffer which someone else will free
		if (fRef) Copy();
		void *pMData = getMData();
		pData = pMData; fRef = true;
		return pMData;
	}

	// * Buffer data operations

	// Create new buffer with given size
	void New(size_t inSize)
	{
		Clear();
		pMData = malloc(iSize = inSize);
		fRef = false;
	}
	// Write data into the buffer
	void Write(const void *pnData, size_t inSize, size_t iAt = 0)
	{
		assert(iAt + inSize <= iSize);
		if (pnData && inSize) std::memcpy(getMPtr(iAt), pnData, inSize);
	}
	// Move data around inside the buffer (checks overlap)
	void Move(size_t iFrom, size_t inSize, size_t iTo = 0)
	{
		assert(iFrom + inSize <= iSize); assert(iTo + inSize <= iSize);
		std::memmove(getMPtr(iTo), getPtr(iFrom), inSize);
	}
	// Compare to memory
	int Compare(const void *pCData, size_t iCSize, size_t iAt = 0) const
	{
		assert(iAt + iCSize <= getSize());
		return std::memcmp(getPtr(iAt), pCData, iCSize);
	}
	// Grow the buffer
	void Grow(size_t iGrow)
	{
		// Grow dereferences
		if (fRef) { Copy(iSize + iGrow); return; }
		if (!iGrow) return;
		// Realloc
		pMData = realloc(pMData, iSize += iGrow);
	}
	// Shrink the buffer
	void Shrink(size_t iShrink)
	{
		assert(iSize >= iShrink);
		// Shrink dereferences
		if (fRef) { Copy(iSize - iShrink); return; }
		if (!iShrink) return;
		// Realloc
		pMData = realloc(pMData, iSize -= iShrink);
	}
	// Clear buffer
	void Clear()
	{
		if (!fRef) free(pMData);
		pMData = nullptr; fRef = true; iSize = 0;
	}
	// Free buffer that had been grabbed
	static void DeletePointer(void *data)
	{
		free(data);
	}

	// * Composed actions

	// Set buffer size (dereferences)
	void SetSize(size_t inSize)
	{
		if (inSize > iSize)
			Grow(inSize - iSize);
		else
			Shrink(iSize - inSize);
	}

	// Write buffer contents into the buffer
	void Write(const StdBuf &Buf2, size_t iAt = 0)
	{
		Write(Buf2.getData(), Buf2.getSize(), iAt);
	}

	// Compare (a part of) this buffer's contents to another's
	int Compare(const StdBuf &Buf2, size_t iAt = 0) const
	{
		return Compare(Buf2.getData(), Buf2.getSize(), iAt);
	}

	// Create a copy of the data (dereferences, obviously)
	void Copy(size_t inSize)
	{
		if (isNull() && !inSize) return;
		const void *pOldData = getData();
		size_t iOldSize = iSize;
		New(inSize);
		Write(pOldData, std::min(iOldSize, inSize));
	}
	void Copy()
	{
		Copy(iSize);
	}
	// Copy data from address
	void Copy(const void *pnData, size_t inSize)
	{
		Ref(pnData, inSize); Copy();
	}
	// Copy from another buffer
	void Copy(const StdBuf &Buf2)
	{
		Copy(Buf2.getData(), Buf2.getSize());
	}
	// Create a copy and return it
	StdBuf Duplicate() const
	{
		StdBuf Buf; Buf.Copy(*this); return Buf;
	}

	// Append data from address
	void Append(const void *pnData, size_t inSize)
	{
		Grow(inSize);
		Write(pnData, inSize, iSize - inSize);
	}
	// Append data from another buffer
	void Append(const StdBuf &Buf2)
	{
		Append(Buf2.getData(), Buf2.getSize());
	}

	// Reference another buffer's contents
	void Ref(const StdBuf &Buf2)
	{
		Ref(Buf2.getData(), Buf2.getSize());
	}
	// Create a reference to this buffer's contents
	StdBuf getRef() const
	{
		return StdBuf(getData(), getSize());
	}
	// take over another buffer's contents
	void Take(StdBuf & Buf2)
	{
		Take(Buf2.GrabPointer(), Buf2.getSize());
	}
	void Take(StdBuf &&Buf2)
	{
		Take(Buf2.GrabPointer(), Buf2.getSize());
	}

	// * File support
	bool LoadFromFile(const char *szFile);
	bool SaveToFile(const char *szFile) const;

	// *** Operators

	// Null check
	bool operator ! () const { return isNull(); }

	// Appending
	StdBuf operator += (const StdBuf &Buf2)
	{
		Append(Buf2);
		return *this;
	}
	StdBuf operator + (const StdBuf &Buf2) const
	{
		StdBuf Buf(getRef());
		Buf.Append(Buf2);
		return Buf;
	}

	// Compare
	bool operator == (const StdBuf &Buf2) const
	{
		return getSize() == Buf2.getSize() && !Compare(Buf2);
	}
	bool operator != (const StdBuf &Buf2) const { return ! operator == (Buf2); }

	// Set (as constructor: take if possible)
	StdBuf &operator = (StdBuf &&Buf2)
	{
		if (Buf2.isRef()) Ref(Buf2); else Take(std::move(Buf2));
		return *this;
	}

	// build a simple hash
	int GetHash() const
	{
		if (isNull()) return 0;
		return crc32(0, reinterpret_cast<const Bytef *>(getData()), getSize());
	}

	// *** Compiling

	void CompileFunc(class StdCompiler *pComp, int iType = 0);

};

// Cast Hide Helpers - MSVC doesn't allow this as member template
template <class elem_t>
const elem_t *getBufPtr(const StdBuf &Buf, size_t iPos = 0)
{
	// assert(iPos + sizeof(elem_t) <= Buf.getSize());
	const void *pPos = reinterpret_cast<const char *>(Buf.getData()) + iPos;
	return reinterpret_cast<const elem_t *>(pPos);
}
template <class elem_t>
elem_t *getMBufPtr(StdBuf &Buf, size_t iPos = 0)
{
	// assert(iPos + sizeof(elem_t) <= Buf.getSize());
	void *pPos = reinterpret_cast<char *>(Buf.getMData()) + iPos;
	return reinterpret_cast<elem_t *>(pPos);
}

// Copy-Buffer - Just copies data in the copy constructor.
class StdCopyBuf : public StdBuf
{
public:

	StdCopyBuf() = default;

	// Set by buffer. Copies data by default.
	StdCopyBuf(const StdBuf &Buf2, bool fCopy = true)
			: StdBuf(Buf2.getRef(), fCopy)
	{ }

	// Set by buffer. Copies data by default.
	StdCopyBuf(const StdCopyBuf &Buf2, bool fCopy = true)
			: StdBuf(Buf2.getRef(), fCopy)
	{ }
	StdCopyBuf(StdBuf & Buf2) noexcept
			: StdBuf(std::move(Buf2))
	{ }
	StdCopyBuf(StdCopyBuf &&Buf2) noexcept
			: StdBuf(std::move(Buf2))
	{ }

	// Set by constant data. Copies data by default.
	StdCopyBuf(const void *pData, size_t iSize, bool fCopy = true)
			: StdBuf(pData, iSize, fCopy)
	{ }

	StdCopyBuf &operator = (const StdBuf &Buf2) { Copy(Buf2); return *this; }
	StdCopyBuf &operator = (const StdCopyBuf &Buf2) { Copy(Buf2); return *this; }

};

// Stringbuffer (operates on null-terminated character buffers)
class StdStrBuf : protected StdBuf
{
public:

	// *** Construction

	StdStrBuf() = default;

	// See StdBuf::StdBuf. Will take data if possible.
	// The static_cast is necessary to pass a rvalue reference to
	// the StdBuf constructor. Without it, the const lvalue
	// StdBuf constructor will be used, which will ref the contents
	// instead of moving them.
	StdStrBuf(StdStrBuf & Buf2, bool fCopy = false)
//			: StdBuf(static_cast<StdStrBuf &>(Buf2), fCopy)
			: StdBuf(Buf2, fCopy)
	{ }

	// This constructor is important, because the compiler will create one
	// otherwise, despite having two other constructors to choose from
	StdStrBuf(const StdStrBuf & Buf2, bool fCopy = true)
			: StdBuf(Buf2, fCopy)
	{ }
	StdStrBuf(StdStrBuf && Buf2) noexcept
			: StdBuf(std::move(Buf2))
	{ }

	// Set by constant data. References data by default, copies if specified.
	explicit StdStrBuf(const char *pData, bool fCopy = false)
			: StdBuf(pData, pData ? strlen(pData) + 1 : 0, fCopy)
	{ }

#ifdef _WIN32
	explicit StdStrBuf(const wchar_t * utf16);
	struct wchar_t_holder {
		wchar_t * p;
		wchar_t_holder(wchar_t * p): p(p) { }
		wchar_t_holder(const wchar_t_holder &);
		~wchar_t_holder() { delete[] p; }
		operator wchar_t * () { return p; }
	};
	wchar_t_holder GetWideChar() const;
	StdBuf GetWideCharBuf();
#endif

	// As previous constructor, but set length manually.
	StdStrBuf(const char *pData, long int iLength)
			: StdBuf(pData, pData ? iLength + 1 : 0, false)
	{ }
	StdStrBuf(const char *pData, size_t iLength, bool fCopy = false)
			: StdBuf(pData, pData ? iLength + 1 : 0, fCopy)
	{ }

public:

	// *** Getters

	bool        isNull()  const { return StdBuf::isNull(); }
	const char *getData() const { return getBufPtr<char>(*this); }
	char       *getMData()      { return getMBufPtr<char>(*this); }
	size_t      getSize() const { return StdBuf::getSize(); }
	size_t      getLength() const { return getSize() ? getSize() - 1 : 0; }
	bool        isRef()   const { return StdBuf::isRef(); }

	const char *getPtr(size_t i) const { return getBufPtr<char>(*this, i); }
	char       *getMPtr(size_t i)      { return getMBufPtr<char>(*this, i); }

	// For convenience. Note that writing can't be allowed.
	char operator [] (size_t i) const { return *getPtr(i); }

	// Analogous to StdBuf
	void Ref(const char *pnData) { StdBuf::Ref(pnData, pnData ? std::strlen(pnData) + 1 : 0); }
	void Ref(const char *pnData, size_t iLength) { assert((!pnData && !iLength) || std::strlen(pnData) == iLength); StdBuf::Ref(pnData, iLength + 1); }
	void Take(char *pnData) { StdBuf::Take(pnData, pnData ? std::strlen(pnData) + 1 : 0); }
	void Take(char *pnData, size_t iLength) { assert((!pnData && !iLength) || std::strlen(pnData) == iLength); StdBuf::Take(pnData, iLength + 1); }
	char *GrabPointer() { return reinterpret_cast<char *>(StdBuf::GrabPointer()); }

	void Ref(const StdStrBuf &Buf2) { StdBuf::Ref(Buf2.getData(), Buf2.getSize()); }
	StdStrBuf getRef() const { return StdStrBuf(getData(), getLength()); }
	void Take(StdStrBuf & Buf2) { StdBuf::Take(Buf2); }
	void Take(StdStrBuf &&Buf2) { StdBuf::Take(std::move(Buf2)); }
	
	void Clear() { StdBuf::Clear(); }
	void Copy() { StdBuf::Copy(); }
	void Copy(const char *pnData) { StdBuf::Copy(pnData, pnData ? std::strlen(pnData) + 1 : 0); }
	void Copy(const StdStrBuf &Buf2) { StdBuf::Copy(Buf2); }
	StdStrBuf Duplicate() const { StdStrBuf Buf; Buf.Copy(*this); return Buf; }
	void Move(size_t iFrom, size_t inSize, size_t iTo = 0) { StdBuf::Move(iFrom, inSize, iTo); }

	// Byte-wise compare (will compare this string from iAt to the full string in Buf2)
	int Compare(const StdStrBuf &Buf2, size_t iAt = 0) const
	{
		assert(iAt <= getLength());
		const int result = StdBuf::Compare(Buf2.getData(), std::min(getLength() - iAt, Buf2.getLength()), iAt);
		if (result) return result;

		if (getLength() < Buf2.getLength() + iAt) return -1;
		else if (getLength() > Buf2.getLength() + iAt) return 1;
		return 0;
	}
	int Compare_(const char *pCData, size_t iAt = 0) const
	{
		StdStrBuf str(pCData); // GCC needs this, for some obscure reason
		return Compare(str, iAt);
	}
	bool BeginsWith(const char *beginning) const
	{
		// Return whether string starts with beginning
		return strncmp((const char 
			*)pData, beginning, strlen(beginning)) == 0;
	}

	// Grows the string to contain the specified number more/less characters.
	// Note: Will set the terminator, but won't initialize - use Append* instead.
	void Grow(size_t iGrow)
	{
		StdBuf::Grow(getSize() ? iGrow : iGrow + 1);
		*getMPtr(getLength()) = '\0';
	}
	void Shrink(size_t iShrink)
	{
		assert(iShrink <= getLength());
		StdBuf::Shrink(iShrink);
		*getMPtr(getLength()) = '\0';
	}
	void SetLength(size_t iLength)
	{
		if (iLength == getLength() && !isNull()) return;
		if (iLength >= getLength())
			Grow(iLength - getLength());
		else
			Shrink(getLength() - iLength);
	}

	// Append string
	void Append(const char *pnData, size_t iChars)
	{
		Grow(iChars);
		Write(pnData, iChars, iSize - iChars - 1);
	}
	void Append(const char *pnData)
	{
		Append(pnData, std::strlen(pnData));
	}
	void Append(const StdStrBuf &Buf2)
	{
		Append(Buf2.getData(), Buf2.getLength());
	}

	// Copy string
	void Copy(const char *pnData, size_t iChars)
	{
		Clear();
		Append(pnData, iChars);
	}

	// * File support
	bool LoadFromFile(const char *szFile);
	bool SaveToFile(const char *szFile) const;

	// * Operators

	bool operator ! () const { return isNull(); }

	StdStrBuf &operator += (const StdStrBuf &Buf2) { Append(Buf2); return *this; }
	StdStrBuf &operator += (const char *szString) { Append(szString); return *this; }
	StdStrBuf operator + (const StdStrBuf &Buf2) const { StdStrBuf Buf = getRef(); Buf.Append(Buf2); return Buf; }
	StdStrBuf operator + (const char *szString) const { StdStrBuf Buf = getRef(); Buf.Append(szString); return Buf; }
	StdStrBuf operator + (char c) const { StdStrBuf Buf = getRef(); Buf.AppendChar(c); return Buf; }

	bool operator == (const StdStrBuf &Buf2) const
	{
		return getLength() == Buf2.getLength() && !Compare(Buf2);
	}
	bool operator != (const StdStrBuf &Buf2) const { return !operator == (Buf2); }

	bool operator == (const char *szString) const { return StdStrBuf(szString) == *this; }
	bool operator != (const char *szString) const { return ! operator == (szString); }

	// Note this references the data.
	StdStrBuf &operator = (const StdStrBuf &Buf2) { Ref(Buf2); return *this; }
	StdStrBuf &operator = (const char *szString) { Ref(szString); return *this; }

	// conversion to "bool"
	operator const void *() const { return getData(); }

	// less-than operation for map
	inline bool operator <(const StdStrBuf &v2) const
	{
		size_t iLen = getLength(), iLen2 = v2.getLength();
		if (iLen == iLen2)
			return iLen ? (std::strcmp(getData(), v2.getData())<0) : false;
		else
			return iLen < iLen2;
	}

	// * String specific

	void AppendChars(char cChar, size_t iCnt)
	{
		Grow(iCnt);
		for (size_t i = getLength() - iCnt; i < getLength(); i++)
			*getMPtr(i) = cChar;
	}
	void AppendChar(char cChar)
	{
		AppendChars(cChar, 1);
	}
	void AppendCharacter(uint32_t unicodechar);
	void AppendBackslash();
	void InsertChar(char cChar, size_t insert_before)
	{
		assert(insert_before <= getLength());
		Grow(1);
		for (size_t i = getLength()-1; i > insert_before; --i)
			*getMPtr(i) = *getPtr(i-1);
		*getMPtr(insert_before) = cChar;
	}

	// Append data until given character (or string end) occurs.
	void AppendUntil(const char *szString, char cUntil)
	{
		const char *pPos = std::strchr(szString, cUntil);
		if (pPos)
			Append(szString, pPos - szString);
		else
			Append(szString);
	}
	// See above
	void CopyUntil(const char *szString, char cUntil)
	{
		Clear();
		AppendUntil(szString, cUntil);
	}
	// cut end after given char into another string. Return whether char was found at all
	bool SplitAtChar(char cSplit, StdStrBuf *psSplit)
	{
		if (!getData()) return false;
		const char *pPos = std::strchr(getData(), cSplit);
		if (!pPos) return false;
		size_t iPos = pPos - getData();
		if (psSplit) psSplit->Take(copyPart(iPos + 1, getLength() - iPos - 1));
		Shrink(getLength() - iPos);
		return true;
	}

	void Format(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
	void FormatV(const char *szFmt, va_list args);
	void AppendFormat(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE_O;
	void AppendFormatV(const char *szFmt, va_list args);

	StdStrBuf copyPart(size_t iStart, size_t inSize) const
	{
		assert(iStart + inSize <= iSize);
		if (!inSize) return StdStrBuf();
		StdStrBuf sResult;
		sResult.Copy(getPtr(iStart), inSize);
		return sResult;
	}

	// replace all occurences of one string with another. Return number of replacements.
	int Replace(const char *szOld, const char *szNew, size_t iStartSearch=0);
	int ReplaceChar(char cOld, char cNew);

	// replace the trailing part of a string with something else
	void ReplaceEnd(size_t iPos, const char *szNewEnd);

	// get an indexed section from the string like Section1;Section2;Section3
	bool GetSection(size_t idx, StdStrBuf *psOutSection, char cSeparator=';') const;

	// Checks whether the content is valid UTF-8, and if not, convert it from windows-1252 to UTF-8 and return true.
	bool EnsureUnicode();

	// convert to lower case
	void ToLowerCase();

	// check if a string consists only of the given chars
	bool ValidateChars(const char *szInitialChars, const char *szMidChars);

	// build a simple hash
	int GetHash() const
	{
		return StdBuf::GetHash();
	}

	void EscapeString()
	{
		Replace("\\", "\\\\");
		Replace("\"", "\\\"");
	}

	bool TrimSpaces(); // kill spaces at beginning and end. Return if changed.

	// * Compiling

	void CompileFunc(class StdCompiler *pComp, int iRawType = 0);

};

// Copy-Stringbuffer - Just copies data in the copy constructor.
class StdCopyStrBuf : public StdStrBuf
{
public:

	StdCopyStrBuf() = default;

	explicit StdCopyStrBuf(const StdStrBuf &Buf2, bool fCopy = true)
			: StdStrBuf(Buf2.getRef(), fCopy)
	{ }

	StdCopyStrBuf(const StdCopyStrBuf &Buf2, bool fCopy = true)
			: StdStrBuf(Buf2.getRef(), fCopy)
	{ }
	StdCopyStrBuf(StdStrBuf && Buf2) noexcept
			: StdStrBuf(std::move(Buf2))
	{ }
	StdCopyStrBuf(StdCopyStrBuf && Buf2) noexcept
			: StdStrBuf(std::move(Buf2))
	{ }

	// Set by constant data. Copies data if desired.
	explicit StdCopyStrBuf(const char *pData, bool fCopy = true)
			: StdStrBuf(pData, fCopy)
	{ }

#ifdef _WIN32
	explicit StdCopyStrBuf(const wchar_t * utf16): StdStrBuf(utf16) {}
#endif

	StdCopyStrBuf(const std::string &s) noexcept
		: StdStrBuf(s.c_str(), s.size(), true)
	{ }

	StdCopyStrBuf &operator = (const StdStrBuf &Buf2) { Copy(Buf2); return *this; }
	StdCopyStrBuf &operator = (const StdCopyStrBuf &Buf2) { Copy(Buf2); return *this; }
	StdCopyStrBuf &operator = (const char *szString) { Copy(szString); return *this; }
	StdCopyStrBuf &operator = (const std::string &s) { Copy(s.c_str(), s.size()); return *this; }

	operator std::string() const
	{
		return std::string(getData(), getLength());
	}
};

// Wrappers
extern StdStrBuf FormatString(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE;
extern StdStrBuf FormatStringV(const char *szFmt, va_list args);

#ifdef _WIN32
// Converts a wide char string to UTF-8 std::string
std::string WStrToString(wchar_t *s);
#endif

#endif
