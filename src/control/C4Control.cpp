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

#include "C4Include.h"
#include "control/C4Control.h"

#include "script/C4AulExec.h"
#include "object/C4Object.h"
#include "control/C4GameSave.h"
#include "gui/C4GameLobby.h"
#include "network/C4Network2Dialogs.h"
#include "lib/C4Random.h"
#include "editor/C4Console.h"
#include "lib/C4Log.h"
#include "game/C4GraphicsSystem.h"
#include "player/C4Player.h"
#include "player/C4RankSystem.h"
#include "control/C4RoundResults.h"
#include "landscape/C4PXS.h"
#include "landscape/C4MassMover.h"
#include "gui/C4GameMessage.h"
#include "landscape/C4Landscape.h"
#include "game/C4Game.h"
#include "game/C4GameScript.h"
#include "player/C4PlayerList.h"
#include "object/C4GameObjects.h"
#include "control/C4GameControl.h"
#include "gui/C4ScriptGuiWindow.h"
#include "gui/C4MessageInput.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"

#ifndef NOAULDEBUG
#include "script/C4AulDebug.h"
#endif

#include "script/C4AulExec.h"

// *** C4ControlPacket
C4ControlPacket::C4ControlPacket()
		: iByClient(::Control.ClientID())
{

}

C4ControlPacket::~C4ControlPacket()
{

}

bool C4ControlPacket::LocalControl() const
{
	return iByClient == ::Control.ClientID();
}

void C4ControlPacket::SetByClient(int32_t inByClient)
{
	iByClient = inByClient;
}

void C4ControlPacket::CompileFunc(StdCompiler *pComp)
{
	// Section must be set by caller
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iByClient), "ByClient", -1));
}

// *** C4Control

C4Control::C4Control()
{

}

C4Control::~C4Control()
{
	Clear();
}

void C4Control::Clear()
{
	Pkts.Clear();
}

bool C4Control::PreExecute() const
{
	bool fReady = true;
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		// recheck packet type: Must be control
		if (pPkt->getPktType() & CID_First)
		{
			C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
			if (pCtrlPkt)
				fReady &= pCtrlPkt->PreExecute();
		}
		else
		{
			LogF("C4Control::PreExecute: WARNING: Ignoring packet type %2x (not control.)", pPkt->getPktType());
		}
	}
	return fReady;
}

void C4Control::Execute() const
{
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		// recheck packet type: Must be control
		if (pPkt->getPktType() & CID_First)
		{
			C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
			if (pCtrlPkt)
				pCtrlPkt->Execute();
		}
		else
		{
			LogF("C4Control::Execute: WARNING: Ignoring packet type %2x (not control.)", pPkt->getPktType());
		}
	}
}

void C4Control::PreRec(C4Record *pRecord) const
{
	for (C4IDPacket *pPkt = firstPkt(); pPkt; pPkt = nextPkt(pPkt))
	{
		C4ControlPacket *pCtrlPkt = static_cast<C4ControlPacket *>(pPkt->getPkt());
		if (pCtrlPkt)
			pCtrlPkt->PreRec(pRecord);
	}
}

void C4Control::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(Pkts);
}

// *** C4ControlSet

void C4ControlSet::Execute() const
{
	switch (eValType)
	{
	case C4CVT_None: break;

	case C4CVT_ControlRate: // adjust control rate
		// host only
		if (iByClient != C4ClientIDHost) break;
		// adjust control rate
		::Control.ControlRate += iData;
		::Control.ControlRate = Clamp<int32_t>(::Control.ControlRate, 1, C4MaxControlRate);
		Game.Parameters.ControlRate = ::Control.ControlRate;
		// write back adjusted control rate to network settings
		if (::Control.isCtrlHost() && !::Control.isReplay() && ::Control.isNetwork())
			Config.Network.ControlRate = ::Control.ControlRate;
		// always show msg
		::GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_NET_CONTROLRATE"),::Control.ControlRate,Game.FrameCounter).getData());
		break;

	case C4CVT_DisableDebug: // force debug mode disabled
	{
		if (Game.DebugMode)
		{
			Game.DebugMode=false;
			::GraphicsSystem.DeactivateDebugOutput();
		}
		// save flag, log
		Game.Parameters.AllowDebug = false;
		C4Client *client = ::Game.Clients.getClientByID(iByClient);
		LogF("Debug mode forced disabled by %s", client ? client->getName() : "<unknown client>");
		break;
	}
	break;

	case C4CVT_MaxPlayer:
		// host only
		if (iByClient != C4ClientIDHost) break;
		// not in league
		if (Game.Parameters.isLeague())
		{
			Log("/set maxplayer disabled in league!");
			C4GUI::GUISound("UI::Error");
			break;
		}
		// set it
		Game.Parameters.MaxPlayers = iData;
		LogF("MaxPlayer = %d", (int)Game.Parameters.MaxPlayers);
		break;

	case C4CVT_TeamDistribution:
		// host only
		if (iByClient != C4ClientIDHost) break;
		// set new value
		Game.Teams.SetTeamDistribution(static_cast<C4TeamList::TeamDist>(iData));
		break;

	case C4CVT_TeamColors:
		// host only
		if (!HostControl()) break;
		// set new value
		Game.Teams.SetTeamColors(!!iData);
		break;
	}
}

void C4ControlSet::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdapt(eValType), "Type", C4CVT_None));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iData), "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlScript

void C4ControlScript::Execute() const
{
	const char *szScript = Script.getData();
	// user script: from host only
	if ((iByClient != C4ClientIDHost) && !Console.Active) return;
	// only allow scripts when debug mode is not forbidden
	if (!Game.Parameters.AllowDebug) return;

	// execute
	C4PropList *pPropList = nullptr;
	if (iTargetObj == SCOPE_Console)
		pPropList = ::GameScript.ScenPropList.getPropList();
	else if (iTargetObj == SCOPE_Global)
		pPropList = ::ScriptEngine.GetPropList();
	else if (!(pPropList = ::Objects.SafeObjectPointer(iTargetObj)))
		// default: Fallback to global context
		pPropList = ::ScriptEngine.GetPropList();
	C4Value rVal(AulExec.DirectExec(pPropList, szScript, "console script", false, fUseVarsFromCallerContext ? AulExec.GetContext(AulExec.GetContextDepth()-1) : nullptr));
#ifndef NOAULDEBUG
	C4AulDebug* pDebug;
	if ( (pDebug = C4AulDebug::GetDebugger()) )
	{
		pDebug->ControlScriptEvaluated(szScript, rVal.GetDataString().getData());
	}
#endif
	// show messages
	// print script
	LogF("-> %s::%s", pPropList->GetName(), szScript);
	// print result
	bool is_local_script = true;
	if (!LocalControl())
	{
		C4Network2Client *pClient = nullptr;
		if (::Network.isEnabled())
		{
			pClient = ::Network.Clients.GetClientByID(iByClient);
			if (pClient != ::Network.Clients.GetLocal())
			{
				is_local_script = false;
			}
		}
		if (pClient)
			LogF(" = %s (by %s)", rVal.GetDataString().getData(), pClient->getName());
		else
			LogF(" = %s (by client %d)", rVal.GetDataString().getData(), iByClient);
	}
	else
		LogF(" = %s", rVal.GetDataString().getData());
	// Editor update
	if (::Console.Active)
	{
		C4Object *returned_object = rVal.getObj();
		if (editor_select_result && is_local_script && returned_object)
		{
			::Console.EditCursor.ClearSelection(returned_object);
			::Console.EditCursor.AddToSelection(returned_object);
			::Console.EditCursor.OnSelectionChanged();
		}
		// Always: refresh property view after script command
		::Console.EditCursor.InvalidateSelection();
	}
}

