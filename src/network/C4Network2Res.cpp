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
#include "network/C4Network2Res.h"

#include "c4group/C4Components.h"
#include "c4group/C4Group.h"
#include "control/C4GameControl.h"
#include "lib/C4Random.h"
#include "game/C4Application.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#pragma warning (disable : 4355)
#endif

// compile debug options
// #define C4NET2RES_LOAD_ALL
#define C4NET2RES_DEBUG_LOG

// helper

class DirSizeHelper
{
	static uint32_t iSize, iMaxSize;
	static bool Helper(const char *szPath)
	{
		if (szPath[SLen(szPath)-1] == '.')
			return false;
		if (iSize > iMaxSize)
			return false;
		if (DirectoryExists(szPath))
			ForEachFile(szPath, &Helper);
		else if (FileExists(szPath))
			iSize += FileSize(szPath);
		return true;
	}
public:
	static bool GetDirSize(const char *szPath, uint32_t *pSize, uint32_t inMaxSize = ~0)
	{
		// Security
		if (!pSize) return false;
		// Fold it
		iSize = 0; iMaxSize = inMaxSize;
		ForEachFile(szPath, &Helper);
		// Return
		*pSize = iSize;
		return true;
	}
};
uint32_t DirSizeHelper::iSize, DirSizeHelper::iMaxSize;

// *** C4Network2ResCore

C4Network2ResCore::C4Network2ResCore()
		: iFileSize(~0u), iFileCRC(~0u), iContentsCRC(~0u),
		iChunkSize(C4NetResChunkSize)
{
}

void C4Network2ResCore::Set(C4Network2ResType enType, int32_t iResID, const char *strFileName, uint32_t inContentsCRC)
{
	// Initialize base data
	eType = enType; iID = iResID; iDerID = -1;
	fLoadable = false;
	iFileSize = iFileCRC = ~0; iContentsCRC = inContentsCRC;
	iChunkSize = C4NetResChunkSize;
	FileName.Copy(strFileName);
}

void C4Network2ResCore::SetLoadable(uint32_t iSize, uint32_t iCRC)
{
	fLoadable = true;
	iFileSize = iSize;
	iFileCRC = iCRC;
}

void C4Network2ResCore::Clear()
{
	eType = NRT_Null;
	iID = iDerID = -1;
	fLoadable = false;
	FileName.Clear();
	iFileSize = iFileCRC = iContentsCRC = ~0u;
	fHasFileSHA = false;
}

// C4PacketBase virtuals

void C4Network2ResCore::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eType, C4Network2ResType_EnumMap), "Type", NRT_Null));
	pComp->Value(mkNamingAdapt(iID, "ID", -1));
	pComp->Value(mkNamingAdapt(iDerID, "DerID", -1));
	pComp->Value(mkNamingAdapt(fLoadable, "Loadable", true));
	if (fLoadable)
	{
		pComp->Value(mkNamingAdapt(iFileSize, "FileSize", 0U));
		pComp->Value(mkNamingAdapt(iFileCRC, "FileCRC", 0U));
		pComp->Value(mkNamingAdapt(iChunkSize, "ChunkSize", C4NetResChunkSize));
		if (!iChunkSize) pComp->excCorrupt("zero chunk size");
	}
	pComp->Value(mkNamingAdapt(iContentsCRC, "ContentsCRC", 0U));
	pComp->Value(mkNamingCountAdapt(fHasFileSHA, "FileSHA"));
	if (fHasFileSHA)
		pComp->Value(mkNamingAdapt(mkHexAdapt(FileSHA), "FileSHA"));
	pComp->Value(mkNamingAdapt(mkNetFilenameAdapt(FileName), "Filename", ""));
}

// *** C4Network2ResLoad

C4Network2ResLoad::C4Network2ResLoad(int32_t inChunk, int32_t inByClient)
		: iChunk(inChunk), Timestamp(time(nullptr)), iByClient(inByClient), pNext(nullptr)
{

}

C4Network2ResLoad::~C4Network2ResLoad() = default;

bool C4Network2ResLoad::CheckTimeout()
{
	return difftime(time(nullptr), Timestamp) >= C4NetResLoadTimeout;
}

// *** C4Network2ResChunkData

C4Network2ResChunkData::C4Network2ResChunkData() = default;

C4Network2ResChunkData::C4Network2ResChunkData(const C4Network2ResChunkData &Data2)
		: C4PacketBase(Data2),
		iChunkCnt(Data2.getChunkCnt())
{
	// add ranges
	Merge(Data2);
}

C4Network2ResChunkData::~C4Network2ResChunkData()
{
	Clear();
}

C4Network2ResChunkData &C4Network2ResChunkData::operator =(const C4Network2ResChunkData &Data2)
{
	// clear, merge
	SetIncomplete(Data2.getChunkCnt());
	Merge(Data2);
	return *this;
}

void C4Network2ResChunkData::SetIncomplete(int32_t inChunkCnt)
{
	Clear();
	// just set total chunk count
	iChunkCnt = inChunkCnt;
}

void C4Network2ResChunkData::SetComplete(int32_t inChunkCnt)
{
	Clear();
	// set total chunk count
	iPresentChunkCnt = iChunkCnt = inChunkCnt;
	// create one range
	ChunkRange *pRange = new ChunkRange;
	pRange->Start = 0; pRange->Length = iChunkCnt;
	pRange->Next = nullptr;
	pChunkRanges = pRange;
}

void C4Network2ResChunkData::AddChunk(int32_t iChunk)
{
	AddChunkRange(iChunk, 1);
}

void C4Network2ResChunkData::AddChunkRange(int32_t iStart, int32_t iLength)
{
	// security
	if (iStart < 0 || iStart + iLength > iChunkCnt || iLength <= 0) return;
	// find position
	ChunkRange *pRange, *pPrev;
	for (pRange = pChunkRanges, pPrev = nullptr; pRange; pPrev = pRange, pRange = pRange->Next)
		if (pRange->Start >= iStart)
			break;
	// create new
	ChunkRange *pNew = new ChunkRange;
	pNew->Start = iStart; pNew->Length = iLength;
	// add to list
	pNew->Next = pRange;
	(pPrev ? pPrev->Next : pChunkRanges) = pNew;
	// counts
	iPresentChunkCnt += iLength; iChunkRangeCnt++;
	// check merges
	if (pPrev && MergeRanges(pPrev))
		while (MergeRanges(pPrev)) {}
	else
		while (MergeRanges(pNew)) {}
}

void C4Network2ResChunkData::Merge(const C4Network2ResChunkData &Data2)
{
	// must have same basis chunk count
	assert(iChunkCnt == Data2.getChunkCnt());
	// add ranges
	for (ChunkRange *pRange = Data2.pChunkRanges; pRange; pRange = pRange->Next)
		AddChunkRange(pRange->Start, pRange->Length);
}

void C4Network2ResChunkData::Clear()
{
	iChunkCnt = iPresentChunkCnt = iChunkRangeCnt = 0;
	// remove all ranges
	while (pChunkRanges)
	{
		ChunkRange *pDelete = pChunkRanges;
		pChunkRanges = pDelete->Next;
		delete pDelete;
	}
}

int32_t C4Network2ResChunkData::GetChunkToRetrieve(const C4Network2ResChunkData &Available, int32_t iLoadingCnt, int32_t *pLoading) const
{
	// (this version is highly calculation-intensitive, yet the most satisfactory
	//  solution I could find)

	// find everything that should not be retrieved
	C4Network2ResChunkData ChData; Available.GetNegative(ChData);
	ChData.Merge(*this);
	for (int32_t i = 0; i < iLoadingCnt; i++)
		ChData.AddChunk(pLoading[i]);
	// nothing to retrieve?
	if (ChData.isComplete()) return -1;
	// invert to get everything that should be retrieved
	C4Network2ResChunkData ChData2; ChData.GetNegative(ChData2);
	// select chunk (random)
	uint32_t iRetrieveChunk = UnsyncedRandom(ChData2.getPresentChunkCnt());
	// return
	return ChData2.getPresentChunk(iRetrieveChunk);
}

