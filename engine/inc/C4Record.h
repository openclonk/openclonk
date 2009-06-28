/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2002, 2004-2007  Sven Eberhardt
 * Copyright (c) 2004-2006, 2008  Peter Wortmann
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
// scenario record functionality

#ifndef INC_C4Record
#define INC_C4Record

class C4Record;

#include "C4Group.h"
#include "C4Control.h"

#ifdef DEBUGREC
	extern int DoNoDebugRec; // debugrec disable counter in C4Record.cpp

	#define DEBUGREC_OFF ++DoNoDebugRec;
	#define DEBUGREC_ON --DoNoDebugRec;

#else
	#define DEBUGREC_OFF
	#define DEBUGREC_ON
#endif

// turn off debugrecs in current block
class C4DebugRecOff
	{
	bool fDoOff;

	public:
		C4DebugRecOff();
		C4DebugRecOff(bool fDoOff);
		~C4DebugRecOff();

		void Clear();
	};

enum C4RecordChunkType // record file chunk type
	{
	RCT_Ctrl   = 0x00,  // control
	RCT_CtrlPkt= 0x01,  // control packet
	RCT_Frame  = 0x02,  // beginning frame
	RCT_End    = 0x10,	// --- the end ---
	RCT_Log    = 0x20,  // log message
	// Streaming
	RCT_File   = 0x30,  // file data
	// DEBUGREC
	RCT_Block   = 0x82,  // point in Game::Execute
	RCT_SetPix  = 0x83,  // set landscape pixel
	RCT_ExecObj = 0x84,  // exec object
	RCT_Random  = 0x85,  // Random()-call
	RCT_Rn3     = 0x86,	 // Rn3()-call
	RCT_MMC     = 0x87,  // create MassMover
	RCT_MMD     = 0x88,  // destroy MassMover
	RCT_CrObj   = 0x89,  // create object
	RCT_DsObj   = 0x8A,  // remove object
	RCT_GetPix  = 0x8B,  // get landscape pixel; let the Gigas flow!
	RCT_RotVtx1 = 0x8C,  // before shape is rotated
	RCT_RotVtx2 = 0x8D,  // after shape is rotated
	RCT_ExecPXS = 0x8E,  // execute pxs system
	RCT_Sin     = 0x8F,  // sin by Shape-Rotation
	RCT_Cos     = 0x90,  // cos by Shape-Rotation
	RCT_Map     = 0x91,  // map dump
	RCT_Ls      = 0x92,  // complete landscape dump!
	RCT_MCT1    = 0x93,  // MapCreatorS2: before transformation
	RCT_MCT2    = 0x94,  // MapCreatorS2: after transformation
	RCT_AulFunc = 0x9A,  // script function call
	RCT_ObjCom  = 0x9B,  // object com
	RCT_PlrCom  = 0x9C,  // player com
	RCT_PlrInCom= 0x9D,  // player InCom
	RCT_MatScan = 0x9E,  // landscape scan execute
	RCT_MatScanDo= 0x9F,  // landscape scan mat change
	RCT_Area    = 0xA0,  // object area change
	RCT_MenuAdd  = 0xA1,  // add menu item
	RCT_MenuAddC = 0xA2,  // add menu item: Following commands
	RCT_OCF      = 0xA3,  // OCF setting of updating
	RCT_DirectExec = 0xA4,  // a DirectExec-script

	RCT_Custom  = 0xc0, // varies

	RCT_Undefined = 0xff,
	};

#ifdef DEBUGREC
	void AddDbgRec(C4RecordChunkType eType, const void *pData=NULL, int iSize=0); // record debug stuff
#endif

#pragma pack(1)

struct C4RecordChunkHead // record file chunk head
	{
	uint8_t iFrm; // frame
	uint8_t Type; // chunk type
	};

struct C4RecordChunk
	{
	int32_t Frame;
	uint8_t Type;
	union
		{
		C4Control *pCtrl;
		C4IDPacket *pPkt;
		class C4PktDebugRec *pDbg;
		class StdBuf *pFileData;
		};
	StdCopyStrBuf Filename; // RCT_File only
public:
	C4RecordChunk();
	void Delete();
	virtual void CompileFunc(StdCompiler *pComp);
	virtual ~C4RecordChunk() {}
	};

struct C4RCSetPix
	{
	int x,y; // pos
	BYTE clr; // new color
	};

struct C4RCExecObj
	{
	int Number; // object number
	int id;
	FIXED fx,fy,fr;
	};

struct C4RCMassMover
	{
	int x,y; // pos
	};

struct C4RCRandom
	{
	int Cnt; // index in seed
	int Range; // random range query
	int Val; // random value
	};

struct C4RCCreateObj
	{
	int oei;
	int id;
	int x,y,ownr;
	};

struct C4RCSin
	{
	// value+return value
	double v,r;
	};

struct C4RCRotVtx
	{
	// shape size
	int x,y,wdt,hgt,r;
	// vertices
	int VtxX[4], VtxY[4];
	};

