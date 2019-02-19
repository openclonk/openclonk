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
#include "C4Include.h"
#include "lib/StdCompiler.h"

// *** StdCompiler

void StdCompiler::Warn(const char *szWarning, ...)
{
	// Got warning callback?
	if (!pWarnCB) return;
	// Format message
	va_list args; va_start(args, szWarning);
	StdStrBuf Msg; Msg.FormatV(szWarning, args);
	// do callback
	(*pWarnCB)(pWarnData, getPosition().getData(), Msg.getData());
}

char StdCompiler::SeparatorToChar(Sep eSep)
{
	switch (eSep)
	{
	case SEP_SEP: return ',';
	case SEP_SEP2: return ';';
	case SEP_SET: return '=';
	case SEP_PART: return '.';
	case SEP_PART2: return ':';
	case SEP_PLUS: return '+';
	case SEP_START: return '(';
	case SEP_END: return ')';
	case SEP_START2: return '[';
	case SEP_END2: return ']';
	case SEP_VLINE: return '|';
	case SEP_DOLLAR: return '$';
	default: assert(!"Unhandled Separator value");
	}
	return ' ';
}

bool StdCompiler::IsStringEnd(char c, RawCompileType eType)
{
	switch (eType)
	{
	case RCT_Escaped: return c == '"' || !c || c == '\n' || c == '\r';
	case RCT_All: return !c || c == '\n' || c == '\r';
		// '-' is needed for Layers in Scenario.txt (C4NameList) and other Material-Texture combinations
	case RCT_Idtf: case RCT_IdtfAllowEmpty: case RCT_ID: return !isalnum((unsigned char)c) && c != '_' && c != '-';
	}
	// unreachable
	return true;
}

// *** StdCompilerBinWrite

void StdCompilerBinWrite::DWord(int32_t &rInt)   { WriteValue(rInt); }
void StdCompilerBinWrite::DWord(uint32_t &rInt)  { WriteValue(rInt); }
void StdCompilerBinWrite::Word(int16_t &rShort)  { WriteValue(rShort); }
void StdCompilerBinWrite::Word(uint16_t &rShort) { WriteValue(rShort); }
void StdCompilerBinWrite::Byte(int8_t &rByte)    { WriteValue(rByte); }
void StdCompilerBinWrite::Byte(uint8_t &rByte)   { WriteValue(rByte); }
void StdCompilerBinWrite::Boolean(bool &rBool)   { WriteValue(rBool); }
void StdCompilerBinWrite::Character(char &rChar) { WriteValue(rChar); }
void StdCompilerBinWrite::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	WriteData(szString, strlen(szString) + 1);
}
void StdCompilerBinWrite::String(char **pszString, RawCompileType eType)
{
	if (*pszString)
		WriteData(*pszString, strlen(*pszString) + 1);
	else
		WriteValue('\0');
}

void StdCompilerBinWrite::String(std::string &str, RawCompileType type)
{
	WriteData(str.c_str(), str.size() + 1);
}

template <class T>
void StdCompilerBinWrite::WriteValue(const T &rValue)
{
	// Copy data
	if (fSecondPass)
		*getMBufPtr<T>(Buf, iPos) = rValue;
	iPos += sizeof(rValue);
}

void StdCompilerBinWrite::WriteData(const void *pData, size_t iSize)
{
	// Copy data
	if (fSecondPass)
		Buf.Write(pData, iSize, iPos);
	iPos += iSize;
}

void StdCompilerBinWrite::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	// Copy data
	if (fSecondPass)
		Buf.Write(pData, iSize, iPos);
	iPos += iSize;
}

void StdCompilerBinWrite::Begin()
{
	fSecondPass = false; iPos = 0;
}

void StdCompilerBinWrite::BeginSecond()
{
	Buf.New(iPos);
	fSecondPass = true; iPos = 0;
}

// *** StdCompilerBinRead

