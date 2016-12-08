/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Control packets contain all player input in the message queue */

#ifndef INC_C4Control
#define INC_C4Control

#include "object/C4Id.h"
#include "network/C4PacketBase.h"
#include "control/C4PlayerInfo.h"
#include "network/C4Client.h"
#include "gui/C4KeyboardInput.h"
#include "object/C4ObjectList.h"

// *** control base classes

class C4ControlPacket : public C4PacketBase
{
public:
	C4ControlPacket();
	~C4ControlPacket();

protected:
	int32_t iByClient;

public:
	int32_t getByClient() const { return iByClient; }
	bool LocalControl() const;
	bool HostControl() const { return iByClient == C4ClientIDHost; }

	void SetByClient(int32_t iByClient);

	virtual bool PreExecute() const { return true; }
	virtual void Execute() const = 0;
	virtual void PreRec(C4Record *pRecord) { }

	// allowed in lobby (without dynamic loaded)?
	virtual bool Lobby() const { return false; }
	// allowed as direct/private control?
	virtual bool Sync() const { return true; }

	virtual void CompileFunc(StdCompiler *pComp);
};

#define DECLARE_C4CONTROL_VIRTUALS \
  virtual void Execute() const; \
  virtual void CompileFunc(StdCompiler *pComp);

class C4Control : public C4PacketBase
{
public:
	C4Control();
	~C4Control();

protected:
	C4PacketList Pkts;

public:

	void Clear();

	// packet list wrappers
	C4IDPacket *firstPkt() const { return Pkts.firstPkt(); }
	C4IDPacket *nextPkt(C4IDPacket *pPkt) const { return Pkts.nextPkt(pPkt); }

	void AddHead(C4PacketType eType, C4ControlPacket *pCtrl) { Pkts.AddHead(eType, pCtrl); }
	void Add(C4PacketType eType, C4ControlPacket *pCtrl)  { Pkts.Add(eType, pCtrl); }

	void Take(C4Control &Ctrl) { Pkts.Take(Ctrl.Pkts); }
	void Append(const C4Control &Ctrl) { Pkts.Append(Ctrl.Pkts); }
	void Copy(const C4Control &Ctrl) { Clear(); Pkts.Append(Ctrl.Pkts); }
	void Remove(C4IDPacket *pPkt) { Pkts.Remove(pPkt); }
	void Delete(C4IDPacket *pPkt) { Pkts.Delete(pPkt); }

	// control execution
	bool PreExecute() const;
	void Execute() const;
	void PreRec(C4Record *pRecord) const;

	virtual void CompileFunc(StdCompiler *pComp);
};

// *** control packets

enum C4CtrlValueType
{
	C4CVT_None = -1,
	C4CVT_ControlRate = 0,
	C4CVT_DisableDebug = 1,
	C4CVT_MaxPlayer = 2,
	C4CVT_TeamDistribution = 3,
	C4CVT_TeamColors = 4,
};

