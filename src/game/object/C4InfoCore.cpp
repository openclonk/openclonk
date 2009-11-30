/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003, 2006-2007  Matthes Bender
 * Copyright (c) 2004, 2006  Günther Brammer
 * Copyright (c) 2005-2008  Sven Eberhardt
 * Copyright (c) 2006  Peter Wortmann
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

/* Structures for object and player info components */

#include <C4Include.h>
#include <C4InfoCore.h>

#include <C4Random.h>
#include <C4RankSystem.h>
#include <C4Group.h>
#include <C4Components.h>
#include <C4Game.h>
#include <C4GameObjects.h>

#include <C4Random.h>


//------------------------------- Player Info ----------------------------------------

C4PlayerInfoCore::C4PlayerInfoCore()
  {
  ZeroMem(this,sizeof(C4PlayerInfoCore));
	Default();
  }

void C4PlayerInfoCore::Default(C4RankSystem *pRanks)
	{
	ZeroMem(this,sizeof(C4PlayerInfoCore));
	Rank=0;
	SCopy("Neuling",PrefName);
	if (pRanks) SCopy(pRanks->GetRankName(Rank,false).getData(),RankName);
	else SCopy("Rang",RankName);
	PrefColor=0;
	PrefColorDw=0xff;
	PrefColor2Dw=0;
	OldPrefControl=C4P_Control_Keyboard1;
	PrefPosition=0;
	PrefMouse=1;
	OldPrefControlStyle = 0;
	PrefAutoContextMenu = 0;
	OldPrefAutoContextMenu = 0;
	PrefControl.Clear();
	ExtraData.Reset();
	}

DWORD C4PlayerInfoCore::GetPrefColorValue(int32_t iPrefColor)
	{
	DWORD valRGB[12] = { 0xFF0000E8, 0xFFF40000, 0xFF00C800, 0xFFFCF41C,
	                     0xFFC48444, 0xFF784830, 0xFFA04400, 0xFFF08050,
	                     0xFF848484, 0xFFFFFFFF, 0xFF0094F8, 0xFFBC00C0 };
	if (Inside<int32_t>(iPrefColor, 0, 11))
		return valRGB[iPrefColor];
		//return RGB(valRGB[iPrefColor] >> 16, valRGB[iPrefColor] >> 8, valRGB[iPrefColor] >> 0);
	return 0xFFAAAAAA;
	}

bool C4PlayerInfoCore::Load(C4Group &hGroup)
  {
	// New version
	StdStrBuf Source;
	if (hGroup.LoadEntryString(C4CFN_PlayerInfoCore,Source))
		{
		// Compile
		StdStrBuf GrpName = hGroup.GetFullName(); GrpName.Append(DirSep C4CFN_PlayerInfoCore);
		if(!CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, Source, GrpName.getData()))
			return false;
		// Pref for AutoContextMenus is still undecided: default by player's control style
		if (OldPrefAutoContextMenu == -1)
			OldPrefAutoContextMenu = OldPrefControlStyle;
		// Determine true color from indexed pref color
		if (!PrefColorDw)
			PrefColorDw = GetPrefColorValue(PrefColor);
		// Validate colors
		PrefColorDw &= 0xffffff;
		PrefColor2Dw &= 0xffffff;
		// Validate name
		CMarkup::StripMarkup(PrefName);
		// Success
		return true;
		}

	// Old version no longer supported - sorry
  return false;
  }

bool C4PlayerInfoCore::Save(C4Group &hGroup)
  {
	StdStrBuf Source, Name = hGroup.GetFullName(); Name.Append(DirSep C4CFN_PlayerInfoCore);
	if(!DecompileToBuf_Log<StdCompilerINIWrite>(*this, &Source, Name.getData()))
		return false;
	if (!hGroup.Add(C4CFN_PlayerInfoCore,Source,false,true))
		return false;
	hGroup.Delete("C4Player.c4b");
	return true;
	}

void C4PlayerInfoCore::Promote(int32_t iRank, C4RankSystem &rRanks)
	{
	Rank=iRank;
	SCopy(rRanks.GetRankName(Rank,true).getData(),RankName,C4MaxName);
	}