void StdCompilerBinRead::DWord(int32_t &rInt)   { ReadValue(rInt); }
void StdCompilerBinRead::DWord(uint32_t &rInt)  { ReadValue(rInt); }
void StdCompilerBinRead::Word(int16_t &rShort)  { ReadValue(rShort); }
void StdCompilerBinRead::Word(uint16_t &rShort) { ReadValue(rShort); }
void StdCompilerBinRead::Byte(int8_t &rByte)    { ReadValue(rByte); }
void StdCompilerBinRead::Byte(uint8_t &rByte)   { ReadValue(rByte); }
void StdCompilerBinRead::Boolean(bool &rBool)   { ReadValue(rBool); }
void StdCompilerBinRead::Character(char &rChar) { ReadValue(rChar); }
void StdCompilerBinRead::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	// At least one byte data needed
	if (iPos >= Buf.getSize())
		{ excEOF(); return; }
	// Copy until no data left
	char *pPos = szString;
	while ((*pPos++ = *getBufPtr<char>(Buf, iPos++)))
		if (iPos >= Buf.getSize())
			{ excEOF(); return; }
		else if (pPos > szString + iMaxLength)
			{ excCorrupt("string too long"); return; }
}

void StdCompilerBinRead::String(char **pszString, RawCompileType eType)
{
	// At least one byte data needed
	if (iPos >= Buf.getSize())
		{ excEOF(); return; }
	int iStart = iPos;
	// Search string end
	while (*getBufPtr<char>(Buf, iPos++))
		if (iPos >= Buf.getSize())
			{ excEOF(); return; }
	// Allocate and copy data
	*pszString = (char *) malloc(iPos - iStart);
	memcpy(*pszString, Buf.getPtr(iStart), iPos - iStart);
}

void StdCompilerBinRead::String(std::string &str, RawCompileType type)
{
	// At least one byte data needed
	if (iPos >= Buf.getSize())
	{
		excEOF(); return;
	}
	int iStart = iPos;
	// Search string end
	while (*getBufPtr<char>(Buf, iPos++))
		if (iPos >= Buf.getSize())
		{
			excEOF(); return;
		}
	// Copy data
	str.assign(getBufPtr<char>(Buf, iStart), getBufPtr<char>(Buf, iPos));
}

void StdCompilerBinRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	if (iPos + iSize > Buf.getSize())
		{ excEOF(); return; }
	// Copy data
	memcpy(pData, Buf.getPtr(iPos), iSize);
	iPos += iSize;
}

StdStrBuf StdCompilerBinRead::getPosition() const
{
	return FormatString("byte %ld", static_cast<unsigned long>(iPos));
}

template <class T>
inline void StdCompilerBinRead::ReadValue(T &rValue)
{
	// Pufferüberhang prüfen
	if (iPos + sizeof(T) > Buf.getSize())
		{ excEOF(); return; }
	// Kopieren
	rValue = *getBufPtr<T>(Buf, iPos);
	iPos += sizeof(T);
}

void StdCompilerBinRead::Begin()
{
	iPos = 0;
}

// *** StdCompilerINIWrite

bool StdCompilerINIWrite::Name(const char *szName)
{
	// Sub-Namesections exist, so it's a section. Write name if not already done so.
	if (fPutName) PutName(true);
	// Push struct
	Naming *pnNaming = new Naming;
	pnNaming->Name.Copy(szName);
	pnNaming->Parent = pNaming;
	pNaming = pnNaming;
	iDepth++;
	// Done
	fPutName = true; fInSection = false;
	return true;
}

void StdCompilerINIWrite::NameEnd(bool fBreak)
{
	// Append newline
	if (!fPutName && !fInSection)
		Buf.Append("\n");
	fPutName = false;
	// Note this makes it impossible to distinguish an empty name section from
	// a non-existing name section.

	// Pop
	assert(iDepth);
	Naming *poNaming = pNaming;
	pNaming = poNaming->Parent;
	delete poNaming;
	iDepth--;
	// We're inside a section now
	fInSection = true;
}

bool StdCompilerINIWrite::Separator(Sep eSep)
{
	if (fInSection)
	{
		// Re-put section name
		PutName(true);
	}
	else
	{
		PrepareForValue();
		Buf.AppendChar(SeparatorToChar(eSep));
	}
	return true;
}