bool C4Network2ResChunkData::MergeRanges(ChunkRange *pRange)
{
	// no next entry?
	if (!pRange || !pRange->Next) return false;
	// do merge?
	ChunkRange *pNext = pRange->Next;
	if (pRange->Start + pRange->Length < pNext->Start) return false;
	// get overlap
	int32_t iOverlap = std::min((pRange->Start + pRange->Length) - pNext->Start, pNext->Length);
	// set new chunk range
	pRange->Length += pNext->Length - iOverlap;
	// remove range
	pRange->Next = pNext->Next;
	delete pNext;
	// counts
	iChunkRangeCnt--; iPresentChunkCnt -= iOverlap;
	// ok
	return true;
}

void C4Network2ResChunkData::GetNegative(C4Network2ResChunkData &Target) const
{
	// clear target
	Target.SetIncomplete(iChunkCnt);
	// add all ranges that are missing
	int32_t iFreeStart = 0;
	for (ChunkRange *pRange = pChunkRanges; pRange; pRange = pRange->Next)
	{
		// add range
		Target.AddChunkRange(iFreeStart, pRange->Start - iFreeStart);
		// safe new start
		iFreeStart = pRange->Start + pRange->Length;
	}
	// add last range
	Target.AddChunkRange(iFreeStart, iChunkCnt - iFreeStart);
}

int32_t C4Network2ResChunkData::getPresentChunk(int32_t iNr) const
{
	for (ChunkRange *pRange = pChunkRanges; pRange; pRange = pRange->Next)
		if (iNr < pRange->Length)
			return iNr + pRange->Start;
		else
			iNr -= pRange->Length;
	return -1;
}

void C4Network2ResChunkData::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	// Data
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iChunkCnt), "ChunkCnt", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iChunkRangeCnt), "ChunkRangeCnt", 0));
	// Ranges
	if (!pComp->Name("Ranges"))
		pComp->excCorrupt("ResChunk ranges expected!");
	ChunkRange *pRange = nullptr;
	for (int32_t i = 0; i < iChunkRangeCnt; i++)
	{
		// Create new range / go to next range
		if (deserializing)
			pRange = (pRange ? pRange->Next : pChunkRanges) = new ChunkRange;
		else
			pRange = pRange ? pRange->Next : pChunkRanges;
		// Separate
		if (i) pComp->Separator();
		// Compile range
		pComp->Value(mkIntPackAdapt(pRange->Start));
		pComp->Separator(StdCompiler::SEP_PART2);
		pComp->Value(mkIntPackAdapt(pRange->Length));
	}
	// Terminate list
	if (deserializing)
		(pRange ? pRange->Next : pChunkRanges) = nullptr;
	pComp->NameEnd();
}

// *** C4Network2Res

C4Network2Res::C4Network2Res(C4Network2ResList *pnParent)
		: fDirty(false),
		fTempFile(false), fStandaloneFailed(false),
		iRefCnt(0), fRemoved(false),
		iLastReqTime(0),
		fLoading(false),
		pCChunks(nullptr), iDiscoverStartTime(0), pLoads(nullptr), iLoadCnt(0),
		pNext(nullptr),
		pParent(pnParent)
{
	szFile[0] = szStandalone[0] = '\0';
}

C4Network2Res::~C4Network2Res()
{
	assert(!pNext);
	Clear();
}

bool C4Network2Res::SetByFile(const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName, bool fSilent)
{
	CStdLock FileLock(&FileCSec);
	// default resource name: relative path
	if (!szResName) szResName = Config.AtRelativePath(strFilePath);
	SCopy(strFilePath, szFile, sizeof(szFile)-1);
	// group?
	C4Group Grp;
	if (Reloc.Open(Grp, strFilePath))
		return SetByGroup(&Grp, fTemp, eType, iResID, szResName, fSilent);
	// so it needs to be a file
	StdStrBuf szFullFile;
	if (!Reloc.LocateItem(szFile, szFullFile))
		{ if (!fSilent) LogF("SetByFile: file %s not found!", strFilePath); return false; }
	// calc checksum
	uint32_t iCRC32;
	if (!GetFileCRC(szFullFile.getData(), &iCRC32)) return false;
#ifdef C4NET2RES_DEBUG_LOG
	// log
	LogSilentF("Network: Resource: complete %d:%s is file %s (%s)", iResID, szResName, szFile, fTemp ? "temp" : "static");
#endif
	// set core
	Core.Set(eType, iResID, Config.AtRelativePath(szFullFile.getData()), iCRC32);
	// set own data
	fDirty = true;
	fTempFile = fTemp;
	fStandaloneFailed = false;
	fRemoved = false;
	iLastReqTime = time(nullptr);
	fLoading = false;
	// ok
	return true;
}

bool C4Network2Res::SetByGroup(C4Group *pGrp, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName, bool fSilent) // by main thread
{
	Clear();
	CStdLock FileLock(&FileCSec);
	// default resource name: relative path
	StdStrBuf sResName;
	if (szResName)
		sResName = szResName;
	else
	{
		StdStrBuf sFullName = pGrp->GetFullName();
		sResName.Copy(Config.AtRelativePath(sFullName.getData()));
	}
	SCopy(pGrp->GetFullName().getData(), szFile, sizeof(szFile)-1);
	// set core
	Core.Set(eType, iResID, sResName.getData(), pGrp->EntryCRC32());
#ifdef C4NET2RES_DEBUG_LOG
	// log
	LogSilentF("Network: Resource: complete %d:%s is file %s (%s)", iResID, sResName.getData(), szFile, fTemp ? "temp" : "static");
#endif
	// set data
	fDirty = true;
	fTempFile = fTemp;
	fStandaloneFailed = false;
	fRemoved = false;
	iLastReqTime = time(nullptr);
	fLoading = false;
	// ok
	return true;
}

bool C4Network2Res::SetByCore(const C4Network2ResCore &nCore, bool fSilent, const char *szAsFilename, int32_t iRecursion) // by main thread
{
	StdStrBuf sFilename;
	// try open local file
	const char *szFilename = szAsFilename ? szAsFilename : GetC4Filename(nCore.getFileName());
	if (SetByFile(szFilename, false, nCore.getType(), nCore.getID(), nCore.getFileName(), fSilent))
	{
		// check contents checksum
		if (Core.getContentsCRC() == nCore.getContentsCRC())
		{
			// set core
			fDirty = true;
			Core = nCore;
			// ok then
			return true;
		}
	}
	// get and search for filename without specified folder (e.g., Castle.ocs when the opened game is Easy.ocf\Castle.ocs)
	const char *szFilenameOnly = GetFilename(szFilename);
	const char *szFilenameC4 = GetC4Filename(szFilename);
	if (szFilenameOnly != szFilenameC4)
	{
		sFilename.Copy(szFilename, SLen(szFilename) - SLen(szFilenameC4));
		sFilename.Append(szFilenameOnly);
		if (SetByCore(nCore, fSilent, szFilenameOnly, Config.Network.MaxResSearchRecursion)) return true;
	}
	// if it could still not be set, try within all folders of root (ignoring special folders), and try as file outside the folder
	// but do not recurse any deeper than set by config (default: One folder)
	if (iRecursion >= Config.Network.MaxResSearchRecursion) return false;
	StdStrBuf sSearchPath; const char *szSearchPath;
	if (!iRecursion)
		szSearchPath = Config.General.ExePath.getData();
	else
	{
		sSearchPath.Copy(szFilename, SLen(szFilename) - SLen(szFilenameC4));
		szSearchPath = sSearchPath.getData();
	}
	StdStrBuf sNetPath; sNetPath.Copy(Config.Network.WorkPath);
	char *szNetPath = sNetPath.GrabPointer();
	TruncateBackslash(szNetPath);
	sNetPath.Take(szNetPath);
	for (DirectoryIterator i(szSearchPath); *i; ++i)
		if (DirectoryExists(*i))
			if (!*GetExtension(*i)) // directories without extension only
				if (!szNetPath || !*szNetPath || !ItemIdentical(*i, szNetPath)) // ignore network path
				{
					// search for complete name at subpath (e.g. MyFolder\Easy.ocf\Castle.ocs)
					sFilename.Format("%s%c%s", *i, DirectorySeparator, szFilenameC4);
					if (SetByCore(nCore, fSilent, sFilename.getData(), iRecursion + 1))
						return true;
				}
	// file could not be found locally
	return false;
}

