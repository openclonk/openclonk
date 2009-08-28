/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2008  Matthes Bender
 * Copyright (c) 2002-2005  Sven Eberhardt
 * Copyright (c) 2002-2004  Peter Wortmann
 * Copyright (c) 2009  Nicolas Hake
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

/* Fullscreen startup log and chat type-in */

#include <C4Include.h>
#include <C4MessageBoard.h>

#ifndef BIG_C4INCLUDE
#include <C4Object.h>
#include <C4Application.h>
#include <C4LoaderScreen.h>
#include <C4Gui.h>
#include <C4Console.h>
#include <C4Network2Dialogs.h>
#include <C4Player.h>
#include <C4GraphicsSystem.h>
#include <C4GraphicsResource.h>
#include <C4MessageInput.h>
#include <C4Game.h>
#include <C4PlayerList.h>
#endif

const int C4LogSize=30000, C4LogMaxLines=1000;

C4MessageBoard::C4MessageBoard() : LogBuffer(C4LogSize, C4LogMaxLines, 0, "  ", false)
	{
	Default();
	}

C4MessageBoard::~C4MessageBoard()
	{
	Clear();
	}

void C4MessageBoard::Default()
	{
	Clear();
	Active=false;
	Delay=-1;
	Fader=0;
	Speed=2;
	Output.Default();
	Startup=false;
	Empty=true;
	ScreenFader=0;
	iMode = 0;
	iLines = 4;
	iBackScroll = -1;
	}

void C4MessageBoard::Clear()
	{
	Active=false;
	LogBuffer.Clear();
	LogBuffer.SetLBWidth(0);
	}

void C4MessageBoard::ChangeMode(int inMode)
{
	if(iMode < 0 || iMode >= 3) return;

	// prepare msg board
	int iHeight = 0;
	switch(inMode)
	{
	case 0: // one line, std

		Config.Graphics.MsgBoard = 1;
		iHeight = iLineHgt; // one line

		// from mode 2?
		if(iMode == 2)
		{
			// move to end of log
			iBackScroll = -1;
			// show nothing
			Empty = true;
			break;
		}

		// set msg pointer to end of log
		iBackScroll = -1;
		Fader = -1;
		Empty = false;
		Speed = 2;
		ScreenFader = C4MSGB_MaxMsgFading * iLineHgt; // msgs faded out

		break;

	case 1: // >= 2 lines

		iLines = Max(iLines, 2);
		Config.Graphics.MsgBoard = iLines;
		// calc position; go to end
		iBackScroll = -1;
		// ok
		iHeight = (iLines + 1) * iLineHgt;
		Fader = 0;
		iBackScroll = -1;
		break;

	case 2: // show nothing

		Config.Graphics.MsgBoard = 0;
		iHeight = 0;

		break;
	}
	// set mode
	iMode = inMode;
	// recalc output facet
	Output.X = 0;
	Output.Y = C4GUI::GetScreenHgt() - iHeight;
	Output.Wdt = C4GUI::GetScreenWdt();
	Output.Hgt = iHeight;
	LogBuffer.SetLBWidth(Output.Wdt);
	// recalc viewports
	::GraphicsSystem.RecalculateViewports();
}

void C4MessageBoard::Execute()
	{
	if (!Active) return;

	// Startup? draw only
	if(Startup) { Draw(Output); return; }

	// which mode?
	switch(iMode)
	{

	case 2: // show nothing

		// TypeIn: Act as in mode 0
		if (!::MessageInput.IsTypeIn())
			{
			ScreenFader = 100;
			iBackScroll = -1;
			break;
			}

	case 0: // one msg

		// typein? fade in
		if(::MessageInput.IsTypeIn())
			ScreenFader = Max(ScreenFader - 20, -100);

		// no curr msg?
		if(iBackScroll<0)
		{
			// ok, it is empty
			Empty = true;
			// draw anyway
			Draw(Output);
			if(!::MessageInput.IsTypeIn())
				ScreenFader += 5;
			return;
		}
		// got new msg?
		if(Empty)
		{
			// start fade in
			Fader = iLineHgt;
			Delay = -1;
			// now we have a msg
			Empty = false;
		}

		// recalc fade/delay speed
		Speed = Max(1, iBackScroll / 5);
		// fade msg in?
		if (Fader>0)
			Fader=Max(Fader-Speed,0);
		// fade msg out?
		if (Fader<0)
			Fader=Max(Fader-Speed,-iLineHgt);
		// hold curr msg? (delay)
		if (Fader==0)
		{
			// no delay set yet?
			if (Delay == -1)
			{
				// set delay based on msg length
				const char *szCurrMsg = LogBuffer.GetLine(Min(-iBackScroll,-1), NULL, NULL, NULL);
				if(szCurrMsg) Delay = strlen(szCurrMsg); else Delay=0;
			}
			// wait...
			if (Delay>0) Delay=Max(Delay-Speed,0);
			// end of delay
			if (Delay==0)
			{
				Fader = Max(-Speed, -iLineHgt); // start fade out
				Delay = -1;
			}
		}

		ScreenFader = Max(ScreenFader - 20, -100);

		// go to next msg? (last msg is completely faded out)
		if (Fader == -iLineHgt)
		{
			// set cursor to next msg (or at end of log)
			iBackScroll = Max(iBackScroll-1, -1);
			// reset fade
			Fader=0;
		}

		break;

	case 1: // > 2 msgs
		break;

	}

	// Draw
	Draw(Output);
	}