void StdCompilerINIWrite::DWord(int32_t &rInt)
{
	PrepareForValue();
	Buf.AppendFormat("%d", rInt);
}
void StdCompilerINIWrite::DWord(uint32_t &rInt)
{
	PrepareForValue();
	Buf.AppendFormat("%u", rInt);
}
void StdCompilerINIWrite::Word(int16_t &rInt)
{
	PrepareForValue();
	Buf.AppendFormat("%d", rInt);
}
void StdCompilerINIWrite::Word(uint16_t &rInt)
{
	PrepareForValue();
	Buf.AppendFormat("%u", rInt);
}
void StdCompilerINIWrite::Byte(int8_t &rByte)
{
	PrepareForValue();
	Buf.AppendFormat("%d", rByte);
}
void StdCompilerINIWrite::Byte(uint8_t &rInt)
{
	PrepareForValue();
	Buf.AppendFormat("%u", rInt);
}
void StdCompilerINIWrite::Boolean(bool &rBool)
{
	PrepareForValue();
	Buf.Append(rBool ? "true" : "false");
}
void StdCompilerINIWrite::Character(char &rChar)
{
	PrepareForValue();
	Buf.AppendFormat("%c", rChar);
}

void StdCompilerINIWrite::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	StringN(szString, strnlen(szString, iMaxLength), eType);
}

void StdCompilerINIWrite::StringN(const char *szString, size_t iLength, RawCompileType eType)
{
	PrepareForValue();
	switch (eType)
	{
	case RCT_Escaped:
		WriteEscaped(szString, szString + iLength);
		break;
	case RCT_All:
	case RCT_Idtf:
	case RCT_IdtfAllowEmpty:
	case RCT_ID:
		Buf.Append(szString);
	}
}

void StdCompilerINIWrite::String(char **pszString, RawCompileType eType)
{
	assert(pszString);
	char cNull = '\0';
	char * szString = *pszString ? *pszString : &cNull;
	String(szString, strlen(szString), eType);
}

void StdCompilerINIWrite::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	switch (eType)
	{
	case RCT_Escaped:
		WriteEscaped(reinterpret_cast<char *>(pData), reinterpret_cast<char *>(pData) + iSize);
		break;
	case RCT_All:
	case RCT_Idtf:
	case RCT_IdtfAllowEmpty:
	case RCT_ID:
		Buf.Append(reinterpret_cast<char *>(pData), iSize);
	}
}

void StdCompilerINIWrite::String(std::string &str, RawCompileType type)
{
	StringN(str.c_str(), str.size(), type);
}

void StdCompilerINIWrite::Begin()
{
	pNaming = nullptr;
	fPutName = false;
	iDepth = 0;
	fInSection = false;
	Buf.Clear();
}

void StdCompilerINIWrite::End()
{
	// Ensure all namings were closed properly
	assert(!iDepth);
}

void StdCompilerINIWrite::PrepareForValue()
{
	// Put name (value-type), if not already done so
	if (fPutName) PutName(false);
	// No data allowed inside of sections
	assert(!fInSection);
	// No values allowed on top-level - must be contained in at least one section
	assert(iDepth > 1);
}

void StdCompilerINIWrite::WriteEscaped(const char *szString, const char *pEnd)
{
	Buf.AppendChar('"');
	// Try to write chunks as huge as possible of "normal" chars.
	// Note this excludes '\0', so the standard Append() can be used.
	const char *pStart, *pPos; pStart = pPos = szString;
	bool fLastNumEscape = false; // catch "\1""1", which must become "\1\61"
	for (; pPos < pEnd; pPos++)
		if (!isprint((unsigned char)(unsigned char) *pPos) || *pPos == '\\' || *pPos == '"' || (fLastNumEscape && isdigit((unsigned char)*pPos)))
		{
			// Write everything up to this point
			if (pPos - pStart) Buf.Append(pStart, pPos - pStart);
			// Escape
			fLastNumEscape = false;
			switch (*pPos)
			{
			case '\a': Buf.Append(R"(\a)"); break;
			case '\b': Buf.Append(R"(\b)"); break;
			case '\f': Buf.Append(R"(\f)"); break;
			case '\n': Buf.Append(R"(\n)"); break;
			case '\r': Buf.Append(R"(\r)"); break;
			case '\t': Buf.Append(R"(\t)"); break;
			case '\v': Buf.Append(R"(\v)"); break;
			case '\"': Buf.Append(R"(\")"); break;
			case '\\': Buf.Append(R"(\\)"); break;
			default:
				Buf.AppendFormat(R"(\%o)", *reinterpret_cast<const unsigned char *>(pPos));
				fLastNumEscape = true;
			}
			// Set pointer
			pStart = pPos + 1;
		}
		else
			fLastNumEscape = false;
	// Write the rest
	if (pEnd - pStart) Buf.Append(pStart, pEnd - pStart);
	Buf.AppendChar('"');
}

