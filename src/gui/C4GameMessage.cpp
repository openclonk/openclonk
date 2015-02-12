/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Text messages drawn inside the viewport */

#include <C4Include.h>
#include <C4GameMessage.h>

#include <C4Object.h>
#include <C4GraphicsResource.h>
#include <C4Player.h>
#include <C4PlayerList.h>

const int32_t ObjectMsgDelayFactor = 2, GlobalMsgDelayFactor = 3; // frames per char message display time

C4GameMessage::C4GameMessage() : pFrameDeco(NULL)
{

}

C4GameMessage::~C4GameMessage()
{
	delete pFrameDeco;
}

void C4GameMessage::Init(int32_t iType, const StdStrBuf & sText, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwClr, C4ID idDecoID, C4PropList *pSrc, uint32_t dwFlags, int width)
{
	// safety!
	if (pTarget && !pTarget->Status) pTarget = NULL;
	// Set data
	Text.Copy(sText);
	Target=pTarget;
	X=iX; Y=iY; Wdt=width;
	Player=iPlayer;
	ColorDw=dwClr;
	Type=iType;
	Delay=Max<int32_t>(C4GM_MinDelay, Text.getLength() * (Target ? ObjectMsgDelayFactor : GlobalMsgDelayFactor));
	DecoID=idDecoID;
	this->dwFlags=dwFlags;
	PictureDef=NULL;
	PictureDefVal.Set0();
	if (pSrc)
		if (pSrc->GetDef() || pSrc->GetObject() || pSrc->GetPropertyPropList(P_Source))
		{
			PictureDef = pSrc;
			PictureDefVal.SetPropList(pSrc);
		}
	// Permanent message
	if ('@' == Text[0])
	{
		Delay=-1;
		Text.Move(1, Text.getLength());
		Text.Shrink(1);
	}
	// frame decoration
	delete pFrameDeco; pFrameDeco = NULL;
	if (DecoID)
	{
		pFrameDeco = new C4GUI::FrameDecoration();
		if (!pFrameDeco->SetByDef(DecoID))
		{
			delete pFrameDeco;
			pFrameDeco = NULL;
		}
	}
}

void C4GameMessage::Append(const char *szText, bool fNoDuplicates)
{
	// Check for duplicates
	if (fNoDuplicates)
		for (const char *pPos = Text.getData(); *pPos; pPos = SAdvancePast(pPos, '|'))
			if (SEqual2(pPos, szText))
				return;
	// Append new line
	Text.AppendFormat("|%s", szText);
	Delay += SLen(szText) * (Target ? ObjectMsgDelayFactor : GlobalMsgDelayFactor);
}

bool C4GameMessage::Execute()
{
	// Position by target
	// currently done in C4GameMessage::Draw for parallaxity
	/*if (Target)
	  { X=Target->x; Y=Target->y-Target->Def->Shape.Hgt/2-5; }*/
	// Delay / removal
	if (Delay>0) Delay--;
	if (Delay==0) return false;
	// Done
	return true;
}

int32_t DrawMessageOffset = -35; // For finding the optimum place to draw startup info & player messages...
int32_t PictureWidth = 64;
int32_t PictureIndent = 10;

