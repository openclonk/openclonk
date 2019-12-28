/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// network resource: data needed for the game (scenario, plr files, definitions...)

#ifndef INC_C4Network2Res
#define INC_C4Network2Res

#include "lib/StdAdaptors.h"
#include "platform/StdSync.h"

#include "lib/SHA1.h"

#include <atomic>

const uint32_t C4NetResChunkSize = 10U * 1024U;

const int32_t C4NetResDiscoverTimeout = 10, // (s)
              C4NetResDiscoverInterval = 1, // (s)
              C4NetResStatusInterval = 1, // (s)
              C4NetResMaxLoad = 5,
              C4NetResLoadTimeout = 60, // (s)
              C4NetResDeleteTime = 60, // (s)
              C4NetResMaxBigicon = 20; // maximum size, in KB, of bigicon

const int32_t C4NetResIDAnonymous = -2;

enum C4Network2ResType
{
	NRT_Null=0,
	NRT_Scenario,
	NRT_Dynamic,
	NRT_Player,
	NRT_Definitions,
	NRT_System,
	NRT_Material
};

const StdEnumEntry<C4Network2ResType> C4Network2ResType_EnumMap[] =
{
	{ "Scenario", NRT_Scenario },
	{ "Dynamic", NRT_Dynamic },
	{ "Player", NRT_Player },
	{ "Definitions", NRT_Definitions },
	{ "System", NRT_System },
	{ "Material", NRT_Material },
};

// damn circular dependencies
#include "network/C4PacketBase.h"
#include "network/C4Network2IO.h"
class C4Network2ResList;
class C4Network2ResChunk;

// classes
class C4Network2ResCore : public C4PacketBase
{
public:
	C4Network2ResCore();

protected:
	C4Network2ResType eType{NRT_Null};
	int32_t iID{-1}, iDerID{-1};
	StdCopyStrBuf FileName;
	bool fLoadable{false};
	uint32_t iFileSize, iFileCRC, iContentsCRC;
	uint8_t fHasFileSHA{false};
	uint8_t FileSHA[SHA_DIGEST_LENGTH];
	uint32_t iChunkSize;

public:
	C4Network2ResType getType()   const { return eType; }
	bool          isNull()        const { return eType == NRT_Null; }
	int32_t       getID()         const { return iID; }
	int32_t       getDerID()      const { return iDerID; }
	bool          isLoadable()    const { return fLoadable; }
	uint32_t      getFileSize()   const { return iFileSize; }
	uint32_t      getFileCRC()    const { return iFileCRC; }
	uint32_t      getContentsCRC()const { return iContentsCRC; }
	bool          hasFileSHA()    const { return !!fHasFileSHA; }
	const uint8_t*getFileSHA()    const { return FileSHA; }
	const char *  getFileName()   const { return FileName.getData(); }
	uint32_t      getChunkSize()  const { return iChunkSize; }
	uint32_t      getChunkCnt()   const { return iFileSize && iChunkSize ? (iFileSize - 1) / iChunkSize + 1 : 0; }

	void Set(C4Network2ResType eType, int32_t iResID, const char *strFileName, uint32_t iContentsCRC);
	void SetID(int32_t inID)            { iID = inID; }
	void SetDerived(int32_t inDerID)    { iDerID = inDerID; }
	void SetLoadable(uint32_t iSize, uint32_t iCRC);
	void SetFileSHA(BYTE *pSHA)         { memcpy(FileSHA, pSHA, SHA_DIGEST_LENGTH); fHasFileSHA = true; }
	void Clear();

	void CompileFunc(StdCompiler *pComp) override;

};

class C4Network2ResLoad
{
	friend class C4Network2Res;
public:
	C4Network2ResLoad(int32_t iChunk, int32_t iByClient);
	~C4Network2ResLoad();

protected:
	// chunk download data
	int32_t iChunk;
	time_t Timestamp;
	int32_t iByClient;

	// list (C4Network2Res)
	C4Network2ResLoad *pNext;

public:
	int32_t     getChunk()        const { return iChunk; }
	int32_t     getByClient()     const { return iByClient; }

	C4Network2ResLoad *Next() const { return pNext; }

	bool CheckTimeout();

};

class C4Network2ResChunkData : public C4PacketBase
{
public:
	C4Network2ResChunkData();
	C4Network2ResChunkData(const C4Network2ResChunkData &Data2);
	~C4Network2ResChunkData() override;

	C4Network2ResChunkData &operator =(const C4Network2ResChunkData &Data2);
protected:
	int32_t iChunkCnt{0}, iPresentChunkCnt{0};

	// present chunk ranges
	struct ChunkRange { int32_t Start, Length; ChunkRange *Next; };
	ChunkRange *pChunkRanges{nullptr};
	int32_t iChunkRangeCnt{0};

public:
	int32_t getChunkCnt()         const { return iChunkCnt; }
	int32_t getPresentChunkCnt()  const { return iPresentChunkCnt; }
	int32_t getPresentPercent()   const { return iPresentChunkCnt * 100 / iChunkCnt; }
	bool isComplete()         const { return iPresentChunkCnt == iChunkCnt; }