void StdCompilerINIWrite::WriteIndent(bool fSection)
{
	// Do not indent level 1 (level 0 values aren't allowed - see above)
	int iIndent = iDepth - 1;
	// Sections are indented more, even though they belong to this level
	if (!fSection) iIndent--;
	// Do indention
	if (iIndent <= 0) return;
	Buf.AppendChars(' ',  iIndent * 2);
}

void StdCompilerINIWrite::PutName(bool fSection)
{
	if (fSection && Buf.getLength())
		Buf.Append("\n");
	WriteIndent(fSection);
	// Put name
	if (fSection)
		Buf.AppendFormat("[%s]\n", pNaming->Name.getData());
	else
		Buf.AppendFormat("%s=", pNaming->Name.getData());
	// Set flag
	fPutName = false;
}

// *** StdCompilerINIRead

StdCompilerINIRead::StdCompilerINIRead() = default;

StdCompilerINIRead::~StdCompilerINIRead()
{
	FreeNameTree();
}

// Naming
bool StdCompilerINIRead::Name(const char *szName)
{
	// Increase depth
	iDepth++;
	// Parent category virtual?
	if (iDepth - 1 > iRealDepth)
		return false;
	// Name must be alphanumerical and non-empty (force it)
	if (!isalpha((unsigned char)*szName))
		{ assert(false); return false; }
	for (const char *p = szName + 1; *p; p++)
		// C4Update needs Name**...
		if (!isalnum((unsigned char)*p) && *p != ' ' && *p != '_' && *p != '*')
			{ assert(false); return false; }
	// Search name
	NameNode *pNode;
	for (pNode = pName->FirstChild; pNode; pNode = pNode->NextChild)
		if (pNode->Pos && pNode->Name == szName)
			break;
	// Not found?
	if (!pNode)
	{
		NotFoundName = szName;
		return false;
	}
	// Save tree position, indicate success
	pName = pNode;
	pPos = pName->Pos;
	pReenter = nullptr;
	iRealDepth++;
	return true;
}
void StdCompilerINIRead::NameEnd(bool fBreak)
{
	assert(iDepth > 0);
	if (iRealDepth == iDepth)
	{
		// Remove childs
		for (NameNode *pNode = pName->FirstChild, *pNext; pNode; pNode = pNext)
		{
			// Report unused entries
			if (pNode->Pos && !fBreak)
				Warn(R"(Unexpected %s "%s"!)", pNode->Section ? "section" : "value", pNode->Name.getData());
			// delete node
			pNext = pNode->NextChild;
			delete pNode;
		}
		// Remove name so it won't be found again
		NameNode *pParent = pName->Parent;
		(pName->PrevChild ? pName->PrevChild->NextChild : pParent->FirstChild) = pName->NextChild;
		(pName->NextChild ? pName->NextChild->PrevChild : pParent->LastChild) = pName->PrevChild;
		delete pName;
		// Go up
		pName = pParent;
		iRealDepth--;
	}
	// Decrease depth
	iDepth--;
	// This is the middle of nowhere
	pPos = nullptr; pReenter = nullptr;
}

bool StdCompilerINIRead::FollowName(const char *szName)
{
	// Current naming virtual?
	if (iDepth > iRealDepth)
		return false;
	// Next section must be the one
	if (!pName->NextChild || pName->NextChild->Name != szName)
	{
		// End current naming
		NameEnd();
		// Go into virtual naming
		iDepth++;
		return false;
	}
	// End current naming
	NameEnd();
	// Start new one
	Name(szName);
	// Done
	return true;
}