void C4GameMessage::Draw(C4TargetFacet &cgo, int32_t iPlayer)
{
	// Globals or player
	if (Type == C4GM_Global || ((Type == C4GM_GlobalPlayer) && (iPlayer == Player)))
	{
		int32_t iTextWdt,iTextHgt;
		StdStrBuf sText;
		int32_t x,y,wdt;
		if (dwFlags & C4GM_XRel) x = X * cgo.Wdt / 100; else x = X;
		if (dwFlags & C4GM_YRel) y = Y * cgo.Hgt / 100; else y = Y;
		if (dwFlags & C4GM_WidthRel) wdt = Wdt * cgo.Wdt / 100; else wdt = Wdt;
		if (~dwFlags & C4GM_NoBreak)
		{
			// Word wrap to cgo width
			if (PictureDef)
			{
				if (!wdt) wdt = Clamp<int32_t>(cgo.Wdt/2, 50, Min<int32_t>(500, cgo.Wdt-10));
				int32_t iUnbrokenTextWidth = ::GraphicsResource.FontRegular.GetTextWidth(Text.getData(), true);
				wdt = Min<int32_t>(wdt, iUnbrokenTextWidth+10);
			}
			else
			{
				if (!wdt)
					wdt = Clamp<int32_t>(cgo.Wdt-50, 50, 500);
				else
					wdt = Clamp<int32_t>(wdt, 10, cgo.Wdt-10);
			}
			iTextWdt = wdt * cgo.Zoom;
			iTextHgt = ::GraphicsResource.FontRegular.BreakMessage(Text.getData(), iTextWdt, &sText, true);
		}
		else
		{
			::GraphicsResource.FontRegular.GetTextExtent(Text.getData(), iTextWdt, iTextHgt, true);
			sText.Ref(Text);
		}
		int32_t iDrawX = cgo.X+x;
		int32_t iDrawY = cgo.Y+y;
		// draw message
		if (PictureDef)
		{
			// message with portrait
			// bottom-placed portrait message: Y-Positioning 0 refers to bottom of viewport
			if (dwFlags & C4GM_Bottom) iDrawY += cgo.Hgt;
			else if (dwFlags & C4GM_VCenter) iDrawY += cgo.Hgt/2;
			if (dwFlags & C4GM_Right) iDrawX += cgo.Wdt;
			else if (dwFlags & C4GM_HCenter) iDrawX += cgo.Wdt/2;
			// draw decoration
			if (pFrameDeco)
			{
				iDrawX *= cgo.Zoom; iDrawY *= cgo.Zoom;
				C4Rect rect(iDrawX-cgo.TargetX, iDrawY-cgo.TargetY, iTextWdt + PictureWidth + PictureIndent + pFrameDeco->iBorderLeft + pFrameDeco->iBorderRight, Max(iTextHgt, PictureWidth) + pFrameDeco->iBorderTop + pFrameDeco->iBorderBottom);
				if (dwFlags & C4GM_Bottom) { rect.y -= rect.Hgt; iDrawY -= rect.Hgt; }
				else if (dwFlags & C4GM_VCenter) { rect.y -= rect.Hgt/2; iDrawY -= rect.Hgt/2; }
				if (dwFlags & C4GM_Right) { rect.x -= rect.Wdt; iDrawX -= rect.Wdt; }
				else if (dwFlags & C4GM_HCenter) { rect.x -= rect.Wdt/2; iDrawX -= rect.Wdt/2; }
				pFrameDeco->Draw(cgo, rect);
				iDrawX += pFrameDeco->iBorderLeft;
				iDrawY += pFrameDeco->iBorderTop;
			}
			else
				iDrawY -= iTextHgt;
			// draw picture
			// can only be def or object because has been checked on assignment
			
			C4Facet facet(cgo.Surface, iDrawX, iDrawY, PictureWidth, PictureWidth);
			if(PictureDef->GetObject())
				PictureDef->GetObject()->DrawPicture(facet);
			else if (PictureDef->GetDef())
				PictureDef->GetDef()->Draw(facet);
			else
				Game.DrawPropListSpecImage(facet, PictureDef);

			// draw message
			pDraw->TextOut(sText.getData(),::GraphicsResource.FontRegular,1.0,cgo.Surface,iDrawX+PictureWidth+PictureIndent,iDrawY,ColorDw,ALeft);
		}
		else
		{
			// message without picture
			iDrawX += (cgo.Wdt/2) * cgo.Zoom;
			iDrawY += (2 * cgo.Hgt / 3 + 50) * cgo.Zoom;
			if (!(dwFlags & C4GM_Bottom)) iDrawY += DrawMessageOffset;
			pDraw->TextOut(sText.getData(),::GraphicsResource.FontRegular,1.0,cgo.Surface,iDrawX,iDrawY,ColorDw,ACenter);
		}
	}
	// Positioned
	else if (Type == C4GM_Target || ((Type == C4GM_TargetPlayer) && (iPlayer == Player)))
	{
		// adjust position by object; care about parallaxity
		float iMsgX, iMsgY, newzoom;
		Target->GetDrawPosition(cgo, iMsgX, iMsgY, newzoom);
		iMsgY -= Target->Def->Shape.Hgt/2+5;
		iMsgX+=X; iMsgY+=Y;
		// check output bounds
		if (Inside<float>((iMsgX - cgo.X) * newzoom, 0, cgo.Wdt * cgo.Zoom - 1))
			if (Inside<float>((iMsgY - cgo.Y) * newzoom, 0, cgo.Hgt * cgo.Zoom - 1))
			{
				// if the message is attached to an object and the object
				// is invisible for that player, the message won't be drawn
				if (Type == C4GM_Target)
					if (!Target->IsVisible(iPlayer, false))
						return;
				// check fog of war
				C4Player *pPlr = ::Players.Get(iPlayer);
				if (pPlr && pPlr->fFogOfWar)
				{
					// TODO: Should we render the message with a shader to apply lighting to it?
					// Otherwise use pDraw->GetFoW() to check for visibility...
					//if (!pPlr->FoWIsVisible(iMsgX + cgo.TargetX - cgo.X, iMsgY + cgo.TargetY - cgo.Y))
					if(false)
					{
						// special: Target objects that ignore FoW should display the message even if within FoW
						if (Type != C4GM_Target && Type != C4GM_TargetPlayer) return;
						if (~Target->Category & C4D_IgnoreFoW) return;
					}
				}
				// Word wrap to cgo width
				StdStrBuf sText;
				if (~dwFlags & C4GM_NoBreak)
					::GraphicsResource.FontRegular.BreakMessage(Text.getData(), Clamp<int32_t>(cgo.Wdt * cgo.Zoom, 50, 200), &sText, true);
				else
					sText.Ref(Text);
				// Adjust position by output boundaries
				float iTX,iTY;
				int iTWdt,iTHgt;
				::GraphicsResource.FontRegular.GetTextExtent(sText.getData(),iTWdt,iTHgt,true);
				// +0.5f for proper rounding; avoids oscillations near pixel border:
				iTX = Clamp<float>((iMsgX - cgo.X) * newzoom, iTWdt/2, cgo.Wdt * cgo.Zoom - iTWdt / 2) + 0.5f;
				iTY = Clamp<float>((iMsgY - cgo.Y) * newzoom - iTHgt, 0, cgo.Hgt * cgo.Zoom - iTHgt) + 0.5f;
				// Draw
				pDraw->TextOut(sText.getData(), ::GraphicsResource.FontRegular, 1.0,
				                           cgo.Surface,
				                           cgo.X + iTX,
				                           cgo.Y + iTY,
				                           ColorDw,ACenter);
				return;
			}
	}
}

