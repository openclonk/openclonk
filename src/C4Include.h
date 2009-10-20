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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
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

#ifdef HAVE_PRECOMPILED_HEADERS
#include "C4Application.h"
#include "C4FullScreen.h"
#include "C4Game.h"
#include "C4GraphicsSystem.h"
#include "C4Prototypes.h"
#include "C4Version.h"
#include "config/C4Config.h"
#include "config/C4ConfigShareware.h"
#include "config/C4Constants.h"
#include "config/C4SecurityCertificates.h"
#include "control/C4Control.h"
#include "control/C4GameControl.h"
#include "control/C4GameParameters.h"
#include "control/C4GameSave.h"
#include "control/C4PlayerInfo.h"
#include "control/C4Record.h"
#include "control/C4RoundResults.h"
#include "control/C4Teams.h"
#include "editor/C4Console.h"
#include "editor/C4DevmodeDlg.h"
#include "editor/C4EditCursor.h"
#include "editor/C4ObjectListDlg.h"
#include "editor/C4PropertyDlg.h"
#include "editor/C4ToolsDlg.h"
#include "game/C4GameVersion.h"
#include "game/C4Physics.h"
#include "game/landscape/C4Landscape.h"
#include "game/landscape/C4MapCreatorS2.h"
#include "game/landscape/C4Map.h"
#include "game/landscape/C4MassMover.h"
#include "game/landscape/C4Material.h"
#include "game/landscape/C4MaterialList.h"
#include "game/landscape/C4Particles.h"
#include "game/landscape/C4PathFinder.h"
#include "game/landscape/C4PXS.h"
#include "game/landscape/C4Region.h"
#include "game/landscape/C4Scenario.h"
#include "game/landscape/C4Sky.h"
#include "game/landscape/C4SolidMask.h"
#include "game/landscape/C4Texture.h"
#include "game/landscape/C4Weather.h"
#include "game/object/C4Command.h"
#include "game/object/C4DefGraphics.h"
#include "game/object/C4Def.h"
#include "game/object/C4GameObjects.h"
#include "game/object/C4Id.h"
#include "game/object/C4IDList.h"
#include "game/object/C4InfoCore.h"
#include "game/object/C4ObjectCom.h"
#include "game/object/C4Object.h"
#include "game/object/C4ObjectInfo.h"
#include "game/object/C4ObjectInfoList.h"
#include "game/object/C4ObjectList.h"
#include "game/object/C4ObjectMenu.h"
#include "game/object/C4Sector.h"
#include "game/object/C4Shape.h"
#include "game/player/C4Player.h"
#include "game/player/C4PlayerList.h"
#include "game/player/C4RankSystem.h"
#include "game/script/C4Effects.h"
#include "game/script/C4FindObject.h"
#include "game/script/C4Script.h"
#include "game/script/C4TransferZone.h"
#include "gui/C4ChatDlg.h"
#include "gui/C4DownloadDlg.h"
#include "gui/C4FileSelDlg.h"
#include "gui/C4Folder.h"
#include "gui/C4GameDialogs.h"
#include "gui/C4GameLobby.h"
#include "gui/C4GameMessage.h"
#include "gui/C4GameOptions.h"
#include "gui/C4GameOverDlg.h"
#include "gui/C4Gui.h"
#include "gui/C4KeyboardInput.h"
#include "gui/C4LoaderScreen.h"
#include "gui/C4MainMenu.h"
#include "gui/C4Menu.h"
#include "gui/C4MessageBoard.h"
#include "gui/C4MessageInput.h"
#include "gui/C4MouseControl.h"
#include "gui/C4PlayerInfoListBox.h"
#include "gui/C4Scoreboard.h"
#include "gui/C4StartupAboutDlg.h"
#include "gui/C4Startup.h"
#include "gui/C4StartupMainDlg.h"
#include "gui/C4StartupNetDlg.h"
#include "gui/C4StartupOptionsDlg.h"
#include "gui/C4StartupPlrSelDlg.h"
#include "gui/C4StartupScenSelDlg.h"
#include "gui/C4UpdateDlg.h"
#include "gui/C4UpperBoard.h"
#include "gui/C4UserMessages.h"
#include "gui/C4Viewport.h"
#include "lib/C4InputValidation.h"
#include "lib/C4LogBuf.h"
#include "lib/C4Log.h"
#include "lib/C4NameList.h"
#include "lib/C4Random.h"
#include "lib/C4Rect.h"
#include "lib/C4RTF.H"
#include "lib/C4Stat.h"
#include "lib/Fixed.h"
#include "lib/PathFinder.h"
#include "lib/Standard.h"
#include "lib/StdAdaptors.h"
#include "lib/StdBase64.h"
#include "lib/StdBuf.h"
#include "lib/StdColors.h"
#include "lib/StdCompiler.h"
#include "lib/StdMarkup.h"
#include "lib/StdResStr2.h"
#include "lib/StdResStr.h"
#include "lib/texture/C4FacetEx.h"
#include "lib/texture/C4Facet.h"
#include "lib/texture/C4GraphicsResource.h"
#include "lib/texture/C4SurfaceFile.h"
#include "lib/texture/C4Surface.h"
#include "lib/texture/StdPNG.h"
#include "network/C4Client.h"
#include "network/C4GameControlNetwork.h"
#include "network/C4InteractiveThread.h"
#include "network/C4League.h"
#include "network/C4NetIO.h"
#include "network/C4Network2Client.h"
#include "network/C4Network2Dialogs.h"
#include "network/C4Network2Discover.h"
#include "network/C4Network2.h"
#include "network/C4Network2IO.h"
#include "network/C4Network2IRC.h"
#include "network/C4Network2Players.h"
#include "network/C4Network2Reference.h"
#include "network/C4Network2Res.h"
#include "network/C4Network2Stats.h"
#include "network/C4PacketBase.h"
#include "platform/Bitmap256.h"
#include "platform/C4FileClasses.h"
#include "platform/C4FileMonitor.h"
#include "platform/C4Fonts.h"
#include "platform/C4GamePadCon.h"
#include "platform/C4MusicFile.h"
#include "platform/C4MusicSystem.h"
#include "platform/C4SoundSystem.h"
#include "platform/C4Video.h"
#include "platform/C4VideoPlayback.h"
#include "platform/Midi.h"
#include "platform/StdConfig.h"
#include "platform/StdD3D.h"
#include "platform/StdD3DShader.h"
#include "platform/StdDDraw2.h"
#include "platform/StdFacet.h"
#include "platform/StdFile.h"
#include "platform/StdFont.h"
#include "platform/StdGL.h"
#include "platform/StdNoGfx.h"
#include "platform/StdRegistry.h"
#include "platform/StdScheduler.h"
#include "platform/StdSurface2.h"
#include "platform/StdSurface8.h"
#include "platform/StdSync.h"
#include "platform/StdVideo.h"
#include "platform/StdWindow.h"
#include "script/C4AList.h"
#include "script/C4Aul.h"
#include "script/C4PropList.h"
#include "script/C4ScriptHost.h"
#include "script/C4StringTable.h"
#include "script/C4Value.h"
#include "script/C4ValueList.h"
#include "script/C4ValueMap.h"
#include "zlib/zutil.h"
#else // HAVE_PRECOMPILED_HEADERS
// Only include some standard headers
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
#endif // HAVE_PRECOMPILED_HEADERS

#endif // INC_C4Include
