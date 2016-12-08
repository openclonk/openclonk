/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
// player info attribute conflict resolving
// e.g., changing colors if two players have the same
// "There must be some easier way to do it"(tm)

#include "C4Include.h"
#include "control/C4PlayerInfo.h"
#include "game/C4Game.h"
#include "control/C4Teams.h"
#include "lib/StdColors.h"
#include "lib/C4Random.h"

// number of times trying new player colors
const int32_t C4MaxPlayerColorChangeTries = 100;
const int32_t C4MaxPlayerNameChangeTries  = 100;

// *** Helpers

DWORD GenerateRandomPlayerColor(int32_t iTry) // generate a random player color for the iTry'th try
{
	// generate a random one biased towards max channel luminance
	// (for greater color difference and less gray-ish colors)
	return C4RGB(std::min<int>(UnsyncedRandom(302), 256), std::min<int>(UnsyncedRandom(302), 256), std::min<int>(UnsyncedRandom(302), 256));
}

bool IsColorConflict(DWORD dwClr1, DWORD dwClr2) // return whether dwClr1 and dwClr2 are closely together
{
	// NEW COLOR CONFLICT METHOD: u'v'-distance
	int R1 = 0xff & (dwClr1 >> 16);
	int G1 = 0xff & (dwClr1 >>  8);
	int B1 = 0xff & (dwClr1      );
	int R2 = 0xff & (dwClr2 >> 16);
	int G2 = 0xff & (dwClr2 >>  8);
	int B2 = 0xff & (dwClr2      );
	double r1=0,g1=0,b1=0,r2=0,g2=0,b2=0,x1=0,y1=0,Y1=0,x2=0,y2=0,Y2=0,u1=0,v1=0,u2=0,v2=0;
	RGB2rgb(R1, G1, B1, &r1, &g1, &b1);
	RGB2rgb(R2, G2, B2, &r2, &g2, &b2);
	rgb2xyY(r1, g1, b1, &x1, &y1, &Y1);
	rgb2xyY(r2, g2, b2, &x2, &y2, &Y2);
	xy2upvp(x1, y1, &u1, &v1);
	xy2upvp(x2, y2, &u2, &v2);
	double Y = (Y1+Y2)/2.0;
	double clrdiff = sqrt((u2-u1)*(u2-u1) + (v2-v1)*(v2-v1)) * Y*Y * 150;
	double lumdiff = (Abs<double>(Y2-Y1) / std::max<double>(Y*Y*5, 0.5)) / 0.10;
	return clrdiff + lumdiff < 1.0;
}


// conflict resolver
class C4PlayerInfoListAttributeConflictResolver
{

private:
	// info packets that need to be checked yet
	// info packets sorted by check priority, i.e. last ones checked first
	C4ClientPlayerInfos **ppCheckInfos;
	int32_t iCheckInfoCount;

	// lists to be checked against
	const C4PlayerInfoList &rPriCheckList, &rSecCheckList;
	C4ClientPlayerInfos *pSecPacket;

	// current attribute being checked
	C4PlayerInfo::Attribute eAttr;

	// handling of current packet to be adjusted
	C4ClientPlayerInfos *pResolvePacket;
	bool fAnyChange;

	// handling of current info to be adjusted
	C4PlayerInfo *pResolveInfo;
	bool fCurrentConflict, fOriginalConflict, fAlternateConflict;
	C4ClientPlayerInfos *pLowPrioOriginalConflictPacket, *pLowPrioAlternateConflictPacket;

public:
	C4PlayerInfoListAttributeConflictResolver(C4PlayerInfoList &rPriCheckList, const C4PlayerInfoList &rSecCheckList, C4ClientPlayerInfos *pSecPacket);
	~C4PlayerInfoListAttributeConflictResolver();

private:
	void ReaddInfoForCheck(C4ClientPlayerInfos *pCheckAdd); // temp readd info to be checked, because another packet reset its attribute with higher priority
	int32_t GetAttributePriorityDifference(const C4PlayerInfo *pInfo1, const C4ClientPlayerInfos *pPck1, const C4PlayerInfo *pInfo2, const C4ClientPlayerInfos *pPck2);
	bool IsAttributeConflict(const C4PlayerInfo *pInfo1, const C4PlayerInfo *pInfo2, C4PlayerInfo::AttributeLevel eLevel);
	void MarkConflicts(C4ClientPlayerInfos &rCheckPacket, bool fTestOriginal);
	void MarkConflicts(const C4PlayerInfoList &rCheckList, bool fTestOriginal);
	void ResolveInInfo();
	void ResolveInPacket();

public:
	void Resolve(); // action go!
};

