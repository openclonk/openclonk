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
#ifndef STDCOMPILER_H
#define STDCOMPILER_H

// Try to avoid casting NotFoundExceptions for trivial cases (MSVC log flood workaround)
#if defined(_MSC_VER)
#define STDCOMPILER_EXCEPTION_WORKAROUND
#endif

// Provides an interface of generalized compiling/decompiling
// (serialization/deserialization - note that the term "compile" is used for both directions)

// The interface is designed to allow both text-type (INI) and binary
// compilation. Structures that want to support StdCompiler must provide
// a function "void CompileFunc(StdCompiler *)" and therein issue calls
// to the data, naming and separation functions as appropriate. If the structure
// in question cannot be changed, it is equally valid to define a function
// void CompileFunc(StdCompiler *, T *) where T is the type of the structure.

// Most details can be hidden inside adaptors (see StdAdaptors.h), so
// the structure can re-use common compiling patterns (namings, arrays...).

class StdCompiler
{

public:

	StdCompiler() = default;

	// *** Overridables (Interface)
	virtual ~StdCompiler() = default;

	// * Properties

	// Needs two passes? Binary compiler uses this for calculating the size.
	virtual bool isDoublePass()                   { return false; }

	// Changes the target?
	virtual bool isDeserializer()                 { return false; }
	inline  bool isSerializer()                   { return !isDeserializer(); }

	// Does the compiler support naming, so values can be omitted without harm to
	// the data structure? Is separation implemented?
	virtual bool hasNaming()                      { return false; }

	// Does the compiler encourage verbosity (like producing more text instead of
	// just a numerical value)?
	virtual bool isVerbose()                      { return hasNaming(); }

	// Is it a registry compiler with special handling for arrays?
	virtual bool isRegistry()                     { return false; }

	// callback by runtime-write-allowed adaptor used by compilers that may set runtime values only
	virtual void setRuntimeWritesAllowed(int32_t iChange) { }

	// * Naming
	// Provides extra data for the compiler so he can deal with reordered data.
	// Note that sections stack and each section will get compiled only once.
	// StartSection won't fail if the naming isn't found while compiling. Name and
	// all value compiling functions will fail, though.
	// Set the NameEnd parameter to true if you are stopping to parse the structure
	// for whatever reason (suppress warning messages).
	virtual bool Name(const char *szName)         { return true; }
	virtual void NameEnd(bool fBreak = false)     { }
	virtual const char *GetNameByIndex(size_t idx) const { return nullptr; }

	// Special: A naming that follows to the currently active naming (on the same level).
	// Note this will end the current naming, so no additional NameEnd() is needed.
	// Only used to maintain backwards compatibility, should not be used in new code.
	virtual bool FollowName(const char *szName)   { NameEnd(); return Name(szName); }

	// Called when a named value omitted because of defaulting (compiler only)
	// Returns whether the value has been handled
	virtual bool Default(const char *szName)      { return true; }

	// Return count of sub-namings. May be unimplemented.
	virtual int NameCount(const char *szName = nullptr) { assert(false); return 0; }


	// * Separation
	// Some data types need separation (note that naming makes this unnecessary).
	// Compilers that implement naming must implement separation. Others may just
	// always return success.
	// If a separator wasn't found, some compilers might react by throwing a
	// NotFound exception for all attempts to read a value. This behaviour will
	// stop when NoSeparator() is called (which just resets this state) or
	// Separator() is called successfully. This behaviour will reset after
	// ending the naming, too.
	enum Sep
	{
		SEP_NONE=0, // No separator ("")
		SEP_SEP, // Array separation (",")
		SEP_SEP2, // Array separation 2 (";")
		SEP_SET, // Map pair separation ("=")
		SEP_PART, // Value part separation (".")
		SEP_PART2, // Value part separation 2 (":")
		SEP_PLUS, // Value separation with a '+' char ("+")
		SEP_START, // Start some sort of list ('(')
		SEP_END, // End some sort of list ('(')
		SEP_START2, // Start some sort of list ('[')
		SEP_END2, // End some sort of list (']')
		SEP_VLINE, // Vertical line separator ('|')
		SEP_DOLLAR // Dollar sign ('$')
	};
	virtual bool Separator(Sep eSep = SEP_SEP)    { return true; }
	virtual void NoSeparator()                    { }