// Separators
bool StdCompilerINIRead::Separator(Sep eSep)
{
	if (iDepth > iRealDepth) return false;
	// In section?
	if (pName->Section)
	{
		// Store current name, search another section with the same name
		StdStrBuf CurrName = pName->Name;
		NameEnd();
		return Name(CurrName.getData());
	}
	// Position saved back from separator mismatch?
	if (pReenter) { pPos = pReenter; pReenter = nullptr; }
	// Nothing to read?
	if (!pPos) return false;
	// Read (while skipping over whitespace)
	SkipWhitespace();
	// Separator mismatch? Let all read attempts fail until the correct separator is found or the naming ends.
	if (*pPos != SeparatorToChar(eSep)) { pReenter = pPos; pPos = nullptr; return false; }
	// Go over separator, success
	pPos++;
	return true;
}

void StdCompilerINIRead::NoSeparator()
{
	// Position saved back from separator mismatch?
	if (pReenter) { pPos = pReenter; pReenter = nullptr; }
}

int StdCompilerINIRead::NameCount(const char *szName)
{
	// not in virtual naming
	if (iDepth > iRealDepth || !pName) return 0;
	// count within current name
	int iCount = 0;
	NameNode *pNode;
	for (pNode = pName->FirstChild; pNode; pNode = pNode->NextChild)
		// if no name is given, all valid subsections are counted
		if (pNode->Pos && (!szName || pNode->Name == szName))
			++iCount;
	return iCount;
}

const char *StdCompilerINIRead::GetNameByIndex(size_t idx) const
{
	// not in virtual naming
	if (iDepth > iRealDepth || !pName) return nullptr;
	// count within current name
	NameNode *pNode;
	for (pNode = pName->FirstChild; pNode; pNode = pNode->NextChild)
		// all valid subsections are counted
		if (pNode->Pos)
				if (!idx--)
					return pNode->Name.getData();
	// index out of range
	return nullptr;
}

// Various data readers
void StdCompilerINIRead::DWord(int32_t &rInt)
{
	rInt = ReadNum();
}
void StdCompilerINIRead::DWord(uint32_t &rInt)
{
	rInt = ReadUNum();
}
void StdCompilerINIRead::Word(int16_t &rShort)
{
	const int MIN = -(1 << 15), MAX = (1 << 15) - 1;
	int iNum = ReadNum();
	if (iNum < MIN || iNum > MAX)
		Warn("number out of range (%d to %d): %d ", MIN, MAX, iNum);
	rShort = Clamp(iNum, MIN, MAX);
}
void StdCompilerINIRead::Word(uint16_t &rShort)
{
	const unsigned int MIN = 0, MAX = (1 << 16) - 1;
	unsigned int iNum = ReadUNum();
	if (iNum > MAX)
		Warn("number out of range (%u to %u): %u ", MIN, MAX, iNum);
	rShort = Clamp(iNum, MIN, MAX);
}
void StdCompilerINIRead::Byte(int8_t &rByte)
{
	const int MIN = -(1 << 7), MAX = (1 << 7) - 1;
	int iNum = ReadNum();
	if (iNum < MIN || iNum > MAX)
		Warn("number out of range (%d to %d): %d ", MIN, MAX, iNum);
	rByte = Clamp(iNum, MIN, MAX);
}
void StdCompilerINIRead::Byte(uint8_t &rByte)
{
	const unsigned int MIN = 0, MAX = (1 << 8) - 1;
	unsigned int iNum = ReadUNum();
	if (iNum > MAX)
		Warn("number out of range (%u to %u): %u ", MIN, MAX, iNum);
	rByte = Clamp(iNum, MIN, MAX);
}
void StdCompilerINIRead::Boolean(bool &rBool)
{
	if (!pPos) { notFound("Boolean"); return; }
	if (*pPos == '1' && !isdigit((unsigned char)*(pPos+1)))
		{ rBool = true; pPos ++; }
	else if (*pPos == '0' && !isdigit((unsigned char)*(pPos+1)))
		{ rBool = false; pPos ++; }
	else if (SEqual2(pPos, "true"))
		{ rBool = true; pPos += 4; }
	else if (SEqual2(pPos, "false"))
		{ rBool = false; pPos += 5; }
	else
		{ notFound("Boolean"); return; }
}
void StdCompilerINIRead::Character(char &rChar)
{
	if (!pPos || !isalpha((unsigned char)*pPos))
		{ notFound("Character"); return; }
	rChar = *pPos++;
}
void StdCompilerINIRead::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	// Read data
	StdBuf Buf = ReadString(iMaxLength, eType, true);
	// Copy
	SCopy(getBufPtr<char>(Buf), szString, iMaxLength);
}
void StdCompilerINIRead::String(char **pszString, RawCompileType eType)
{
	// Get length
	size_t iLength = GetStringLength(eType);
	// Read data
	StdBuf Buf = ReadString(iLength, eType, true);
	// Set
	*pszString = reinterpret_cast<char *>(Buf.GrabPointer());
}
void StdCompilerINIRead::String(std::string &str, RawCompileType type)
{
	// Get length
	size_t iLength = GetStringLength(type);
	// Read data
	StdBuf Buf = ReadString(iLength, type, true);
	str = getBufPtr<char>(Buf);
}
void StdCompilerINIRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	// Read data
	StdBuf Buf = ReadString(iSize, eType, false);
	// Correct size?
	if (Buf.getSize() != iSize)
		Warn("got %u bytes raw data, but %u bytes expected!", Buf.getSize(), iSize);
	// Copy
	MemCopy(Buf.getData(), pData, iSize);
}