// resolve function calling resolver
void C4PlayerInfoList::ResolvePlayerAttributeConflicts(C4ClientPlayerInfos *pSecPacket)
{
	C4PlayerInfoListAttributeConflictResolver r(*this, Game.RestorePlayerInfos, pSecPacket);
	r.Resolve();
}

// implementation of conflict resolver
C4PlayerInfoListAttributeConflictResolver::C4PlayerInfoListAttributeConflictResolver(C4PlayerInfoList &rPriCheckList, const C4PlayerInfoList &rSecCheckList, C4ClientPlayerInfos *pSecPacket)
		: ppCheckInfos(nullptr), iCheckInfoCount(0), rPriCheckList(rPriCheckList), rSecCheckList(rSecCheckList), pSecPacket(pSecPacket)
{
	// prepare check array
	int32_t iMaxCheckCount = rPriCheckList.GetInfoCount() + !!pSecPacket;
	if (!iMaxCheckCount) return;
	ppCheckInfos = new C4ClientPlayerInfos *[iMaxCheckCount];
	// resolve all primary packets in reverse order, so late clients have lower priority
	for (int32_t i=0; i<rPriCheckList.GetInfoCount(); ++i)
	{
		C4ClientPlayerInfos *pInfos = rPriCheckList.GetIndexedInfo(i);
		if (pInfos != pSecPacket) // skip packet if it was explicitly passed for update
			ppCheckInfos[iCheckInfoCount++] = pInfos;
		else
		{
			// if the additional packet is in the list already, it needn't be a check packed (only resolve packet)
			this->pSecPacket = nullptr;
		}
	}
	// must check sec packet first
	if (pSecPacket) ppCheckInfos[iCheckInfoCount++] = pSecPacket;
}

C4PlayerInfoListAttributeConflictResolver::~C4PlayerInfoListAttributeConflictResolver()
{
	delete [] ppCheckInfos;
}

void C4PlayerInfoListAttributeConflictResolver::ReaddInfoForCheck(C4ClientPlayerInfos *pCheckAdd)
{
	// do not add twice<
	for (int32_t i=0; i<iCheckInfoCount; ++i) if (ppCheckInfos[i] == pCheckAdd) return;
	// readd it - must have been in there before, so array is large enough
	// and it must have been at the head of the list
	ppCheckInfos[iCheckInfoCount++] = pCheckAdd;
}

int32_t C4PlayerInfoListAttributeConflictResolver::GetAttributePriorityDifference(const C4PlayerInfo *pInfo1, const C4ClientPlayerInfos *pPck1, const C4PlayerInfo *pInfo2, const C4ClientPlayerInfos *pPck2)
{
	// return info1.prio - info2.prio
	// highest priority if joined already and attributes may not be changed
	if (pInfo1->IsJoined())
		return pInfo2->IsJoined() ? 0 : +1;
	if (pInfo2->IsJoined())
		return -1;
	// Computer players lower priority than user players
	int32_t iTypeDiff = int(pInfo2->GetType()) - int(pInfo1->GetType());
	if (iTypeDiff) return iTypeDiff;
	// All unjoined: Priority by score
	int32_t iScoreDiff = pInfo1->getLeagueScore() - pInfo2->getLeagueScore();
	if (iScoreDiff) return iScoreDiff;
	// No scores known. Developers higher than others
	if (pPck1->IsDeveloperPacket())
		return pPck2->IsDeveloperPacket() ? 0 : +1;
	if (pPck2->IsDeveloperPacket())
		return -1;
	// equal priority
	return 0;
}

bool C4PlayerInfoListAttributeConflictResolver::IsAttributeConflict(const C4PlayerInfo *pInfo1, const C4PlayerInfo *pInfo2, C4PlayerInfo::AttributeLevel eLevel)
{
	// check for conflict of colors and names
	if (eAttr == C4PlayerInfo::PLRATT_Color)
	{
		uint32_t dwClr1 = pInfo1->GetColor(), dwClr2 = 0;
		switch (eLevel)
		{
		case C4PlayerInfo::PLRAL_Current: dwClr2 = pInfo2->GetColor(); break;
		case C4PlayerInfo::PLRAL_Original: dwClr2 = pInfo2->GetOriginalColor(); break;
		case C4PlayerInfo::PLRAL_Alternate: dwClr2 = pInfo2->GetAlternateColor(); break;
		}
		return IsColorConflict(dwClr1, dwClr2);
	}
	else if (eAttr == C4PlayerInfo::PLRATT_Name)
	{
		const char *szName1 = pInfo1->GetName(), *szName2 = "";
		switch (eLevel)
		{
		case C4PlayerInfo::PLRAL_Current: szName2 = pInfo2->GetName(); break;
		case C4PlayerInfo::PLRAL_Original: szName2 = pInfo2->GetOriginalName(); break;
		default: return SEqualNoCase(szName1, szName2);
		}
	}
	return false;
}