bool C4Network2Res::SetLoad(const C4Network2ResCore &nCore) // by main thread
{
	Clear();
	CStdLock FileLock(&FileCSec);
	// must be loadable
	if (!nCore.isLoadable()) return false;
	// save core, set chunks
	Core = nCore;
	Chunks.SetIncomplete(Core.getChunkCnt());
	// create temporary file
	if (!pParent->FindTempResFileName(Core.getFileName(), szFile))
		return false;
#ifdef C4NET2RES_DEBUG_LOG
	// log
	Application.InteractiveThread.ThreadLogS("Network: Resource: loading %d:%s to file %s", Core.getID(), Core.getFileName(), szFile);
#endif
	// set standalone (result is going to be binary-compatible)
	SCopy(szFile, szStandalone, sizeof(szStandalone) - 1);
	// set flags
	fDirty = false;
	fTempFile = true;
	fStandaloneFailed = false;
	fRemoved = false;
	iLastReqTime = time(nullptr);
	fLoading = true;
	// No discovery yet
	iDiscoverStartTime = 0;
	return true;
}

bool C4Network2Res::SetDerived(const char *strName, const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iDResID)
{
	Clear();
	CStdLock FileLock(&FileCSec);
	// set core
	Core.Set(eType, C4NetResIDAnonymous, strName, ~0);
	Core.SetDerived(iDResID);
	// save file path
	SCopy(strFilePath, szFile, _MAX_PATH);
	*szStandalone = '\0';
	// set flags
	fDirty = false;
	fTempFile = fTemp;
	fStandaloneFailed = false;
	fRemoved = false;
	iLastReqTime = time(nullptr);
	fLoading = false;
	// Do not set any chunk data - anonymous resources are very likely to change.
	// Wait for FinishDerived()-call.
	return true;
}

void C4Network2Res::ChangeID(int32_t inID)
{
	Core.SetID(inID);
}

bool C4Network2Res::IsBinaryCompatible()
{
	// returns wether the standalone of this resource is binary compatible
	// to the official version (means: matches the file checksum)

	CStdLock FileLock(&FileCSec);
	// standalone set? ok then (see GetStandalone)
	if (szStandalone[0]) return true;
	// is a directory?
	if (DirectoryExists(szFile))
		// forget it - if the file is packed now, the creation time and author
		// won't match.
		return false;
	// try to create the standalone
	return GetStandalone(nullptr, 0, false, false, true);
}

bool C4Network2Res::GetStandalone(char *pTo, int32_t iMaxL, bool fSetOfficial, bool fAllowUnloadable, bool fSilent)
{
	// already set?
	if (szStandalone[0])
	{
		if (pTo) SCopy(szStandalone, pTo, iMaxL);
		return true;
	}
	// already tried and failed? No point in retrying
	if (fStandaloneFailed) return false;
	// not loadable? Wo won't be able to check the standalone as the core will lack the needed information.
	// the standalone won't be interesting in this case, anyway.
	if (!fSetOfficial && !Core.isLoadable()) return false;
	// set flag, so failure below will let future calls fail
	fStandaloneFailed = true;
	// lock file
	CStdLock FileLock(&FileCSec);

	// directory?
	SCopy(szFile, szStandalone, sizeof(szStandalone)-1);
	if (DirectoryExists(szFile))
	{
		// size check for the directory, if allowed
		if (fAllowUnloadable)
		{
			uint32_t iDirSize;
			if (!DirSizeHelper::GetDirSize(szFile, &iDirSize, Config.Network.MaxLoadFileSize))
				{ if (!fSilent) LogF("Network: could not get directory size of %s!", szFile); szStandalone[0] = '\0'; return false; }
			if (iDirSize > uint32_t(Config.Network.MaxLoadFileSize))
				{ if (!fSilent) LogSilentF("Network: %s over size limit, will be marked unloadable!", szFile); szStandalone[0] = '\0'; return false; }
		}
		// log - this may take a few seconds
		if (!fSilent) LogF(LoadResStr("IDS_PRC_NETPACKING"), GetFilename(szFile));
		// pack inplace?
		if (!fTempFile)
		{
			if (!pParent->FindTempResFileName(szFile, szStandalone))
				{ if (!fSilent) Log("GetStandalone: could not find free name for temporary file!"); szStandalone[0] = '\0'; return false; }
			if (!C4Group_PackDirectoryTo(szFile, szStandalone))
				{ if (!fSilent) Log("GetStandalone: could not pack directory!"); szStandalone[0] = '\0'; return false; }
		}
		else if (!C4Group_PackDirectory(szStandalone))
			{ if (!fSilent) Log("GetStandalone: could not pack directory!"); if (!SEqual(szFile, szStandalone)) EraseDirectory(szStandalone); szStandalone[0] = '\0'; return false; }
		// make sure directory is packed
		if (DirectoryExists(szStandalone))
			{ if (!fSilent) Log("GetStandalone: directory hasn't been packed!"); if (!SEqual(szFile, szStandalone)) EraseDirectory(szStandalone); szStandalone[0] = '\0'; return false; }
		// fallthru
	}

	// doesn't exist physically?
	if (!FileExists(szStandalone))
	{
		// try C4Group (might be packed)
		if (!pParent->FindTempResFileName(szFile, szStandalone))
			{ if (!fSilent) Log("GetStandalone: could not find free name for temporary file!"); szStandalone[0] = '\0'; return false; }
		if (!C4Group_CopyItem(szFile, szStandalone))
			{ if (!fSilent) Log("GetStandalone: could not copy to temporary file!"); szStandalone[0] = '\0'; return false; }
	}

	// remains missing? give up.
	if (!FileExists(szStandalone))
		{ if (!fSilent) Log("GetStandalone: file not found!"); szStandalone[0] = '\0'; return false; }

	// do optimizations (delete unneeded entries)
	if (!OptimizeStandalone(fSilent))
		{ if (!SEqual(szFile, szStandalone)) EraseItem(szStandalone); szStandalone[0] = '\0'; return false; }

	// get file size
	size_t iSize = FileSize(szStandalone);
	// size limit
	if (fAllowUnloadable)
		if (iSize > uint32_t(Config.Network.MaxLoadFileSize))
			{ if (!fSilent) LogSilentF("Network: %s over size limit, will be marked unloadable!", szFile); szStandalone[0] = '\0'; return false; }
	// check
	if (!fSetOfficial && iSize != Core.getFileSize())
	{
		// remove file
		if (!SEqual(szFile, szStandalone)) EraseItem(szStandalone);
		szStandalone[0] = '\0';
		// sorry, this version isn't good enough :(
		return false;
	}

	// calc checksum
	uint32_t iCRC32;
	if (!GetFileCRC(szStandalone, &iCRC32))
		{ if (!fSilent) Log("GetStandalone: could not calculate checksum!"); return false; }
	// set / check
	if (!fSetOfficial && iCRC32 != Core.getFileCRC())
	{
		// remove file, return
		if (!SEqual(szFile, szStandalone)) EraseItem(szStandalone);
		szStandalone[0] = '\0';
		return false;
	}

	// we didn't fail
	fStandaloneFailed = false;
	// mark resource as loadable and safe file information
	Core.SetLoadable(iSize, iCRC32);
	// set up chunk data
	Chunks.SetComplete(Core.getChunkCnt());
	// ok
	return true;
}

