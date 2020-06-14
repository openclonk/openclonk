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
#include "script/C4Aul.h"
#include "script/C4AulDefFunc.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4PlayerScript.h"

static long FnGetColor(C4Player * player)
{
	return player->ColorDw;
}

static long FnEliminate(C4Player * player, bool remove_direct)
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

static bool FnSurrender(C4Player * player)
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
		F(GetColor);
		F(Eliminate);
		F(Surrender);
	#undef F
	prototype->Freeze();
}
