/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2018, The OpenClonk Team and contributors
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

#include "control/C4GameControl.h"
#include "object/C4Object.h"
#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4PlayerScript.h"


static long FnEliminate(C4Player *player, bool remove_direct)
{
	// direct removal?
	if (remove_direct)
	{
		// do direct removal (no fate)
		if (::Control.isCtrlHost())
		{
			::Players.CtrlRemove(player->ID, false);
		}
		return true;
	}
	else
	{
		// do regular elimination
		if (player->Eliminated)
		{
			return false;
		}
		player->Eliminate();
	}
	return true;
}

static long FnGetColor(C4Player *player)
{
	return player->ColorDw;
}

static C4Object *FnGetCursor(C4Player *player)
{
	return player->Cursor;
}

static C4Object *FnGetHiRank(C4Player *player)
{
	return player->GetHiRankActiveCrew();
}

static C4Object *FnGetViewCursor(C4Player *player)
{
	return player->ViewCursor ? player->ViewCursor : player->Cursor;
}

static long FnGetViewMode(C4Player *player)
{
	if (::Control.SyncMode())
	{
		return -1;
	}
	return player->ViewMode;
}

static C4Object *FnGetViewTarget(C4Player *player)
{
	if (player->ViewMode != C4PVM_Target)
	{
		return nullptr;
	}
	return player->ViewTarget;
}

static void FnResetCursorView(C4Player *player, bool immediate_position)
{
	player->ResetCursorView(immediate_position);
}

static bool FnSetCursor(C4Player *player, C4Object *target, bool no_select_arrow)
{
	if ((target && !target->Status) || (target && target->CrewDisabled))
	{
		return false;
	}
	player->SetCursor(target, !no_select_arrow);
	return true;
}

static bool FnSetViewCursor(C4Player *player, C4Object *target)
{
	player->ViewCursor = target;
	return true; // For same behaviour as in SetCursor
}

static bool FnSetViewTarget(C4Player *player, C4Object *target, bool immediate_position)
{
	player->SetViewMode(C4PVM_Target, target, immediate_position);
	return true;
}

static bool FnSurrender(C4Player *player)
{
	if (player->Eliminated)
	{
		return false;
	}
	player->Surrender();
	return true;
}

void C4PlayerScript::RegisterWithEngine(C4AulScriptEngine *engine)
{
    C4PropListStatic* prototype = new C4PropListStatic(nullptr, nullptr, ::Strings.RegString(PROTOTYPE_NAME_ENGINE));
	engine->RegisterGlobalConstant(PROTOTYPE_NAME_ENGINE, C4VPropList(prototype));
	#define F(f) ::AddFunc(prototype, #f, Fn##f)
		F(Eliminate);
		F(GetColor);
		F(GetCursor);
		F(GetHiRank);
		F(GetViewCursor);
	    F(GetViewMode);
	    F(GetViewTarget);
		F(ResetCursorView);
		F(SetCursor);
		F(SetViewCursor);
	    F(SetViewTarget);
		F(Surrender);
	#undef F
	prototype->Freeze();
}