	// * Data
	// Compiling functions for different data types
	virtual void DWord(int32_t &rInt)             = 0; // Needs separator!
	virtual void DWord(uint32_t &rInt)            = 0; // Needs separator!
	virtual void Word(int16_t &rShort)            = 0; // Needs separator!
	virtual void Word(uint16_t &rShort)           = 0; // Needs separator!
	virtual void Byte(int8_t &rByte)              = 0; // Needs separator!
	virtual void Byte(uint8_t &rByte)             = 0; // Needs separator!
	virtual void Boolean(bool &rBool)             = 0;
	virtual void Character(char &rChar)           = 0; // Alphanumerical only!


	// Compile raw data (strings)
	enum RawCompileType
	{
		RCT_Escaped=0,// Any data allowed, no separator needed (default)
		RCT_All,      // Printable characters only, must be last element in naming.
		RCT_Idtf,     // Alphanumerical characters or '_', separator needed.
		RCT_IdtfAllowEmpty, // Like RCT_Idtf, but empty strings are also allowed
		RCT_ID        // Like RCT_Idtf (only used for special compilers that treat IDs differently)
	};
	// Note that string won't allow '\0' inside the buffer, even with escaped compiling!
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) = 0;
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped) = 0;
	virtual void String(std::string &str, RawCompileType type = RCT_Escaped) = 0;
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) = 0;

	// * Position
	// May return information about the current position of compilation (used for errors and warnings)
	virtual StdStrBuf getPosition()         const { return StdStrBuf(); }

	// * Passes
	virtual void Begin()                          { }
	virtual void BeginSecond()                    { }
	virtual void End()                            { }

	// *** Composed

	// Generic compiler function (plus specializations)
	template <class T> void Value(const T &rStruct)   { rStruct.CompileFunc(this); }
	template <class T> void Value(T &rStruct)     { CompileFunc(rStruct, this); }

	void Value(int32_t &rInt)  { DWord(rInt); }
	void Value(uint32_t &rInt) { DWord(rInt); }
	void Value(int16_t &rInt)  { Word(rInt); }
	void Value(uint16_t &rInt) { Word(rInt); }
	void Value(int8_t &rInt)   { Byte(rInt); }
	void Value(uint8_t &rInt)  { Byte(rInt); }
	void Value(bool &rBool)    { Boolean(rBool); }

	// Compiling/Decompiling (may throw a data format exception!)
	template <class T> inline void Compile(T &&rStruct)
	{
		assert(isDeserializer());
		DoCompilation(rStruct);
	}
	template <class T> inline void Decompile(const T &rStruct)
	{
		assert(!isDeserializer());
		DoCompilation(const_cast<T &>(rStruct));
	}

protected:

	// Compilation process
	template <class T>
	inline void DoCompilation(T &rStruct)
	{
		// Start compilation, do first pass
		Begin();
		Value(rStruct);
		// Second pass needed?
		if (isDoublePass())
		{
			BeginSecond();
			Value(rStruct);
		}
		// Finish
		End();
	}