uint32_t StdCompilerINIRead::getLineNumberOfPos(const char *pos) const
{
	// Figure out quickly whether we already know which line this is
	auto entry = std::lower_bound(lineBreaks.begin(), lineBreaks.end(), pos);
	if (entry != lineBreaks.end())
	{
		return std::distance(lineBreaks.begin(), entry) + 1;
	}
	// Otherwise search through the buffer until we find out, filling the
	// cache in the process
	const char *cursor = Buf.getData();
	if (!lineBreaks.empty())
		cursor = *(lineBreaks.end() - 1) + 1;
	for (;;)
	{
		if (*cursor == '\0' || *cursor == '\n')
		{
			lineBreaks.push_back(cursor);

			// If we're at the end of the file or have found the line break
			// past the requested position, we're done for now
			if (*cursor == '\0' || pos < cursor)
			{
				break;
			}
		}
		++cursor;
	}
	return std::distance(lineBreaks.begin(),
		std::lower_bound(lineBreaks.begin(), lineBreaks.end(), pos)) + 1;
}

StdStrBuf StdCompilerINIRead::getPosition() const
{
	if (pPos)
		return FormatString("line %d", getLineNumberOfPos(pPos));
	else if (iDepth == iRealDepth)
		return FormatString(pName->Section ? R"(section "%s", after line %d)" : R"(value "%s", line %d)", pName->Name.getData(), getLineNumberOfPos(pName->Pos));
	else if (iRealDepth)
		return FormatString(R"(missing value/section "%s" inside section "%s" (line %d))", NotFoundName.getData(), pName->Name.getData(), getLineNumberOfPos(pName->Pos));
	else
		return FormatString(R"(missing value/section "%s")", NotFoundName.getData());
}

void StdCompilerINIRead::Begin()
{
	// Already running? This may happen if someone confuses Compile with Value.
	assert(!iDepth && !iRealDepth && !pNameRoot);
	// Create tree
	CreateNameTree();
	// Start must be inside a section
	iDepth = iRealDepth = 0;
	pPos = nullptr; pReenter = nullptr;
}
void StdCompilerINIRead::End()
{
	assert(!iDepth && !iRealDepth);
	FreeNameTree();
}