bool C4PlayerInfoCore::CheckPromotion(C4RankSystem &rRanks)
	{
	if (Score>=rRanks.Experience(Rank+1))
		{
		Promote(Rank+1,rRanks);
		return true;
		}
	return false;
	}

void C4PlayerInfoCore::CompileFunc(StdCompiler *pComp)
	{

	pComp->Name("Player");
	pComp->Value(mkNamingAdapt(toC4CStr(PrefName),"Name",									"Neuling"));
	pComp->Value(mkNamingAdapt(toC4CStr(Comment),	"Comment",							""));
	pComp->Value(mkNamingAdapt(Rank,							"Rank",									0));
	pComp->Value(mkNamingAdapt(toC4CStr(RankName),"RankName",							/*pRanks ? pRanks->Name(Rank,false) :*/ LoadResStr("IDS_MSG_RANK"))); // TODO: check if this would be desirable
	pComp->Value(mkNamingAdapt(Score,							"Score",								0));
	pComp->Value(mkNamingAdapt(Rounds,						"Rounds",								0));
	pComp->Value(mkNamingAdapt(RoundsWon,					"RoundsWon",						0));
	pComp->Value(mkNamingAdapt(RoundsLost,				"RoundsLost",						0));
	pComp->Value(mkNamingAdapt(TotalPlayingTime,	"TotalPlayingTime",			0));
	pComp->Value(mkNamingAdapt(ExtraData,					"ExtraData",						C4ValueMapData()));
	pComp->Value(mkNamingAdapt(toC4CStr(LeagueName),"LeagueName",					""));
	pComp->NameEnd();

	pComp->Name("Preferences");
	pComp->Value(mkNamingAdapt(PrefColor,                "Color",            0));
	pComp->Value(mkNamingAdapt(PrefColorDw,              "ColorDw",          0xffu));
	pComp->Value(mkNamingAdapt(PrefColor2Dw,             "AlternateColorDw", 0u));
	pComp->Value(mkNamingAdapt(PrefPosition,             "Position",         0));
	pComp->Value(mkNamingAdapt(PrefAutoContextMenu,      "AutoContextMenu2", 0)); // temp solution - to be removed
	pComp->Value(mkNamingAdapt(PrefMouse,                "Mouse",            1));
	pComp->Value(mkNamingAdapt(OldPrefControl,           "Control",          C4P_Control_Keyboard2));
	pComp->Value(mkNamingAdapt(OldPrefControlStyle,      "AutoStopControl",  0));
	pComp->Value(mkNamingAdapt(OldPrefAutoContextMenu,   "AutoContextMenu",  -1)); // compiling default is -1  (if this is detected, AutoContextMenus will be defaulted by control style)
	pComp->Value(mkNamingAdapt(PrefControl,              "ControlSet",       StdStrBuf()));
	pComp->NameEnd();

	pComp->Value(mkNamingAdapt(LastRound,                "LastRound"));

	}

//------------------------------- Physical Info ----------------------------------------

struct C4PhysInfoNameMap_t { const char *szName; C4PhysicalInfo::Offset off; } C4PhysInfoNameMap[] = {
	{ "Energy",          &C4PhysicalInfo::Energy          },
	{ "Breath",          &C4PhysicalInfo::Breath          },
	{ "Walk",            &C4PhysicalInfo::Walk            },
	{ "Jump",            &C4PhysicalInfo::Jump            },
	{ "Scale",           &C4PhysicalInfo::Scale           },
	{ "Hangle",          &C4PhysicalInfo::Hangle          },
	{ "Dig",             &C4PhysicalInfo::Dig             },
	{ "Swim",            &C4PhysicalInfo::Swim            },
	{ "Throw",           &C4PhysicalInfo::Throw           },
	{ "Push",            &C4PhysicalInfo::Push            },
	{ "Fight",           &C4PhysicalInfo::Fight           },
	{ "Magic",           &C4PhysicalInfo::Magic           },
	{ "Float",           &C4PhysicalInfo::Float           },
	{ "CanScale",        &C4PhysicalInfo::CanScale        },
	{ "CanHangle",       &C4PhysicalInfo::CanHangle       },
	{ "CanDig",          &C4PhysicalInfo::CanDig          },
	{ "CanConstruct",    &C4PhysicalInfo::CanConstruct    },
	{ "CanChop",         &C4PhysicalInfo::CanChop         },
	{ "CanSwimDig",      &C4PhysicalInfo::CanSwimDig      },
	{ "CanFly",          &C4PhysicalInfo::CanFly          },
	{ "CorrosionResist", &C4PhysicalInfo::CorrosionResist },
	{ "BreatheWater",    &C4PhysicalInfo::BreatheWater    },
	{ NULL, NULL } };