public:

	// Compiler exception - thrown when something is wrong with the data to compile
	struct Exception
	{
		StdStrBuf Pos;
		StdStrBuf Msg;
protected:
		Exception(StdStrBuf Pos, StdStrBuf Msg) : Pos(std::move(Pos)), Msg(std::move(Msg)) { }
private:
		// do not copy
		Exception(const Exception &Exc) { }
	};
	class NotFoundException : public Exception
	{
		friend class StdCompiler;
		NotFoundException(StdStrBuf Pos, StdStrBuf Msg) : Exception(Pos, Msg) { }
	};
	class EOFException : public Exception
	{
		friend class StdCompiler;
		EOFException(StdStrBuf Pos, StdStrBuf Msg) : Exception(Pos, Msg)  { }
	};
	class CorruptException : public Exception
	{
		friend class StdCompiler;
		CorruptException(StdStrBuf Pos, StdStrBuf Msg) : Exception(Pos, Msg) { }
	};

	// Throw helpers (might redirect)
	void excNotFound(const char *szMessage, ...)
	{
#ifdef STDCOMPILER_EXCEPTION_WORKAROUND
		// Exception workaround: Just set a flag in failesafe mode.
		if (fFailSafe) { fFail = true; return; }
#endif
		// Throw the appropriate exception
		va_list args; va_start(args, szMessage);
		throw new NotFoundException(getPosition(), FormatStringV(szMessage, args));
	}
	void excEOF(const char *szMessage = "EOF", ...)
	{
		// Throw the appropriate exception
		va_list args; va_start(args, szMessage);
		throw new EOFException(getPosition(), FormatStringV(szMessage, args));
	}
	void excCorrupt(const char *szMessage, ...)
	{
		// Throw the appropriate exception
		va_list args; va_start(args, szMessage);
		throw new CorruptException(getPosition(), FormatStringV(szMessage, args));
	}

protected:

	// Exception workaround
#ifdef STDCOMPILER_EXCEPTION_WORKAROUND
	bool fFailSafe{false}, fFail{false};

	void beginFailSafe() { fFailSafe = true; fFail = false; }
	bool endFailSafe() { fFailSafe = false; return !fFail; }

public:
	template <class T> bool ValueSafe(const T &rStruct) { rStruct.CompileFunc(this); return true; }
	template <class T> bool ValueSafe(T &rStruct)       { CompileFunc(rStruct, this); return true; }

	bool ValueSafe(int32_t &rInt)  { beginFailSafe(); DWord(rInt);    return endFailSafe(); }
	bool ValueSafe(uint32_t &rInt) { beginFailSafe(); DWord(rInt);    return endFailSafe(); }
	bool ValueSafe(int16_t &rInt)  { beginFailSafe(); Word(rInt);     return endFailSafe(); }
	bool ValueSafe(uint16_t &rInt) { beginFailSafe(); Word(rInt);     return endFailSafe(); }
	bool ValueSafe(int8_t &rInt)   { beginFailSafe(); Byte(rInt);     return endFailSafe(); }
	bool ValueSafe(uint8_t &rInt)  { beginFailSafe(); Byte(rInt);     return endFailSafe(); }
	bool ValueSafe(bool &rBool)    { beginFailSafe(); Boolean(rBool); return endFailSafe(); }
#endif

public:

	// * Warnings
	typedef void (*WarnCBT)(void *, const char *, const char *);
	void setWarnCallback(WarnCBT pnWarnCB, void *pData) { pWarnCB = pnWarnCB; pWarnData = pData; }
	void Warn(const char *szWarning, ...);

private:

	// Warnings
	WarnCBT pWarnCB{nullptr};
	void *pWarnData{nullptr};

protected:

	// Standard separator character
	static char SeparatorToChar(Sep eSep);
	// String end test depending on encoding type
	static bool IsStringEnd(char c, RawCompileType eType);
};

// Standard compile funcs
template <class T>
inline void CompileFunc(T &rStruct, StdCompiler *pComp)
{
	// If the compiler doesn't like this line, you tried to compile
	// something the compiler doesn't know how to handle.
	// Possible reasons:
	// a) You are compiling a class/structure without a CompileFunc
	//    (you may add a specialization of this function, too)
	// b) You are trying to compile a pointer. Use a PtrAdapt instead.
	// c) You are trying to compile a simple value that has no
	//    fixed representation (float, int). Use safe types instead.
	rStruct.CompileFunc(pComp);
}