void C4ControlScript::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iTargetObj, "TargetObj", -1));
	pComp->Value(mkNamingAdapt(fUseVarsFromCallerContext, "UseVarsFromCallerContext", false));
	pComp->Value(mkNamingAdapt(editor_select_result, "EditorSelectResult", false));
	pComp->Value(mkNamingAdapt(Script, "Script", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlMsgBoardReply
void C4ControlMsgBoardReply::Execute() const
{
	C4Object *target_object = ::Objects.SafeObjectPointer(target);
	C4Player *target_player = ::Players.Get(player);

	// remove query
	if (!target_player) return;
	if (!target_player->RemoveMessageBoardQuery(target_object)) return;

	// execute callback if answer present
	if (!reply) return;
	C4AulParSet pars(C4VString(reply), player);
	if (target_object)
		target_object->Call(PSF_InputCallback, &pars);
	else
		::GameScript.Call(PSF_InputCallback, &pars);
}

void C4ControlMsgBoardReply::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(target, "TargetObj", -1));
	pComp->Value(mkNamingAdapt(player, "Player", NO_OWNER));
	pComp->Value(mkNamingAdapt(reply, "Reply", nullptr));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlMsgBoardCmd
void C4ControlMsgBoardCmd::Execute() const
{
	// don't handle this if the game isn't actually running
	if (!::Game.IsRunning) return;

	// fetch command script
	C4MessageBoardCommand *cmd = ::MessageInput.GetCommand(command.getData());
	if (!cmd) return;
	StdCopyStrBuf script(cmd->Script);

	// interpolate parameters as required
	script.Replace("%player%", FormatString("%d", player).getData());
	if (parameter)
	{
		script.Replace("%d", FormatString("%d", std::atoi(parameter.getData())).getData());
		StdCopyStrBuf escaped_param(parameter);
		escaped_param.EscapeString();
		script.Replace("%s", escaped_param.getData());
	}

	// Run script
	C4Value rv(::AulExec.DirectExec(::ScriptEngine.GetPropList(), script.getData(), "message board command"));
#ifndef NOAULDEBUG
	C4AulDebug* pDebug = C4AulDebug::GetDebugger();
	if (pDebug)
		pDebug->ControlScriptEvaluated(script.getData(), rv.GetDataString().getData());
#endif
}

void C4ControlMsgBoardCmd::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(player, "Player", NO_OWNER));
	pComp->Value(mkNamingAdapt(command, "Command"));
	pComp->Value(mkNamingAdapt(parameter, "Parameter"));
	C4ControlPacket::CompileFunc(pComp);
}
// *** C4ControlPlayerSelect

C4ControlPlayerSelect::C4ControlPlayerSelect(int32_t iPlr, const C4ObjectList &Objs, bool fIsAlt)
		: iPlr(iPlr), fIsAlt(fIsAlt), iObjCnt(Objs.ObjectCount())
{
	pObjNrs = new int32_t[iObjCnt];
	int32_t i = 0;
	for (C4Object *obj : Objs)
		pObjNrs[i++] = obj->Number;
	assert(i == iObjCnt);
}

void C4ControlPlayerSelect::Execute() const
{
	// get player
	C4Player *pPlr = ::Players.Get(iPlr);
	if (!pPlr) return;

	// Check object list
	C4Object *pObj;
	int32_t iControlChecksum = 0;
	for (int32_t i = 0; i < iObjCnt; i++)
		if ((pObj = ::Objects.SafeObjectPointer(pObjNrs[i])))
		{
			iControlChecksum += pObj->Number * (iControlChecksum+4787821);
			// user defined object selection: callback to object
			if (pObj->Category & C4D_MouseSelect)
			{
				if (fIsAlt)
					pObj->Call(PSF_MouseSelectionAlt, &C4AulParSet(iPlr));
				else
					pObj->Call(PSF_MouseSelection, &C4AulParSet(iPlr));
			}
		}
	// count
	pPlr->CountControl(C4Player::PCID_Command, iControlChecksum);
}

void C4ControlPlayerSelect::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(iPlr, "Player", -1));
	pComp->Value(mkNamingAdapt(fIsAlt, "IsAlt", false));
	pComp->Value(mkNamingAdapt(iObjCnt, "ObjCnt", 0));
	// Compile array
	if (pComp->isDeserializer())
		{ delete[] pObjNrs; pObjNrs = new int32_t [iObjCnt]; }
	pComp->Value(mkNamingAdapt(mkArrayAdapt(pObjNrs, iObjCnt), "Objs", 0));

	C4ControlPacket::CompileFunc(pComp);
}


// *** C4ControlPlayerControl

void C4ControlPlayerControl::Execute() const
{
	C4PlayerControl *pTargetCtrl = nullptr;
	if (iPlr == -1)
	{
		// neutral control packet: Execute in global control
	}
	else
	{
		// player-based control: Execute on control owned by player
		C4Player *pPlr=::Players.Get(iPlr);
		if (pPlr)
		{
			pTargetCtrl = &(pPlr->Control);
		}
	}
	if (pTargetCtrl) pTargetCtrl->ExecuteControlPacket(this);
}

void C4ControlPlayerControl::ControlItem::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(iControl);
	pComp->Separator();
	pComp->Value(iTriggerMode);
}

void C4ControlPlayerControl::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlr), "Player", -1));
	pComp->Value(mkNamingAdapt(mkIntAdapt(state), "State", 0));
	pComp->Value(mkNamingAdapt(ExtraData, "ExtraData", C4KeyEventData()));
	pComp->Value(mkNamingAdapt(mkSTLContainerAdapt(ControlItems), "Controls", ControlItemVec()));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlPlayerMouse
C4ControlPlayerMouse *C4ControlPlayerMouse::Hover(const C4Player *player, const C4Object *target, const C4Object *old_target, const C4Object *drag)
{
	assert(player != nullptr);
	if (!player) return nullptr;

	auto control = new C4ControlPlayerMouse();
	control->action = CPM_Hover;
	control->player = player->Number;
	control->drag_obj = drag ? drag->Number : 0;
	control->target_obj = target ? target->Number : 0;
	control->old_obj = old_target ? old_target->Number : 0;
	return control;
}
C4ControlPlayerMouse *C4ControlPlayerMouse::DragDrop(const C4Player *player, const C4Object *target, const C4Object *drag)
{
	assert(player != nullptr);
	if (!player) return nullptr;

	auto control = new C4ControlPlayerMouse();
	control->action = CPM_Drop;
	control->player = player->Number;
	control->drag_obj = drag ? drag->Number : 0;
	control->target_obj = target ? target->Number : 0;
	return control;
}

void C4ControlPlayerMouse::Execute() const
{
	const char *callback_name = nullptr;
	C4AulParSet pars(player);

	switch (action)
	{
	case CPM_NoAction:
		return;

	case CPM_Hover:
		// Mouse movement, object hover state changed
		callback_name = PSF_MouseHover;
		pars[1] = C4VObj(::Objects.SafeObjectPointer(old_obj));
		pars[2] = C4VObj(::Objects.SafeObjectPointer(target_obj));
		pars[3] = C4VObj(::Objects.SafeObjectPointer(drag_obj));
		break;
	
	case CPM_Drop:
		// Drag/Drop operation
		callback_name = PSF_MouseDragDrop;
		pars[1] = C4VObj(::Objects.SafeObjectPointer(drag_obj));
		pars[2] = C4VObj(::Objects.SafeObjectPointer(target_obj));
		break;
	}
	
	// Do call
	if (!callback_name) return;
	::ScriptEngine.Call(callback_name, &pars);
}

void C4ControlPlayerMouse::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(action, "Action"));
	pComp->Value(mkNamingAdapt(player, "Player", NO_OWNER));
	pComp->Value(mkNamingAdapt(target_obj, "TargetObj"));
	pComp->Value(mkNamingAdapt(drag_obj, "DragObj"));
	pComp->Value(mkNamingAdapt(old_obj, "OldObj"));
}

// *** C4ControlMenuCommand

C4ControlMenuCommand::C4ControlMenuCommand(int32_t actionID, int32_t player, int32_t menuID, int32_t subwindowID, C4Object *target, int32_t actionType)
	: actionID(actionID), player(player), menuID(menuID), subwindowID(subwindowID), target(target ? target->Number : 0), actionType(actionType)
{

}

void C4ControlMenuCommand::Execute() const
{
	// invalid action? The action needs to be in bounds!
	if (actionType < 0 || actionType >= C4ScriptGuiWindowPropertyName::_lastProp)
	{
		// this could only come from a malicious attempt to crash the engine!
		Log("Warning: invalid action type for C4ControlMenuCommand!");
		return;
	}
	C4ScriptGuiWindow *menu = ::Game.ScriptGuiRoot->GetChildByID(menuID);
	// menu was closed?
	if (!menu) return;

	C4Object *obj = target ? ::Objects.ObjectPointer(target) : 0;
	// target has been removed in the meantime? abort now
	if (target && !obj) return;

	menu->ExecuteCommand(actionID, player, subwindowID, actionType, obj);
}

