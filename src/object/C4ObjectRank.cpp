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

/* Logic for C4Object: Rank system */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "gui/C4GameMessage.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4ObjectInfo.h"
#include "platform/C4SoundSystem.h"
#include "player/C4RankSystem.h"


void C4Object::DoExperience(int32_t change)
{
	const int32_t MaxExperience = 100000000;

	if (!Info) return;

	Info->Experience=Clamp<int32_t>(Info->Experience+change,0,MaxExperience);

	// Promotion check
	if (Info->Experience<MaxExperience)
		if (Info->Experience>=::DefaultRanks.Experience(Info->Rank+1))
			Promote(Info->Rank+1, false, false);
}

bool C4Object::Promote(int32_t torank, bool exception, bool fForceRankName)
{
	if (!Info) return false;
	// get rank system
	C4Def *pUseDef = C4Id2Def(Info->id);
	C4RankSystem *pRankSys;
	if (pUseDef && pUseDef->pRankNames)
		pRankSys = pUseDef->pRankNames;
	else
		pRankSys = &::DefaultRanks;
	// always promote info
	Info->Promote(torank,*pRankSys, fForceRankName);
	// silent update?
	if (!pRankSys->GetRankName(torank,false)) return false;
	GameMsgObject(FormatString(LoadResStr("IDS_OBJ_PROMOTION"),GetName (),Info->sRankName.getData()).getData(),this);

	// call to object
	Call(PSF_Promotion);

	StartSoundEffect("UI::Trumpet",false,100,this);
	return true;
}