void C4PhysicalInfo::PromotionUpdate(int32_t iRank, bool fUpdateTrainablePhysicals, C4Def *pTrainDef)
	{
	if (iRank>=0) { CanDig=1; CanChop=1; CanConstruct=1; }
	if (iRank>=0) { CanScale=1; }
	if (iRank>=0) { CanHangle=1; }
	Energy= Max<int32_t>( Energy, (50+5*BoundBy<int32_t>(iRank,0,10)) *C4MaxPhysical/100 );
	if (fUpdateTrainablePhysicals && pTrainDef)
		{
		// do standard training: Expect everything to be trained fully at rank 20
		int32_t iTrainRank = BoundBy<int32_t>(iRank, 0,20);
		Scale = pTrainDef->Physical.Scale + (C4MaxPhysical - pTrainDef->Physical.Scale) * iTrainRank / 20;
		Hangle = pTrainDef->Physical.Hangle + (C4MaxPhysical - pTrainDef->Physical.Hangle) * iTrainRank / 20;
		Swim = pTrainDef->Physical.Swim + (C4MaxPhysical - pTrainDef->Physical.Swim) * iTrainRank / 20;
		Fight = pTrainDef->Physical.Fight + (C4MaxPhysical - pTrainDef->Physical.Fight) * iTrainRank / 20;
		// do script updates for any physicals as required (this will train stuff like magic)
		const char *szPhysName; C4PhysicalInfo::Offset PhysOff;
		for (int32_t iPhysIdx=0; szPhysName = GetNameByIndex(iPhysIdx, &PhysOff); ++iPhysIdx)
			{
			C4Value PhysVal(this->*PhysOff);
			C4AulParSet Pars(C4VString(szPhysName), C4VInt(iRank), C4VRef(&PhysVal));
			if (!!pTrainDef->Script.Call(PSF_GetFairCrewPhysical, 0, &Pars))
				{
				this->*PhysOff = PhysVal.getInt();
				}
			}
		}
	}

C4PhysicalInfo::C4PhysicalInfo()
	{
	Default();
	}

void C4PhysicalInfo::Default()
	{
	ZeroMem(this,sizeof(C4PhysicalInfo));
	}

bool C4PhysicalInfo::GetOffsetByName(const char *szPhysicalName, Offset *pmpiOut)
	{
	// query map
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (SEqual(entry->szName, szPhysicalName))
			{
			*pmpiOut = entry->off;
			return true;
			}
	return false;
	}

const char *C4PhysicalInfo::GetNameByOffset(Offset mpiOff)
	{
	// query map
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (entry->off == mpiOff)
			return entry->szName;
	return NULL;
	}

const char *C4PhysicalInfo::GetNameByIndex(int32_t iIdx, Offset *pmpiOut)
	{
	// query map
	if (!Inside<int32_t>(iIdx, 0, sizeof(C4PhysInfoNameMap)/sizeof(C4PhysInfoNameMap_t))) return NULL;
	if (pmpiOut) *pmpiOut = C4PhysInfoNameMap[iIdx].off;
	return C4PhysInfoNameMap[iIdx].szName;
	}

void C4PhysicalInfo::CompileFunc(StdCompiler *pComp)
	{
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		pComp->Value(mkNamingAdapt((this->*(entry->off)),	entry->szName, 0));
	}