void C4MessageBoard::Init(C4Facet &cgo, bool fStartup)
	{
	Active=true;
	Output=cgo;
	Startup=fStartup;
	iLineHgt=::GraphicsResource.FontRegular.iLineHgt;
	LogBuffer.SetLBWidth(Output.Wdt);

	if(!Startup)
	{
		// set cursor to end of log
		iBackScroll = -1;
		// load msgboard mode from config
		iLines = Config.Graphics.MsgBoard;
		if(iLines == 0) ChangeMode(2);
		if(iLines == 1) ChangeMode(0);
		if(iLines >= 2) ChangeMode(1);
	}

	}

void C4MessageBoard::Draw(C4Facet &cgo)
	{
	if (!Active || !Application.Active) return;

	// Startup: draw Loader
	if (Startup)
		{
		if (::GraphicsSystem.pLoaderScreen)
			::GraphicsSystem.pLoaderScreen->Draw(cgo, Game.InitProgress, &LogBuffer);
		else
			// loader not yet loaded: black BG
			Application.DDraw->DrawBoxDw(cgo.Surface, 0,0, cgo.Wdt, cgo.Hgt, 0x00000000);
		return;
		}

	// Game running: message fader
	// Background
	Application.DDraw->BlitSurfaceTile(::GraphicsResource.fctBackground.Surface,cgo.Surface,cgo.X,cgo.Y,cgo.Wdt,cgo.Hgt,-cgo.X,-cgo.Y);

	// draw messages
	if(iMode != 2 || C4ChatInputDialog::IsShown())
		{
		// how many "extra" messages should be shown?
		int iMsgFader = iMode == 1 ? 0 : C4MSGB_MaxMsgFading;
		// check screenfader range
		if (ScreenFader >= C4MSGB_MaxMsgFading*iLineHgt)
			{
			ScreenFader = C4MSGB_MaxMsgFading*iLineHgt;
			iMsgFader = 0;
			}
		// show all msgs
		int iLines = (iMode == 1) ? this->iLines : 2;
		for(int iMsg = -iMsgFader-iLines; iMsg < 0; iMsg++)
			{
			// get message at pos
			if (iMsg-iBackScroll >= 0) break;
			const char *Message = LogBuffer.GetLine(iMsg-iBackScroll, NULL, NULL, NULL);
			if (!Message || !*Message) continue;
			// calc target position (y)
			int iMsgY = cgo.Y + (iMsg+(iLines-1)) * iLineHgt + Fader;

			// player message color?
			C4Player *pPlr = GetMessagePlayer(Message);

			DWORD dwColor;
			if (pPlr)
				dwColor = PlrClr2TxtClr(pPlr->ColorDw) & 0xffffff;
			else
				dwColor = 0xffffff;
			// fade out (msg fade)
			DWORD dwFade;
			if (iMsgY < cgo.Y)
				{
				dwFade = (0xff - BoundBy((cgo.Y - iMsgY + Max(ScreenFader, 0)) * 256 / Max(iMsgFader, 1) / iLineHgt, 0, 0xff)) << 24;
				::GraphicsSystem.OverwriteBg();
				}
			else
				dwFade = 0xff000000;
			dwColor |= dwFade;
			// Draw
			Application.DDraw->StringOut(Message,::GraphicsResource.FontRegular,1.0,cgo.Surface,cgo.X,iMsgY,dwColor);
			}
		}
	}

void C4MessageBoard::EnsureLastMessage()
	{
	// Ingore if startup or typein
	if (!Active || Startup) return;
	// not active: do nothing
	if (iMode == 2) return;
	// "console" mode: just set BackScroll to 0 and draw
	if (iMode == 1) { iBackScroll = 0; ::GraphicsSystem.Execute(); return; }
	// scroll mode: scroll until end of log
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
	// Not active
	if (!Active) return;
	// safety
	if (!szMessage || !*szMessage) return;
	// make sure new message will be drawn
	++iBackScroll;
	// register message in standard messageboard font
	LogBuffer.AppendLines(szMessage, &::GraphicsResource.FontRegular, 0, NULL);
	}

void C4MessageBoard::ClearLog()
	{
	LogBuffer.Clear();
	}

void C4MessageBoard::LogNotify()
	{
	// Not active
	if (!Active) return;
	// do not show startup board if GUI is active
	if (::pGUI && ::pGUI->IsActive()) return;
	// Reset
	iBackScroll=0;
	// Draw
	Draw(Output);
	// startup: Draw message board only and do page flip
	if (Startup) Application.DDraw->PageFlip();
	}

C4Player* C4MessageBoard::GetMessagePlayer(const char *szMessage)
	{
	// Scan message text for heading player name
	if(SEqual2(szMessage, "* "))
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
	return NULL;
	}

bool C4MessageBoard::ControlScrollUp()
	{
	if (!Active) return false;
	Delay=-1; Fader=0; Empty=false;
	iBackScroll++;
	return true;
	}

bool C4MessageBoard::ControlScrollDown()
	{
	if (!Active) return false;
	Delay=-1; Fader=0; Empty=false;
	if (iBackScroll > -1) iBackScroll--;
	return true;
	}

bool C4MessageBoard::ControlChangeMode()
	{
	if (!Active) return false;
	// toggle messageboard size
	ChangeMode((iMode+1)%3);
	return true;
	}