void C4PlayerInfoListAttributeConflictResolver::MarkConflicts(C4ClientPlayerInfos &rCheckPacket, bool fTestOriginal)
{
	C4PlayerInfo *pCheckAgainstInfo;
	// check current and original attribute against all player infos
	for (int32_t j=0; (pCheckAgainstInfo = rCheckPacket.GetPlayerInfo(j)); ++j)
	{
		if (pCheckAgainstInfo->IsUsingAttribute(eAttr)) if (!pResolveInfo->GetID() || pResolveInfo->GetID() != pCheckAgainstInfo->GetID()) if (pResolveInfo != pCheckAgainstInfo)
				{
					// current conflict is marked only if the checked packet has same of lower priority than the one compared to
					// if the priority is higher, the attribute shall be changed in the other, low priority info instead!
					bool fHasHigherPrio = (GetAttributePriorityDifference(pResolveInfo, pResolvePacket, pCheckAgainstInfo, &rCheckPacket) > 0);
					if (!fHasHigherPrio)
						if (IsAttributeConflict(pCheckAgainstInfo, pResolveInfo, C4PlayerInfo::PLRAL_Current))
							fCurrentConflict = true;
					if (fTestOriginal)
					{
						if (IsAttributeConflict(pCheckAgainstInfo, pResolveInfo, C4PlayerInfo::PLRAL_Original))
						{
							if (fHasHigherPrio && !fOriginalConflict && !pLowPrioOriginalConflictPacket)
							{
								// original attribute is taken by a low prio packet - do not mark an original conflict, but remember the packet
								// that's blocking it
								pLowPrioOriginalConflictPacket = &rCheckPacket;
							}
							else
							{
								// original attribute is taken by either one higher/equal priority by packet, or by two low prio packets
								// in this case, don't revert to original
								pLowPrioOriginalConflictPacket = nullptr;
								fOriginalConflict = true;
							}
						}
						if (IsAttributeConflict(pCheckAgainstInfo, pResolveInfo, C4PlayerInfo::PLRAL_Alternate))
						{
							if (fHasHigherPrio && !fAlternateConflict && !pLowPrioAlternateConflictPacket)
								pLowPrioAlternateConflictPacket = &rCheckPacket;
							else
								fAlternateConflict = true;
						}
					}
				}
	}
}

void C4PlayerInfoListAttributeConflictResolver::MarkConflicts(const C4PlayerInfoList &rCheckList, bool fTestOriginal)
{
	// mark in all infos of given list...
	for (int32_t i=0; i<rCheckList.GetInfoCount(); ++i)
		MarkConflicts(*rCheckList.GetIndexedInfo(i), fTestOriginal);
}