void C4PhysicalInfo::TrainValue(int32_t *piVal, int32_t iTrainBy, int32_t iMaxTrain)
	{
	// only do training if value was nonzero before (e.g., Magic for revaluated Clonks)
	if (*piVal)
		// do train value: Do not increase above maximum, but never decrease either
		*piVal = Max(Min(*piVal + iTrainBy, iMaxTrain), *piVal);
	}

void C4PhysicalInfo::Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
	{
	// train own value
	TrainValue(&(this->*mpiOffset), iTrainBy, iMaxTrain);
	}

bool C4PhysicalInfo::operator ==(const C4PhysicalInfo &cmp) const
	{
	// all fields must be equal
	for (C4PhysInfoNameMap_t *entry = C4PhysInfoNameMap; entry->szName; ++entry)
		if (this->*(entry->off) != cmp.*(entry->off))
			return false;
	return true;
	}

void C4TempPhysicalInfo::CompileFunc(StdCompiler *pComp)
	{
	C4PhysicalInfo::CompileFunc(pComp);
	pComp->Value(mkNamingAdapt( mkSTLContainerAdapt(Changes), "Changes", std::vector<C4PhysicalChange>()));
	}

void C4TempPhysicalInfo::Train(Offset mpiOffset, int32_t iTrainBy, int32_t iMaxTrain)
	{
	// train own value
	C4PhysicalInfo::Train(mpiOffset, iTrainBy, iMaxTrain);
	// train all temp values
	for (std::vector<C4PhysicalChange>::iterator i = Changes.begin(); i != Changes.end(); ++i)
		if (i->mpiOffset == mpiOffset)
			TrainValue(&(i->PrevVal), iTrainBy, iMaxTrain);
	}

bool C4TempPhysicalInfo::HasChanges(C4PhysicalInfo *pRefPhysical)
	{
	// always return true if there are temp changes
	if (!Changes.empty()) return true;
	// also return true if any value deviates from the reference
	if (pRefPhysical)
		{
		if (!(*pRefPhysical == *this)) return true;
		}
	// no change known
	return false;
		}

void C4TempPhysicalInfo::RegisterChange(C4PhysicalInfo::Offset mpiOffset)
	{
	// append physical change to list
	Changes.push_back(C4PhysicalChange(this->*mpiOffset, mpiOffset));
	}

bool C4TempPhysicalInfo::ResetPhysical(C4PhysicalInfo::Offset mpiOffset)
	{
	// search last matching physical check (should always be last if well scripted)
	for (std::vector<C4PhysicalChange>::reverse_iterator i = Changes.rbegin(); i != Changes.rend(); ++i)
		if ((*i).mpiOffset == mpiOffset)
			{
			this->*mpiOffset = (*i).PrevVal;
			Changes.erase((i+1).base());
			return true;
			}
	return false;
	}

void C4PhysicalChange::CompileFunc(StdCompiler *pComp)
	{
	// name=oldval
	char phyn[C4MaxName+1];
	const char *szPhyn = C4PhysicalInfo::GetNameByOffset(mpiOffset);
	if (szPhyn) SCopy(szPhyn, phyn, C4MaxName); else *phyn='\0';
	pComp->Value(mkStringAdapt(phyn, C4MaxName, StdCompiler::RCT_Idtf));
	if (!C4PhysicalInfo::GetOffsetByName(phyn, &mpiOffset)) pComp->excNotFound("Physical change name \"%s\" not found.");
	pComp->Seperator(StdCompiler::SEP_SET);
	pComp->Value(PrevVal);
	}

//------------------------------- Object Info ----------------------------------------

C4ObjectInfoCore::C4ObjectInfoCore()
  {
	Default();
  }