void StdCompilerINIRead::CreateNameTree()
{
	FreeNameTree();
	// Create root node
	pName = pNameRoot = new NameNode();
	// No input? Stop
	if (!Buf) return;
	// Start scanning
	pPos = Buf.getPtr(0);
	while (*pPos)
	{
		// Go over whitespace
		int iIndent = 0;
		while (*pPos == ' ' || *pPos == '\t')
			{ pPos++; iIndent++; }
		// Name/Section?
		bool fSection = *pPos == '[' && isalpha((unsigned char)*(pPos+1));
		if (fSection || isalpha((unsigned char)*pPos))
		{
			// Treat values as if they had more indention
			// (so they become children of sections on the same level)
			if (!fSection) iIndent++; else pPos++;
			// Go up in tree structure if there is less indention
			while (pName->Parent && pName->Indent >= iIndent)
				pName = pName->Parent;
			// Copy name
			StdStrBuf Name;
			while (isalnum((unsigned char)*pPos) || *pPos == ' ' || *pPos == '_')
				Name.AppendChar(*pPos++);
			while (*pPos == ' ' || *pPos == '\t') pPos++;
			if ( *pPos != (fSection ? ']' : '=') )
				// Warn, ignore
				Warn(isprint((unsigned char)*pPos) ? "Unexpected character ('%c'): %s ignored" : "Unexpected character ('0x%02x'): %s ignored", unsigned(*pPos), fSection ? "section" : "value");
			else
			{
				pPos++;
				// Create new node
				NameNode *pPrev = pName->LastChild;
				pName =
				  pName->LastChild =
				    (pName->LastChild ? pName->LastChild->NextChild : pName->FirstChild) =
				      new NameNode(pName);
				pName->PrevChild = pPrev;
				pName->Name.Take(std::move(Name));
				pName->Pos = pPos;
				pName->Indent = iIndent;
				pName->Section = fSection;
				// Values don't have children (even if the indention looks like it)
				if (!fSection)
					pName = pName->Parent;
			}
		}
		// Skip line
		while (*pPos && (*pPos != '\n' && *pPos != '\r'))
			pPos++;
		while (*pPos == '\n' || *pPos == '\r')
			pPos++;
	}
	// Set pointer back
	pName = pNameRoot;
}

void StdCompilerINIRead::FreeNameTree()
{
	// free all nodes
	FreeNameNode(pNameRoot);
	pName = pNameRoot = nullptr;
}

void StdCompilerINIRead::FreeNameNode(NameNode *pDelNode)
{
	NameNode *pNode = pDelNode;
	while (pNode)
	{
		if (pNode->FirstChild)
			pNode = pNode->FirstChild;
		else
		{
			NameNode *pDelete = pNode;
			if (pDelete == pDelNode) { delete pDelete; break; }
			if (pNode->NextChild)
				pNode = pNode->NextChild;
			else
			{
				pNode = pNode->Parent;
				if (pNode) pNode->FirstChild = nullptr;
			}
			delete pDelete;
		}
	}
}

void StdCompilerINIRead::SkipWhitespace()
{
	while (*pPos == ' ' || *pPos == '\t')
		pPos++;
}

void StdCompilerINIRead::SkipNum()
{
	while (*pPos == '+' || *pPos == '-' || isdigit((unsigned char)*pPos))
		pPos++;
}

long StdCompilerINIRead::ReadNum()
{
	if (!pPos)
		{ notFound("Number"); return 0; }
	// Skip whitespace
	SkipWhitespace();
	// Read number. If this breaks, Günther is to blame!
	const char *pnPos = pPos;
	long iNum = strtol(pPos, const_cast<char **>(&pnPos), 10);
	// Could not read?
	if (!iNum && pnPos == pPos)
		{ notFound("Number"); return 0; }
	// Get over it
	pPos = pnPos;
	return iNum;
}

unsigned long StdCompilerINIRead::ReadUNum()
{
	if (!pPos)
		{ notFound("Number"); return 0; }
	// Skip whitespace
	SkipWhitespace();
	// Read number. If this breaks, Günther is to blame!
	const char *pnPos = pPos;
	unsigned long iNum = strtoul(pPos, const_cast<char **>(&pnPos), 10);
	// Could not read?
	if (!iNum && pnPos == pPos)
		{ notFound("Number"); return 0; }
	// Get over it
	pPos = pnPos;
	return iNum;
}

size_t StdCompilerINIRead::GetStringLength(RawCompileType eRawType)
{
	// Excpect valid position
	if (!pPos)
		{ notFound("String"); return 0; }
	// Skip whitespace
	SkipWhitespace();
	// Save position
	const char *pStart = pPos;
	// Escaped? Go over '"'
	if (eRawType == RCT_Escaped && *pPos++ != '"')
		{ notFound("Escaped string"); return 0; }
	// Search end of string
	size_t iLength = 0;
	while (!TestStringEnd(eRawType))
	{
		// Read a character (we're just counting atm)
		if (eRawType == RCT_Escaped)
			ReadEscapedChar();
		else
			pPos++;
		// Count it
		iLength++;
	}
	// Reset position, return the length
	pPos = pStart;
	return iLength;
}