inline void CompileFunc(std::string &s, StdCompiler *comp)
{
	comp->String(s);
}

template <class T>
void CompileNewFunc(T *&pStruct, StdCompiler *pComp)
{
	// Create new object.
	// If this line doesn't compile, you either have to
	// a) Define a standard constructor for T
	// b) Specialize this function to do whatever the correct
	//    behaviour is to construct the object from compiler data
	std::unique_ptr<T> temp(new T); // exception-safety
	// Compile
	pComp->Value(*temp);
	pStruct = temp.release();
}

template <class T, typename ... P>
void CompileNewFunc(T *&pStruct, StdCompiler *pComp, P && ... pars)
{
	// Create new object.
	// If this line doesn't compile, you either have to
	// a) Define a standard constructor for T
	// b) Specialize this function to do whatever the correct
	//    behaviour is to construct the object from compiler data
	std::unique_ptr<T> temp(new T); // exception-safety
	// Compile
	pComp->Value(mkParAdapt(*temp, std::forward<P>(pars)...));
	pStruct = temp.release();
}

template <class T, class ContextT>
void CompileNewFuncCtx(T *&pStruct, StdCompiler *pComp, const ContextT& rCtx)
{
	// Create new object.
	// If this line doesn't compile, you either have to
	// a) Define an appropriate constructor for T
	// b) Specialize this function to do whatever the correct
	//    behaviour is to construct the object from compiler data
	//    and context
	std::unique_ptr<T> temp(new T(rCtx)); // exception-safety
	// Compile
	pComp->Value(*temp);
	pStruct = temp.release();
}

template <class T, class ContextT, class P>
void CompileNewFuncCtx(T *&pStruct, StdCompiler *pComp, const ContextT& rCtx, const P& rPar)
{
	// Create new object.
	// If this line doesn't compile, you either have to
	// a) Define an appropriate constructor for T
	// b) Specialize this function to do whatever the correct
	//    behaviour is to construct the object from compiler data
	//    and context
	std::unique_ptr<T> temp(new T(rCtx));  // exception-safety
	// Compile
	pComp->Value(mkParAdapt(*temp, rPar));
	pStruct = temp.release();
}

// Helpers for buffer-based compiling (may throw a data format exception!)
template <class CompT, class StructT>
void CompileFromBuf(StructT &&TargetStruct, const typename CompT::InT &SrcBuf)
{
	CompT Compiler;
	Compiler.setInput(SrcBuf.getRef());
	Compiler.Compile(TargetStruct);
}
template <class CompT, class StructT>
StructT * CompileFromBufToNew(const typename CompT::InT &SrcBuf)
{
	StructT *pStruct = nullptr;
	CompileFromBuf<CompT>(mkPtrAdaptNoNull(pStruct), SrcBuf);
	return pStruct;
}
template <class CompT, class StructT>
StructT * CompileFromBufToNewNamed(const typename CompT::InT &SrcBuf, const char *szName)
{
	StructT *pStruct = nullptr;
	CompileFromBuf<CompT>(mkNamingAdapt(mkPtrAdaptNoNull(pStruct), szName), SrcBuf);
	return pStruct;
}
template <class CompT, class StructT>
typename CompT::OutT DecompileToBuf(const StructT &SrcStruct)
{
	CompT Compiler;
	Compiler.Decompile(SrcStruct);
	return Compiler.getOutput();
}

// *** Null compiler

// Naming supported, nothing is returned. Used for setting default values.

class StdCompilerNull : public StdCompiler
{
public:

	// Properties
	bool isDeserializer() override { return true; }
	bool hasNaming() override { return true; }

	// Naming
	bool Name(const char *szName) override { return false; }
	int NameCount(const char *szName = nullptr) override { return 0; }

