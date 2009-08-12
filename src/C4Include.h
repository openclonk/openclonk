/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2005  Tobias Zwick
 * Copyright (c) 2005, 2008  Sven Eberhardt
 * Copyright (c) 2005-2006  Günther Brammer
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

/* Main header to include all others */

#ifndef INC_C4Include
#define INC_C4Include

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#pragma warning(disable: 4706)
#pragma warning(disable: 4239)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif //HAVE_CONFIG_H

#ifdef _WIN32
	#define C4_OS "win32"
#elif defined(__linux__)
	#define C4_OS "linux"
#elif defined(__APPLE__)
	#define C4_OS "mac"
#else
	#define C4_OS "unknown";
#endif


#ifndef HAVE_CONFIG_H
// different debugrec options
//#define DEBUGREC

// define directive STAT here to activate statistics
#undef STAT

#endif // HAVE_CONFIG_H

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

// fmod
#if defined USE_FMOD && !defined HAVE_SDL_MIXER
#define C4SOUND_USE_FMOD
#endif

#ifdef _WIN32
// resources
#include "../res/resource.h"
#endif // _WIN32

// Probably not working
#if defined(HAVE_MIDI_H) && !defined(USE_FMOD)
#define USE_WINDOWS_MIDI
#endif

#include <Standard.h>
#include <CStdFile.h>
#include <Fixed.h>
#include <StdAdaptors.h>
#include <StdBuf.h>
#include <StdConfig.h>
#include <StdCompiler.h>
#include <StdDDraw2.h>
#include <StdFacet.h>
#include <StdFile.h>
#include <StdFont.h>
#include <StdMarkup.h>
#include <StdPNG.h>
#include <StdResStr2.h>
#include <StdSurface2.h>

#include "C4Id.h"
#include "C4Prototypes.h"

#ifdef _WIN32
#include <mmsystem.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <stdarg.h>

//#define BIG_C4INCLUDE

#if defined(BIG_C4INCLUDE) && defined(C4ENGINE)
#include "C4AList.h"
#include "C4Application.h"
#include "C4Aul.h"
#include "C4ChatDlg.h"
#include "C4Client.h"
#include "C4Command.h"
#include "C4ComponentHost.h"
#include "C4Components.h"
#include "C4Config.h"
#include "C4Console.h"
#include "C4Control.h"
#include "C4DefGraphics.h"
#include "C4Def.h"
#include "C4DevmodeDlg.h"
#include "C4EditCursor.h"
#include "C4Effects.h"
#include "C4Extra.h"
#include "C4FacetEx.h"
#include "C4Facet.h"
#include "C4FileClasses.h"
#include "C4FileSelDlg.h"
#include "C4FindObject.h"
#include "C4Fonts.h"
#include "C4FullScreen.h"
#include "C4GameControl.h"
#include "C4GameControlNetwork.h"
#include "C4GameDialogs.h"
#include "C4Game.h"
#include "C4GameLobby.h"
#include "C4GameMessage.h"
#include "C4GameObjects.h"
#include "C4GameOptions.h"
#include "C4GameOverDlg.h"
#include "C4GamePadCon.h"
#include "C4GameSave.h"
#include "C4GraphicsResource.h"
#include "C4GraphicsSystem.h"
#include "C4Group.h"
#include "C4GroupSet.h"
#include "C4Gui.h"
#include "C4IDList.h"
#include "C4InfoCore.h"
#include "C4InputValidation.h"
#include "C4KeyboardInput.h"
#include "C4Landscape.h"
#include "C4LangStringTable.h"
#include "C4Language.h"
#ifndef NONETWORK
# include "C4League.h"
#endif
#include "C4LoaderScreen.h"
#include "C4LogBuf.h"
#include "C4Log.h"
#include "C4MapCreatorS2.h"
#include "C4Map.h"
#include "C4MassMover.h"
#include "C4Material.h"
#include "C4MaterialList.h"
#include "C4Menu.h"
#include "C4MessageBoard.h"
#include "C4MessageInput.h"
#include "C4MouseControl.h"
#include "C4MusicFile.h"
#include "C4MusicSystem.h"
#include "C4NameList.h"
#include "C4NetIO.h"
#include "C4Network2Client.h"
#include "C4Network2Dialogs.h"
#include "C4Network2Discover.h"
#include "C4Network2.h"
#include "C4Network2IO.h"
#include "C4Network2Players.h"
#include "C4Network2Res.h"
#include "C4Network2Stats.h"
#include "C4ObjectCom.h"
#include "C4Object.h"
#include "C4ObjectInfo.h"
#include "C4ObjectInfoList.h"
#include "C4ObjectList.h"
#include "C4ObjectMenu.h"
#include "C4PacketBase.h"
#include "C4Particles.h"
#include "C4PathFinder.h"
#include "C4Physics.h"
#include "C4Player.h"
#include "C4PlayerInfo.h"
#include "C4PlayerInfoListBox.h"
#include "C4PlayerList.h"
#include "C4PropertyDlg.h"
#include "C4PXS.h"
#include "C4Random.h"
#include "C4RankSystem.h"
#include "C4Record.h"
#include "C4Region.h"
#include "C4RoundResults.h"
#include "C4RTF.H"
#include "C4Scenario.h"
#include "C4Scoreboard.h"
#include "C4Script.h"
#include "C4ScriptHost.h"
#include "C4Sector.h"
#include "C4Shape.h"
#include "C4Sky.h"
#include "C4SolidMask.h"
#include "C4SoundSystem.h"
#include "C4Startup.h"
#include "C4StartupMainDlg.h"
#include "C4StartupNetDlg.h"
#include "C4StartupOptionsDlg.h"
#include "C4StartupAboutDlg.h"
#include "C4StartupPlrSelDlg.h"
#include "C4StartupScenSelDlg.h"
#include "C4Stat.h"
#include "C4StringTable.h"
#include "C4SurfaceFile.h"
#include "C4Surface.h"
#include "C4Teams.h"
#include "C4Texture.h"
#include "C4ToolsDlg.h"
#include "C4TransferZone.h"
#include "C4Update.h"
#include "C4UpperBoard.h"
#include "C4UserMessages.h"
#include "C4Value.h"
#include "C4ValueList.h"
#include "C4ValueMap.h"
#include "C4Video.h"
#include "C4Viewport.h"
#include "C4Weather.h"
#endif

#endif // INC_C4Include
