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

/* Fullscreen startup log and chat type-in */

#include "C4Include.h"
#include "gui/C4MessageBoard.h"

#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4Gui.h"
#include "gui/C4LoaderScreen.h"
#include "gui/C4MessageInput.h"
#include "lib/StdColors.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

const int C4LogSize=30000, C4LogMaxLines=1000;

C4MessageBoard::C4MessageBoard() : LogBuffer(C4LogSize, C4LogMaxLines, 0, "  ", false)
{
	Delay = -1;
	Fader = 0;
	Speed = 2;
	Output.Default();
	Startup = false;
	ScreenFader = 0;
	iBackScroll = -1;
	ScrollUpBinding = nullptr;
	ScrollDownBinding = nullptr;
	iLineHgt = 1; // Prevent unitialized access with USE_CONSOLE
}

C4MessageBoard::~C4MessageBoard()
{
	LogBuffer.Clear();
	LogBuffer.SetLBWidth(0);
}

void C4MessageBoard::Execute()
{
	// Startup? draw only
	if (Startup) { Draw(Output); return; }

	// typein or messages waiting? fade in
	if (::MessageInput.IsTypeIn() || iBackScroll >= 0)
		ScreenFader = std::max(ScreenFader - 0.20f, -1.0f);

	// no curr msg?
	if (iBackScroll<0)
	{
		// draw anyway
		Draw(Output);
		if (!::MessageInput.IsTypeIn())
			ScreenFader = std::min(ScreenFader + 0.05f, 1.0f);
		return;
	}

	// recalc fade/delay speed
	Speed = std::max(1, iBackScroll / 5);
	// fade msg in?
	if (Fader > 0)
		Fader = std::max(Fader - Speed, 0);
	// hold curr msg? (delay)
	if (Fader <= 0)
	{
		// no delay set yet?
		if (Delay == -1)
		{
			// set delay based on msg length
			const char *szCurrMsg = LogBuffer.GetLine(std::min(-iBackScroll, -1), nullptr, nullptr, nullptr);
			if (szCurrMsg) Delay = strlen(szCurrMsg); else Delay = 0;
		}
		// wait...
		if (Delay > 0) Delay = std::max(Delay - Speed, 0);
		// end of delay
		if (Delay == 0)
		{
			// set cursor to next msg (or at end of log)
			iBackScroll = std::max(iBackScroll - 1, -1);
			// reset fade
			Fader = iLineHgt;
			Delay = -1;
		}
	}

	// Draw
	Draw(Output);
}

void C4MessageBoard::Init(C4Facet &cgo, bool fStartup)
{
	Output=cgo;
	Startup=fStartup;
	iLineHgt=::GraphicsResource.FontRegular.GetLineHeight();
	LogBuffer.SetLBWidth(Output.Wdt);

	if (!Startup)
	{
		// set cursor to end of log
		iBackScroll = -1;
		Fader = 0;
		Speed = 2;
		ScreenFader = 1.0f; // msgs faded out

		LogBuffer.SetLBWidth(Output.Wdt);
	}

	// messageboard
	ScrollUpBinding = std::make_unique<C4KeyBinding>(C4KeyCodeEx(K_UP, KEYS_Shift), "MsgBoardScrollUp", KEYSCOPE_Fullscreen, new C4KeyCB  <C4MessageBoard>(*GraphicsSystem.MessageBoard, &C4MessageBoard::ControlScrollUp));
	ScrollDownBinding = std::make_unique<C4KeyBinding>(C4KeyCodeEx(K_DOWN, KEYS_Shift), "MsgBoardScrollDown", KEYSCOPE_Fullscreen, new C4KeyCB  <C4MessageBoard>(*GraphicsSystem.MessageBoard, &C4MessageBoard::ControlScrollDown));
}

