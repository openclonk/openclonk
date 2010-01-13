/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003  Matthes Bender
 * Copyright (c) 2001, 2004  Sven Eberhardt
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

/* Holds crew member information */

#ifndef INC_C4ObjectInfo
#define INC_C4ObjectInfo

#include <C4Surface.h>
#include <C4InfoCore.h>
#include <C4Object.h>
#include <C4FacetEx.h>

class C4ObjectInfo: public C4ObjectInfoCore
  {
  public:
	  C4ObjectInfo();
		~C4ObjectInfo();
  public:
    bool WasInAction;
    bool InAction;
		int32_t	InActionTime;
		bool HasDied;
		int32_t ControlCount;
		class C4Def *pDef; // definition to ID - only eresolved if defs were loaded at object info loading time
		C4Portrait Portrait;		     // portrait link (usually to def graphics)
		C4Portrait *pNewPortrait;    // new permanent portrait link (usually to def graphics)
		C4Portrait *pCustomPortrait; // if assigned, the Clonk has a custom portrait to be set via SetPortrait("custom")
	  char Filename[_MAX_PATH+1];
    C4ObjectInfo *Next;
  public:
	  void Default();
	  void Clear();
	  void Evaluate();
	  void Retire();
	  void Recruit();
	  void SetBirthday();
	  void Draw(C4Facet &cgo, bool fShowPortrait, C4Object *pOfObj);
	  bool Save(C4Group &hGroup, bool fStoreTiny, C4DefList *pDefs);
	  bool Load(C4Group &hGroup, bool fLoadPortrait);
	  bool Load(C4Group &hMother, const char *szEntryname, bool fLoadPortrait);
		bool SetRandomPortrait(C4ID idSourceDef, bool fAssignPermanently, bool fCopyFile);
		bool SetPortrait(const char *szPortraitName, C4Def *pSourceDef, bool fAssignPermanently, bool fCopyFile);
		bool SetPortrait(C4PortraitGraphics *pNewPortraitGfx, bool fAssignPermanently, bool fCopyFile);
		bool ClearPortrait(bool fPermanently);
   };

#endif
