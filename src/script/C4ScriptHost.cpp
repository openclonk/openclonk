/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001-2002, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2003-2005  Peter Wortmann
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2006-2007  Günther Brammer
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

/* Handles script file components (calls, inheritance, function maps) */

#include <C4Include.h>
#include <C4ScriptHost.h>

#include <C4Console.h>
#include <C4ObjectCom.h>
#include <C4Object.h>
#include <C4Game.h>
#include <C4GameObjects.h>

/*--- C4ScriptHost ---*/

C4ScriptHost::C4ScriptHost() { Default(); }
C4ScriptHost::~C4ScriptHost() { Clear(); }

void C4ScriptHost::Default()
	{
	C4AulScript::Default();
	C4ComponentHost::Default();
	}

void C4ScriptHost::Clear()
	{
	C4AulScript::Clear();
	C4ComponentHost::Clear();
	}

bool C4ScriptHost::Load(const char *szName, C4Group &hGroup, const char *szFilename,
												const char *szLanguage, C4Def *pDef, class C4LangStringTable *pLocalTable, bool fLoadTable)
	{
	// Set definition and id
	Def = pDef;
	// Base load
	bool fSuccess = C4ComponentHost::LoadAppend(szName,hGroup,szFilename,szLanguage);
	// String Table
	stringTable = pLocalTable;
	// load it if specified
	if (stringTable && fLoadTable)
		stringTable->LoadEx("StringTbl", hGroup, C4CFN_ScriptStringTbl, szLanguage);
	// set name
	ScriptName.Format("%s" DirSep "%s", hGroup.GetFullName().getData(), Filename);
	// preparse script
	MakeScript();
	// Success
	return fSuccess;
	}


void C4ScriptHost::MakeScript()
	{
	// clear prev
	Script.Clear();

	// create script
	if (stringTable)
	{
		stringTable->ReplaceStrings(Data, Script, FilePath);
	}
	else
	{
		Script.Ref(Data);
	}

	// preparse script
	Preparse();
	}

void C4ScriptHost::Close()
	{
	// Base close
	C4ComponentHost::Close();
	// Make executable script
	MakeScript();
	// Update console
	Console.UpdateInputCtrl();
	}

int32_t C4ScriptHost::GetControlMethod(int32_t com, int32_t first, int32_t second)
  {
	return ((first >> com) & 0x01) | (((second >> com) & 0x01) << 1);
  }

void C4ScriptHost::GetControlMethodMask(const char *szFunctionFormat, int32_t& first, int32_t& second)
	{
	first = second = 0;
	//int32_t iResult = 0;

	if (!Script) return;

	// Scan for com defined control functions
	int32_t iCom;
	char szFunction[256+1];
	for (iCom=0; iCom<ComOrderNum; iCom++)
		{
		sprintf( szFunction, szFunctionFormat, ComName( ComOrder(iCom) ) );
		C4AulScriptFunc* func = GetSFunc(szFunction);

		if(func)
			{
		  first  |= ((func->ControlMethod     ) & 0x01) << iCom;
			second |= ((func->ControlMethod >> 1) & 0x01) << iCom;
			}
		}
	}


C4Value C4ScriptHost::Call(const char *szFunction, C4Object *pObj, C4AulParSet *Pars, bool fPrivateCall, bool fPassError)
	{
	// get function
	C4AulScriptFunc *pFn;
	if (!(pFn = GetSFunc(szFunction, AA_PRIVATE))) return C4VNull;
	// Call code
	return pFn->Exec(pObj,Pars, fPassError);
	}

bool C4ScriptHost::ReloadScript(const char *szPath)
  {
  // this?
  if(SEqualNoCase(szPath, FilePath) || (stringTable && SEqualNoCase(szPath, stringTable->GetFilePath())))
  {
    // try reload
    char szParentPath[_MAX_PATH + 1]; C4Group ParentGrp;
    if(GetParentPath(szPath, szParentPath))
      if(ParentGrp.Open(szParentPath))
        if(Load(Name, ParentGrp, Filename, Config.General.Language, NULL, stringTable))
          return true;
  }
  // call for childs
  return C4AulScript::ReloadScript(szPath);
  }

void C4ScriptHost::SetError(const char *szMessage)
	{

	}