bool C4Network2Res::CalculateSHA()
{
	// already present?
	if (Core.hasFileSHA()) return true;
	// get the file
	char szStandalone[_MAX_PATH_LEN];
	if (!GetStandalone(szStandalone, _MAX_PATH, false))
		SCopy(szFile, szStandalone, _MAX_PATH);
	// get the hash
	BYTE hash[SHA_DIGEST_LENGTH];
	if (!GetFileSHA1(szStandalone, hash))
		return false;
	// save it back
	Core.SetFileSHA(hash);
	// okay
	return true;
}


C4Network2Res::Ref C4Network2Res::Derive()
{
	// Called before the file is changed. Rescues all files and creates a
	// new resource for the file. This resource is flagged as "anonymous", as it
	// has no official core (no res ID, to be exact).
	// The resource gets its final core when FinishDerive() is called.

	// For security: This doesn't make much sense if the resource is currently being
	// loaded. So better assume the caller doesn't know what he's doing and check.
	if (isLoading()) return nullptr;

	CStdLock FileLock(&FileCSec);
	// Save back original file name
	char szOrgFile[_MAX_PATH_LEN];
	SCopy(szFile, szOrgFile, _MAX_PATH);
	bool fOrgTempFile = fTempFile;

	// Create a copy of the file, if neccessary
	if (!*szStandalone || SEqual(szStandalone, szFile))
	{
		if (!pParent->FindTempResFileName(szOrgFile, szFile))
			{ Log("Derive: could not find free name for temporary file!"); return nullptr; }
		if (!C4Group_CopyItem(szOrgFile, szFile))
			{ Log("Derive: could not copy to temporary file!"); return nullptr; }
		// set standalone
		if (*szStandalone)
			SCopy(szFile, szStandalone, _MAX_PATH);
		fTempFile = true;
	}
	else
	{
		// Standlone exists: Just set szFile to point on the standlone. It's
		// assumed that the original file isn't of intrest after this point anyway.
		SCopy(szStandalone, szFile, _MAX_PATH);
		fTempFile = true;
	}

	Application.InteractiveThread.ThreadLogS("Network: Resource: deriving from %d:%s, original at %s", getResID(), Core.getFileName(), szFile);

	// (note: should remove temp file if something fails after this point)

	// create new resource
	C4Network2Res::Ref pDRes = new C4Network2Res(pParent);
	if (!pDRes) return nullptr;

	// initialize
	if (!pDRes->SetDerived(Core.getFileName(), szOrgFile, fOrgTempFile, getType(), getResID()))
		return nullptr;

	// add to list
	pParent->Add(pDRes);

	// return new resource
	return pDRes;
}

bool C4Network2Res::FinishDerive() // by main thread
{
	// All changes have been made. Register this resource with a new ID.

	// security
	if (!isAnonymous()) return false;

	CStdLock FileLock(&FileCSec);
	// Save back data
	int32_t iDerID = Core.getDerID();
	char szName[_MAX_PATH_LEN]; SCopy(Core.getFileName(), szName, _MAX_PATH);
	char szFileC[_MAX_PATH_LEN]; SCopy(szFile, szFileC, _MAX_PATH);
	// Set by file
	if (!SetByFile(szFileC, fTempFile, getType(), pParent->nextResID(), szName))
		return false;
	// create standalone
	if (!GetStandalone(nullptr, 0, true))
		return false;
	// Set ID
	Core.SetDerived(iDerID);
	// announce derive
	pParent->getIOClass()->BroadcastMsg(MkC4NetIOPacket(PID_NetResDerive, Core));
	// derivation is dirty bussines
	fDirty = true;
	// ok
	return true;
}

bool C4Network2Res::FinishDerive(const C4Network2ResCore &nCore)
{
	// security
	if (!isAnonymous()) return false;
	// Set core
	Core = nCore;
	// Set chunks (assume the resource is complete)
	Chunks.SetComplete(Core.getChunkCnt());

	// Note that the Contents-CRC is /not/ checked. Derivation needs to be
	// synchronized outside of C4Network2Res.

	// But note that the resource /might/ be binary compatible (though very
	// unlikely), so do not set fNotBinaryCompatible.

	// ok
	return true;
}

C4Group *C4Network2Res::OpenAsGrp() const
{
	C4Group *pnGrp = new C4Group();
	if (!pnGrp->Open(szFile))
	{
		delete pnGrp;
		return nullptr;
	}
	return pnGrp;
}

void C4Network2Res::Remove()
{
	// schedule for removal
	fRemoved = true;
}

bool C4Network2Res::SendStatus(C4Network2IOConnection *pTo)
{
	// pack status
	C4NetIOPacket Pkt = MkC4NetIOPacket(PID_NetResStat, C4PacketResStatus(Core.getID(), Chunks));
	// to one client?
	if (pTo)
		return pTo->Send(Pkt);
	else
	{
		// reset dirty flag
		fDirty = false;
		// broadcast status
		assert(pParent && pParent->getIOClass());
		return pParent->getIOClass()->BroadcastMsg(Pkt);
	}
}

bool C4Network2Res::SendChunk(uint32_t iChunk, int32_t iToClient)
{
	assert(pParent && pParent->getIOClass());
	if (!szStandalone[0] || iChunk >= Core.getChunkCnt()) return false;
	// find connection for given client (one of the rare uses of the data connection)
	C4Network2IOConnection *pConn = pParent->getIOClass()->GetDataConnection(iToClient);
	if (!pConn) return false;
	// save last request time
	iLastReqTime = time(nullptr);
	// create packet
	CStdLock FileLock(&FileCSec);
	C4Network2ResChunk ResChunk;
	ResChunk.Set(this, iChunk);
	// send
	bool fSuccess = pConn->Send(MkC4NetIOPacket(PID_NetResData, ResChunk));
	pConn->DelRef();
	return fSuccess;
}

void C4Network2Res::AddRef()
{
	++iRefCnt;
}

void C4Network2Res::DelRef()
{
	if (--iRefCnt == 0)
		delete this;
}

void C4Network2Res::OnDiscover(C4Network2IOConnection *pBy)
{
	if (!IsBinaryCompatible()) return;
	// discovered
	iLastReqTime = time(nullptr);
	// send status back
	SendStatus(pBy);
}