void C4ObjectInfoCore::Default(C4ID n_id,
															 C4DefList *pDefs,
															 const char *cpNames)
  {

	// Def
	C4Def *pDef=NULL;
	if (pDefs) pDef = pDefs->ID2Def(n_id);

	// Defaults
	id=n_id;
  Participation=1;
  Rank=0;
	Experience=0;
	Rounds=0;
	DeathCount=0;
	Birthday=0;
	TotalPlayingTime=0;
	SCopy("Clonk",Name,C4MaxName);
	SCopy("Clonk",TypeName,C4MaxName);
	sRankName.Copy("Clonk");
	sNextRankName.Clear();
	NextRankExp=0;
	DeathMessage[0]='\0';
	*PortraitFile=0;
	Age=0;
	ExtraData.Reset();

	// Type
	if (pDef) SCopy(pDef->GetName(),TypeName,C4MaxName);

	// Name
	if (cpNames)
		{
		SCopySegment(cpNames,Random(SCharCount(0x0A,cpNames)),Name,0x0A,C4MaxName+1);
		SClearFrontBack(Name);
		SReplaceChar(Name,0x0D,0x00);
		if (!Name[0]) SCopy("Clonk",Name,C4MaxName);
		}

	if (pDefs) UpdateCustomRanks(pDefs);

	// Physical
	Physical.Default();
	if (pDef) Physical = pDef->Physical;
	Physical.PromotionUpdate(Rank);

	// Old format

	}

void C4ObjectInfoCore::Promote(int32_t iRank, C4RankSystem &rRanks, bool fForceRankName)
	{
	Rank=iRank;
	Physical.PromotionUpdate(Rank);
	// copy new rank name if defined only, or forced to use highest defined rank for too high info ranks
	StdStrBuf sNewRank(rRanks.GetRankName(Rank,fForceRankName));
	if (sNewRank) sRankName.Copy(sNewRank);
	}

void C4ObjectInfoCore::UpdateCustomRanks(C4DefList *pDefs)
	{
	assert(pDefs);
	C4Def *pDef = pDefs->ID2Def(id);
	if (!pDef) return;
	if (pDef->pRankNames)
		{
		StdStrBuf sRank(pDef->pRankNames->GetRankName(Rank,false));
		if (sRank) sRankName.Copy(sRank);
		// next rank data
		StdStrBuf sNextRank(pDef->pRankNames->GetRankName(Rank+1,false));
		if (sNextRank)
			{
			sNextRankName.Copy(sNextRank);
			NextRankExp = pDef->pRankNames->Experience(Rank+1);
			}
		else
			{
			// no more promotion possible by custom rank system
			sNextRankName.Clear();
			NextRankExp = C4RankSystem::EXP_NoPromotion;
			}
		}
	else
		{
		// definition does not have custom rank names
		sNextRankName.Clear();
		NextRankExp = 0;
		}
	}

bool C4ObjectInfoCore::GetNextRankInfo(C4RankSystem &rDefaultRanks, int32_t *piNextRankExp, StdStrBuf *psNextRankName)
	{
	int32_t iNextRankExp;
	// custom rank assigned?
	if (NextRankExp)
		{
		iNextRankExp = NextRankExp;
		if (psNextRankName) psNextRankName->Copy(sNextRankName);
		}
	else
		{
		// no custom rank: Get from default set
		StdStrBuf sRank(rDefaultRanks.GetRankName(Rank+1, false));
		if (sRank)
			{
			iNextRankExp = rDefaultRanks.Experience(Rank+1);
			if (psNextRankName) psNextRankName->Copy(sRank);
			}
		else
			// no more promotion
			iNextRankExp = C4RankSystem::EXP_NoPromotion;
		}
	// return result
	if (piNextRankExp) *piNextRankExp = iNextRankExp;
	// return value is whether additional promotion is possible
	return iNextRankExp != C4RankSystem::EXP_NoPromotion;
	}

bool C4ObjectInfoCore::Load(C4Group &hGroup)
	{
	StdStrBuf Source;
	return hGroup.LoadEntryString(C4CFN_ObjectInfoCore, Source) &&
				 Compile(Source.getData());
	}

bool C4ObjectInfoCore::Save(C4Group &hGroup, C4DefList *pDefs)
	{
	// rank overload by def: Update any NextRank-stuff
	if (pDefs) UpdateCustomRanks(pDefs);
	char *Buffer; size_t BufferSize;
	if (!Decompile(&Buffer,&BufferSize))
		return false;
	if (!hGroup.Add(C4CFN_ObjectInfoCore,Buffer,BufferSize,false,true) )
		{ delete [] Buffer; return false; }
	return true;
	}