void C4ControlMenuCommand::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(actionID), "ID", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(player), "Player", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(menuID), "Menu", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(subwindowID), "Window", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(actionType), "Action", 0));
	pComp->Value(mkNamingAdapt(target, "Target", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlPlayerAction
C4ControlPlayerAction::C4ControlPlayerAction(const C4Player *source)
	: action(CPA_NoAction), source(source ? source->Number : NO_OWNER), target(NO_OWNER), param_int(0)
{
}

C4ControlPlayerAction *C4ControlPlayerAction::Surrender(const C4Player *source)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_Surrender;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::Eliminate(const C4Player *source)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_Eliminate;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::ActivateGoal(const C4Player *source, const C4Object *goal)
{
	assert(source);
	assert(goal);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_ActivateGoal;
	control->target = goal->Number;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::ActivateGoalMenu(const C4Player *source)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_ActivateGoalMenu;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::SetHostility(const C4Player *source, const C4Player *target, bool hostile)
{
	assert(source);
	assert(target);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_SetHostility;
	control->target = target ? target->Number : NO_OWNER;
	control->param_int = hostile;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::SetTeam(const C4Player *source, int32_t team)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_SetTeam;
	control->target = team;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::InitScenarioPlayer(const C4Player *source, int32_t team)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_InitScenarioPlayer;
	control->target = team;
	return control;
}
C4ControlPlayerAction *C4ControlPlayerAction::InitPlayerControl(const C4Player *source, const C4PlayerControlAssignmentSet *ctrl_set)
{
	assert(source);
	C4ControlPlayerAction *control = new C4ControlPlayerAction(source);
	control->action = CPA_InitPlayerControl;
	if (ctrl_set)
	{
		control->param_str = ctrl_set->GetName();
		if (ctrl_set->HasKeyboard())
			control->param_int |= CPA_IPC_HasKeyboard;
		if (ctrl_set->HasMouse())
			control->param_int |= CPA_IPC_HasMouse;
		if (ctrl_set->HasGamepad())
			control->param_int |= CPA_IPC_HasGamepad;
	}
	return control;
}

void C4ControlPlayerAction::Execute() const
{
	// The originating player must exist
	C4Player *source_player = ::Players.Get(source);
	if (!source_player) return;

	switch (action)
	{
	case CPA_Surrender:
		source_player->Surrender();
		break;

	case CPA_Eliminate:
		source_player->Eliminate();
		break;
	
	case CPA_ActivateGoal:
	{
		// Make sure the object actually exists
		C4Object *goal = ::Objects.SafeObjectPointer(target);
		if (!goal) return;
		// Call it
		C4AulParSet pars(source_player->Number);
		goal->Call("Activate", &pars);
		break;
	}

	case CPA_ActivateGoalMenu:
		// open menu
		source_player->Menu.ActivateGoals(source_player->Number, source_player->LocalControl && !::Control.isReplay());
		break;

	case CPA_SetHostility:
	{
		// Can only set hostility towards a player that exists
		C4Player *target_player = ::Players.Get(target);
		if (!target_player) return;
		
		// Proxy the hostility change through C4Aul, in case a script wants to capture it
		C4AulParSet pars(source_player->Number, target_player->Number, param_int != 0);
		::ScriptEngine.Call("SetHostility", &pars);
		break;
	}

	case CPA_SetTeam:
	{
		// Make sure team switching is allowed in the first place
		if (!::Game.Teams.IsTeamSwitchAllowed()) return;

		// We can't change teams to one that doesn't exist
		C4Team *team = ::Game.Teams.GetTeamByID(target);
		if (!team && target != TEAMID_New) return;

		// Proxy the team switch through C4Aul, in case a script wants to capture it
		C4AulParSet pars(source_player->Number, target);
		::ScriptEngine.Call("SetPlayerTeam", &pars);
		break;
	}

	case CPA_InitScenarioPlayer:
	{
		// Proxy the call through C4Aul, in case a script wants to capture it
		C4AulParSet pars(source_player->Number, target);
		::ScriptEngine.Call("InitScenarioPlayer", &pars);
		break;
	}

	case CPA_InitPlayerControl:
	{
		// Notify scripts about player control selection
		const char *callback_name = PSF_InitializePlayerControl;
		
		C4AulParSet pars(source_player->Number);
		// If the player is using a control set, its name is stored in param_str
		if (param_str)
		{
			pars[1] = C4VString(param_str);
			pars[2] = C4VBool(CPA_IPC_HasKeyboard == (param_int & CPA_IPC_HasKeyboard));
			pars[3] = C4VBool(CPA_IPC_HasMouse == (param_int & CPA_IPC_HasMouse));
			pars[4] = C4VBool(CPA_IPC_HasGamepad == (param_int & CPA_IPC_HasGamepad));
		}
		::ScriptEngine.Call(callback_name, &pars);
		break;
	}

	case CPA_NoAction: break;
	}
}

void C4ControlPlayerAction::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(source, "Player", NO_OWNER));
	const StdEnumEntry<Action> ActionNames[] =
	{
		{ "Surrender",             CPA_Surrender  },
		{ "ActivateGoal",          CPA_ActivateGoal  },
		{ "ActivateGoalMenu",      CPA_ActivateGoalMenu  },
		{ "Eliminate",             CPA_Eliminate  },
		{ "SetHostility",          CPA_SetHostility  },
		{ "SetTeam",               CPA_SetTeam  },
		{ "InitScenarioPlayer",    CPA_InitScenarioPlayer  },
		{ "InitPlayerControl",     CPA_InitPlayerControl  },
		{ nullptr, CPA_NoAction }
	};
	pComp->Value(mkNamingAdapt(mkEnumAdapt<Action, int32_t>(action, ActionNames), "Action", CPA_NoAction));
	pComp->Value(mkNamingAdapt(target, "Target", NO_OWNER));
	pComp->Value(mkNamingAdapt(param_int, "DataI", 0));
	pComp->Value(mkNamingAdapt(param_str, "DataS", nullptr));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlSyncCheck

C4ControlSyncCheck::C4ControlSyncCheck()
{
}

void C4ControlSyncCheck::Set()
{
	Frame = Game.FrameCounter;
	ControlTick = ::Control.ControlTick;
	RandomCount = ::RandomCount;
	AllCrewPosX = GetAllCrewPosX();
	PXSCount = ::PXS.Count;
	MassMoverIndex = ::MassMover.CreatePtr;
	ObjectCount = ::Objects.ObjectCount();
	ObjectEnumerationIndex = C4PropListNumbered::GetEnumerationIndex();
	SectShapeSum = ::Objects.Sectors.getShapeSum();
}

int32_t C4ControlSyncCheck::GetAllCrewPosX()
{
	int32_t cpx=0;
	for (C4Player *pPlr=::Players.First; pPlr; pPlr=pPlr->Next)
		for (C4Object *member : pPlr->Crew)
			cpx += fixtoi(member->fix_x, 100);
	return cpx;
}

void C4ControlSyncCheck::Execute() const
{
	// control host?
	if (::Control.isCtrlHost()) return;

	// get the saved sync check data
	C4ControlSyncCheck* pSyncCheck = ::Control.GetSyncCheck(Frame), &SyncCheck = *pSyncCheck;
	if (!pSyncCheck)
	{
		::Control.SyncChecks.Add(CID_SyncCheck, new C4ControlSyncCheck(*this));
		return;
	}

	// Not equal
	if ( Frame                  != pSyncCheck->Frame
	     ||(ControlTick            != pSyncCheck->ControlTick   && !::Control.isReplay())
	     || RandomCount            != pSyncCheck->RandomCount
	     || AllCrewPosX            != pSyncCheck->AllCrewPosX
	     || PXSCount               != pSyncCheck->PXSCount
	     || MassMoverIndex         != pSyncCheck->MassMoverIndex
	     || ObjectCount            != pSyncCheck->ObjectCount
	     || ObjectEnumerationIndex != pSyncCheck->ObjectEnumerationIndex
	     || SectShapeSum           != pSyncCheck->SectShapeSum)
	{
		const char *szThis = "Client", *szOther = ::Control.isReplay() ? "Rec ":"Host";
		if (iByClient != ::Control.ClientID())
			{ const char *szTemp = szThis; szThis = szOther; szOther = szTemp; }
		// Message
		LogFatal("Network: Synchronization loss!");
		LogFatal(FormatString("Network: %s Frm %i Ctrl %i Rnc %i Cpx %i PXS %i MMi %i Obc %i Oei %i Sct %i", szThis, Frame,ControlTick,RandomCount,AllCrewPosX,PXSCount,MassMoverIndex,ObjectCount,ObjectEnumerationIndex, SectShapeSum).getData());
		LogFatal(FormatString("Network: %s Frm %i Ctrl %i Rnc %i Cpx %i PXS %i MMi %i Obc %i Oei %i Sct %i", szOther, SyncCheck.Frame,SyncCheck.ControlTick,SyncCheck.RandomCount,SyncCheck.AllCrewPosX,SyncCheck.PXSCount,SyncCheck.MassMoverIndex,SyncCheck.ObjectCount,SyncCheck.ObjectEnumerationIndex, SyncCheck.SectShapeSum).getData());
		StartSoundEffect("UI::SyncError");
#ifdef _DEBUG
		// Debug safe
		C4GameSaveNetwork SaveGame(false);
		SaveGame.Save(Config.AtExePath("Desync.ocs"));
#endif
		// league: Notify regular client disconnect within the game
		::Network.LeagueNotifyDisconnect(C4ClientIDHost, C4LDR_Desync);
		// Deactivate / end
		if (::Control.isReplay())
			Game.DoGameOver();
		else if (::Control.isNetwork())
		{
			Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, "Network: Synchronization loss!");
			::Network.Clear();
		}
	}

}

void C4ControlSyncCheck::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(Frame), "Frame", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ControlTick), "ControlTick", 0));
	pComp->Value(mkNamingAdapt(RandomCount, "RandomCount", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(AllCrewPosX), "AllCrewPosX", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(PXSCount), "PXSCount", 0));
	pComp->Value(mkNamingAdapt(MassMoverIndex, "MassMoverIndex", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ObjectCount), "ObjectCount", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(ObjectEnumerationIndex), "ObjectEnumerationIndex", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(SectShapeSum), "SectShapeSum", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlSynchronize

void C4ControlSynchronize::Execute() const
{
	Game.Synchronize(fSavePlrFiles);
	if (fSyncClearance) Game.SyncClearance();
}

void C4ControlSynchronize::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(fSavePlrFiles, "SavePlrs", false));
	pComp->Value(mkNamingAdapt(fSyncClearance, "SyncClear", false));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlClientJoin

void C4ControlClientJoin::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	// add client
	C4Client *pClient = Game.Clients.Add(Core);
	if (!pClient) return;
	// log
	LogF(LoadResStr("IDS_NET_CLIENT_JOIN"), Core.getName());
	// lobby callback
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby) pLobby->OnClientJoin(pClient);
	// console callback
	if (Console.Active) Console.UpdateMenus();
}

void C4ControlClientJoin::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Core, "ClientCore"));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4Control

void C4ControlClientUpdate::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost && eType != CUT_SetReady) return;
	// find client
	C4Client *pClient = Game.Clients.getClientByID(iID);
	if (!pClient) return;
	StdCopyStrBuf strClient(LoadResStr(pClient->isLocal() ? "IDS_NET_LOCAL_CLIENT" : "IDS_NET_CLIENT"));
	// do whatever specified
	switch (eType)
	{
	case CUT_None: break;
	case CUT_Activate:
		// nothing to do?
		if (pClient->isActivated() == !!iData) break;
		// log
		LogF(LoadResStr(iData ? "IDS_NET_CLIENT_ACTIVATED" : "IDS_NET_CLIENT_DEACTIVATED"), strClient.getData(), pClient->getName());
		// activate/deactivate
		pClient->SetActivated(!!iData);
		// local?
		if (pClient->isLocal())
			::Control.SetActivated(!!iData);
		break;
	case CUT_SetObserver:
		// nothing to do?
		if (pClient->isObserver()) break;
		// log
		LogF(LoadResStr("IDS_NET_CLIENT_OBSERVE"), strClient.getData(), pClient->getName());
		// set observer (will deactivate)
		pClient->SetObserver();
		// local?
		if (pClient->isLocal())
			::Control.SetActivated(false);
		// remove all players ("soft kick")
		::Players.RemoveAtClient(iID, true);
		break;
	case CUT_SetReady:
		{
		// nothing to do?
		if (pClient->isLobbyReady() == !!iData) break;
		// ready/unready (while keeping track of time)
		time_t last_change_time = MinReadyAnnouncementDelay;
		pClient->SetLobbyReady(!!iData, &last_change_time);
		// log to others, but don't spam
		if (last_change_time >= MinReadyAnnouncementDelay)
		{
			if (!pClient->isLocal())
			{
				LogF(LoadResStr(iData ? "IDS_NET_CLIENT_READY" : "IDS_NET_CLIENT_UNREADY"), strClient.getData(), pClient->getName());
			}
			// Also update icons
			C4GameLobby::MainDlg *lobby = ::Network.GetLobby();
			if (lobby) lobby->OnClientReadyStateChange();
		}
		break;
		}
	}
	// Update console net menu to reflect activation/etc.
	::Console.UpdateNetMenu();
}

void C4ControlClientUpdate::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", CUT_None));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iID), "ClientID", C4ClientIDUnknown));
	if (eType == CUT_Activate)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iData), "Data", 0));
	if (eType == CUT_SetReady)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iData), "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlClientRemove

void C4ControlClientRemove::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	if (iID == C4ClientIDHost) return;
	// find client
	C4Client *pClient = Game.Clients.getClientByID(iID);
	if (!pClient)
	{
		// TODO: in replays, client list is not yet synchronized
		// remove players anyway
		if (::Control.isReplay()) ::Players.RemoveAtClient(iID, true);
		return;
	}
	StdCopyStrBuf strClient(LoadResStr(pClient->isLocal() ? "IDS_NET_LOCAL_CLIENT" : "IDS_NET_CLIENT"));
	// local?
	if (pClient->isLocal())
	{
		StdStrBuf sMsg;
		sMsg.Format(LoadResStr("IDS_NET_CLIENT_REMOVED"), strClient.getData(), pClient->getName(), strReason.getData());
		Log(sMsg.getData());
		Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, sMsg.getData());
		::Control.ChangeToLocal();
		return;
	}
	// remove client
	if (!Game.Clients.Remove(pClient)) return;
	// log
	LogF(LoadResStr("IDS_NET_CLIENT_REMOVED"), strClient.getData(), pClient->getName(), strReason.getData());
	// remove all players
	::Players.RemoveAtClient(iID, true);
	// remove all resources
	if (::Network.isEnabled())
		::Network.ResList.RemoveAtClient(iID);
	// lobby callback
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby && ::pGUI) pLobby->OnClientPart(pClient);
	// player list callback
	::Network.Players.OnClientPart(pClient);
	// console callback
	if (Console.Active) Console.UpdateMenus();

	// delete
	delete pClient;
}

