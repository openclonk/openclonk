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

/* Logic for C4Object: Menus */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "object/C4Object.h"

#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "lib/StdColors.h"
#include "object/C4Def.h"
#include "object/C4ObjectMenu.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "script/C4AulExec.h"


static void DrawMenuSymbol(int32_t iMenu, C4Facet &cgo, int32_t iOwner)
{
	C4Facet ccgo;

	DWORD dwColor=0;
	if (ValidPlr(iOwner)) dwColor=::Players.Get(iOwner)->ColorDw;

	switch (iMenu)
	{
	case C4MN_Buy:
		::GraphicsResource.fctFlagClr.DrawClr(ccgo = cgo.GetFraction(75, 75), true, dwColor);
		::GraphicsResource.fctWealth.Draw(ccgo = cgo.GetFraction(100, 50, C4FCT_Left, C4FCT_Bottom));
		::GraphicsResource.fctArrow.Draw(ccgo = cgo.GetFraction(70, 70, C4FCT_Right, C4FCT_Center), false, 0);
		break;
	case C4MN_Sell:
		::GraphicsResource.fctFlagClr.DrawClr(ccgo = cgo.GetFraction(75, 75), true, dwColor);
		::GraphicsResource.fctWealth.Draw(ccgo = cgo.GetFraction(100, 50, C4FCT_Left, C4FCT_Bottom));
		::GraphicsResource.fctArrow.Draw(ccgo = cgo.GetFraction(70, 70, C4FCT_Right, C4FCT_Center), false, 1);
		break;
	}
}

bool C4Object::ActivateMenu(int32_t iMenu, int32_t iMenuSelect,
                            int32_t iMenuData, int32_t iMenuPosition,
                            C4Object *pTarget)
{
	// Variables
	C4FacetSurface fctSymbol;
	C4IDList ListItems;
	// Close any other menu
	if (Menu && Menu->IsActive()) if (!Menu->TryClose(true, false)) return false;
	// Create menu
	if (!Menu) Menu = new C4ObjectMenu; else Menu->ClearItems();
	// Open menu
	switch (iMenu)
	{
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Activate:
		// No target specified: use own container as target
		if (!pTarget) if (!(pTarget=Contained)) break;
		// Opening contents menu blocked by RejectContents
		if (!!pTarget->Call(PSF_RejectContents)) return false;
		// Create symbol
		fctSymbol.Create(C4SymbolSize,C4SymbolSize);
		pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
		// Init
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_None,0,iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Buy:
		// No target specified: container is base
		if (!pTarget) if (!(pTarget=Contained)) break;
		// Create symbol
		fctSymbol.Create(C4SymbolSize,C4SymbolSize);
		DrawMenuSymbol(C4MN_Buy, fctSymbol, pTarget->Owner);
		// Init menu
		Menu->Init(fctSymbol,LoadResStr("IDS_PLR_NOBUY"),this,C4MN_Extra_Value,0,iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Sell:
		// No target specified: container is base
		if (!pTarget) if (!(pTarget=Contained)) break;
		// Create symbol & init
		fctSymbol.Create(C4SymbolSize,C4SymbolSize);
		DrawMenuSymbol(C4MN_Sell, fctSymbol, pTarget->Owner);
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_Value,0,iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Get:
	case C4MN_Contents:
		// No target specified
		if (!pTarget) break;
		// Opening contents menu blocked by RejectContents
		if (!!pTarget->Call(PSF_RejectContents)) return false;
		// Create symbol & init
		fctSymbol.Create(C4SymbolSize,C4SymbolSize);
		pTarget->Def->Draw(fctSymbol,false,pTarget->Color,pTarget);
		Menu->Init(fctSymbol,FormatString(LoadResStr("IDS_OBJ_EMPTY"),pTarget->GetName()).getData(),this,C4MN_Extra_None,0,iMenu);
		Menu->SetPermanent(true);
		Menu->SetRefillObject(pTarget);
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	case C4MN_Info:
		// Target by parameter
		if (!pTarget) break;
		// Create symbol & init menu
		fctSymbol.Create(C4SymbolSize, C4SymbolSize); GfxR->fctOKCancel.Draw(fctSymbol,true,0,1);
		Menu->Init(fctSymbol, pTarget->GetName(), this, C4MN_Extra_None, 0, iMenu, C4MN_Style_Info);
		Menu->SetPermanent(true);
		Menu->SetAlignment(C4MN_Align_Free);
		C4Viewport *pViewport = ::Viewports.GetViewport(Controller); // Hackhackhack!!!
		if (pViewport) Menu->SetLocation((pTarget->GetX() + pTarget->Shape.GetX() + pTarget->Shape.Wdt + 10 - pViewport->GetViewX()) * pViewport->GetZoom(),
			                                 (pTarget->GetY() + pTarget->Shape.GetY() - pViewport->GetViewY()) * pViewport->GetZoom());
		// Add info item
		fctSymbol.Create(C4PictureSize, C4PictureSize); pTarget->Def->Draw(fctSymbol, false, pTarget->Color, pTarget);
		Menu->Add(pTarget->GetName(), fctSymbol, "", C4MN_Item_NoCount, nullptr, pTarget->GetInfoString().getData());
		fctSymbol.Default();
		// Success
		return true;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	}
	// Invalid menu identification
	CloseMenu(true);
	return false;
}

bool C4Object::CloseMenu(bool fForce)
{
	if (Menu)
	{
		if (Menu->IsActive()) if (!Menu->TryClose(fForce, false)) return false;
		if (!Menu->IsCloseQuerying()) { delete Menu; Menu=nullptr; } // protect menu deletion from recursive menu operation calls
	}
	return true;
}

bool C4Object::MenuCommand(const char *szCommand)
{
	// Native script execution
	if (!Def || !Status) return false;
	return !! ::AulExec.DirectExec(this, szCommand, "MenuCommand");
}