	void SetIncomplete(int32_t iChunkCnt);
	void SetComplete(int32_t iChunkCnt);

	void AddChunk(int32_t iChunk);
	void AddChunkRange(int32_t iStart, int32_t iLength);
	void Merge(const C4Network2ResChunkData &Data2);

	void Clear();

	int32_t GetChunkToRetrieve(const C4Network2ResChunkData &Available, int32_t iLoadingCnt, int32_t *pLoading) const;

protected:
	// helpers
	bool MergeRanges(ChunkRange *pRange);
	void GetNegative(C4Network2ResChunkData &Target) const;
	int32_t getPresentChunk(int32_t iNr) const;

public:
	void CompileFunc(StdCompiler *pComp) override;
};

class C4Network2Res
{
	friend class C4Network2ResList;
	friend class C4Network2ResChunk;
public:

	// helper for reference-holding
	class Ref
	{
	public:
		Ref() = default;
		Ref(C4Network2Res *pRes) : pRes(pRes) { if (pRes) pRes->AddRef(); }
		Ref(const Ref &rCopy) : pRes(rCopy.pRes) { if (pRes) pRes->AddRef(); }
		~Ref() { Clear(); }
		Ref &operator = (C4Network2Res *pnRes) { Set(pnRes); return *this; }
		Ref &operator = (const Ref &rCopy) { Set(rCopy.pRes); return *this; }
	private:
		C4Network2Res *pRes{nullptr};
	public:
		operator C4Network2Res *() const { return pRes; }
		bool operator ! () const { return !pRes; }
		C4Network2Res * operator ->() const { return pRes; }
		void Clear() { if (pRes) pRes->DelRef(); pRes = nullptr; }
		void Set(C4Network2Res *pnRes) { if (pRes == pnRes) return; Clear(); pRes = pnRes; if (pRes) pRes->AddRef(); }
	};

	C4Network2Res(C4Network2ResList *pnParent);
	~C4Network2Res();

protected:
	// core, chunk data
	C4Network2ResCore Core;
	C4Network2ResChunkData Chunks; // (only valid while loading)
	bool fDirty;

	// local file data
	CStdCSec FileCSec;
	char szFile[_MAX_PATH_LEN], szStandalone[_MAX_PATH_LEN];
	bool fTempFile, fStandaloneFailed;

	// references
	std::atomic_long iRefCnt;
	bool fRemoved;

	// being load?
	int32_t iLastReqTime;

	// loading
	bool fLoading;
	struct ClientChunks { C4Network2ResChunkData Chunks; int32_t ClientID; ClientChunks *Next; }
	*pCChunks;
	time_t iDiscoverStartTime;
	C4Network2ResLoad *pLoads;
	int32_t iLoadCnt;

	// list (C4Network2ResList)
	C4Network2Res *pNext;
	C4Network2ResList *pParent;

public:
	C4Network2ResType getType() const { return Core.getType(); }
	const C4Network2ResCore &getCore() const { return Core; }
	bool        isDirty()       const { return fDirty; }
	bool        isAnonymous()   const { return getResID() == C4NetResIDAnonymous; }
	int32_t     getResID()      const { return Core.getID(); }
	int32_t     getResClient()  const { return Core.getID() >> 16; }
	const char *getFile()       const { return szFile; }
	CStdCSec   *getFileCSec()         { return &FileCSec; }
	int32_t     getLastReqTime()const { return iLastReqTime; }
	bool        isRemoved()     const { return fRemoved; }
	bool        isLoading()     const { return fLoading; }
	bool        isComplete()    const { return !fLoading; }
	int32_t     getPresentPercent() const { return fLoading ? Chunks.getPresentPercent() : 100; }
	bool        isTempFile()    const { return fTempFile; }

	bool SetByFile(const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName = nullptr, bool fSilent = false);
	bool SetByGroup(C4Group *pGrp, bool fTemp, C4Network2ResType eType, int32_t iResID, const char *szResName = nullptr, bool fSilent = false);
	bool SetByCore(const C4Network2ResCore &nCore, bool fSilent = false, const char *szAsFilename = nullptr, int32_t iRecursion=0);
	bool SetLoad(const C4Network2ResCore &nCore);

	bool SetDerived(const char *strName, const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iDResID);

	void ChangeID(int32_t inID);

	bool IsBinaryCompatible();
	bool GetStandalone(char *pTo, int32_t iMaxL, bool fSetOfficial, bool fAllowUnloadable = false, bool fSilent = false);
	bool CalculateSHA();

	bool SaveBackFile();
	C4Network2Res::Ref Derive();
	bool FinishDerive();
	bool FinishDerive(const C4Network2ResCore &nCore);

	bool SendStatus(C4Network2IOConnection *pTo = nullptr);
	bool SendChunk(uint32_t iChunk, int32_t iToClient);

	// references
	void AddRef(); void DelRef();

	// events
	void OnDiscover(C4Network2IOConnection *pBy);
	void OnStatus(const C4Network2ResChunkData &rChunkData, C4Network2IOConnection *pBy);
	void OnChunk(const C4Network2ResChunk &rChunk);
	bool DoLoad();