void C4ControlClientRemove::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iID), "ClientID", CUT_None));
	pComp->Value(mkNamingAdapt(strReason, "Reason", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlJoinPlayer

C4ControlJoinPlayer::C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo, const C4Network2ResCore &ResCore)
		: Filename(szFilename, true), iAtClient(iAtClient),
		idInfo(iIDInfo), fByRes(true), ResCore(ResCore)
{
}

C4ControlJoinPlayer::C4ControlJoinPlayer(const char *szFilename, int32_t iAtClient, int32_t iIDInfo)
		: Filename(szFilename, true), iAtClient(iAtClient),
		idInfo(iIDInfo), fByRes(false)
{
	// load from file if filename is given - which may not be the case for script players
	StdStrBuf filename;
	if (szFilename && Reloc.LocateItem(szFilename, filename))
	{
		bool file_is_temp = false;
		if (DirectoryExists(filename.getData()))
		{
			// the player file is unpacked - temp pack and read
			StdStrBuf filename_buf;
			filename_buf.Copy(Config.AtTempPath(GetFilenameOnly(filename.getData())));
			MakeTempFilename(&filename_buf);
			if (C4Group_PackDirectoryTo(filename.getData(), filename_buf.getData()))
			{
				filename.Take(filename_buf);
				file_is_temp = true;
			}
			else
			{
				// pack failed
				LogF("[!]Error packing player file %s to %s for join: Pack failed.", filename.getData(), filename_buf.getData());
				assert(false);
			}
		}
		bool fSuccess = PlrData.LoadFromFile(filename.getData());
		if (!fSuccess)
		{
			LogF("[!]Error loading player file from %s.", filename.getData());
			assert(false);
		}
		if (file_is_temp) EraseFile(filename.getData());
	}
	else if(szFilename)
	{
		LogF("[!]Error loading player file from %s.", szFilename);
		assert(false);
	}
}

void C4ControlJoinPlayer::Execute() const
{
	const char *szFilename = Filename.getData();

	// get client
	C4Client *pClient = Game.Clients.getClientByID(iAtClient);
	if (pClient)
	{
		// get info
		C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(idInfo);
		if (!pInfo)
		{
			LogF("ERROR: Ghost player join: No info for %d", idInfo);
			assert(false);
		}
		else if (LocalControl())
		{
			// Local player: Just join from local file
			Game.JoinPlayer(szFilename, iAtClient, pClient->getName(), pInfo);
		}
		else if (!fByRes)
		{
			if (PlrData.getSize())
			{
				// create temp file
				StdStrBuf PlayerFilename; PlayerFilename.Format("%s-%s", pClient->getName(), GetFilename(szFilename));
				PlayerFilename = Config.AtTempPath(PlayerFilename.getData());
				// copy to it
				if (PlrData.SaveToFile(PlayerFilename.getData()))
				{
					Game.JoinPlayer(PlayerFilename.getData(), iAtClient, pClient->getName(), pInfo);
					EraseFile(PlayerFilename.getData());
				}
			}
			else if (pInfo->GetType() == C4PT_Script)
			{
				// script players may join without data
				Game.JoinPlayer(nullptr, iAtClient, pClient->getName(), pInfo);
			}
			else
			{
				// no player data for user player present: Must not happen
				LogF("ERROR: Ghost player join: No player data for %s", (const char*)pInfo->GetName());
				assert(false);
			}
		}
		else if (::Control.isNetwork())
		{
			// Find resource
			C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
			if (pRes && pRes->isComplete())
				Game.JoinPlayer(pRes->getFile(), iAtClient, pClient->getName(), pInfo);
		}
		else if (::Control.isReplay())
		{
			// Expect player in scenario file
			StdStrBuf PlayerFilename; PlayerFilename.Format("%s" DirSep "%d-%s", Game.ScenarioFilename, ResCore.getID(), GetFilename(ResCore.getFileName()));
			Game.JoinPlayer(PlayerFilename.getData(), iAtClient, pClient ? pClient->getName() : "Unknown", pInfo);
		}
		else
		{
			// Shouldn't happen
			assert(false);
		}
	}
	// After last of the initial player joins, do a game callback
	::Game.OnPlayerJoinFinished();
}

void C4ControlJoinPlayer::Strip()
{
	// By resource? Can't touch player file, then.
	if (fByRes) return;
	// create temp file
	StdStrBuf PlayerFilename; PlayerFilename = GetFilename(Filename.getData());
	PlayerFilename = Config.AtTempPath(PlayerFilename.getData());
	// Copy to it
	if (PlrData.SaveToFile(PlayerFilename.getData()))
	{
		// open as group
		C4Group Grp;
		if (!Grp.Open(PlayerFilename.getData()))
			{ EraseFile(PlayerFilename.getData()); return; }
		// remove bigicon, if the file size is too large
		size_t iBigIconSize=0;
		if (Grp.FindEntry(C4CFN_BigIcon, nullptr, &iBigIconSize))
			if (iBigIconSize > C4NetResMaxBigicon*1024)
				Grp.Delete(C4CFN_BigIcon);
		Grp.Close();
		// Set new data
		StdBuf NewPlrData;
		if (!NewPlrData.LoadFromFile(PlayerFilename.getData()))
			{ EraseFile(PlayerFilename.getData()); return; }
		PlrData = std::move(NewPlrData);
		// Done
		EraseFile(PlayerFilename.getData());
	}
}

bool C4ControlJoinPlayer::PreExecute() const
{
	// all data included in control packet?
	if (!fByRes) return true;
	// client lost?
	if (!Game.Clients.getClientByID(iAtClient)) return true;
	// network only
	if (!::Control.isNetwork()) return true;
	// search resource
	C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
	// doesn't exist? start loading
	if (!pRes) { pRes = ::Network.ResList.AddByCore(ResCore, true); }
	if (!pRes) return true;
	// is loading or removed?
	return !pRes->isLoading();
}

void C4ControlJoinPlayer::PreRec(C4Record *pRecord)
{
	if (!pRecord) return;
	if (fByRes)
	{
		// get local file by id
		C4Network2Res::Ref pRes = ::Network.ResList.getRefRes(ResCore.getID());
		if (!pRes || pRes->isRemoved()) return;
		// create a copy of the resource
		StdStrBuf szTemp; szTemp.Copy(pRes->getFile());
		MakeTempFilename(&szTemp);
		if (C4Group_CopyItem(pRes->getFile(), szTemp.getData()))
		{
			// add to record
			StdStrBuf szTarget = FormatString("%d-%s", ResCore.getID(), GetFilename(ResCore.getFileName()));
			pRecord->AddFile(szTemp.getData(), szTarget.getData(), true);
		}
	}
	else
	{
		// player data raw within control: Will be used directly in record
	}
}

void C4ControlJoinPlayer::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkNetFilenameAdapt(Filename), "Filename", ""));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iAtClient), "AtClient", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(idInfo), "InfoID", -1));
	pComp->Value(mkNamingAdapt(fByRes, "ByRes", false));
	if (fByRes)
		pComp->Value(mkNamingAdapt(ResCore, "ResCore"));
	else
		pComp->Value(mkNamingAdapt(PlrData, "PlrData"));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlEMMoveObject

C4ControlEMMoveObject::C4ControlEMMoveObject(C4ControlEMObjectAction eAction, C4Real tx, C4Real ty, C4Object *pTargetObj,
    int32_t iObjectNum, int32_t *pObjects, const char *szScript, bool drag_finished)
		: eAction(eAction), tx(tx), ty(ty), iTargetObj(pTargetObj ? pTargetObj->Number : 0),
		iObjectNum(iObjectNum), pObjects(pObjects), StringParam(szScript, true), drag_finished(drag_finished)
{

}

C4ControlEMMoveObject *C4ControlEMMoveObject::CreateObject(const C4ID &id, C4Real x, C4Real y, C4Object *container)
{
#ifdef WITH_QT_EDITOR
	::StartSoundEffect("UI::Click2");
#endif
	auto ctl = new C4ControlEMMoveObject(EMMO_Create, x, y, container);
	ctl->StringParam = id.ToString();
	return ctl;
}

C4ControlEMMoveObject::~C4ControlEMMoveObject()
{
	delete [] pObjects; pObjects = nullptr;
}

void C4ControlEMMoveObject::MoveObject(C4Object *moved_object, bool move_forced) const
{
	// move given object by this->tx/ty and do callbacks
	if (!moved_object || !moved_object->Status) return;
	int32_t old_x = moved_object->GetX(), old_y = moved_object->GetY();
	C4Real tx = this->tx;
	if (moved_object->Def->NoHorizontalMove && !move_forced) tx = Fix0;
	moved_object->ForcePosition(moved_object->fix_x + tx, moved_object->fix_y + ty);
	moved_object->xdir = moved_object->ydir = 0;
	moved_object->Mobile = false;
	C4AulParSet pars(C4VInt(old_x), C4VInt(old_y), C4VBool(drag_finished));
	if (moved_object->Call(PSF_EditCursorMoved, &pars))
	{
		::Console.EditCursor.InvalidateSelection();
	}
}