void C4PlayerInfoListAttributeConflictResolver::ResolveInInfo()
{
	// trial-loop for assignment of new player colors/names
	int32_t iTries = 0;
	// original/alternate conflict evaluated once only
	fOriginalConflict = false;
	fAlternateConflict = (eAttr == C4PlayerInfo::PLRATT_Name) || !pResolveInfo->GetAlternateColor(); // mark as conflict if there is no alternate color/name
	for (;;)
	{
		// check against all other player infos, and given info, too (may be redundant)
		fCurrentConflict = false;
		pLowPrioOriginalConflictPacket = pLowPrioAlternateConflictPacket = nullptr;
		MarkConflicts(rPriCheckList, !iTries);
		// check secondary list, too. But only for colors, not for names, because secondary list is Restore list
		// and colors are retained in restore while names are always taken from new joins
		if (eAttr != C4PlayerInfo::PLRATT_Name) MarkConflicts(rSecCheckList, !iTries);
		// and mark conflicts in additional packet that' sbeen passed
		if (pSecPacket) MarkConflicts(*pSecPacket, !iTries);
		// color conflict resolving
		if (eAttr == C4PlayerInfo::PLRATT_Color)
		{
			// original color free but not used?
			if (!iTries)
			{
				if (pResolveInfo->GetColor() != pResolveInfo->GetOriginalColor())
				{
					if (!fOriginalConflict)
					{
						// revert to original color!
						pResolveInfo->SetColor(pResolveInfo->GetOriginalColor());
						// in case a lower priority packet was blocking the attribute, re-check that packet
						// note that the may readd the current resolve packet, but the conflict will occur with the
						// lower priority packet in the next loop
						if (pLowPrioOriginalConflictPacket) ReaddInfoForCheck(pLowPrioOriginalConflictPacket);
						// done with this player (breaking the trial-loop)
						break;
					}
					// neither original nor alternate color used but alternate color free?
					else if (pResolveInfo->GetColor() != pResolveInfo->GetAlternateColor() && !fAlternateConflict)
					{
						// revert to alternate
						pResolveInfo->SetColor(pResolveInfo->GetAlternateColor());
						if (pLowPrioAlternateConflictPacket) ReaddInfoForCheck(pLowPrioAlternateConflictPacket);
						// done with this player (breaking the trial-loop)
						break;
					}
				}
			}
			// conflict found?
			if (!fCurrentConflict)
				// done with this player, then - break the trial-loop
				break;
			// try to get a new, unused player color
			uint32_t dwNewClr;
			if (++iTries > C4MaxPlayerColorChangeTries)
			{
				LogF(LoadResStr("IDS_PRC_NOREPLPLRCLR"), pResolveInfo->GetName() ? pResolveInfo->GetName() : "<NONAME>");
				// since there's a conflict anyway, change to original
				pResolveInfo->SetColor(pResolveInfo->GetOriginalColor());
				break;
			}
			else
				dwNewClr = GenerateRandomPlayerColor(iTries);
			pResolveInfo->SetColor(dwNewClr);
		}
		else // if (eAttr == PLRATT_Name)
		{
			// name conflict resolving
			// original name free but not used?
			if (!SEqualNoCase(pResolveInfo->GetName(), pResolveInfo->GetOriginalName()))
				if (!fOriginalConflict)
				{
					// revert to original name!
					pResolveInfo->SetForcedName(nullptr);
					if (pLowPrioOriginalConflictPacket) ReaddInfoForCheck(pLowPrioOriginalConflictPacket);
					// done with this player (breaking the trial-loop)
					break;
				}
			// conflict found?
			if (!fCurrentConflict)
				// done with this player, then - break the trial-loop
				break;
			// generate new name by appending an index
			if (++iTries > C4MaxPlayerNameChangeTries) break;
			pResolveInfo->SetForcedName(FormatString("%s (%d)", pResolveInfo->GetOriginalName(), iTries+1).getData());
		}
	}
}

void C4PlayerInfoListAttributeConflictResolver::ResolveInPacket()
{
	// check all player infos
	fAnyChange = false;
	int32_t iCheck = 0;
	while ((pResolveInfo = pResolvePacket->GetPlayerInfo(iCheck++)))
	{
		// not already joined? Joined player must not change their attributes!
		if (pResolveInfo->HasJoined()) continue;
		DWORD dwPrevColor = pResolveInfo->GetColor();
		StdStrBuf sPrevForcedName; sPrevForcedName.Copy(pResolveInfo->GetForcedName());
		// check attributes: Name and color
		for (eAttr = C4PlayerInfo::PLRATT_Color; eAttr != C4PlayerInfo::PLRATT_Last; eAttr = (C4PlayerInfo::Attribute) (eAttr+1))
		{
			if (eAttr == C4PlayerInfo::PLRATT_Color)
			{
				// no color change in savegame associations
				if (pResolveInfo->GetAssociatedSavegamePlayerID()) continue;
				// or forced team colors
				if (Game.Teams.IsTeamColors() && Game.Teams.GetTeamByID(pResolveInfo->GetTeam())) continue;
			}
			else if (eAttr == C4PlayerInfo::PLRATT_Name)
			{
				// no name change if a league name is used
				if (pResolveInfo->getLeagueAccount() && *pResolveInfo->getLeagueAccount()) continue;
			}
			// not if attributes are otherwise fixed (e.g., for script players)
			if (pResolveInfo->IsAttributesFixed()) continue;
			// resolve in this info
			ResolveInInfo();
		}
		// mark change for return value if anything was changed
		if (pResolveInfo->GetColor() != dwPrevColor || (pResolveInfo->GetForcedName() != sPrevForcedName))
			fAnyChange = true;
		// next player info check
	}
	// mark update if anything was changed
	if (fAnyChange) pResolvePacket->SetUpdated();
}

void C4PlayerInfoListAttributeConflictResolver::Resolve()
{
	// resolve in all packets in list until list is empty
	// resolving in reverse order, because highest priority first in the list
	while (iCheckInfoCount)
	{
		pResolvePacket = ppCheckInfos[--iCheckInfoCount];
		ResolveInPacket();
	}
}