void C4GameMessage::UpdateDef(C4ID idUpdDef)
{
	// frame deco might be using updated/deleted def
	if (pFrameDeco)
	{
		if (!pFrameDeco->UpdateGfx())
		{
			delete pFrameDeco;
			pFrameDeco = NULL;
		}
	}
}

C4GameMessageList::C4GameMessageList()
{
	Default();
}

C4GameMessageList::~C4GameMessageList()
{
	Clear();
}

void C4GameMessageList::Default()
{
	First=NULL;
}

void C4GameMessageList::ClearPointers(C4Object *pObj)
{
	C4GameMessage *cmsg,*next,*prev=NULL;
	for (cmsg=First; cmsg; cmsg=next)
	{
		next=cmsg->Next;
		if (cmsg->Target==pObj)
			{ delete cmsg; if (prev) prev->Next=next; else First=next; }
		else
			prev=cmsg;
	}
}

void C4GameMessageList::Clear()
{
	C4GameMessage *cmsg,*next;
	for (cmsg=First; cmsg; cmsg=next)
	{
		next=cmsg->Next;
		delete cmsg;
	}
	First=NULL;
}

void C4GameMessageList::Execute()
{
	C4GameMessage *cmsg,*next,*prev=NULL;
	for (cmsg=First; cmsg; cmsg=next)
	{
		next=cmsg->Next;
		if (!cmsg->Execute())
			{ delete cmsg; if (prev) prev->Next=next; else First=next; }
		else
			prev=cmsg;
	}
}