void C4ControlEMMoveObject::Execute() const
{
	bool fLocalCall = LocalControl();
	switch (eAction)
	{
	case EMMO_Move:
	case EMMO_MoveForced:
	{
		if (!pObjects) break;
		// move all given objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				if (pObj->Status)
				{
					MoveObject(pObj, eAction == EMMO_MoveForced);
					// attached objects: Also move attachment target
					while (pObj->GetProcedure() == DFA_ATTACH)
					{
						pObj = pObj->Action.Target;
						if (!pObj) break; // leftover action cancelled next frame
						for (int j = 0; j < iObjectNum; ++j) if (pObjects[j] == pObj->Number) { pObj = nullptr; break; } // ensure we aren't moving twice
						if (!pObj) break;
						MoveObject(pObj, eAction==EMMO_MoveForced);
					}
				}
	}
	break;
	case EMMO_Enter:
	{
		if (!pObjects) break;
		// enter all given objects into target
		C4Object *pObj, *pTarget = ::Objects.SafeObjectPointer(iTargetObj);
		if (pTarget)
			for (int i=0; i<iObjectNum; ++i)
				if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				{
					pObj->Enter(pTarget);
					if (!pTarget->Status) break;
					C4AulParSet pars(C4VObj(pObj));
					if (pObj && pObj->Status && pTarget->Status) pTarget->Call(P_EditorCollection, &pars);
					if (!pTarget->Status) break;
				}
	}
	break;
	case EMMO_Duplicate:
	{
		if (!pObjects) break;
		// perform duplication
		::Console.EditCursor.PerformDuplication(pObjects, iObjectNum, fLocalCall);
	}
	break;
	case EMMO_Script:
	{
		if (!pObjects) return;
		// execute script ...
		C4ControlScript ScriptCtrl(StringParam.getData(), C4ControlScript::SCOPE_Global);
		ScriptCtrl.SetByClient(iByClient);
		// ... for each object in selection
		for (int i=0; i<iObjectNum; ++i)
		{
			ScriptCtrl.SetTargetObj(pObjects[i]);
			ScriptCtrl.Execute();
		}
	}
	break;
	case EMMO_Remove:
	{
		if (!pObjects) return;
		// remove all objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				pObj->AssignRemoval();
	}
	break;
	case EMMO_Exit:
	{
		if (!pObjects) return;
		// exit all objects
		C4Object *pObj;
		for (int i=0; i<iObjectNum; ++i)
			if ((pObj = ::Objects.SafeObjectPointer(pObjects[i])))
				pObj->Exit(pObj->GetX(), pObj->GetY(), pObj->GetR());
	}
	break;
	case EMMO_Create:
	{
		// Check max object count
		C4ID iddef = C4ID(StringParam);
		C4Def *def = C4Id2Def(iddef);
		if (!def) return;
		int32_t placement_limit = def->GetPropertyInt(P_EditorPlacementLimit);
		if (placement_limit)
		{
			if (Game.ObjectCount(iddef) >= placement_limit)
			{
				// Too many objects
				::Console.Message(FormatString(LoadResStr("IDS_CNS_CREATORTOOMANYINSTANCES"), int(placement_limit)).getData());
				return;
			}
		}
		// Create object outside or contained
		// If container is desired but not valid, do nothing (don't create object outside instead)
		C4Object *container = nullptr;
		if (iTargetObj)
		{
			container = ::Objects.SafeObjectPointer(iTargetObj);
			if (!container || !container->Status) return;
		}
		bool create_centered = false;
#ifdef WITH_QT_EDITOR
		// Qt editor: Object creation is done through creator; centered creation is usually more convenient
		create_centered = true;
#endif
		C4Object *obj = ::Game.CreateObject(iddef, nullptr, NO_OWNER, fixtoi(tx), fixtoi(ty), 0, create_centered);
		if (container && obj && container->Status && obj->Status)
		{
			obj->Enter(container);
			C4AulParSet pars(C4VObj(obj));
			if (obj && obj->Status) container->Call(P_EditorCollection, &pars);
		}
		if (obj && obj->Status) obj->Call(P_EditorInitialize); // specific initialization when placed in editor
	}
	break;
	case EMMO_Transform:
	{
		C4Object *pTarget = ::Objects.SafeObjectPointer(iTargetObj);
		if (pTarget)
		{
			int32_t new_rot = fixtoi(this->tx, 1);
			int32_t new_con = fixtoi(this->ty, FullCon/100);
			if (pTarget->Def->Rotateable) pTarget->SetRotation(new_rot);
			if (pTarget->Def->GrowthType) pTarget->DoCon(new_con - pTarget->GetCon(), false);
		}
	}
	}
	// update property dlg & status bar
	if (fLocalCall && eAction != EMMO_Move && eAction != EMMO_MoveForced)
		Console.EditCursor.OnSelectionChanged();
}

