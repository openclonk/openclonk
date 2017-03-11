/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// user input validation functions

#ifndef INC_C4InputValidation
#define INC_C4InputValidation

#include "lib/StdBuf.h"
#include "platform/StdFile.h"
#include "lib/StdAdaptors.h"

const unsigned int C4MaxName = 30; // player names, etc.
const unsigned int C4MaxLongName = 120; // scenario titles, etc. - may include markup
const unsigned int C4MaxComment = 256; // network game and player comments

namespace C4InVal
{
	// validation options
	enum ValidationOption
	{
		VAL_Filename,        // regular filenames only (Sven2.ocp)
		VAL_SubPathFilename, // filenames and optional subpath (Spieler\Sven2.ocp)
		VAL_FullPath,        // full filename paths (C:\Clonk\Sven2.ocp; ..\..\..\..\AutoExec.bat)
		VAL_NameAllowEmpty,  // stuff like player names (Sven2). No markup. Max. C4MaxName characters. Spaces trimmed.
		VAL_NameNoEmpty,     // same as above, but empty string not allowed
		VAL_NameExAllowEmpty,// stuff like Clonk names (Joki the {{WIPF}}). Markup allowed. Max. C4MaxLongName  characters. Spaces trimmed.
		VAL_NameExNoEmpty,   // same as above, but empty string not allowed
		VAL_IRCName,         // nickname for IRC. a-z, A-Z, _^{[]} only; 0-9|- inbetween
		VAL_IRCPass,         // password for IRC
		VAL_IRCChannel,      // IRC channel name
		VAL_Comment         // comment - just a length limit
	};
	// input validation functions: Validate input by changing it to an allowed value if possible
	// issues warnings in log and returns true if such an action is performed
	bool ValidateString(char *szString, ValidationOption eOption, size_t iMaxSize);
	bool ValidateString(StdStrBuf &rsString, ValidationOption eOption);
	bool ValidateInt(int32_t &riVal, int32_t iMinVal, int32_t iMaxVal);

	inline bool ValidateFilename(char *szFilename, size_t iMaxSize=_MAX_PATH) { return ValidateString(szFilename, VAL_Filename, iMaxSize); }
}

// Validation adapter: Call ValidateString on string after compiling it
template <class T> struct C4StrValAdapt
{
	T &rValue; C4InVal::ValidationOption eValType;
	explicit C4StrValAdapt(T &rValue, C4InVal::ValidationOption eValType) : rValue(rValue), eValType(eValType) { }
	inline void CompileFunc(StdCompiler *pComp)
	{
		pComp->Value(rValue);
		if (pComp->isDeserializer()) C4InVal::ValidateString(rValue.GetObj(), eValType); // works on Par adapt only :(
	}
	template <class D> inline bool operator == (const D &nValue) const { return rValue == nValue; }
	template <class D> inline C4StrValAdapt<T> &operator = (const D &nValue) { rValue = nValue; return *this; }
};
template <class T> inline C4StrValAdapt<T> mkStrValAdapt(T &&rValue, C4InVal::ValidationOption eValType) { return C4StrValAdapt<T>(rValue, eValType); }

// StdStrBuf that validates on compilation
struct ValidatedStdCopyStrBufBase : public StdCopyStrBuf
{
	ValidatedStdCopyStrBufBase(const char *szCopy) : StdCopyStrBuf(szCopy) {}
	ValidatedStdCopyStrBufBase() : StdCopyStrBuf() {}

	inline void CompileFunc(StdCompiler *pComp, int iRawType = 0)
	{
		pComp->Value(mkParAdapt(static_cast<StdCopyStrBuf &>(*this), iRawType));
		if (pComp->isDeserializer()) Validate();
	}

	virtual bool Validate() = 0;

	void CopyValidated(const char *szFromVal)
	{
		Copy(szFromVal);
		Validate();
	}
	void CopyValidated(const StdStrBuf &sFromVal)
	{
		Copy(sFromVal);
		Validate();
	}

	virtual ~ValidatedStdCopyStrBufBase() { }
};

template <int V> struct ValidatedStdCopyStrBuf : public ValidatedStdCopyStrBufBase
{
	ValidatedStdCopyStrBuf(const char *szCopy) : ValidatedStdCopyStrBufBase(szCopy) {}
	ValidatedStdCopyStrBuf() : ValidatedStdCopyStrBufBase() {}

	virtual bool Validate()
	{
		return C4InVal::ValidateString(*this, (C4InVal::ValidationOption) V);
	}

	template <class D> inline bool operator == (const D &nValue) const { return static_cast<const StdCopyStrBuf &>(*this) == nValue; }
	template <class D> inline ValidatedStdCopyStrBuf<V> &operator = (const D &nValue) { static_cast<StdCopyStrBuf &>(*this) = nValue; return *this; }
};

#endif // INC_C4InputValidation