void C4Network2Res::OnStatus(const C4Network2ResChunkData &rChunkData, C4Network2IOConnection *pBy)
{
	if (!fLoading) return;
	// discovered a source: reset timeout
	iDiscoverStartTime = 0;
	// check if the chunk data is valid
	if (rChunkData.getChunkCnt() != Chunks.getChunkCnt())
		return;
	// add chunk data
	ClientChunks *pChunks;
	for (pChunks = pCChunks; pChunks; pChunks = pChunks->Next)
		if (pChunks->ClientID == pBy->getClientID())
			break;
	// not found? add
	if (!pChunks)
	{
		pChunks = new ClientChunks();
		pChunks->Next = pCChunks;
		pCChunks = pChunks;
	}
	pChunks->ClientID = pBy->getClientID();
	pChunks->Chunks = rChunkData;
	// check load
	if (!StartLoad(pChunks->ClientID, pChunks->Chunks))
		RemoveCChunks(pCChunks);
}

void C4Network2Res::OnChunk(const C4Network2ResChunk &rChunk)
{
	if (!fLoading) return;
	// correct resource?
	if (rChunk.getResID() != getResID()) return;
	// add resource data
	CStdLock FileLock(&FileCSec);
	bool fSuccess = rChunk.AddTo(this, pParent->getIOClass());
#ifdef C4NET2RES_DEBUG_LOG
	// log
	Application.InteractiveThread.ThreadLogS("Network: Res: %s chunk %d to resource %s (%s)%s", fSuccess ? "added" : "could not add", rChunk.getChunkNr(), Core.getFileName(), szFile, fSuccess ? "" : "!");
#endif
	if (fSuccess)
	{
		// status changed
		fDirty = true;
		// remove load waits
		for (C4Network2ResLoad *pLoad = pLoads, *pNext; pLoad; pLoad = pNext)
		{
			pNext = pLoad->Next();
			if (static_cast<uint32_t>(pLoad->getChunk()) == rChunk.getChunkNr())
				RemoveLoad(pLoad);
		}
	}
	// complete?
	if (Chunks.isComplete())
		EndLoad();
	// check: start new loads?
	else
		StartNewLoads();
}

bool C4Network2Res::DoLoad()
{
	if (!fLoading) return true;
	// any loads currently active?
	if (iLoadCnt)
	{
		// check for load timeouts
		int32_t iLoadsRemoved = 0;
		for (C4Network2ResLoad *pLoad = pLoads, *pNext; pLoad; pLoad = pNext)
		{
			pNext = pLoad->Next();
			if (pLoad->CheckTimeout())
			{
				RemoveLoad(pLoad);
				iLoadsRemoved++;
			}
		}
		// start new loads
		if (iLoadsRemoved) StartNewLoads();
	}
	else
	{
		// discover timeout?
		if (iDiscoverStartTime)
			if (difftime(time(nullptr), iDiscoverStartTime) > C4NetResDiscoverTimeout)
				return false;
	}
	// ok
	return true;
}

bool C4Network2Res::NeedsDiscover()
{
	// loading, but no active load sources?
	if (fLoading && !iLoadCnt)
	{
		// set timeout, if this is the first discover
		if (!iDiscoverStartTime)
			iDiscoverStartTime = time(nullptr);
		// do discover
		return true;
	}
	return false;
}

void C4Network2Res::Clear()
{
	CStdLock FileLock(&FileCSec);
	// delete files
	if (fTempFile)
		if (FileExists(szFile))
			if (!EraseFile(szFile))
				LogSilentF("Network: Could not delete temporary resource file (%s)", strerror(errno));
	if (szStandalone[0] && !SEqual(szFile, szStandalone))
		if (FileExists(szStandalone))
			if (!EraseFile(szStandalone))
				LogSilentF("Network: Could not delete temporary resource file (%s)", strerror(errno));
	szFile[0] = szStandalone[0] = '\0';
	fDirty = false;
	fTempFile = false;
	Core.Clear();
	Chunks.Clear();
	fRemoved = false;
	ClearLoad();
}

int32_t C4Network2Res::OpenFileRead()
{
	CStdLock FileLock(&FileCSec);
	if (!GetStandalone(nullptr, 0, false, false, true)) return -1;
	// FIXME: Use standard OC file access api here
#ifdef _WIN32
	return _wopen(GetWideChar(szStandalone), _O_BINARY | O_RDONLY);
#else
	return open(szStandalone, _O_BINARY | O_CLOEXEC | O_RDONLY);
#endif
}

int32_t C4Network2Res::OpenFileWrite()
{
	CStdLock FileLock(&FileCSec);
	// FIXME: Use standard OC file access api here
#ifdef _WIN32
	return _wopen(GetWideChar(szStandalone), _O_BINARY | O_CREAT | O_WRONLY, S_IREAD | S_IWRITE);
#else
	return open(szStandalone, _O_BINARY | O_CLOEXEC | O_CREAT | O_WRONLY, S_IREAD | S_IWRITE);
#endif
}

void C4Network2Res::StartNewLoads()
{
	if (!pCChunks) return;
	// count clients
	int32_t iCChunkCnt = 0; ClientChunks *pChunks;
	for (pChunks = pCChunks; pChunks; pChunks = pChunks->Next)
		iCChunkCnt++;
	// create array
	ClientChunks **pC = new ClientChunks *[iCChunkCnt];
	// initialize
	int32_t i;
	for (i = 0; i < iCChunkCnt; i++) pC[i] = nullptr;
	// create shuffled order
	for (pChunks = pCChunks, i = 0; i < iCChunkCnt; i++, pChunks = pChunks->Next)
	{
		// determine position
		int32_t iPos = UnsyncedRandom(iCChunkCnt - i);
		// find & set
		for (int32_t j = 0; ; j++)
			if (!pC[j] && !iPos--)
			{
				pC[j] = pChunks;
				break;
			}
	}
	// start new load until maximum count reached
	while (iLoadCnt + 1 <= C4NetResMaxLoad)
	{
		int32_t ioLoadCnt = iLoadCnt;
		// search someone
		for (i = 0; i < iCChunkCnt; i++)
			if (pC[i])
			{
				// try to start load
				if (!StartLoad(pC[i]->ClientID, pC[i]->Chunks))
					{ RemoveCChunks(pC[i]); pC[i] = nullptr; continue; }
				// success?
				if (iLoadCnt > ioLoadCnt) break;
			}
		// not found?
		if (i >= iCChunkCnt)
			break;
	}
	// clear up
	delete [] pC;
}

bool C4Network2Res::StartLoad(int32_t iFromClient, const C4Network2ResChunkData &Available)
{
	assert(pParent && pParent->getIOClass());
	// all slots used? ignore
	if (iLoadCnt + 1 >= C4NetResMaxLoad) return true;
	// is there already a load by this client? ignore
	for (C4Network2ResLoad *pPos = pLoads; pPos; pPos = pPos->Next())
		if (pPos->getByClient() == iFromClient)
			return true;
	// find chunk to retrieve
	int32_t iLoads[C4NetResMaxLoad]; int32_t i = 0;
	for (C4Network2ResLoad *pLoad = pLoads; pLoad; pLoad = pLoad->Next())
		iLoads[i++] = pLoad->getChunk();
	int32_t iRetrieveChunk = Chunks.GetChunkToRetrieve(Available, i, iLoads);
	// nothing? ignore
	if (iRetrieveChunk < 0 || (uint32_t)iRetrieveChunk >= Core.getChunkCnt())
		return true;
	// search message connection for client
	C4Network2IOConnection *pConn = pParent->getIOClass()->GetMsgConnection(iFromClient);
	if (!pConn) return false;
	// send request
	if (!pConn->Send(MkC4NetIOPacket(PID_NetResReq, C4PacketResRequest(Core.getID(), iRetrieveChunk))))
		{ pConn->DelRef(); return false; }
	pConn->DelRef();
#ifdef C4NET2RES_DEBUG_LOG
	// log
	Application.InteractiveThread.ThreadLogS("Network: Res: requesting chunk %d of %d:%s (%s) from client %d",
	    iRetrieveChunk, Core.getID(), Core.getFileName(), szFile, iFromClient);
#endif
	// create load class
	C4Network2ResLoad *pnLoad = new C4Network2ResLoad(iRetrieveChunk, iFromClient);
	// add to list
	pnLoad->pNext = pLoads;
	pLoads = pnLoad;
	iLoadCnt++;
	// ok
	return true;
}