void C4ControlEMMoveObject::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eAction), "Action"));
	pComp->Value(mkNamingAdapt(tx, "tx", 0));
	pComp->Value(mkNamingAdapt(ty, "ty", 0));
	pComp->Value(mkNamingAdapt(iTargetObj, "TargetObj", -1));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iObjectNum), "ObjectNum", 0));
	if (pComp->isDeserializer()) { delete [] pObjects; pObjects = new int32_t [iObjectNum]; }
	pComp->Value(mkNamingAdapt(mkArrayAdapt(pObjects, iObjectNum), "Objs", -1));
	if (eAction == EMMO_Script)
		pComp->Value(mkNamingAdapt(StringParam, "Script", ""));
	else if (eAction == EMMO_Create)
		pComp->Value(mkNamingAdapt(StringParam, "ID", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlEMDrawTool

C4ControlEMDrawTool::C4ControlEMDrawTool(C4ControlEMDrawAction eAction, LandscapeMode iMode,
    int32_t iX, int32_t iY, int32_t iX2, int32_t iY2, int32_t iGrade,
    const char *szMaterial, const char *szTexture, const char *szBackMaterial, const char *szBackTexture)
		: eAction(eAction), iMode(iMode), iX(iX), iY(iY), iX2(iX2), iY2(iY2), iGrade(iGrade),
		Material(szMaterial, true), Texture(szTexture, true),
		BackMaterial(szBackMaterial, true), BackTexture(szBackTexture, true)
{

}

void C4ControlEMDrawTool::Execute() const
{
	// set new mode
	if (eAction == EMDT_SetMode)
	{
		Console.ToolsDlg.SetLandscapeMode(iMode, iX==1, true);
		return;
	}
	// check current mode
	assert(::Landscape.GetMode() == iMode);
	if (::Landscape.GetMode() != iMode) return;
	// assert validity of parameters
	if (!Material.getSize()) return;
	const char *szMaterial = Material.getData(),
	           *szTexture = Texture.getData();
	const char *szBackMaterial = BackMaterial.getData(),
	           *szBackTexture = BackTexture.getData();
	// perform action
	switch (eAction)
	{
	case EMDT_Brush: // brush tool
		if (!Texture.getSize()) break;
		::Landscape.DrawBrush(iX, iY, iGrade, szMaterial, szTexture, szBackMaterial, szBackTexture);
		break;
	case EMDT_Line: // line tool
		if (!Texture.getSize()) break;
		::Landscape.DrawLine(iX,iY,iX2,iY2, iGrade, szMaterial, szTexture, szBackMaterial, szBackTexture);
		break;
	case EMDT_Rect: // rect tool
		if (!Texture.getSize()) break;
		::Landscape.DrawBox(iX,iY,iX2,iY2, iGrade, szMaterial, szTexture, szBackMaterial, szBackTexture);
		break;
	case EMDT_Fill: // fill tool
	{
		int iMat = ::MaterialMap.Get(szMaterial);
		if (!MatValid(iMat)) return;
		for (int cnt=0; cnt<iGrade; cnt++)
		{
			int32_t itX=iX+Random(iGrade)-iGrade/2;
			int32_t itY=iY+Random(iGrade)-iGrade/2;
			::Landscape.InsertMaterial(iMat,&itX,&itY);
		}
	}
	break;
	default:
		break;
	}
}


void C4ControlEMDrawTool::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eAction), "Action"));
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(iMode), "Mode", LandscapeMode::Undefined));
	pComp->Value(mkNamingAdapt(iX, "X", 0));
	pComp->Value(mkNamingAdapt(iY, "Y", 0));
	pComp->Value(mkNamingAdapt(iX2, "X2", 0));
	pComp->Value(mkNamingAdapt(iY2, "Y2", 0));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iGrade),  "Grade", 0));
	pComp->Value(mkNamingAdapt(Material, "Material", ""));
	pComp->Value(mkNamingAdapt(Texture, "Texture", ""));
	pComp->Value(mkNamingAdapt(BackMaterial, "BackMaterial", ""));
	pComp->Value(mkNamingAdapt(BackTexture, "BackTexture", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlMessage

void C4ControlMessage::Execute() const
{
	const char *szMessage = Message.getData();
	// get player
	C4Player *pPlr = (iPlayer < 0 ? nullptr : ::Players.Get(iPlayer));
	// security
	if (pPlr && pPlr->AtClient != iByClient) return;
	// do not record message as control, because it is not synced!
	// get lobby to forward to
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	StdStrBuf str;
	switch (eType)
	{
	case C4CMT_Normal:
	case C4CMT_Me:
		// log it
		if (pPlr)
		{
			if (pPlr->AtClient != iByClient) break;
			str.Format((eType == C4CMT_Normal ? "<c %x><<i></i>%s> %s</c>" : "<c %x> * %s %s</c>"),
			           pPlr->ColorDw, pPlr->GetName(), szMessage);
		}
		else
		{
			C4Client *pClient = Game.Clients.getClientByID(iByClient);
			str.Format((eType == C4CMT_Normal ? "<%s> %s" : " * %s %s"),
			           pClient ? pClient->getNick() : "???", szMessage);
		}
		// to lobby
		if (pLobby)
			pLobby->OnMessage(Game.Clients.getClientByID(iByClient), str.getData());
#ifndef USE_CONSOLE
		else
#endif
		// to log
		Log(str.getData());
		break;

	case C4CMT_Say:
		// show as game message above player cursor
		if (pPlr && pPlr->Cursor)
		{
			if (Game.C4S.Head.Film == C4SFilm_Cinematic)
			{
				StdStrBuf sMessage; sMessage.Format("<%s> %s", pPlr->Cursor->GetName(), szMessage);
				uint32_t dwClr = pPlr->Cursor->Color;
				if (!dwClr) dwClr = 0xff;
				GameMsgObjectDw(sMessage.getData(), pPlr->Cursor, dwClr|0xff000000);
			}
			else
				GameMsgObjectDw(szMessage, pPlr->Cursor, pPlr->ColorDw|0xff000000);
		}
		break;

	case C4CMT_Team:
	{
		// show only if sending player is allied with a local one
		if (pPlr)
		{
			// for running game mode, check actual hostility
			C4Player *pLocalPlr;
			for (int cnt = 0; (pLocalPlr = ::Players.GetLocalByIndex(cnt)); cnt++)
				if (!Hostile(pLocalPlr->Number, iPlayer))
					break;
			if (pLocalPlr) Log(FormatString("<c %x>{%s} %s</c>", pPlr->ColorDw, pPlr->GetName(), szMessage).getData());
		}
		else if (pLobby)
		{
			// in lobby mode, no player has joined yet - check teams of unjoined players
			if (!Game.PlayerInfos.HasSameTeamPlayers(iByClient, Game.Clients.getLocalID())) break;
			// OK - permit message
			C4Client *pClient = Game.Clients.getClientByID(iByClient);
			pLobby->OnMessage(Game.Clients.getClientByID(iByClient),
			                  FormatString("{%s} %s", pClient ? pClient->getNick() : "???", szMessage).getData());
		}
	}
	break;

	case C4CMT_Private:
	{
		if (!pPlr) break;
		// show only if the target player is local
		C4Player *pLocalPlr;
		for (int cnt = 0; (pLocalPlr = ::Players.GetLocalByIndex(cnt)); cnt++)
			if (pLocalPlr->ID == iToPlayer)
				break;
		if (pLocalPlr)
		{
			Log(FormatString("<c %x>[%s] %s</c>", pPlr->ColorDw, pPlr->GetName(), szMessage).getData());
		}
	}
	break;

	case C4CMT_Sound:
	{
		// tehehe, sound!
		C4Client *singer = Game.Clients.getClientByID(iByClient);
		if (!singer || !singer->IsIgnored())
			if (!StartSoundEffect(szMessage, false, 100, nullptr))
				// probably wrong sound file name
				break;
		// Sound icon even if someone you ignored just tried. So you know you still need to ignore.
		if (pLobby) pLobby->OnClientSound(singer);
		if (C4Network2ClientListDlg::GetInstance()) C4Network2ClientListDlg::GetInstance()->OnSound(singer);
		break;
	}

	case C4CMT_Alert:
		// notify inactive users
		Application.NotifyUserIfInactive();
		break;

	case C4CMT_System:
		// sender must be host
		if (!HostControl()) break;
		// show
		LogF("Network: %s", szMessage);
		break;

	}
}

void C4ControlMessage::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", C4CMT_Normal));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlayer), "Player", -1));
	if (eType == C4CMT_Private)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iToPlayer), "ToPlayer", -1));
	pComp->Value(mkNamingAdapt(Message, "Message", ""));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlPlayerInfo

void C4ControlPlayerInfo::Execute() const
{
	// join to player info list
	// replay and local control: direct join
	if (::Control.isReplay() || !::Control.isNetwork())
	{
		// add info directly
		Game.PlayerInfos.AddInfo(new C4ClientPlayerInfos(PlrInfo));
		// make sure team list reflects teams set in player infos
		Game.Teams.RecheckPlayers();
		// replay: actual player join packet will follow
		// offline game: Issue the join
		if (::Control.isLocal())
			Game.PlayerInfos.LocalJoinUnjoinedPlayersInQueue();
	}
	else
		// network:
		::Network.Players.HandlePlayerInfo(PlrInfo);
}

void C4ControlPlayerInfo::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(PlrInfo);
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlRemovePlr

void C4ControlRemovePlr::Execute() const
{
	// host only
	if (iByClient != C4ClientIDHost) return;
	// remove
	::Players.Remove(iPlr, fDisconnected, false);
}

void C4ControlRemovePlr::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iPlr), "Plr", -1));
	pComp->Value(mkNamingAdapt(fDisconnected, "Disconnected", false));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlDebugRec

void C4ControlDebugRec::Execute() const
{

}

void C4ControlDebugRec::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(Data);
}

// *** C4ControlVote

StdStrBuf C4ControlVote::getDesc() const
{
	// Describe action
	StdStrBuf Action;
	switch (eType)
	{
	case VT_Cancel:
		Action = LoadResStr("IDS_VOTE_CANCELTHEROUND"); break;
	case VT_Kick:
		if (iData == iByClient)
			Action = LoadResStr("IDS_VOTE_LEAVETHEGAME");
		else
		{
			C4Client *pTargetClient = Game.Clients.getClientByID(iData);
			Action.Format(LoadResStr("IDS_VOTE_KICKCLIENT"), pTargetClient ? pTargetClient->getName() : "???");
		}
		break;
	case VT_Pause:
		if (iData)
			Action = LoadResStr("IDS_TEXT_PAUSETHEGAME");
		else
			Action = LoadResStr("IDS_TEXT_UNPAUSETHEGAME");
		break;
	default:
		Action = "perform some mysterious action"; break;
	}
	return Action;
}

StdStrBuf C4ControlVote::getDescWarning() const
{
	StdStrBuf Warning;
	switch (eType)
	{
	case VT_Cancel:
		Warning = LoadResStr("IDS_TEXT_WARNINGIFTHEGAMEISCANCELL"); break;
	case VT_Kick:
		Warning = LoadResStr("IDS_TEXT_WARNINGNOLEAGUEPOINTSWILL"); break;
	default:
		Warning = ""; break;
	}
	return Warning;
}