class C4ControlSet : public C4ControlPacket // sync, lobby
{
public:
	C4ControlSet()
			: eValType(C4CVT_None), iData(0)
	{ }
	C4ControlSet(C4CtrlValueType eValType, int32_t iData)
			: eValType(eValType), iData(iData)
	{ }
protected:
	C4CtrlValueType eValType;
	int32_t iData;
public:
	// C4CVT_TeamDistribution and C4CVT_TeamColors are lobby-packets
	virtual bool Lobby() const { return eValType == C4CVT_TeamDistribution || eValType == C4CVT_TeamColors; }

	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlScript : public C4ControlPacket // sync
{
public:
	enum { SCOPE_Console=-2, SCOPE_Global=-1 }; // special scopes to be passed as target objects

	C4ControlScript()
			: iTargetObj(-1), fUseVarsFromCallerContext(false)
	{ }
	C4ControlScript(const char *szScript, int32_t iTargetObj, bool fUseVarsFromCallerContext = false, bool editor_select_result = false)
			: iTargetObj(iTargetObj), fUseVarsFromCallerContext(fUseVarsFromCallerContext), Script(szScript, true), editor_select_result(editor_select_result)
	{ }
protected:
	int32_t iTargetObj;
	bool fUseVarsFromCallerContext;
	bool editor_select_result; // if true and executed script from local client in editor mode, select the object returned by this script
	StdStrBuf Script;
public:
	void SetTargetObj(int32_t iObj) { iTargetObj = iObj; }
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlMsgBoardReply : public C4ControlPacket // sync
{
public:
	C4ControlMsgBoardReply()
		: target(-1), player(NO_OWNER)
	{}
	C4ControlMsgBoardReply(const char *reply, int32_t target, int32_t player)
		: reply(reply), target(target), player(player)
	{}

private:
	StdCopyStrBuf reply;
	int32_t target;
	int32_t player;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlMsgBoardCmd : public C4ControlPacket // sync
{
public:
	C4ControlMsgBoardCmd()
		: player(NO_OWNER)
	{}
	C4ControlMsgBoardCmd(const char *command, const char *parameter, int32_t player)
		: command(command), parameter(parameter), player(player)
	{}

private:
	StdCopyStrBuf command;
	StdCopyStrBuf parameter;
	int32_t player;

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlPlayerSelect : public C4ControlPacket // sync
{
public:
	C4ControlPlayerSelect()
			: iPlr(-1), fIsAlt(false), iObjCnt(0), pObjNrs(nullptr) { }
	C4ControlPlayerSelect(int32_t iPlr, const C4ObjectList &Objs, bool fIsAlt);
	~C4ControlPlayerSelect() { delete[] pObjNrs; }
protected:
	int32_t iPlr;
	bool fIsAlt;
	int32_t iObjCnt;
	int32_t *pObjNrs;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlPlayerControl : public C4ControlPacket // sync
{
public:
	C4ControlPlayerControl() : iPlr(-1), state(C4PlayerControl::CONS_Down) {}
	C4ControlPlayerControl(int32_t iPlr, C4PlayerControl::ControlState state, const C4KeyEventData &rExtraData)
			: iPlr(iPlr), state(state), ExtraData(rExtraData) { }
	C4ControlPlayerControl(int32_t iPlr, int32_t iControl, int32_t iExtraData) // old-style menu com emulation
			: iPlr(iPlr), state(C4PlayerControl::CONS_Down), ExtraData(iExtraData,0,0,0,0) { AddControl(iControl,0); }

	struct ControlItem
	{
		int32_t iControl;
		int32_t iTriggerMode;
		ControlItem() : iControl(-1), iTriggerMode(0) {}
		ControlItem(int32_t iControl, int32_t iTriggerMode) : iControl(iControl), iTriggerMode(iTriggerMode) {}
		void CompileFunc(StdCompiler *pComp);
		bool operator ==(const struct ControlItem &cmp) const { return iControl==cmp.iControl && iTriggerMode == cmp.iTriggerMode; }
	};
	typedef std::vector<ControlItem> ControlItemVec;
protected:
	int32_t iPlr;
	int32_t state;
	C4KeyEventData ExtraData;
	ControlItemVec ControlItems;
public:
	DECLARE_C4CONTROL_VIRTUALS
	void AddControl(int32_t iControl, int32_t iTriggerMode)
	{ ControlItems.push_back(ControlItem(iControl, iTriggerMode)); }
	const ControlItemVec &GetControlItems() const { return ControlItems; }
	C4PlayerControl::ControlState GetState() const { return static_cast<C4PlayerControl::ControlState>(state); }
	const C4KeyEventData &GetExtraData() const { return ExtraData; }
	void SetExtraData(const C4KeyEventData &new_extra_data) { ExtraData = new_extra_data; }
};

class C4ControlPlayerMouse : public C4ControlPacket // sync
{
public:
	enum Action
	{
		CPM_NoAction = 0,

		CPM_Hover = 0x01,
		CPM_Drop = 0x02
	};

	C4ControlPlayerMouse() : action(CPM_NoAction), player(NO_OWNER), target_obj(0), drag_obj(0), old_obj(0) {}
	static C4ControlPlayerMouse *Hover(const C4Player *player, const C4Object *target, const C4Object *old_target, const C4Object *drag = nullptr);
	static C4ControlPlayerMouse *DragDrop(const C4Player *player, const C4Object *target, const C4Object *drag);

private:
	int32_t action;
	int32_t player;
	int32_t target_obj;
	int32_t drag_obj;
	int32_t old_obj;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlMenuCommand : public C4ControlPacket // sync
{
public:
	C4ControlMenuCommand()
			: menuID(0), subwindowID(0) { }
	C4ControlMenuCommand(int32_t actionID, int32_t player, int32_t menuID, int32_t subwindowID,
	                       C4Object *target, int32_t actionType);
protected:
	int32_t actionID, player, menuID, subwindowID, target, actionType;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlPlayerAction : public C4ControlPacket // sync
{
public:
	enum Action
	{
		CPA_NoAction = 0,

		CPA_Surrender = 0x01,
		CPA_ActivateGoal = 0x02,
		CPA_ActivateGoalMenu = 0x03,
		CPA_Eliminate = 0x04,

		CPA_SetHostility = 0x10,
		CPA_SetTeam = 0x11,
		
		CPA_InitScenarioPlayer = 0x20,
		CPA_InitPlayerControl = 0x21
	};

	C4ControlPlayerAction(const C4Player *source = nullptr);
	static C4ControlPlayerAction *Surrender(const C4Player *source);
	static C4ControlPlayerAction *Eliminate(const C4Player *source);
	static C4ControlPlayerAction *ActivateGoalMenu(const C4Player *source);
	static C4ControlPlayerAction *ActivateGoal(const C4Player *source, const C4Object *target);
	static C4ControlPlayerAction *SetHostility(const C4Player *source, const C4Player *target, bool hostile);
	static C4ControlPlayerAction *SetTeam(const C4Player *source, int32_t team);
	static C4ControlPlayerAction *InitScenarioPlayer(const C4Player *source, int32_t team);
	static C4ControlPlayerAction *InitPlayerControl(const C4Player *source, const C4PlayerControlAssignmentSet *ctrl_set = nullptr);

private:
	Action action;
	int32_t source;
	int32_t target;
	int32_t param_int;
	StdCopyStrBuf param_str;

	enum IpcParam
	{
		CPA_IPC_HasKeyboard = 1<<0,
		CPA_IPC_HasMouse = 1<<1,
		CPA_IPC_HasGamepad = 1<<2
	};

public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlSyncCheck : public C4ControlPacket // not sync
{
public:
	C4ControlSyncCheck();
protected:
	int32_t Frame;
	int32_t ControlTick;
	int32_t RandomCount;
	int32_t AllCrewPosX;
	int32_t PXSCount;
	int32_t MassMoverIndex;
	int32_t ObjectCount;
	int32_t ObjectEnumerationIndex;
	int32_t SectShapeSum;
public:
	void Set();
	int32_t getFrame() const { return Frame; }
	virtual bool Sync() const { return false; }
	DECLARE_C4CONTROL_VIRTUALS
protected:
	static int32_t GetAllCrewPosX();
};

class C4ControlSynchronize : public C4ControlPacket // sync
{
public:
	C4ControlSynchronize(bool fSavePlrFiles = false, bool fSyncClearance = false)
			: fSavePlrFiles(fSavePlrFiles), fSyncClearance(fSyncClearance)
	{ }
protected:
	bool fSavePlrFiles, fSyncClearance;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlClientJoin : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlClientJoin() { }
	C4ControlClientJoin(const C4ClientCore &Core) : Core(Core) { }
public:
	C4ClientCore Core;
public:
	virtual bool Sync() const { return false; }
	virtual bool Lobby() const { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlClientUpdType
{
	CUT_None = -1, CUT_Activate = 0, CUT_SetObserver = 1, CUT_SetReady = 2
};

class C4ControlClientUpdate : public C4ControlPacket // sync, lobby
{
public:
	C4ControlClientUpdate() : iID(0), eType(CUT_None), iData(0) { }
	C4ControlClientUpdate(int32_t iID, C4ControlClientUpdType eType, int32_t iData = 0)
			: iID(iID), eType(eType), iData(iData)
	{ }
private:
	static const int32_t MinReadyAnnouncementDelay = 1; // seconds that need to pass between ready-state announcements to prevent spam
public:
	int32_t iID;
	C4ControlClientUpdType eType;
	int32_t iData;
public:
	virtual bool Sync() const { return false; }
	virtual bool Lobby() const { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlClientRemove : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlClientRemove() { }
	C4ControlClientRemove(int32_t iID, const char *szReason = "") : iID(iID), strReason(szReason) { }
public:
	int32_t iID;
	StdCopyStrBuf strReason;
public:
	virtual bool Sync() const { return false; }
	virtual bool Lobby() const { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

// control used for initial player info, as well as for player info updates
class C4ControlPlayerInfo : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlPlayerInfo()
	{ }
	C4ControlPlayerInfo(const C4ClientPlayerInfos &PlrInfo)
			: PlrInfo(PlrInfo)
	{ }
protected:
	C4ClientPlayerInfos PlrInfo;
public:
	const C4ClientPlayerInfos &GetInfo() const { return PlrInfo; }
	virtual bool Sync() const { return false; }
	virtual bool Lobby() const { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

struct C4ControlJoinPlayer : public C4ControlPacket // sync
{
public:
	C4ControlJoinPlayer() : iAtClient(-1), idInfo(-1), fByRes(false) { }
	C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo, const C4Network2ResCore &ResCore);
	C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo);
protected:
	StdStrBuf Filename;
	int32_t iAtClient;
	int32_t idInfo;
	bool fByRes;
	StdBuf PlrData;               // for fByRes == false
	C4Network2ResCore ResCore;    // for fByRes == true
public:
	DECLARE_C4CONTROL_VIRTUALS
	virtual bool PreExecute() const;
	virtual void PreRec(C4Record *pRecord);
	void Strip();
};

enum C4ControlEMObjectAction
{
	EMMO_Move,      // move objects by offset
	EMMO_MoveForced,// move objects by offset and ignore HorizontalFixed
	EMMO_Enter,     // enter objects into iTargetObj
	EMMO_Duplicate, // duplicate objects at same position; reset EditCursor
	EMMO_Script,    // execute Script
	EMMO_Remove,    // remove objects
	EMMO_Exit,      // exit objects
	EMMO_Create,    // create a new object (used by C4Game::DropDef)
	EMMO_Transform  // adjust rotation / con of selected object
};

class C4ControlEMMoveObject : public C4ControlPacket // sync
{
public:
	C4ControlEMMoveObject() : eAction(EMMO_Move), tx(Fix0), ty(Fix0), iTargetObj(0), iObjectNum(0), pObjects(nullptr), drag_finished(false) { }
	C4ControlEMMoveObject(C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj,
	                      int32_t iObjectNum = 0, int32_t *pObjects = nullptr, const char *szScript = nullptr, bool drag_finished = false);
	static C4ControlEMMoveObject *CreateObject(const C4ID &id, C4Real x, C4Real y, C4Object *container);
	~C4ControlEMMoveObject();
protected:
	C4ControlEMObjectAction eAction; // action to be performed
	C4Real tx,ty;        // target position
	int32_t iTargetObj;   // enumerated ptr to target object
	int32_t iObjectNum;   // number of objects moved
	int32_t *pObjects;    // pointer on array of objects moved
	StdStrBuf StringParam; // script to execute, or ID of object to create
	bool drag_finished;    // Movement only: Set when mouse drag operation concluded (i.e. mouse up)
private:
	void MoveObject(C4Object *moved_object, bool move_forced) const;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlEMDrawAction
{
	EMDT_SetMode,     // set new landscape mode
	EMDT_Brush,       // drawing tool
	EMDT_Fill,        // drawing tool
	EMDT_Line,        // drawing tool
	EMDT_Rect       // drawing tool
};

enum class LandscapeMode;
class C4ControlEMDrawTool : public C4ControlPacket // sync
{
public:
	C4ControlEMDrawTool() : eAction(EMDT_SetMode), iX(0), iY(0), iX2(0), iY2(0), iGrade(0) { }
	C4ControlEMDrawTool(C4ControlEMDrawAction eAction, LandscapeMode iMode,
	                    int32_t iX=-1, int32_t iY=-1, int32_t iX2=-1, int32_t iY2=-1, int32_t iGrade=-1,
	                    const char *szMaterial=nullptr, const char *szTexture=nullptr,
	                    const char *szBackMaterial=nullptr, const char *szBackTexture=nullptr);
protected:
	C4ControlEMDrawAction eAction;  // action to be performed
	LandscapeMode iMode;        // new mode, or mode action was performed in (action will fail if changed)
	int32_t iX,iY,iX2,iY2,iGrade; // drawing parameters
	StdStrBuf Material; // used material
	StdStrBuf Texture;  // used texture
	StdStrBuf BackMaterial; // used background material
	StdStrBuf BackTexture;  // used background texture
public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlMessageType
{
	C4CMT_Normal    = 0,
	C4CMT_Me        = 1,
	C4CMT_Say       = 2,
	C4CMT_Team      = 3,
	C4CMT_Private   = 4,
	C4CMT_Sound     = 5, // "message" is played as a sound instead
	C4CMT_Alert     = 6, // no message. just flash taskbar for inactive clients.
	C4CMT_System    = 10
};

class C4ControlMessage : public C4ControlPacket // not sync, lobby
{
public:
	C4ControlMessage()
			: eType(C4CMT_Normal), iPlayer(-1) { }
	C4ControlMessage(C4ControlMessageType eType, const char *szMessage, int32_t iPlayer = -1, int32_t iToPlayer = -1)
			: eType(eType), iPlayer(iPlayer), iToPlayer(iToPlayer), Message(szMessage, true)
	{ }
protected:
	C4ControlMessageType eType;
	int32_t iPlayer, iToPlayer;
	StdStrBuf Message;
public:
	virtual bool Sync() const { return false; }
	virtual bool Lobby() const { return true; }
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlRemovePlr : public C4ControlPacket // sync
{
public:
	C4ControlRemovePlr()
			: iPlr(-1), fDisconnected(false) { }
	C4ControlRemovePlr(int32_t iPlr, bool fDisconnected)
			: iPlr(iPlr), fDisconnected(fDisconnected) { }
protected:
	int32_t iPlr;
	bool fDisconnected;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlDebugRec : public C4ControlPacket // sync
{
public:
	C4ControlDebugRec()
	{ }
	C4ControlDebugRec(StdBuf &Data)
			: Data(Data) { }
protected:
	StdBuf Data;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

enum C4ControlVoteType
{
	VT_None = -1,
	VT_Cancel,
	VT_Kick,
	VT_Pause
};

class C4ControlVote : public C4ControlPacket
{
public:
	C4ControlVote(C4ControlVoteType eType = VT_None, bool fApprove = true, int iData = 0)
			: eType(eType), fApprove(fApprove), iData(iData)
	{ }

private:
	C4ControlVoteType eType;
	bool fApprove;
	int32_t iData;

public:
	C4ControlVoteType getType() const { return eType; }
	bool isApprove() const { return fApprove; }
	int32_t getData() const { return iData; }

	StdStrBuf getDesc() const;
	StdStrBuf getDescWarning() const;

	virtual bool Sync() const { return false; }

	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlVoteEnd : public C4ControlVote
{
public:
	C4ControlVoteEnd(C4ControlVoteType eType = VT_None, bool fApprove = true, int iData = 0)
			: C4ControlVote(eType, fApprove, iData)
	{ }

	virtual bool Sync() const { return true; }

	DECLARE_C4CONTROL_VIRTUALS
};

struct C4ControlReInitScenario : public C4ControlPacket // sync
{
public:
	C4ControlReInitScenario();
protected:
	StdBuf data;
public:
	DECLARE_C4CONTROL_VIRTUALS
};

class C4ControlEditGraph : public C4ControlPacket // sync
{
public:
	enum Action
	{
		CEG_None=0,
		CEG_SetVertexPos,
		CEG_EditEdge,
		CEG_InsertVertex,
		CEG_InsertEdge,
		CEG_RemoveVertex,
		CEG_RemoveEdge
	};
	C4ControlEditGraph() {}
	C4ControlEditGraph(const char *path, Action action, int32_t index, int32_t x, int32_t y)
		: path(path), action(action), index(index), x(x), y(y) { }
private:
	StdCopyStrBuf path;
	Action action=CEG_None;
	int32_t index=-1, x=0, y=0;
public:
	DECLARE_C4CONTROL_VIRTUALS

	const char *GetPath() const { return path.getData(); }
	Action GetAction() const { return action; }
	int32_t GetIndex() const { return index; }
	int32_t GetX() const { return x; }
	int32_t GetY() const { return y; }
};

#endif