const char *C4ScriptHost::GetControlDesc(const char *szFunctionFormat, int32_t iCom, C4ID *pidImage, int32_t* piImagePhase)
	{
	// Compose script function
	char szFunction[256+1];
	sprintf(szFunction,szFunctionFormat,ComName(iCom));
	// Remove failsafe indicator
	if (szFunction[0]=='~') memmove(szFunction,szFunction+1,sizeof(szFunction));
	// Find function reference
	C4AulScriptFunc *pFn = GetSFunc(szFunction);
	// Get image id
	if (pidImage) { if (Def) *pidImage=Def->id; else *pidImage=0; if (pFn) *pidImage=pFn->idImage; }
	// Get image phase
	if(piImagePhase) { *piImagePhase = 0; if(pFn) *piImagePhase = pFn->iImagePhase; }
	// Return function desc
	if (pFn && pFn->Desc.getLength()) return pFn->DescText.getData();
	// No function
	return NULL;
	}


/*--- C4DefScriptHost ---*/

void C4DefScriptHost::Default()
	{
	C4ScriptHost::Default();
	SFn_CalcValue = SFn_SellTo = SFn_ControlTransfer = SFn_CustomComponents = NULL;
	ControlMethod[0]=ControlMethod[1]=ContainedControlMethod[0]=ContainedControlMethod[1]=ActivationControlMethod[0]=ActivationControlMethod[1]=0;
	}

void C4DefScriptHost::AfterLink()
	{
	C4AulScript::AfterLink();
	// Search cached functions
	SFn_CalcValue       = GetSFunc(PSF_CalcValue      , AA_PROTECTED);
	SFn_SellTo          = GetSFunc(PSF_SellTo         , AA_PROTECTED);
	SFn_ControlTransfer = GetSFunc(PSF_ControlTransfer, AA_PROTECTED);
	SFn_CustomComponents = GetSFunc(PSF_GetCustomComponents, AA_PROTECTED);
	if (Def)
		{
		C4AulAccess CallAccess = /*Strict ? AA_PROTECTED : */AA_PRIVATE;
		Def->TimerCall=GetSFuncWarn((const char *) Def->STimerCall, CallAccess, "TimerCall");
		}
	// Check if there are any Control/Contained/Activation script functions
	GetControlMethodMask(PSF_Control, ControlMethod[0], ControlMethod[1]);
	GetControlMethodMask(PSF_ContainedControl, ContainedControlMethod[0], ContainedControlMethod[1]);
	GetControlMethodMask(PSF_Activate, ActivationControlMethod[0], ActivationControlMethod[1]);
	}



/*--- C4GameScriptHost ---*/

C4GameScriptHost::C4GameScriptHost(): Counter(0), Go(false) { }
C4GameScriptHost::~C4GameScriptHost() { }


void C4GameScriptHost::Default()
	{
	C4ScriptHost::Default();
	Counter=0;
	Go=false;
	}

bool C4GameScriptHost::Execute()
	{
	if (!Script) return false;
	char buffer[500];
	if (Go && !::Game.iTick10)
		{
		sprintf(buffer,PSF_Script,Counter++);
		return !! Call(buffer);
		}
	return false;
	}

C4Value C4GameScriptHost::GRBroadcast(const char *szFunction, C4AulParSet *pPars, bool fPassError, bool fRejectTest)
	{
	// call objects first - scenario script might overwrite hostility, etc...
	C4Object *pObj;
	for (C4ObjectLink *clnk=::Objects.ObjectsInt().First; clnk; clnk=clnk->Next) if (pObj=clnk->Obj)
		if (pObj->Category & (C4D_Goal | C4D_Rule | C4D_Environment))
			if (pObj->Status)
				{
				C4Value vResult(pObj->Call(szFunction, pPars, fPassError));
				// rejection tests abort on first nonzero result
				if (fRejectTest) if (!!vResult) return vResult;
				}
	// scenario script call
	return Call(szFunction, 0, pPars, fPassError);
	}

void C4GameScriptHost::CompileFunc(StdCompiler *pComp)
  {
	pComp->Value(mkNamingAdapt(Go,             "Go",                    false));
	pComp->Value(mkNamingAdapt(Counter,        "Counter",               0));
  }