StdBuf StdCompilerINIRead::ReadString(size_t iLength, RawCompileType eRawType, bool fAppendNull)
{
	// Excpect valid position
	if (!pPos)
		{ notFound("String"); return StdBuf(); }
	// Skip whitespace
	SkipWhitespace();
	// Escaped? Go over '"'
	if (eRawType == RCT_Escaped && *pPos++ != '"')
		{ notFound("Escaped string"); return StdBuf(); }
	// Create buffer
	StdBuf OutBuf; OutBuf.New(iLength + (fAppendNull ? sizeof('\0') : 0));
	// Read
	char *pOut = getMBufPtr<char>(OutBuf);
	while (iLength && !TestStringEnd(eRawType))
	{
		// Read a character
		if (eRawType == RCT_Escaped)
			*pOut++ = ReadEscapedChar();
		else
			*pOut++ = *pPos++;
		// Count it
		iLength--;
	}
	// Escaped: Go over '"'
	if (eRawType == RCT_Escaped)
	{
		while (*pPos != '"')
		{
			if (!*pPos || *pPos == '\n' || *pPos == '\r')
			{
				Warn("string not terminated!");
				pPos--;
				break;
			}
			pPos++;
		}
		pPos++;
	}
	// Nothing read? Identifiers need to be non-empty
	if (pOut == OutBuf.getData() && (eRawType == RCT_Idtf || eRawType == RCT_ID))
		{ notFound("String"); return StdBuf(); }
	// Append null
	if (fAppendNull)
		*pOut = '\0';
	// Shrink, if less characters were read
	OutBuf.Shrink(iLength);
	// Done
	return OutBuf;
}

char StdCompilerINIRead::ReadEscapedChar()
{
	// Catch some no-noes like \0, \n etc.
	if (*pPos >= 0 && iscntrl((unsigned char)*pPos))
	{
		Warn("Nonprintable character found in string: %02x", static_cast<unsigned char>(*pPos));
		return *pPos;
	}
	// Not escaped? Just return it
	if (*pPos != '\\') return *pPos++;
	// What type of escape?
	switch (*++pPos)
	{
	case 'a': pPos++; return '\a';
	case 'b': pPos++; return '\b';
	case 'f': pPos++; return '\f';
	case 'n': pPos++; return '\n';
	case 'r': pPos++; return '\r';
	case 't': pPos++; return '\t';
	case 'v': pPos++; return '\v';
	case '\'': pPos++; return '\'';
	case '"': pPos++; return '"';
	case '\\': pPos++; return '\\';
	case '?': pPos++; return '?';
	case 'x':
		// Treat '\x' as 'x' - damn special cases
		if (!isxdigit((unsigned char)*++pPos))
			return 'x';
		else
		{
			// Read everything that looks like it might be hexadecimal - MSVC does it this way, so do not sue me.
			int iCode = 0;
			do
				{ iCode = iCode * 16 + (isdigit((unsigned char)*pPos) ? *pPos - '0' : *pPos - 'a' + 10); pPos++; }
			while (isxdigit((unsigned char)*pPos));
			// Done. Don't bother to check the range (we aren't doing anything mission-critical here, are we?)
			return char(iCode);
		}
	default:
		// Not octal? Let it pass through.
		if (!isdigit((unsigned char)*pPos) || *pPos >= '8')
			return *pPos++;
		else
		{
			// Read it the octal way.
			int iCode = 0;
			do
				{ iCode = iCode * 8 + (*pPos - '0'); pPos++;}
			while (isdigit((unsigned char)*pPos) && *pPos < '8');
			// Done. See above.
			return char(iCode);
		}
	}
	// unreachable
	assert (false);
}

void StdCompilerINIRead::notFound(const char *szWhat)
{
	excNotFound("%s expected", szWhat);
}

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
{
	const char *szName = reinterpret_cast<const char *>(pData);
	if (!szPosition || !*szPosition)
		DebugLogF("WARNING: %s (in %s)", szError, szName);
	else
		DebugLogF("WARNING: %s (in %s, %s)", szError, szPosition, szName);
}