bool C4GameMessageList::New(int32_t iType, const char *szText, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwClr, C4ID idDecoID, C4PropList *pSrc, uint32_t dwFlags, int32_t width)
{
	return New(iType, StdStrBuf(szText), pTarget, iPlayer, iX, iY, dwClr, idDecoID, pSrc, dwFlags, width);
}

bool C4GameMessageList::New(int32_t iType, const StdStrBuf & sText, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t dwClr, C4ID idDecoID, C4PropList *pSrc, uint32_t dwFlags, int32_t width)
{
	if (!(dwFlags & C4GM_Multiple))
	{
		// Clear messages with same target
		if (pTarget) ClearPointers(pTarget);

		// Clear other global messages for the given player
		if (iType == C4GM_Global || iType == C4GM_GlobalPlayer) ClearPlayers(iPlayer, dwFlags & C4GM_PositioningFlags);
	}

	// Object deleted?
	if (pTarget && !pTarget->Status) return false;

	// Empty message? (only deleting old message)
	if (!sText.getLength()) return true;

	// Add new message
	C4GameMessage *msgNew = new C4GameMessage;
	msgNew->Init(iType, sText,pTarget,iPlayer,iX,iY,dwClr, idDecoID, pSrc, dwFlags, width);
	msgNew->Next=First;
	First=msgNew;

	return true;
}

bool C4GameMessageList::Append(int32_t iType, const char *szText, C4Object *pTarget, int32_t iPlayer, int32_t iX, int32_t iY, uint32_t bCol, bool fNoDuplicates)
{
	C4GameMessage *cmsg = NULL;
	if (iType == C4GM_Target)
	{
		for (cmsg=::Messages.First; cmsg; cmsg=cmsg->Next)
			if (pTarget == cmsg->Target)
				break;
	}
	if (iType == C4GM_Global || iType == C4GM_GlobalPlayer)
	{
		for (cmsg=::Messages.First; cmsg; cmsg=cmsg->Next)
			if (iPlayer == cmsg->Player)
				break;
	}
	if (!cmsg || pTarget!=cmsg->Target)
	{
		New(iType, szText, pTarget, iPlayer, iX, iY, bCol);
	}
	else
	{
		cmsg->Append(szText, fNoDuplicates);
	}
	return true;
}

void C4GameMessageList::ClearPlayers(int32_t iPlayer, int32_t dwPositioningFlags)
{
	C4GameMessage *cmsg,*next,*prev=NULL;
	for (cmsg=First; cmsg; cmsg=next)
	{
		next=cmsg->Next;
		if ( (cmsg->Type == C4GM_Global || cmsg->Type == C4GM_GlobalPlayer) && cmsg->Player==iPlayer && cmsg->GetPositioningFlags() == dwPositioningFlags)
			{ delete cmsg; if (prev) prev->Next=next; else First=next; }
		else
			prev=cmsg;
	}
}

void C4GameMessageList::UpdateDef(C4ID idUpdDef)
{
	C4GameMessage *cmsg;
	for (cmsg=First; cmsg; cmsg=cmsg->Next) cmsg->UpdateDef(idUpdDef);
}

void C4GameMessageList::Draw(C4TargetFacet &gui_cgo, C4TargetFacet &cgo, int32_t iPlayer)
{
	C4GameMessage *cmsg;
	for (cmsg=First; cmsg; cmsg=cmsg->Next)
	{
		if ((cmsg->Target && (cmsg->Target->Category & C4D_Foreground)) || cmsg->Type == C4GM_Global || cmsg->Type == C4GM_GlobalPlayer)
			cmsg->Draw(gui_cgo,iPlayer);
		else
			cmsg->Draw(cgo,iPlayer);
	}
}

void GameMsgObjectError(const char *szText, C4Object *pTarget, bool Red)
{
	::Messages.New(C4GM_TargetPlayer,szText,pTarget,pTarget->Controller,0,0, Red ? C4RGB(0xca, 0, 0) : C4RGB(0xff, 0xff, 0xff));
}

C4GameMessageList Messages;
