/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005, 2007  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005, 2008-2009  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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
// Standard buffer classes

#ifndef STDBUF_H
#define STDBUF_H

#include "Standard.h"

#include <zlib.h>
#include <assert.h>
#include <stdarg.h>

// debug memory management
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include <string>

// Base buffer class. Either references or holds data.
class StdBuf : public std::string
{
public:
  // *** Construction
  // Standard constructor
	StdBuf() { }

  // Constructor from other buffer (copy construction):
  // Will take over buffer ownership. Copies data if specified.
  // Note: Construct with Buf2.getRef() to construct a reference (This will work for a constant Buf2, too)
	StdBuf(const std::string &other, bool = false)
		: std::string(other)
  {}

  // Set by constant data. Copies data if desired.
  StdBuf(const void *pData, size_t iSize, bool = false)
		: std::string(static_cast<const char*>(pData), iSize)
  {}

  ~StdBuf()
  {}

	ALLOW_TEMP_TO_REF(StdBuf)

public:

  // *** Getters

	bool        isNull()  const { return empty(); }
	const void *getData() const { return empty()?NULL:c_str(); }
  void       *getMData()      { return &operator[](0); }
  const void *getPtr(size_t i) const { return c_str() + i; }
  void       *getMPtr(size_t i)      { return &operator[](i); }
  size_t      getSize() const { return size(); }
  DEPRECATED bool        isRef()   const { return true; }


  StdBuf getPart(size_t iStart, size_t inSize) const
  {
    assert(iStart + inSize <= size());
    return StdBuf(getPtr(iStart), inSize);
  }

  // *** Setters

  // * Direct setters

  // Reference given data
  void Ref(const void *pnData, size_t inSize)
  {
		if(pnData)
			assign(static_cast<const char*>(pnData), inSize);
		else
			clear();
  }
  // Take over data (hold it)
  void Take(const void *pnData, size_t inSize) { Ref(pnData, inSize); }

	// Returns a copy of the contents
	char *GrabPointer() const
	{
		if(isNull()) return NULL;
		char *ptr = new char[size()+1];
		copy(ptr, size()); // doesn't null-terminate
		ptr[size()] = '\0';
		return ptr;
	}

  // * Buffer data operations

  // Create new buffer with given size
  void New(size_t inSize)
	{ resize(inSize); }
  // Write data into the buffer
  void Write(const void *pnData, size_t inSize, size_t iAt = 0)
  {
		replace(iAt, inSize, static_cast<const char*>(pnData), inSize);
  }
  // Move data around inside the buffer (checks overlap)
  void Move(size_t iFrom, size_t inSize, size_t iTo = 0)
  {
		replace(iTo, inSize, *this, iFrom, inSize);
  }
	// Compare to memory
	int Compare(const void *pCData, size_t iCSize, size_t iAt = 0) const
	{
		assert(iAt + iCSize <= getSize());
		return memcmp(getPtr(iAt), pCData, iCSize);
	}
  // Grow the buffer
  void Grow(size_t iGrow)
  {
    resize(size() + iGrow);
  }
  // Shrink the buffer
  void Shrink(size_t iShrink)
  {
    assert(size() >= iShrink);
    resize(size() - iShrink);
  }
  // Clear buffer
  void Clear()
  {
		clear();
  }
	// Free buffer that had been grabbed
	static void DeletePointer(const void *data)
	{
		delete[] static_cast<const char*>(data);
	}

  // * Composed actions