void C4ControlVote::Execute() const
{
	// Log
	C4Client *pClient = Game.Clients.getClientByID(iByClient);
	if (fApprove)
		LogF(LoadResStr("IDS_VOTE_WANTSTO"), pClient->getName(), getDesc().getData());
	else
		LogF(LoadResStr("IDS_VOTE_DOESNOTWANTTO"), pClient->getName(), getDesc().getData());
	// Save vote back
	if (::Network.isEnabled())
		::Network.AddVote(*this);
	// Vote done?
	if (::Control.isCtrlHost())
	{
		// Count votes
		int32_t iPositive = 0, iNegative = 0, iVotes = 0;
		// If there are no teams, count as if all were in the same team
		// (which happens to be equivalent to "everyone is in his own team" here)
		for (int32_t i = 0; i < std::max<int32_t>(Game.Teams.GetTeamCount(), 1); i++)
		{
			C4Team *pTeam = Game.Teams.GetTeamByIndex(i);
			// Votes for this team
			int32_t iPositiveTeam = 0, iNegativeTeam = 0, iVotesTeam = 0;
			// Check each player
			for (int32_t j = 0; j < (pTeam ? pTeam->GetPlayerCount() : Game.PlayerInfos.GetPlayerCount()); j++)
			{
				int32_t iClientID = C4ClientIDUnknown;
				C4PlayerInfo *pNfo;
				if (!pTeam)
				{
					pNfo = Game.PlayerInfos.GetPlayerInfoByIndex(j);
					if (!pNfo) continue; // shouldn't happen
					iClientID = Game.PlayerInfos.GetClientInfoByPlayerID(pNfo->GetID())->GetClientID();
				}
				else
				{
					pNfo = Game.PlayerInfos.GetPlayerInfoByID(pTeam->GetIndexedPlayer(j), &iClientID);
					if (!pNfo) continue; // shouldn't happen
				}
				if (iClientID < 0) continue;
				// Client disconnected?
				if (!Game.Clients.getClientByID(iClientID)) continue;
				// Player eliminated or never joined?
				if (!pNfo->IsJoined()) continue;
				// Okay, this player can vote
				iVotesTeam++;
				// Search vote of this client on the subject
				C4IDPacket *pPkt; C4ControlVote *pVote;
				if ((pPkt = ::Network.GetVote(iClientID, eType, iData)))
					if ((pVote = static_cast<C4ControlVote *>(pPkt->getPkt())))
					{
						if (pVote->isApprove())
							iPositiveTeam++;
						else
							iNegativeTeam++;
					}
			}
			// Any votes available?
			if (iVotesTeam)
			{
				iVotes++;
				// Approval by team? More then 50% needed
				if (iPositiveTeam * 2 > iVotesTeam)
					iPositive++;
				// Disapproval by team? More then 50% needed
				else if (iNegativeTeam * 2 >= iVotesTeam)
					iNegative++;
			}
		}
		// Approval? More then 50% needed
		if (iPositive * 2 > iVotes)
			::Control.DoInput(CID_VoteEnd,
			                  new C4ControlVoteEnd(eType, true, iData),
			                  CDT_Sync);
		// Disapproval?
		else if (iNegative * 2 >= iVotes)
			::Control.DoInput(CID_VoteEnd,
			                  new C4ControlVoteEnd(eType, false, iData),
			                  CDT_Sync);
	}
}

void C4ControlVote::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(eType), "Type", VT_None));
	pComp->Value(mkNamingAdapt(fApprove, "Approve", true));
	pComp->Value(mkNamingAdapt(iData, "Data", 0));
	C4ControlPacket::CompileFunc(pComp);
}

// *** C4ControlVoteEnd

void C4ControlVoteEnd::Execute() const
{
	// End the voting process
	if (!HostControl()) return;
	if (::Network.isEnabled())
		::Network.EndVote(getType(), isApprove(), getData());
	// Log
	StdStrBuf sMsg;
	if (isApprove())
		sMsg.Format(LoadResStr("IDS_TEXT_ITWASDECIDEDTO"), getDesc().getData());
	else
		sMsg.Format(LoadResStr("IDS_TEXT_ITWASDECIDEDNOTTO"), getDesc().getData());
	Log(sMsg.getData());
	// Approved?
	if (!isApprove()) return;
	// Do it
	C4ClientPlayerInfos *pInfos; C4PlayerInfo *pInfo;
	int iClient, iInfo;
	switch (getType())
	{
	case VT_Cancel:
		// Flag players
		if (!Game.GameOver)
			for (iClient = 0; (pInfos = Game.PlayerInfos.GetIndexedInfo(iClient)); iClient++)
				for (iInfo = 0; (pInfo = pInfos->GetPlayerInfo(iInfo)); iInfo++)
					if (!pInfo->IsRemoved())
						pInfo->SetVotedOut();
		// Abort the game
		Game.Abort(true);
		break;
	case VT_Kick:
		// Flag players
		pInfos = Game.PlayerInfos.GetInfoByClientID(getData());
		if (!Game.GameOver)
			if (pInfos)
				for (iInfo = 0; (pInfo = pInfos->GetPlayerInfo(iInfo)); iInfo++)
					if (!pInfo->IsRemoved())
						pInfo->SetVotedOut();
		// Remove the client
		if (::Control.isCtrlHost())
		{
			C4Client *pClient = Game.Clients.getClientByID(getData());
			if (pClient)
				Game.Clients.CtrlRemove(pClient, LoadResStr("IDS_VOTE_VOTEDOUT"));
		}
		// It is ourselves that have been voted out?
		if (getData() == Game.Clients.getLocalID())
		{
			// otherwise, we have been kicked by the host.
			// Do a regular disconnect and display reason in game over dialog, so the client knows what has happened!
			Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, FormatString(LoadResStr("IDS_ERR_YOUHAVEBEENREMOVEDBYVOTIN"), sMsg.getData()).getData());
			::Network.Clear();
			// Game over immediately, so poor player won't continue game alone
			Game.DoGameOver();
		}
		break;
	default:
		// TODO
		break;
	}
}

void C4ControlVoteEnd::CompileFunc(StdCompiler *pComp)
{
	C4ControlVote::CompileFunc(pComp);
}


// *** C4ControlReInitScenario

C4ControlReInitScenario::C4ControlReInitScenario()
{
	// Create a temp file with the scenario files to be loaded as a section
	char *tmp_fn = const_cast<char *>(Config.AtTempPath("ReinitSectionSave.ocs"));
	MakeTempFilename(tmp_fn);
	C4Group grp;
	grp.Open(tmp_fn, true);
	StdBuf buf;
	bool success = true;
	const char *section_components[] = { C4CFN_ScenarioCore, C4CFN_ScenarioObjects, C4CFN_ScenarioObjectsScript, C4CFN_Map, C4CFN_MapFg, C4CFN_MapBg };
	for (const char *section_component : section_components)
	{
		if (::Game.ScenarioFile.LoadEntry(section_component, &buf))
		{
			if (!grp.Add(section_component, buf, false, true)) success = false;
		}
		buf.Clear();
	}
	if (!grp.Save(false)) success = false;
	if (!success) return;
	// Move into buffer to be sent via queue
	success = data.LoadFromFile(tmp_fn);
	EraseFile(tmp_fn);
	if (!success) return;
}

void C4ControlReInitScenario::CompileFunc(StdCompiler *comp)
{
	comp->Value(data);
}

void C4ControlReInitScenario::Execute() const
{
	// Valid?
	if (!data.getSize()) return;
	// Store section group to temp file
	char *tmp_fn = const_cast<char *>(Config.AtTempPath("ReinitSection.ocs"));
	MakeTempFilename(tmp_fn);
	if (!data.SaveToFile(tmp_fn)) return;
	// Group to section
	const char *reinit_section_name = "EditorReloadSection";
	if (!::Game.CreateSectionFromTempFile(reinit_section_name, tmp_fn))
	{
		EraseFile(tmp_fn);
		return;
	}
	// Load that section!
	::Game.LoadScenarioSection(reinit_section_name, C4S_REINIT_SCENARIO);
}

void C4ControlEditGraph::CompileFunc(StdCompiler *comp)
{
	comp->Value(mkNamingAdapt(path, "Path", StdCopyStrBuf()));
	comp->Value(mkNamingAdapt(mkIntAdaptT<uint8_t>(action), "Action", CEG_None));
	comp->Value(mkNamingAdapt(index, "Index", -1));
	comp->Value(mkNamingAdapt(x, "x", 0));
	comp->Value(mkNamingAdapt(y, "y", 0));
	C4ControlPacket::CompileFunc(comp);
}

void C4ControlEditGraph::Execute() const
{
	// Forward to console for execution
	::Console.EditGraphControl(this);
}