	// Data readers
	void DWord(int32_t &rInt) override { }
	void DWord(uint32_t &rInt) override { }
	void Word(int16_t &rShort) override { }
	void Word(uint16_t &rShort) override { }
	void Byte(int8_t &rByte) override { }
	void Byte(uint8_t &rByte) override { }
	void Boolean(bool &rBool) override { }
	void Character(char &rChar) override { }
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override { }
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override { }
	void String(std::string &str, RawCompileType eType = RCT_Escaped) override {}
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override { }
};

// *** Binary compiler

// No naming supported, everything is read/written binary.


// binary writer
class StdCompilerBinWrite : public StdCompiler
{
public:

	// Result
	typedef StdBuf OutT;
	inline OutT getOutput() { return Buf; }

	// Properties
	bool isDoublePass() override { return true; }

	// Data writers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType eType = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	void Begin() override;
	void BeginSecond() override;

protected:
	// Process data
	bool fSecondPass;
	int iPos;
	StdBuf Buf;

	// Helpers
	template <class T> void WriteValue(const T &rValue);
	void WriteData(const void *pData, size_t iSize);
};

// binary read
class StdCompilerBinRead : public StdCompiler
{
public:

	// Input
	typedef StdBuf InT;
	void setInput(InT &&In) { Buf = std::move(In); }

	// Properties
	bool isDeserializer() override { return true; }

	// Data readers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType eType = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Position
	StdStrBuf getPosition() const override;

	// Passes
	void Begin() override;

	// Data
	size_t getPosition() { return iPos; }
	size_t getRemainingBytes() { return Buf.getSize() - iPos; }

protected:
	// Process data
	size_t iPos;
	StdBuf Buf;

	// Helper
	template <class T> void ReadValue(T &rValue);
};

// *** INI compiler

// Naming and separators supported, so defaulting can be used through
// the appropriate adaptors.

// Example:

// [Sect1]
//   [Sect1a]
//   Val1=4
//   Val2=5
// Val4=3,5

// will result from:

// int v1=4, v2=5, v3=0, v4[3] = { 3, 5, 0 };
// DecompileToBuf<StdCompilerINIWrite>(
//   mkNamingAdapt(
//     mkNamingAdapt(
//       mkNamingAdapt(v1, "Val1", 0) +
//       mkNamingAdapt(v2, "Val2", 0) +
//       mkNamingAdapt(v3, "Val3", 0),
//     "Sect1a") +
//     mkNamingAdapt(mkArrayAdapt(v4, 3, 0), "Val4", 0),
//   "Sect1")
// )


// text writer
class StdCompilerINIWrite : public StdCompiler
{
public:
	// Input
	typedef StdStrBuf OutT;
	inline OutT getOutput() { return Buf; }

	// Properties
	bool hasNaming() override { return true; }

	// Naming
	bool Name(const char *szName) override;
	void NameEnd(bool fBreak = false) override;

	// Separators
	bool Separator(Sep eSep) override;

	// Data writers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void StringN(const char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped);
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType eType = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	void Begin() override;
	void End() override;

protected:

	// Result
	StdStrBuf Buf;

	// Naming stack
	struct Naming
	{
		StdStrBuf Name;
		Naming *Parent;
	};
	Naming *pNaming;
	// Recursion depth
	int iDepth;

	// Name not put yet (it's not clear wether it is a value or a section)
	bool fPutName,
	// Currently inside a section, so raw data can't be printed
	fInSection;

	void PrepareForValue();
	void WriteEscaped(const char *szString, const char *pEnd);
	void WriteIndent(bool fSectionName);
	void PutName(bool fSection);
};

// text reader
class StdCompilerINIRead : public StdCompiler
{
public:

	StdCompilerINIRead();
	~StdCompilerINIRead() override;

	// Input
	typedef StdStrBuf InT;
	void setInput(const InT &In) { Buf.Ref(In); lineBreaks.clear(); }

	// Properties
	bool isDeserializer() override { return true; }
	bool hasNaming() override { return true; }

	// Naming
	bool Name(const char *szName) override;
	void NameEnd(bool fBreak = false) override;
	bool FollowName(const char *szName) override;
	const char *GetNameByIndex(size_t idx) const override;