  // Set buffer size (dereferences)
  void SetSize(size_t inSize)
  {
    resize(inSize);
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

  DEPRECATED void Copy(size_t inSize) {}
  DEPRECATED void Copy() {}
	void Copy(const void *pnData, size_t inSize) { Ref(pnData, inSize); }
  // Copy from another buffer
	void Copy(const StdBuf &Buf2) { assign(Buf2); }

	// Create a copy and return it
	StdBuf Duplicate() const { return *this; }

  // Append data from address
  void Append(const void *pnData, int inSize)
  {
		append(static_cast<const char*>(pnData), inSize);
  }
  // Append data from another buffer
  void Append(const StdBuf &Buf2)
  {
    append(Buf2);
  }

	// Reference another buffer's contents
	void Ref(const StdBuf &Buf2) { assign(Buf2); }
	// Create a reference to this buffer's contents
	StdBuf getRef() const { return *this;	}
  // take over another buffer's contents
	void Take(const StdBuf &Buf2) { assign(Buf2); }

  // * File support
  bool LoadFromFile(const char *szFile);
  bool SaveToFile(const char *szFile) const;

  // *** Operators

  // Null check
	operator bool() const { return !empty(); }
	operator const void*() const { return data(); }
  bool operator ! () const { return isNull(); }

  // Appending
  StdBuf operator += (const StdBuf &Buf2) {
    Append(Buf2);
    return *this;
  }
  StdBuf operator + (const StdBuf &Buf2) const
  {
    StdBuf Buf(getRef());
    Buf.Append(Buf2);
    return Buf;
  }

  // Set (as constructor: take if possible)
	StdBuf &operator = (const std::string &Buf2)
	{
		assign(Buf2);
    return *this;
  }

  // build a simple hash
  int GetHash() const
  {
    if(isNull()) return 0;
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
typedef StdBuf StdCopyBuf;

// Stringbuffer (operates on null-terminated character buffers)
class StdStrBuf : public StdBuf
{
public:
  // *** Construction
  // Standard constructor
	StdStrBuf() { }

  // Constructor from other buffer (copy construction):
  // Will take over buffer ownership. Copies data if specified.
  // Note: Construct with Buf2.getRef() to construct a reference (This will work for a constant Buf2, too)
	StdStrBuf(const char *data, bool = false)
		: StdBuf(std::string(data))
	{}

	StdStrBuf(const std::string &other, bool = false)
		: StdBuf(other)
  {}

  // Set by constant data. Copies data if desired.
  StdStrBuf(const char *pData, size_t iSize, bool = false)
		: StdBuf(pData, iSize)
  {}

  ~StdStrBuf()
  {}

	ALLOW_TEMP_TO_REF(StdStrBuf)
public:

  // *** Getters
  size_t      getLength() const { return getSize() ? getSize() - 1 : 0; }
	const char *getData() const { return empty()?NULL:c_str(); }
  char       *getMData()      { return &operator[](0); }
  const char *getPtr(size_t i) const { return getData() + i; }
  char       *getMPtr(size_t i)      { return getMData() + i; }

  // Analogous to StdBuf
	using StdBuf::Ref;
  void Ref(const char *pnData) { if(pnData) assign(pnData); else clear(); }
	using StdBuf::Take;
  void Take(const char *pnData) { Ref(pnData); }
	using StdBuf::Copy;
  void Copy(const char *pnData) { Ref(pnData); }

	StdStrBuf getRef() const { return *this; }
	StdStrBuf Duplicate() const { return *this; }

	void SetLength(size_t inSize) { SetSize(inSize+1); }

	// * Operators

	StdStrBuf &operator += (const std::string &Buf2) { append(Buf2); return *this; }
  StdStrBuf &operator += (const char *szString) { Append(szString); return *this; }
	StdStrBuf operator + (const std::string &Buf2) const { StdStrBuf Buf = getRef(); Buf.append(Buf2); return Buf; }
  StdStrBuf operator + (const char *szString) const { StdStrBuf Buf = getRef(); Buf.append(szString); return Buf; }

	// Note this references the data.
	StdStrBuf &operator = (const std::string &Buf2) { assign(Buf2); return *this; }
  StdStrBuf &operator = (const char *szString) { Ref(szString); return *this; }

  // * String specific
	using StdBuf::Append;
	void Append(const char *str) { if(str) append(str); }

	void AppendChars(char cChar, size_t iCnt) { append(iCnt, cChar); }
	void AppendChar(char cChar) { push_back(cChar); }
	void InsertChar(char cChar, size_t insert_before) { insert(insert_before, 1, cChar); }

  // Append data until given character (or string end) occurs.
  void AppendUntil(const char *szString, char cUntil)
  {
    const char *pPos = strchr(szString, cUntil);
    if(pPos)
      Append(szString, pPos - szString);
    else
      append(szString);
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
		const char *pPos = strchr(getData(), cSplit);
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
    assert(iStart + inSize <= size());
		if (!inSize) return StdStrBuf();
		StdStrBuf sResult;
		sResult.Copy(getPtr(iStart), inSize);
		return sResult;
  }

	// replace all occurences of one string with another. Return number of replacements.
	int Replace(const char *szOld, const char *szNew, size_t iStartSearch=0);
	int ReplaceChar(char cOld, char cNew, size_t iStartSearch=0);

	// replace the trailing part of a string with something else
	void ReplaceEnd(size_t iPos, const char *szNewEnd);

	// get an indexed section from the string like Section1;Section2;Section3
	bool GetSection(size_t idx, StdStrBuf *psOutSection, char cSeparator=';') const;

	// Checks wether the contents are valid UTF-8, and if not, convert them from windows-1252 to UTF-8.
	void EnsureUnicode();

	// convert to lower case
	void ToLowerCase();

	// check if a string consists only of the given chars
	bool ValidateChars(const char *szInitialChars, const char *szMidChars);

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
typedef StdStrBuf StdCopyStrBuf;

// Wrappers
extern StdStrBuf FormatString(const char *szFmt, ...) GNUC_FORMAT_ATTRIBUTE;
extern StdStrBuf FormatStringV(const char *szFmt, va_list args);

#endif