struct C4RCExecPXS
	{
	// pos
	FIXED	x,y;
	// mat
	int32_t iMat;
	// execution pos
	int32_t pos;
	};

struct C4RCTrf
	{
	int x,y,Turbulence,Rotate;
	};

struct C4RCPos
	{
	int x,y;
	};

struct C4RCObjectCom
	{
	BYTE com;
	int32_t data;
	int32_t o;
	};

struct C4RCMatScan
	{
	int32_t cx, cy, mat, conv_to, dir, mconvs;
	};

struct C4RCArea
	{
	char op;
	int32_t obj;
	int32_t x1,y1, xL, yL, dpitch;
	bool out;
	};

struct C4RCMenuAdd
	{
	int32_t iObjNum;
	int32_t iCount;
	C4ID idID;
	bool fOwnValue;
	int32_t iValue;
	bool fIsSelectable;
	};

struct C4RCOCF
	{
	uint32_t dwOCFOld;
	uint32_t dwOCFNew;
	bool fUpdate;
	};

#pragma pack()

// debug record packet
class C4PktDebugRec : public C4PktBuf
	{
	protected:
		C4RecordChunkType eType;
	public:
		C4PktDebugRec() : C4PktBuf(), eType(RCT_Undefined) {}
		C4PktDebugRec(const C4PktDebugRec &rCopy) : C4PktBuf(rCopy), eType(rCopy.eType) {}
		C4PktDebugRec(C4RecordChunkType eType, const StdBuf &rCpyData)
			: C4PktBuf(rCpyData), eType(eType) {}

		C4RecordChunkType getType() const { return eType; }

	virtual void CompileFunc(StdCompiler *pComp);
	ALLOW_TEMP_TO_REF(C4PktDebugRec);
	};

class C4Record // demo recording
	{
	private:
		CStdFile CtrlRec; // control file handle
		StdStrBuf sFilename; // recorded scenario file name
		C4Group RecordGrp; // record scenario group
		bool fRecording; // set if recording is active
		uint32_t iLastFrame; // frame of last chunk written
		bool fStreaming; // perdiodically sent new control to server
		unsigned int iStreamingPos; // Position of current buffer in stream
		StdBuf StreamingData; // accumulated control data since last stream sync
	public:
		C4Record(); // constructor; creates control file etc
		C4Record(const char *szPlaybackFile, const char *szRecordFile, const char *szTempRecFile); // start recording from replay into record
		~C4Record(); // destructor; close file; create demo scen
		int Index;

		bool IsRecording() const { return fRecording; } // return whether Start() has been called
		unsigned int GetStreamingPos() const { return iStreamingPos; }
		const StdBuf &GetStreamingBuf() const { return StreamingData; }

		BOOL Start(bool fInitial);
    BOOL Stop(StdStrBuf *pRecordName = NULL, BYTE *pRecordSHA1 = NULL);

    bool Rec(const C4Control &Ctrl, int iFrame); // record control
    bool Rec(C4PacketType eCtrlType, C4ControlPacket *pCtrl, int iFrame); // record control packet
    bool Rec(int iFrame, const StdBuf &sBuf, C4RecordChunkType eType);

		bool AddFile(const char *szLocalFilename, const char *szAddAs, bool fDelete = false);

		bool StartStreaming(bool fInitial);
		void ClearStreamingBuf(unsigned int iAmount);
		void StopStreaming();

	private:
		void Stream(const C4RecordChunkHead &Head, const StdBuf &sBuf);
		bool StreamFile(const char *szFilename, const char *szAddAs);
	};

class C4Playback // demo playback
	{
	private:
		typedef std::list<C4RecordChunk> chunks_t;
		chunks_t chunks;
		chunks_t::iterator currChunk;
		bool Finished;    // if set, free playback in next frame
		CStdFile playbackFile; // if open, try reading additional chunks from this file
		bool fLoadSequential;  // used for debugrecs: Sequential reading of files
		StdBuf sequentialBuffer; // buffer to manage sequential reads
		uint32_t iLastSequentialFrame; // frame number of last chunk read
		void Finish(); // end playback
#ifdef DEBUGREC
    C4PacketList DebugRec;
#endif
	public:
		C4Playback(); // constructor; init playback
		~C4Playback(); // destructor; deinit playback

		BOOL Open(C4Group &rGrp);
		BOOL ReadBinary(const StdBuf &Buf);
		BOOL ReadText(const StdStrBuf &Buf);
		void NextChunk(); // point to next prepared chunk in mem or read it
		bool NextSequentialChunk(); // read from seq file until a new chunk has been filled
		StdStrBuf ReWriteText();
		StdBuf ReWriteBinary();
		void Strip();
		BOOL ExecuteControl(C4Control *pCtrl, int iFrame); // assign control
		BOOL IsFinished() { return Finished; }
		void Clear();
#ifdef DEBUGREC
		void Check(C4RecordChunkType eType, const uint8_t *pData, int iSize); // compare with debugrec
    void DebugRecError(const char *szError);
#endif
		static bool StreamToRecord(const char *szStream, StdStrBuf *pRecord);
	};

#endif