void C4ObjectInfoCore::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(mkC4IDAdapt(id),					"id",								C4ID_None));
	pComp->Value(mkNamingAdapt(toC4CStr(Name),					"Name",							"Clonk"));
	pComp->Value(mkNamingAdapt(toC4CStr(DeathMessage),	"DeathMessage",			""));
	pComp->Value(mkNamingAdapt(toC4CStr(PortraitFile),	"PortraitFile",			""));
	pComp->Value(mkNamingAdapt(Rank,										"Rank",							0));
	pComp->Value(mkNamingAdapt(sRankName,								"RankName",					"Clonk"));
	pComp->Value(mkNamingAdapt(sNextRankName,						"NextRankName",			""));
	pComp->Value(mkNamingAdapt(toC4CStr(TypeName),			"TypeName",					"Clonk"));
	pComp->Value(mkNamingAdapt(Participation,						"Participation",		1));
	pComp->Value(mkNamingAdapt(Experience,							"Experience",				0));
	pComp->Value(mkNamingAdapt(NextRankExp,							"NextRankExp",			0));
	pComp->Value(mkNamingAdapt(Rounds,									"Rounds",						0));
	pComp->Value(mkNamingAdapt(DeathCount,							"DeathCount",				0));
	pComp->Value(mkNamingAdapt(Birthday,								"Birthday",					0));
	pComp->Value(mkNamingAdapt(TotalPlayingTime,				"TotalPlayingTime",	0));
	pComp->Value(mkNamingAdapt(Age,											"Age",							0));
	pComp->Value(mkNamingAdapt(ExtraData,								"ExtraData",				C4ValueMapData()));

	pComp->FollowName("Physical");
	pComp->Value(Physical);
	}

bool C4ObjectInfoCore::Compile(const char *szSource)
	{
	bool ret = CompileFromBuf_LogWarn<StdCompilerINIRead>(
								mkNamingAdapt(*this, "ObjectInfo"),
								StdStrBuf(szSource),
								"ObjectInfo");
	// Do a promotion update to set physicals right
	Physical.PromotionUpdate(Rank);
	// DeathMessages are not allowed to stay forever
	if ('@' == DeathMessage[0]) DeathMessage[0] = ' ';
	return ret;
	}

bool C4ObjectInfoCore::Decompile(char **ppOutput, size_t *ipSize)
	{
	StdStrBuf Buf;
	if(!DecompileToBuf_Log<StdCompilerINIWrite>(
								mkNamingAdapt(*this, "ObjectInfo"),
								&Buf,
								"ObjectInfo"))
		{
		if(ppOutput) *ppOutput = NULL;
		if(ipSize) *ipSize = 0;
		return false;
		}
	if(ppOutput) *ppOutput = Buf.GrabPointer();
	if(ipSize) *ipSize = Buf.getSize();
	return true;
	}


//------------------------------- Round Info ------------------------------------------

C4RoundResult::C4RoundResult()
	{
	Default();
	}

void C4RoundResult::Default()
	{
	ZeroMem(this,sizeof(C4RoundResult));
	}

void C4RoundResult::CompileFunc(StdCompiler *pComp)
	{
	pComp->Value(mkNamingAdapt(Title,			"Title",							""));
	pComp->Value(mkNamingAdapt(Date,								"Date",								0u));
	pComp->Value(mkNamingAdapt(Duration,						"Duration",						0));
	pComp->Value(mkNamingAdapt(Won,									"Won",								0));
	pComp->Value(mkNamingAdapt(Score,								"Score",							0));
	pComp->Value(mkNamingAdapt(FinalScore,					"FinalScore",					0));
	pComp->Value(mkNamingAdapt(TotalScore,					"TotalScore",					0));
	pComp->Value(mkNamingAdapt(Bonus,								"Bonus",							0));
	pComp->Value(mkNamingAdapt(Level,								"Level",							0));
	}