void C4Network2Res::EndLoad()
{
	// clear loading data
	ClearLoad();
	// set complete
	fLoading = false;
	// call handler
	assert(pParent);
	pParent->OnResComplete(this);
}

void C4Network2Res::ClearLoad()
{
	// remove client chunks and loads
	fLoading = false;
	while (pCChunks) RemoveCChunks(pCChunks);
	while (pLoads) RemoveLoad(pLoads);
	iDiscoverStartTime = iLoadCnt = 0;
}

void C4Network2Res::RemoveLoad(C4Network2ResLoad *pLoad)
{
	if (pLoad == pLoads)
		pLoads = pLoad->Next();
	else
	{
		// find previous entry
		C4Network2ResLoad *pPrev;
		for (pPrev = pLoads; pPrev && pPrev->Next() != pLoad; pPrev = pPrev->Next()) {}
		// remove
		if (pPrev)
			pPrev->pNext = pLoad->Next();
	}
	// delete
	delete pLoad;
	iLoadCnt--;
}

void C4Network2Res::RemoveCChunks(ClientChunks *pChunks)
{
	if (pChunks == pCChunks)
		pCChunks = pChunks->Next;
	else
	{
		// find previous entry
		ClientChunks *pPrev;
		for (pPrev = pCChunks; pPrev && pPrev->Next != pChunks; pPrev = pPrev->Next) {}
		// remove
		if (pPrev)
			pPrev->Next = pChunks->Next;
	}
	// delete
	delete pChunks;
}