	bool NeedsDiscover();

	C4Group *OpenAsGrp() const;

	void Remove();
	void Clear();

protected:
	int32_t OpenFileRead(); int32_t OpenFileWrite();

	void StartNewLoads();
	bool StartLoad(int32_t iFromClient, const C4Network2ResChunkData &Chunks);
	void EndLoad();
	void ClearLoad();

	void RemoveLoad(C4Network2ResLoad *pLoad);
	void RemoveCChunks(ClientChunks *pChunks);

	bool OptimizeStandalone(bool fSilent);

};

class C4Network2ResChunk : public C4PacketBase
{
public:
	C4Network2ResChunk();
	~C4Network2ResChunk() override;

protected:
	int32_t iResID;
	uint32_t iChunk;
	StdBuf Data;

public:
	int32_t   getResID()   const { return iResID; }
	uint32_t  getChunkNr() const { return iChunk; }

	bool Set(C4Network2Res *pRes, uint32_t iChunk);
	bool AddTo(C4Network2Res *pRes, C4Network2IO *pIO) const;

	void CompileFunc(StdCompiler *pComp) override;
};

class C4Network2ResList : protected CStdCSecExCallback // run by network thread
{
	friend class C4Network2Res;
	friend class C4Network2;
public:
	C4Network2ResList();
	~C4Network2ResList() override;

protected:

	C4Network2Res *pFirst{nullptr};
	CStdCSecEx ResListCSec;
	CStdCSec ResListAddCSec;

	int32_t iClientID{-1}, iNextResID;
	CStdCSec ResIDCSec;

	// timings
	int32_t iLastDiscover{0}, iLastStatus{0};

	// object used for network i/o
	C4Network2IO *pIO{nullptr};

public:

	// initialization
	bool Init(int32_t iClientID, C4Network2IO *pIOClass); // by main thread
	void SetLocalID(int32_t iClientID); // by both

protected:
	int32_t nextResID(); // by main thread

	C4Network2Res *getRes(int32_t iResID); // by both
	C4Network2Res *getRes(const char *szFile, bool fLocalOnly); // by both

public:
	// returns referenced resource ptrs
	C4Network2Res::Ref getRefRes(int32_t iResID); // by both
	C4Network2Res::Ref getRefRes(const char *szFile, bool fLocalOnly = false); // by both
	C4Network2Res::Ref getRefNextRes(int32_t iResID); // by both

	void Add(C4Network2Res *pRes); // by both
	C4Network2Res::Ref AddByFile(const char *strFilePath, bool fTemp, C4Network2ResType eType, int32_t iResID = -1, const char *szResName = nullptr, bool fAllowUnloadable = false); // by both
	C4Network2Res::Ref AddByGroup(C4Group *pGrp, bool fTemp, C4Network2ResType eType, int32_t iResID = -1, const char *szResName = nullptr, bool fAllowUnloadable = false); // by both
	C4Network2Res::Ref AddByCore(const C4Network2ResCore &Core, bool fLoad = true); // by main thread
	C4Network2Res::Ref AddLoad(const C4Network2ResCore &Core); // by main thread

	void RemoveAtClient(int32_t iClientID); // by main thread
	void Clear(); // by main thread

	bool SendDiscover(C4Network2IOConnection *pTo = nullptr); // by both
	void OnClientConnect(C4Network2IOConnection *pConn); // by main thread

	// interface for C4Network2IO
	void HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn);
	void OnTimer();

	// CStdCSecExCallback
	void OnShareFree(CStdCSecEx *pCSec) override;

	// for C4Network2Res
	C4Network2IO *getIOClass() { return pIO; }

protected:
	void OnResComplete(C4Network2Res *pRes);

	// misc
	bool CreateNetworkFolder();
	bool FindTempResFileName(const char *szFilename, char *pTarget);

};

// * Packets *

class C4PacketResStatus : public C4PacketBase
{
public:
	C4PacketResStatus();
	C4PacketResStatus(int32_t iResID, const C4Network2ResChunkData &nChunks);

protected:
	int32_t iResID;
	C4Network2ResChunkData Chunks;

public:
	int32_t getResID() const { return iResID; }
	const C4Network2ResChunkData &getChunks() const { return Chunks; }

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketResDiscover : public C4PacketBase
{
public:
	C4PacketResDiscover();

protected:
	int32_t iDisIDs[16], iDisIDCnt{0};

public:
	int32_t getDisIDCnt()       const { return iDisIDCnt; }
	int32_t getDisID(int32_t i) const { return iDisIDs[i]; }
	bool isIDPresent(int32_t iID) const;

	bool AddDisID(int32_t iID);

	void CompileFunc(StdCompiler *pComp) override;
};

class C4PacketResRequest : public C4PacketBase
{
public:
	C4PacketResRequest(int32_t iID = -1, int32_t iChunk = -1);

protected:
	int32_t iReqID, iReqChunk;

public:
	int32_t getReqID()    const { return iReqID; }
	int32_t getReqChunk() const { return iReqChunk; }

	void CompileFunc(StdCompiler *pComp) override;
};

#endif // INC_C4Network2Res