void C4MessageBoard::Draw(C4Facet &cgo)
{
	if (!Application.Active) return;

	// Startup: draw Loader
	if (Startup)
	{
		if (::GraphicsSystem.pLoaderScreen)
			::GraphicsSystem.pLoaderScreen->Draw(cgo, C4LoaderScreen::Flag::ALL, Game.InitProgress, &LogBuffer);
		else
			// loader not yet loaded: black BG
			pDraw->DrawBoxDw(cgo.Surface, 0,0, cgo.Wdt, cgo.Hgt, 0x00000000);
		return;
	}

	// Game running: message fader

	// draw messages
	// how many "extra" messages should be shown?
	int iMsgFader = C4MSGB_MaxMsgFading;
	// check screenfader range
	if (ScreenFader >= 1.0f)
	{
		return;
	}
	::GraphicsSystem.OverwriteBg();
	// show msgs
	for (int iMsg = -iMsgFader; iMsg < 0; iMsg++)
	{
		// get message at pos
		if (iMsg-iBackScroll >= 0) break;
		const char *Message = LogBuffer.GetLine(iMsg-iBackScroll, nullptr, nullptr, nullptr);
		if (!Message || !*Message) continue;
		// calc target position (y)
		int iMsgY = cgo.Y + cgo.Hgt + iMsg * iLineHgt + Fader;

		// player message color?
		C4Player *pPlr = GetMessagePlayer(Message);

		DWORD dwColor;
		if (pPlr)
			dwColor = PlrClr2TxtClr(pPlr->ColorDw) & 0xffffff;
		else
			dwColor = 0xffffff;
		// fade out (msg fade)
		float fade = std::max(ScreenFader, 0.0f) + ((iMsg + 2.0f + float(Fader) / iLineHgt) / std::min(2-iMsgFader, -1));
		DWORD dwFade = (0xff - Clamp(int(fade * 0xff), 0, 0xff)) << 24;
		dwColor |= dwFade;
		// Draw
		pDraw->StringOut(Message,::GraphicsResource.FontRegular,1.0,cgo.Surface,cgo.X,iMsgY,dwColor);
	}
}

void C4MessageBoard::EnsureLastMessage()
{
	// Ingore if startup or typein
	if (Startup) return;
	// scroll until end of log
	for (int i = 0; i < 100; i++)
	{
		::GraphicsSystem.Execute();
		Execute();
		if (iBackScroll < 0) break;
		Delay=0;
	}
}

void C4MessageBoard::AddLog(const char *szMessage)
{
	// safety
	if (!szMessage || !*szMessage) return;
	// make sure new message will be drawn
	++iBackScroll;
	// register message in standard messageboard font
	LogBuffer.AppendLines(szMessage, &::GraphicsResource.FontRegular, 0, nullptr);
}

void C4MessageBoard::ClearLog()
{
	LogBuffer.Clear();
}

void C4MessageBoard::LogNotify()
{
	// do not show startup board if GUI is active
	if (::pGUI->IsActive()) return;
	// Reset
	iBackScroll=0;
	// Draw
	if (pDraw)
	{
		Draw(Output);
		// startup: Draw message board only and do page flip
		if (Startup) FullScreen.pSurface->PageFlip();
	}
}

C4Player* C4MessageBoard::GetMessagePlayer(const char *szMessage)
{
	// Scan message text for heading player name
	if (SEqual2(szMessage, "* "))
	{
		StdStrBuf str;
		str.CopyUntil(szMessage + 2,' ');
		return ::Players.GetByName(str.getData());
	}
	if (SCharCount(':',szMessage))
	{
		StdStrBuf str;
		str.CopyUntil(szMessage + 2,':');
		return ::Players.GetByName(str.getData());
	}
	return nullptr;
}

bool C4MessageBoard::ControlScrollUp()
{
	Delay=-1; Fader=0;
	iBackScroll++;
	return true;
}

bool C4MessageBoard::ControlScrollDown()
{
	Delay=-1; Fader=0;
	if (iBackScroll > -1) iBackScroll--;
	return true;
}