	// Separators
	bool Separator(Sep eSep) override;
	void NoSeparator() override;

	// Counters
	int NameCount(const char *szName = nullptr) override;

	// Data writers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType eType = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Position
	StdStrBuf getPosition() const override;

	// Passes
	void Begin() override;
	void End() override;

protected:

	// * Data

	// Name tree
	struct NameNode
	{
		// Name
		StdStrBuf Name;
		// Section?
		bool Section{false};
		// Tree structure
		NameNode *Parent,
		*FirstChild{nullptr}, *PrevChild{nullptr}, *NextChild{nullptr}, *LastChild{nullptr};
		// Indent level
		int Indent{-1};
		// Name number in parent map
		const char *Pos{nullptr};
		// Constructor
		NameNode(NameNode *pParent = nullptr) :
			Parent(pParent)
		{ }
	};
	NameNode *pNameRoot{nullptr}, *pName;
	// Current depth
	int iDepth{0};
	// Real depth (depth of recursive Name()-calls - if iDepth != iRealDepth, we are in a nonexistant namespace)
	int iRealDepth{0};

	// Data
	StdStrBuf Buf;
	// Position
	const char *pPos;

	// Reenter position (if an nonexistant separator was specified)
	const char *pReenter;

	// Uppermost name that wasn't found
	StdCopyStrBuf NotFoundName;

	// * Implementation

	// Name tree
	void CreateNameTree();
	void FreeNameTree();
	void FreeNameNode(NameNode *pNode);

	// Navigation
	void SkipWhitespace();
	void SkipNum();
	long ReadNum();
	size_t GetStringLength(RawCompileType eTyped);
	StdBuf ReadString(size_t iLength, RawCompileType eTyped, bool fAppendNull = true);
	bool TestStringEnd(RawCompileType eType) { return IsStringEnd(*pPos, eType); }
	char ReadEscapedChar();
	unsigned long ReadUNum();

	void notFound(const char *szWhat);

private:
	uint32_t getLineNumberOfPos(const char *pos) const;
	mutable std::vector<const char *> lineBreaks;
};

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError);

template <class CompT, class StructT>
bool CompileFromBuf_Log(StructT &&TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
{
	try
	{
		CompileFromBuf<CompT>(TargetStruct, SrcBuf);
		return true;
	}
	catch (StdCompiler::Exception *pExc)
	{
		if (!pExc->Pos.getLength())
			LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
		else
			LogF("ERROR: %s (in %s, %s)", pExc->Msg.getData(), pExc->Pos.getData(), szName);
		delete pExc;
		return false;
	}
}
template <class CompT, class StructT>
bool CompileFromBuf_LogWarn(StructT &&TargetStruct, const typename CompT::InT &SrcBuf, const char *szName)
{
	try
	{
		CompT Compiler;
		Compiler.setInput(SrcBuf.getRef());
		Compiler.setWarnCallback(StdCompilerWarnCallback, reinterpret_cast<void *>(const_cast<char *>(szName)));
		Compiler.Compile(TargetStruct);
		return true;
	}
	catch (StdCompiler::Exception *pExc)
	{
		if (!pExc->Pos.getLength())
			LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
		else
			LogF("ERROR: %s (in %s, %s)", pExc->Msg.getData(), pExc->Pos.getData(), szName);
		delete pExc;
		return false;
	}
}
template <class CompT, class StructT>
bool DecompileToBuf_Log(StructT &&TargetStruct, typename CompT::OutT *pOut, const char *szName)
{
	if (!pOut) return false;
	try
	{
		pOut->Take(DecompileToBuf<CompT>(TargetStruct));
		return true;
	}
	catch (StdCompiler::Exception *pExc)
	{
		LogF("ERROR: %s (in %s)", pExc->Msg.getData(), szName);
		delete pExc;
		return false;
	}
}

#endif // STDCOMPILER_H