bool C4Network2Res::OptimizeStandalone(bool fSilent)
{
	CStdLock FileLock(&FileCSec);
	// for now: player files only
	if (Core.getType() == NRT_Player)
	{
		// log - this may take a few seconds
		if (!fSilent) LogF(LoadResStr("IDS_PRC_NETPREPARING"), GetFilename(szFile));
		// copy to temp file, if needed
		if (!fTempFile && SEqual(szFile, szStandalone))
		{
			char szNewStandalone[_MAX_PATH_LEN];
			if (!pParent->FindTempResFileName(szStandalone, szNewStandalone))
				{ if (!fSilent) Log("OptimizeStandalone: could not find free name for temporary file!"); return false; }
			if (!C4Group_CopyItem(szStandalone, szNewStandalone))
				{ if (!fSilent) Log("OptimizeStandalone: could not copy to temporary file!"); return false; } /* TODO: Test failure */
			SCopy(szNewStandalone, szStandalone, sizeof(szStandalone) - 1);
		}
		// open as group
		C4Group Grp;
		if (!Grp.Open(szStandalone))
			{ if (!fSilent) Log("OptimizeStandalone: could not open player file!"); return false; }
		// remove bigicon, if the file size is too large
		size_t iBigIconSize=0;
		if (Grp.FindEntry(C4CFN_BigIcon, nullptr, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon*1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
	}
	return true;
}

// *** C4Network2ResChunk

C4Network2ResChunk::C4Network2ResChunk() = default;

C4Network2ResChunk::~C4Network2ResChunk() = default;

bool C4Network2ResChunk::Set(C4Network2Res *pRes, uint32_t inChunk)
{
	const C4Network2ResCore &Core = pRes->getCore();
	iResID = pRes->getResID();
	iChunk = inChunk;
	// calculate offset and size
	int32_t iOffset = iChunk * Core.getChunkSize(),
	                  iSize = std::min<int32_t>(Core.getFileSize() - iOffset, C4NetResChunkSize);
	if (iSize < 0) { LogF("Network: could not get chunk from offset %d from resource file %s: File size is only %d!", iOffset, pRes->getFile(), Core.getFileSize()); return false; }
	// open file
	int32_t f = pRes->OpenFileRead();
	if (f == -1) { LogF("Network: could not open resource file %s!", pRes->getFile()); return false; }
	// seek
	if (iOffset)
		if (lseek(f, iOffset, SEEK_SET) != iOffset)
			{ close(f); LogF("Network: could not read resource file %s!", pRes->getFile()); return false; }
	// read chunk of data
	char *pBuf = (char *) malloc(iSize);
	if (read(f, pBuf, iSize) != iSize)
		{ free(pBuf); close(f); LogF("Network: could not read resource file %s!", pRes->getFile()); return false; }
	// set
	Data.Take(pBuf, iSize);
	// close
	close(f);
	// ok
	return true;
}

bool C4Network2ResChunk::AddTo(C4Network2Res *pRes, C4Network2IO *pIO) const
{
	assert(pRes); assert(pIO);
	const C4Network2ResCore &Core = pRes->getCore();
	// check
	if (iResID != pRes->getResID())
	{
#ifdef C4NET2RES_DEBUG_LOG
		Application.InteractiveThread.ThreadLogS("C4Network2ResChunk(%d)::AddTo(%s [%d]): Resource ID mismatch!", (int) iResID, (const char *) Core.getFileName(), (int) pRes->getResID());
#endif
		return false;
	}
	// calculate offset and size
	int32_t iOffset = iChunk * Core.getChunkSize();
	if (iOffset + Data.getSize() > Core.getFileSize())
	{
#ifdef C4NET2RES_DEBUG_LOG
		Application.InteractiveThread.ThreadLogS("C4Network2ResChunk(%d)::AddTo(%s [%d]): Adding %d bytes at offset %d exceeds expected file size of %d!", (int) iResID, (const char *) Core.getFileName(), (int) pRes->getResID(), (int) Data.getSize(), (int) iOffset, (int) Core.getFileSize());
#endif
		return false;
	}
	// open file
	int32_t f = pRes->OpenFileWrite();
	if (f == -1)
	{
#ifdef C4NET2RES_DEBUG_LOG
		Application.InteractiveThread.ThreadLogS("C4Network2ResChunk(%d)::AddTo(%s [%d]): Open write file error: %s!", (int) iResID, (const char *) Core.getFileName(), (int) pRes->getResID(), strerror(errno));
#endif
		return false;
	}
	// seek
	if (iOffset)
		if (lseek(f, iOffset, SEEK_SET) != iOffset)
		{
#ifdef C4NET2RES_DEBUG_LOG
			Application.InteractiveThread.ThreadLogS("C4Network2ResChunk(%d)::AddTo(%s [%d]): lseek file error: %s!", (int) iResID, (const char *) Core.getFileName(), (int) pRes->getResID(), strerror(errno));
#endif
			close(f);
			return false;
		}
	// write
	if (write(f, Data.getData(), Data.getSize()) != int32_t(Data.getSize()))
	{
#ifdef C4NET2RES_DEBUG_LOG
		Application.InteractiveThread.ThreadLogS("C4Network2ResChunk(%d)::AddTo(%s [%d]): write error: %s!", (int) iResID, (const char *) Core.getFileName(), (int) pRes->getResID(), strerror(errno));
#endif
		close(f);
		return false;
	}
	// ok, add chunks
	close(f);
	pRes->Chunks.AddChunk(iChunk);
	return true;
}

void C4Network2ResChunk::CompileFunc(StdCompiler *pComp)
{
	// pack header
	pComp->Value(mkNamingAdapt(iResID, "ResID", -1));
	pComp->Value(mkNamingAdapt(iChunk, "Chunk", ~0U));
	// Data
	pComp->Value(mkNamingAdapt(Data, "Data"));
}

// *** C4Network2ResList

C4Network2ResList::C4Network2ResList()
		: ResListCSec(this)
		, iNextResID((~0u) << 16)
{}

C4Network2ResList::~C4Network2ResList()
{
	Clear();
}

bool C4Network2ResList::Init(int32_t inClientID, C4Network2IO *pIOClass) // by main thread
{
	// clear old list
	Clear();
	// safe IO class
	pIO = pIOClass;
	// set client id
	iNextResID = iClientID = 0;
	SetLocalID(inClientID);
	// create network path
	if (!CreateNetworkFolder()) return false;
	// ok
	return true;
}

void C4Network2ResList::SetLocalID(int32_t inClientID)
{
	CStdLock ResIDLock(&ResIDCSec);
	int32_t iOldClientID = iClientID;
	int32_t iIDDiff = (inClientID - iClientID) << 16;
	// set new counter
	iClientID = inClientID;
	iNextResID += iIDDiff;
	// change resource ids
	CStdLock ResListLock(&ResListCSec);
	for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
		if (pRes->getResClient() == iOldClientID)
			pRes->ChangeID(pRes->getResID() + iIDDiff);
}

int32_t C4Network2ResList::nextResID() // by main thread
{
	CStdLock ResIDLock(&ResIDCSec);
	assert(iNextResID >= (iClientID << 16));
	if (iNextResID >= ((iClientID+1) << 16) - 1)
		iNextResID = std::max<int32_t>(0, iClientID) << 16;
	// find free
	while (getRes(iNextResID))
		iNextResID++;
	return iNextResID++;
}

C4Network2Res *C4Network2ResList::getRes(int32_t iResID)
{
	CStdShareLock ResListLock(&ResListCSec);
	for (C4Network2Res *pCur = pFirst; pCur; pCur = pCur->pNext)
		if (pCur->getResID() == iResID)
			return pCur;
	return nullptr;
}

C4Network2Res *C4Network2ResList::getRes(const char *szFile, bool fLocalOnly)
{
	CStdShareLock ResListLock(&ResListCSec);
	for (C4Network2Res *pCur = pFirst; pCur; pCur = pCur->pNext)
		if (!pCur->isAnonymous())
			if (SEqual(pCur->getFile(), szFile))
				if (!fLocalOnly || pCur->getResClient()==iClientID)
					return pCur;
	return nullptr;
}

C4Network2Res::Ref C4Network2ResList::getRefRes(int32_t iResID)
{
	CStdShareLock ResListLock(&ResListCSec);
	return getRes(iResID);
}

C4Network2Res::Ref C4Network2ResList::getRefRes(const char *szFile, bool fLocalOnly)
{
	CStdShareLock ResListLock(&ResListCSec);
	return getRes(szFile, fLocalOnly);
}

C4Network2Res::Ref C4Network2ResList::getRefNextRes(int32_t iResID)
{
	CStdShareLock ResListLock(&ResListCSec);
	C4Network2Res *pRes = nullptr;
	for (C4Network2Res *pCur = pFirst; pCur; pCur = pCur->pNext)
		if (!pCur->isRemoved() && pCur->getResID() >= iResID)
			if (!pRes || pRes->getResID() > pCur->getResID())
				pRes = pCur;
	return pRes;
}

void C4Network2ResList::Add(C4Network2Res *pRes)
{
	// get locks
	CStdShareLock ResListLock(&ResListCSec);
	CStdLock ResListAddLock(&ResListAddCSec);
	// reference
	pRes->AddRef();
	// add
	pRes->pNext = pFirst;
	pFirst = pRes;
}

C4Network2Res::Ref C4Network2ResList::AddByFile(const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName, bool fAllowUnloadable)
{
	// already in list?
	C4Network2Res::Ref pRes = getRefRes(strFilePath);
	if (pRes) return pRes;
	// get resource ID
	if (iResID < 0) iResID = nextResID();
	if (iResID < 0) { Log("AddByFile: no more resource IDs available!"); return nullptr; }
	// create new
	pRes = new C4Network2Res(this);
	// initialize
	if (!pRes->SetByFile(strFilePath, fTemp, eType, iResID, szResName)) { return nullptr; }
	// create standalone for non-system files
	// system files shouldn't create a standalone; they should never be marked loadable!
	if (eType != NRT_System)
		if (!pRes->GetStandalone(nullptr, 0, true, fAllowUnloadable))
			if (!fAllowUnloadable)
			{
				delete pRes;
				return nullptr;
			}
	// add to list
	Add(pRes);
	return pRes;
}

C4Network2Res::Ref C4Network2ResList::AddByGroup(C4Group *pGrp, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName, bool fAllowUnloadable)
{
	// get resource ID
	if (iResID < 0) iResID = nextResID();
	if (iResID < 0) { Log("AddByGroup: no more resource IDs available!"); return nullptr; }
	// create new
	C4Network2Res::Ref pRes = new C4Network2Res(this);
	// initialize
	if (!pRes->SetByGroup(pGrp, fTemp, eType, iResID, szResName))
	{
		delete pRes;
		return nullptr;
	}
	// create standalone
	if (!pRes->GetStandalone(nullptr, 0, true, fAllowUnloadable))
		if (!fAllowUnloadable)
		{
			delete pRes;
			return nullptr;
		}
	// add to list
	Add(pRes);
	return pRes;
}

C4Network2Res::Ref C4Network2ResList::AddByCore(const C4Network2ResCore &Core, bool fLoad) // by main thread
{
	// already in list?
	C4Network2Res::Ref pRes = getRefRes(Core.getID());
	if (pRes) return pRes;
#ifdef C4NET2RES_LOAD_ALL
	// load without check (if possible)
	if (Core.isLoadable()) return AddLoad(Core);
#endif
	// create new
	pRes = new C4Network2Res(this);
	// try set by core
	if (!pRes->SetByCore(Core, true))
	{
		pRes.Clear();
		// try load (if specified)
		return fLoad ? AddLoad(Core) : nullptr;
	}
	// log
	Application.InteractiveThread.ThreadLogS("Network: Found identical %s. Not loading.", pRes->getCore().getFileName());
	// add to list
	Add(pRes);
	// ok
	return pRes;
}

C4Network2Res::Ref C4Network2ResList::AddLoad(const C4Network2ResCore &Core) // by main thread
{
	// marked unloadable by creator?
	if (!Core.isLoadable())
	{
		// show error msg
		Application.InteractiveThread.ThreadLog("Network: Cannot load %s (marked unloadable)", Core.getFileName());
		return nullptr;
	}
	// create new
	C4Network2Res::Ref pRes = new C4Network2Res(this);
	// initialize
	pRes->SetLoad(Core);
	// log
	Application.InteractiveThread.ThreadLogS("Network: loading %s...", Core.getFileName());
	// add to list
	Add(pRes);
	return pRes;
}

void C4Network2ResList::RemoveAtClient(int32_t iClientID) // by main thread
{
	CStdShareLock ResListLock(&ResListCSec);
	for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
		if (pRes->getResClient() == iClientID)
			pRes->Remove();
}

void C4Network2ResList::Clear()
{
	CStdShareLock ResListLock(&ResListCSec);
	for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
	{
		pRes->Remove();
		pRes->iLastReqTime = 0;
	}
	iClientID = C4ClientIDUnknown;
	iLastDiscover = iLastStatus = 0;
}

void C4Network2ResList::OnClientConnect(C4Network2IOConnection *pConn) // by main thread
{
	// discover resources
	SendDiscover(pConn);
}

template<class T>
const T& GetPkt(const C4PacketBase *pPacket) {
	// Wish we had templated lambdas yet
	assert(pPacket);
	return static_cast<const T&>(*pPacket);
}

void C4Network2ResList::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn)
{
	// security
	if (!pConn) return;

	switch (cStatus)
	{

	case PID_NetResDis: // resource discover
	{
		if (!pConn->isOpen()) break;
		auto Pkt = GetPkt<C4PacketResDiscover>(pPacket);
		// search matching resources
		CStdShareLock ResListLock(&ResListCSec);
		for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
			if (Pkt.isIDPresent(pRes->getResID()))
				// must be binary compatible
				if (pRes->IsBinaryCompatible())
					pRes->OnDiscover(pConn);
	}
	break;

	case PID_NetResStat: // resource status
	{
		if (!pConn->isOpen()) break;
		auto Pkt = GetPkt<C4PacketResStatus>(pPacket);
		// matching resource?
		CStdShareLock ResListLock(&ResListCSec);
		C4Network2Res *pRes = getRes(Pkt.getResID());
		// present / being loaded? call handler
		if (pRes)
			pRes->OnStatus(Pkt.getChunks(), pConn);
	}
	break;

	case PID_NetResDerive: // resource derive
	{
		auto Core = GetPkt<C4Network2ResCore>(pPacket);
		if (Core.getDerID() < 0) break;
		// Check if there is a anonymous derived resource with matching parent.
		CStdShareLock ResListLock(&ResListCSec);
		for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
			if (pRes->isAnonymous() && pRes->getCore().getDerID() == Core.getDerID())
				pRes->FinishDerive(Core);
	}
	break;

	case PID_NetResReq: // resource request
	{
		auto Pkt = GetPkt<C4PacketResRequest>(pPacket);
		// find resource
		CStdShareLock ResListLock(&ResListCSec);
		C4Network2Res *pRes = getRes(Pkt.getReqID());
		// send requested chunk
		if (pRes && pRes->IsBinaryCompatible()) pRes->SendChunk(Pkt.getReqChunk(), pConn->getClientID());
	}
	break;

	case PID_NetResData: // a chunk of data is coming in
	{
		auto Chunk = GetPkt<C4Network2ResChunk>(pPacket);
		// find resource
		CStdShareLock ResListLock(&ResListCSec);
		C4Network2Res *pRes = getRes(Chunk.getResID());
		// send requested chunk
		if (pRes) pRes->OnChunk(Chunk);
	}
	break;
	}
}

void C4Network2ResList::OnTimer()
{
	CStdShareLock ResListLock(&ResListCSec);
	C4Network2Res *pRes;
	// do loads, check timeouts
	for (pRes = pFirst; pRes; pRes = pRes->pNext)
		if (pRes->isLoading() && !pRes->isRemoved())
			if (!pRes->DoLoad())
				pRes->Remove();
	// discovery time?
	if (!iLastDiscover || difftime(time(nullptr), iLastDiscover) >= C4NetResDiscoverInterval)
	{
		// needed?
		bool fSendDiscover = false;
		for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
			if (pRes->isLoading() && !pRes->isRemoved())
				fSendDiscover |= pRes->NeedsDiscover();
		// send
		if (fSendDiscover)
			SendDiscover();
	}
	// status update?
	if (!iLastStatus || difftime(time(nullptr), iLastStatus) >= C4NetResStatusInterval)
	{
		// any?
		bool fStatusUpdates = false;
		for (pRes = pFirst; pRes; pRes = pRes->pNext)
			if (pRes->isDirty() && !pRes->isRemoved())
				fStatusUpdates |= pRes->SendStatus();
		// set time accordingly
		iLastStatus = fStatusUpdates ? time(nullptr) : 0;
	}
}

void C4Network2ResList::OnShareFree(CStdCSecEx *pCSec)
{
	if (pCSec == &ResListCSec)
	{
		// remove entries
		for (C4Network2Res *pRes = pFirst, *pNext, *pPrev = nullptr; pRes; pRes = pNext)
		{
			pNext = pRes->pNext;
			if (pRes->isRemoved() && (!pRes->getLastReqTime() || difftime(time(nullptr), pRes->getLastReqTime()) > C4NetResDeleteTime))
			{
				// unlink
				(pPrev ? pPrev->pNext : pFirst) = pNext;
				// remove
				pRes->pNext = nullptr;
				pRes->DelRef();
			}
			else
				pPrev = pRes;
		}
	}
}

bool C4Network2ResList::SendDiscover(C4Network2IOConnection *pTo) // by both
{
	// make packet
	C4PacketResDiscover Pkt;
	// add special retrieves
	CStdShareLock ResListLock(&ResListCSec);
	for (C4Network2Res *pRes = pFirst; pRes; pRes = pRes->pNext)
		if (!pRes->isRemoved())
			if (pRes->isLoading())
				Pkt.AddDisID(pRes->getResID());
	ResListLock.Clear();
	// empty?
	if (!Pkt.getDisIDCnt()) return false;
	// broadcast?
	if (!pTo)
	{
		// save time
		iLastDiscover = time(nullptr);
		// send
		return pIO->BroadcastMsg(MkC4NetIOPacket(PID_NetResDis, Pkt));
	}
	else
		return pTo->Send(MkC4NetIOPacket(PID_NetResDis, Pkt));
}

void C4Network2ResList::OnResComplete(C4Network2Res *pRes)
{
	// log (network thread -> ThreadLog)
	Application.InteractiveThread.ThreadLogS("Network: %s received.", pRes->getCore().getFileName());
	// call handler (ctrl might wait for this resource)
	::Control.Network.OnResComplete(pRes);
}

bool C4Network2ResList::CreateNetworkFolder()
{
	// get network path without trailing backslash
	char szNetworkPath[_MAX_PATH_LEN];
	SCopy(Config.AtNetworkPath(""), szNetworkPath, _MAX_PATH);
	TruncateBackslash(szNetworkPath);
	// but make sure that the configured path has one
	AppendBackslash(Config.Network.WorkPath);
	// does not exist?
	if (!DirectoryExists(szNetworkPath))
	{
		if (!CreatePath(szNetworkPath))
			{ LogFatal("Network: could not create network path!"); return false; }
		return true;
	}
	return true;
}

bool C4Network2ResList::FindTempResFileName(const char *szFilename, char *pTarget)
{
	char safeFilename[_MAX_PATH];
	char* safePos = safeFilename;
	while (*szFilename)
	{
		if ((*szFilename >= 'a' && *szFilename <= 'z') ||
		    (*szFilename >= 'A' && *szFilename <= 'Z') ||
		    (*szFilename >= '0' && *szFilename <= '9') ||
		    (*szFilename == '.') || (*szFilename == '/'))
			*safePos = *szFilename;
		else
			*safePos = '_';

		++safePos;
		++szFilename;
	}
	*safePos = 0;
	szFilename = safeFilename;

	// create temporary file
	SCopy(Config.AtNetworkPath(GetFilename(szFilename)), pTarget, _MAX_PATH);
	// file name is free?
	if (!ItemExists(pTarget)) return true;
	// find another file name
	char szFileMask[_MAX_PATH_LEN];
	SCopy(pTarget, szFileMask, GetExtension(pTarget)-1-pTarget);
	SAppend("_%d", szFileMask, _MAX_PATH);
	SAppend(GetExtension(pTarget)-1, szFileMask, _MAX_PATH);
	for (int32_t i = 2; i < 1000; i++)
	{
		snprintf(pTarget, _MAX_PATH, szFileMask, i);
		// doesn't exist?
		if (!ItemExists(pTarget))
			return true;
	}
	// not found
	return false;
}
