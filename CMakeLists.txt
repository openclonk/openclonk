# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2009-2011  Günther Brammer
# Copyright (c) 2009-2012  Nicolas Hake
# Copyright (c) 2009  David Dormagen
# Copyright (c) 2009-2012  Armin Burgmeier
# Copyright (c) 2009-2010  Sven Eberhardt
# Copyright (c) 2009  Tobias Zwick
# Copyright (c) 2009  Richard Gerum
# Copyright (c) 2010-2011  Julius Michaelis
# Copyright (c) 2010-2011  Peter Wortmann
# Copyright (c) 2010-2011  Martin Plicht
# Copyright (c) 2010  Manuel Riecke
#
# Portions might be copyrighted by other authors who have contributed
# to OpenClonk.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# See isc_license.txt for full license and disclaimer.
#
# "Clonk" is a registered trademark of Matthes Bender.
# See clonk_trademark_license.txt for full license.

cmake_minimum_required (VERSION 2.6.0)
project (openclonk CXX C)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Version.txt)

set(CMAKE_ADDITIONAL_DEPS_PATH deps CACHE PATH "Additional directory to search for libraries and headers")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_ADDITIONAL_DEPS_PATH})
set(OC_CXX_FLAGS ${CMAKE_CXX_FLAGS})
separate_arguments(OC_CXX_FLAGS)
set(OC_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
separate_arguments(OC_CXX_FLAGS_DEBUG)
set(OC_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
separate_arguments(OC_EXE_LINKER_FLAGS)
set(OC_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
separate_arguments(OC_EXE_LINKER_FLAGS_DEBUG)

############################################################################
# User selectable options
############################################################################
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
option(USE_GL "Enable OpenGL support" ON)
SET(INITIAL_USE_SDL_MAINLOOP_VALUE OFF)
SET(INITIAL_USE_OPEN_AL OFF)
option(USE_SDL_MAINLOOP "Use SDL to create windows etc." ${INITIAL_USE_SDL_MAINLOOP_VALUE})
if(WIN32 AND FALSE) # disable DX option while it doesn't work anyway
	option(USE_DIRECTX "Enable DirectX support" ON)
	if(EXISTS $ENV{DXSDK_DIR})
		list(APPEND CMAKE_INCLUDE_PATH $ENV{DXSDK_DIR}/Include)
		if(CMAKE_CL_64)
			list(APPEND CMAKE_LIBRARY_PATH $ENV{DXSDK_DIR}/Lib/x64)
		else()
			list(APPEND CMAKE_LIBRARY_PATH $ENV{DXSDK_DIR}/Lib/x86)
		endif()
	else()

	endif()
else()
	SET(USE_DIRECTX OFF)
	SET(HAVE_FMOD OFF)
endif()
if(NOT USE_GL AND NOT USE_DIRECTX)
	message(STATUS "No graphics display enabled; building dedicated server")
	SET(USE_CONSOLE ON CACHE INTERNAL "Build dedicated server")
endif()
if(UNIX AND NOT USE_SDL_MAINLOOP AND NOT APPLE AND NOT USE_CONSOLE)
	option(USE_X11 "Use X11 to create windows etc." ON)
	option(USE_GTK "Use GTK for the developer mode" ON)
	option(USE_GTK3 "Use GTK3 instead of GTK2" OFF)
else()
	SET(USE_X11 OFF)
	SET(USE_GTK OFF)
	SET(USE_GTK3 OFF)
endif()
if(APPLE)
	option(USE_COCOA "Use Apple Cocoa for the developer mode and the windows." ON)
	option(USE_APPLE_CLANG "Use Apple Clang Compiler as C++ compiler." ON)
	SET(INITIAL_USE_OPEN_AL ON)
else()
	option(USE_COCOA OFF)
endif()
option(USE_OPEN_AL "Use OpenAL to play sounds" ${INITIAL_USE_OPEN_AL})
option(DEBUGREC "Debug records" OFF)
option(OC_BUILD_MULTIPROCESSOR "Use all processor cores to build" OFF)
option(WITH_AUTOMATIC_UPDATE "Build engine without automatic update support" OFF)

############################################################################
# Check for compiler quirks and features
############################################################################
include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX)
	CHECK_CXX_COMPILER_FLAG("-std=gnu++0x" USE_GCC_STD_0X)
	if(USE_GCC_STD_0X)
		list(APPEND OC_CXX_FLAGS "-std=gnu++0x")
	endif()
endif()

# ck 09-09-20: CMAKE_REQUIRED_FLAGS requires a string, not a list.
include(CheckCXXSourceCompiles)
foreach(FLAG ${OC_CXX_FLAGS})
	set(OC_REQUIRED_FLAGS "${OC_REQUIRED_FLAGS} ${FLAG}")
endforeach()

# ck 10-09-04: Reset CMAKE_REQUIRED_FLAGS after having done the C++0x checks
# otherwise they are used for C library checks as well and the C compiler bails
# out because it does not understand -std=gnu++0x
set(SAFE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS ${OC_REQUIRED_FLAGS})
CHECK_CXX_SOURCE_COMPILES("void f(struct D&&); int main() { return 0; }" HAVE_RVALUE_REF)
CHECK_CXX_SOURCE_COMPILES("int main() { void *d = nullptr; }" HAVE_NULLPTR)
CHECK_CXX_SOURCE_COMPILES("int main() { static_assert(true, \"\"); }" HAVE_STATIC_ASSERT)
set(CMAKE_REQUIRED_FLAGS "${SAFE_CMAKE_REQUIRED_FLAGS}")

if(MSVC_VERSION GREATER 1499)
	# Activate minimal rebuild
	if(OC_BUILD_MULTIPROCESSOR)
		list(APPEND OC_CXX_FLAGS /MP)
		list(REMOVE_ITEM OC_CXX_FLAGS_DEBUG /Gm)
	else()
		list(REMOVE_ITEM OC_CXX_FLAGS /MP)
		list(APPEND OC_CXX_FLAGS_DEBUG /Gm)
	endif()
	if (NOT CMAKE_CL_64)
		# Activate edit-and-continue
		list(REMOVE_ITEM OC_CXX_FLAGS_DEBUG /Zi)
		list(APPEND OC_CXX_FLAGS_DEBUG /ZI /Gy)
	endif()
	# do not link the release CRT in debug builds
	list(APPEND OC_EXE_LINKER_FLAGS_DEBUG "/NODEFAULTLIB:MSVCRT")
	set(HAVE_PRECOMPILED_HEADERS ON CACHE INTERNAL "Compiler supports precompiled headers")
endif()
if(MSVC_VERSION)
	# Suppress warnings about "non-secure" functions
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
	# Disable warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
	list(APPEND OC_CXX_FLAGS "/wd4244")
	# Disable warning C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data (64 bit build only)
	list(APPEND OC_CXX_FLAGS "/wd4267")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND OC_CXX_FLAGS "-Wall -Wextra -Wredundant-decls -Wendif-labels -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Winit-self -Wsign-promo -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual")
endif()

if(WIN32 AND MINGW)
	# Activate DEP and ASLR
	list(APPEND OC_EXE_LINKER_FLAGS "-Wl,--nxcompat -Wl,--dynamicbase")
endif()
############################################################################
# List target source files. Don't directly create a target since we condi-
# tionally add more sources depending on configuration options later.
############################################################################
# OC_CLONK_SOURCES: Sources for OpenClonk that are needed by every build.
# OC_SYSTEM_SOURCES: Sources for OpenClonk that are only needed by specific
# 	configurations.
set(OC_CLONK_SOURCES
	src/C4Globals.cpp
	src/c4group/C4ComponentHost.cpp
	src/c4group/C4ComponentHost.h
	src/c4group/C4Components.h
	src/c4group/C4Extra.cpp
	src/c4group/C4Extra.h
	src/c4group/C4Group.cpp
	src/c4group/C4Group.h
	src/c4group/C4GroupSet.cpp
	src/c4group/C4GroupSet.h
	src/c4group/C4LangStringTable.cpp
	src/c4group/C4LangStringTable.h
	src/c4group/C4Language.cpp
	src/c4group/C4Language.h
	src/c4group/C4Update.cpp
	src/c4group/C4Update.h
	src/c4group/CStdFile.cpp
	src/c4group/CStdFile.h
	src/C4Include.h
	src/C4Prototypes.h
	src/C4Version.h
	src/config/C4Config.cpp
	src/config/C4Config.h
	src/config/C4Constants.h
	src/config/C4Reloc.cpp
	src/config/C4Reloc.h
	src/control/C4Control.cpp
	src/control/C4Control.h
	src/control/C4GameControl.cpp
	src/control/C4GameControl.h
	src/control/C4GameParameters.cpp
	src/control/C4GameParameters.h
	src/control/C4GameSave.cpp
	src/control/C4GameSave.h
	src/control/C4PlayerControl.cpp
	src/control/C4PlayerControl.h
	src/control/C4PlayerInfoConflicts.cpp
	src/control/C4PlayerInfo.cpp
	src/control/C4PlayerInfo.h
	src/control/C4Record.cpp
	src/control/C4Record.h
	src/control/C4RoundResults.cpp
	src/control/C4RoundResults.h
	src/control/C4Teams.cpp
	src/control/C4Teams.h
	src/editor/C4Console.cpp
	src/editor/C4ConsoleGUICommon.h
	src/editor/C4ConsoleGUI.h
	src/editor/C4Console.h
	src/editor/C4EditCursor.cpp
	src/editor/C4EditCursor.h
	src/editor/C4ObjectListDlg.cpp
	src/editor/C4ObjectListDlg.h
	src/editor/C4ToolsDlg.cpp
	src/editor/C4ToolsDlg.h
	src/editor/C4ViewportWindow.cpp
	src/editor/C4ViewportWindow.h
	src/game/C4Application.cpp
	src/game/C4Application.h
	src/game/C4FullScreen.cpp
	src/game/C4FullScreen.h
	src/game/C4Game.cpp
	src/game/C4Game.h
	src/game/C4GameVersion.h
	src/game/C4GraphicsSystem.cpp
	src/game/C4GraphicsSystem.h
	src/game/C4Physics.h
	src/game/C4Viewport.cpp
	src/game/C4Viewport.h
	src/gamescript/C4Effect.cpp
	src/gamescript/C4Effect.h
	src/gamescript/C4FindObject.cpp
	src/gamescript/C4FindObject.h
	src/gamescript/C4GameScript.cpp
	src/gamescript/C4Script.h
	src/gamescript/C4TransferZone.cpp
	src/gamescript/C4TransferZone.h
	src/graphics/Bitmap256.cpp
	src/graphics/Bitmap256.h
	src/graphics/C4Draw.cpp
	src/graphics/C4DrawD3D.cpp
	src/graphics/C4DrawD3D.h
	src/graphics/C4DrawD3DShader.cpp
	src/graphics/C4DrawD3DShader.h
	src/graphics/C4DrawGL.cpp
	src/graphics/C4DrawGLCtx.cpp
	src/graphics/C4DrawGL.h
	src/graphics/C4DrawT.cpp
	src/graphics/C4DrawT.h
	src/graphics/C4Draw.h
	src/graphics/C4Facet.cpp
	src/graphics/C4FacetEx.cpp
	src/graphics/C4FacetEx.h
	src/graphics/C4Facet.h
	src/graphics/C4Fonts.cpp
	src/graphics/C4Fonts.h
	src/graphics/C4GraphicsResource.cpp
	src/graphics/C4GraphicsResource.h
	src/graphics/C4Surface.cpp
	src/graphics/C4Surface.h
	src/graphics/C4SurfaceLoaders.cpp
	src/graphics/CSurface8.cpp
	src/graphics/CSurface8.h
	src/graphics/StdPNG.cpp
	src/graphics/StdPNG.h
	src/gui/C4ChatDlg.cpp
	src/gui/C4ChatDlg.h
	src/gui/C4DownloadDlg.cpp
	src/gui/C4DownloadDlg.h
	src/gui/C4FileSelDlg.cpp
	src/gui/C4FileSelDlg.h
	src/gui/C4Folder.cpp
	src/gui/C4Folder.h
	src/gui/C4GameDialogs.cpp
	src/gui/C4GameDialogs.h
	src/gui/C4GameLobby.cpp
	src/gui/C4GameLobby.h
	src/gui/C4GameMessage.cpp
	src/gui/C4GameMessage.h
	src/gui/C4GameOptions.cpp
	src/gui/C4GameOptions.h
	src/gui/C4GameOverDlg.cpp
	src/gui/C4GameOverDlg.h
	src/gui/C4GfxErrorDlg.cpp
	src/gui/C4GfxErrorDlg.h
	src/gui/C4GuiButton.cpp
	src/gui/C4GuiCheckBox.cpp
	src/gui/C4GuiComboBox.cpp
	src/gui/C4GuiContainers.cpp
	src/gui/C4Gui.cpp
	src/gui/C4GuiDialogs.cpp
	src/gui/C4GuiEdit.cpp
	src/gui/C4Gui.h
	src/gui/C4GuiLabels.cpp
	src/gui/C4GuiListBox.cpp
	src/gui/C4GuiMenu.cpp
	src/gui/C4GuiTabular.cpp
	src/gui/C4KeyboardInput.cpp
	src/gui/C4KeyboardInput.h
	src/gui/C4LoaderScreen.cpp
	src/gui/C4LoaderScreen.h
	src/gui/C4MainMenu.cpp
	src/gui/C4MainMenu.h
	src/gui/C4Menu.cpp
	src/gui/C4Menu.h
	src/gui/C4MessageBoard.cpp
	src/gui/C4MessageBoard.h
	src/gui/C4MessageInput.cpp
	src/gui/C4MessageInput.h
	src/gui/C4MouseControl.cpp
	src/gui/C4MouseControl.h
	src/gui/C4PlayerInfoListBox.cpp
	src/gui/C4PlayerInfoListBox.h
	src/gui/C4Scoreboard.cpp
	src/gui/C4Scoreboard.h
	src/gui/C4StartupAboutDlg.cpp
	src/gui/C4StartupAboutDlg.h
	src/gui/C4Startup.cpp
	src/gui/C4Startup.h
	src/gui/C4StartupMainDlg.cpp
	src/gui/C4StartupMainDlg.h
	src/gui/C4StartupNetDlg.cpp
	src/gui/C4StartupNetDlg.h
	src/gui/C4StartupOptionsDlg.cpp
	src/gui/C4StartupOptionsDlg.h
	src/gui/C4StartupPlrSelDlg.cpp
	src/gui/C4StartupPlrSelDlg.h
	src/gui/C4StartupScenSelDlg.cpp
	src/gui/C4StartupScenSelDlg.h
	src/gui/C4UpperBoard.cpp
	src/gui/C4UpperBoard.h
	src/landscape/C4Landscape.cpp
	src/landscape/C4Landscape.h
	src/landscape/C4LandscapeRenderClassic.cpp
	src/landscape/C4LandscapeRender.cpp
	src/landscape/C4LandscapeRender.h
	src/landscape/C4Map.cpp
	src/landscape/C4MapCreatorS2.cpp
	src/landscape/C4MapCreatorS2.h
	src/landscape/C4Map.h
	src/landscape/C4MassMover.cpp
	src/landscape/C4MassMover.h
	src/landscape/C4Material.cpp
	src/landscape/C4Material.h
	src/landscape/C4MaterialList.cpp
	src/landscape/C4MaterialList.h
	src/landscape/C4Particles.cpp
	src/landscape/C4Particles.h
	src/landscape/C4PathFinder.cpp
	src/landscape/C4PathFinder.h
	src/landscape/C4PXS.cpp
	src/landscape/C4PXS.h
	src/landscape/C4Region.cpp
	src/landscape/C4Region.h
	src/landscape/C4Scenario.cpp
	src/landscape/C4Scenario.h
	src/landscape/C4Sky.cpp
	src/landscape/C4Sky.h
	src/landscape/C4SolidMask.cpp
	src/landscape/C4SolidMask.h
	src/landscape/C4Texture.cpp
	src/landscape/C4Texture.h
	src/landscape/C4Weather.cpp
	src/landscape/C4Weather.h
	src/lib/C4InputValidation.cpp
	src/lib/C4InputValidation.h
	src/lib/C4LogBuf.cpp
	src/lib/C4LogBuf.h
	src/lib/C4Log.cpp
	src/lib/C4Log.h
	src/lib/C4Markup.cpp
	src/lib/C4Markup.h
	src/lib/C4NameList.cpp
	src/lib/C4NameList.h
	src/lib/C4Random.cpp
	src/lib/C4Random.h
	src/lib/C4Real.cpp
	src/lib/C4Real.h
	src/lib/C4Rect.cpp
	src/lib/C4Rect.h
	src/lib/C4RTF.cpp
	src/lib/C4RTF.h
	src/lib/C4Stat.cpp
	src/lib/C4Stat.h
	src/lib/PathFinder.cpp
	src/lib/PathFinder.h
	src/lib/SHA1.h
	src/lib/Standard.cpp
	src/lib/Standard.h
	src/lib/StdAdaptors.h
	src/lib/StdBase64.cpp
	src/lib/StdBase64.h
	src/lib/StdBuf.cpp
	src/lib/StdBuf.h
	src/lib/StdColors.h
	src/lib/StdCompiler.cpp
	src/lib/StdCompiler.h
	src/lib/StdMesh.cpp
	src/lib/StdMesh.h
	src/lib/StdMeshLoaderBinaryChunks.cpp
	src/lib/StdMeshLoaderBinaryChunks.h
	src/lib/StdMeshLoaderBinary.cpp
	src/lib/StdMeshLoaderDataStream.h
	src/lib/StdMeshLoader.h
	src/lib/StdMeshLoaderXml.cpp
	src/lib/StdMeshMaterial.cpp
	src/lib/StdMeshMaterial.h
	src/lib/StdMeshMath.cpp
	src/lib/StdMeshMath.h
	src/lib/StdMeshUpdate.cpp
	src/lib/StdMeshUpdate.h
	src/lib/StdResStr2.cpp
	src/lib/StdResStr2.h
	src/lib/StdResStr.h
	src/network/C4Client.cpp
	src/network/C4Client.h
	src/network/C4GameControlNetwork.cpp
	src/network/C4GameControlNetwork.h
	src/network/C4InteractiveThread.cpp
	src/network/C4InteractiveThread.h
	src/network/C4League.cpp
	src/network/C4League.h
	src/network/C4NetIO.cpp
	src/network/C4NetIO.h
	src/network/C4Network2Client.cpp
	src/network/C4Network2Client.h
	src/network/C4Network2.cpp
	src/network/C4Network2Dialogs.cpp
	src/network/C4Network2Dialogs.h
	src/network/C4Network2Discover.cpp
	src/network/C4Network2Discover.h
	src/network/C4Network2.h
	src/network/C4Network2IO.cpp
	src/network/C4Network2IO.h
	src/network/C4Network2IRC.cpp
	src/network/C4Network2IRC.h
	src/network/C4Network2Players.cpp
	src/network/C4Network2Players.h
	src/network/C4Network2Reference.cpp
	src/network/C4Network2Reference.h
	src/network/C4Network2Res.cpp
	src/network/C4Network2ResDlg.cpp
	src/network/C4Network2Res.h
	src/network/C4Network2Stats.cpp
	src/network/C4Network2Stats.h
	src/network/C4Network2UPnP.h
	src/network/C4Packet2.cpp
	src/network/C4PacketBase.h
	src/object/C4Action.cpp
	src/object/C4Command.cpp
	src/object/C4Command.h
	src/object/C4Def.cpp
	src/object/C4DefGraphics.cpp
	src/object/C4DefGraphics.h
	src/object/C4Def.h
	src/object/C4DefList.cpp
	src/object/C4DefList.h
	src/object/C4GameObjects.cpp
	src/object/C4GameObjects.h
	src/object/C4Id.cpp
	src/object/C4Id.h
	src/object/C4IDList.cpp
	src/object/C4IDList.h
	src/object/C4InfoCore.cpp
	src/object/C4InfoCore.h
	src/object/C4MeshAnimation.cpp
	src/object/C4MeshAnimation.h
	src/object/C4Movement.cpp
	src/object/C4ObjectCom.cpp
	src/object/C4ObjectCom.h
	src/object/C4Object.cpp
	src/object/C4Object.h
	src/object/C4ObjectInfo.cpp
	src/object/C4ObjectInfo.h
	src/object/C4ObjectInfoList.cpp
	src/object/C4ObjectInfoList.h
	src/object/C4ObjectList.cpp
	src/object/C4ObjectList.h
	src/object/C4ObjectMenu.cpp
	src/object/C4ObjectMenu.h
	src/object/C4ObjectPtr.cpp
	src/object/C4ObjectPtr.h
	src/object/C4ObjectScript.cpp
	src/object/C4Sector.cpp
	src/object/C4Sector.h
	src/object/C4Shape.cpp
	src/object/C4Shape.h
	src/platform/C4App.cpp
	src/platform/C4App.h
	src/platform/C4AppT.cpp
	src/platform/C4AppWin32Impl.h
	src/platform/C4FileMonitor.cpp
	src/platform/C4FileMonitor.h
	src/platform/C4GamePadCon.cpp
	src/platform/C4GamePadCon.h
	src/platform/C4MusicFile.cpp
	src/platform/C4MusicFile.h
	src/platform/C4MusicSystem.cpp
	src/platform/C4MusicSystem.h
	src/platform/C4SoundLoaders.cpp
	src/platform/C4SoundLoaders.h
	src/platform/C4SoundSystem.cpp
	src/platform/C4SoundSystem.h
	src/platform/C4Video.cpp
	src/platform/C4Video.h
	src/platform/C4Window.h
	src/platform/C4windowswrapper.h
	src/platform/GetTime.cpp
	src/platform/PlatformAbstraction.cpp
	src/platform/PlatformAbstraction.h
	src/platform/StdFile.cpp
	src/platform/StdFile.h
	src/platform/StdFont.cpp
	src/platform/StdFont.h
	src/platform/StdRegistry.cpp
	src/platform/StdRegistry.h
	src/platform/StdScheduler.cpp
	src/platform/StdScheduler.h
	src/platform/StdSync.h
	src/platform/StdVideo.cpp
	src/platform/StdVideo.h
	src/player/C4Player.cpp
	src/player/C4Player.h
	src/player/C4PlayerList.cpp
	src/player/C4PlayerList.h
	src/player/C4RankSystem.cpp
	src/player/C4RankSystem.h
	src/script/C4Aul.cpp
	src/script/C4AulDebug.cpp
	src/script/C4AulDebug.h
	src/script/C4AulDefFunc.h
	src/script/C4AulExec.cpp
	src/script/C4AulExec.h
	src/script/C4AulFunc.cpp
	src/script/C4AulFunc.h
	src/script/C4Aul.h
	src/script/C4AulLink.cpp
	src/script/C4AulParse.cpp
	src/script/C4PropList.cpp
	src/script/C4PropList.h
	src/script/C4Script.cpp
	src/script/C4ScriptHost.cpp
	src/script/C4ScriptHost.h
	src/script/C4StringTable.cpp
	src/script/C4StringTable.h
	src/script/C4ValueArray.cpp
	src/script/C4ValueArray.h
	src/script/C4Value.cpp
	src/script/C4Value.h
	src/script/C4ValueMap.cpp
	src/script/C4ValueMap.h
	src/zlib/gzio.c
	src/zlib/gzio.h
	src/zlib/zutil.h
	thirdparty/timsort/sort.h
)
mark_as_advanced(OC_CLONK_SOURCES)
mark_as_advanced(OC_SYSTEM_SOURCES)

# source files specific to an operating system
if(APPLE)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4FileMonitorMac.mm
		src/platform/C4AppDelegate.h
		src/platform/C4AppDelegate.mm
	)
else()
	list(APPEND OC_SYSTEM_SOURCES
		src/game/ClonkMain.cpp
	)
endif()
if(WIN32)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4CrashHandlerWin32.cpp
		src/res/resource.h
	)

	if(MINGW)
		# cmake does not support compiling resources with MinGW
		# natively, see http://www.cmake.org/Bug/view.php?id=4068.
		# cross-compiler sets CMAKE_RC_COMPILER in toolchain file
		if(NOT CMAKE_RC_COMPILER)
			SET(CMAKE_RC_COMPILER windres)
		endif()

		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/engine_rc.o
		                   COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_BINARY_DIR}
		                                                -i${CMAKE_CURRENT_SOURCE_DIR}/src/res/engine.rc
		                                                -o ${CMAKE_CURRENT_BINARY_DIR}/engine_rc.o
		                   MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/res/engine.rc)

		list(APPEND OC_SYSTEM_SOURCES
			${CMAKE_CURRENT_BINARY_DIR}/engine_rc.o
		)
	else(MINGW)
		list(APPEND OC_SYSTEM_SOURCES
			src/res/engine.rc
		)
	endif(MINGW)
endif()

# source files specific to a GUI library
if(USE_GTK)
	list(APPEND OC_CLONK_SOURCES
		src/res/Brush.h
		src/res/Cursor.h
		src/res/Dynamic.h
		src/res/Exact.h
		src/res/Fill.h
		src/res/Halt.h
		src/res/Ift.h
		src/res/Line.h
		src/res/Mouse.h
		src/res/NoIft.h
		src/res/Picker.h
		src/res/Play.h
		src/res/Rect.h
		src/res/resource.h
		src/res/Static.h
	)

	list(APPEND OC_SYSTEM_SOURCES
		src/editor/C4ConsoleGTK.cpp
		src/editor/C4ConsoleGTKDlg.cpp
		src/editor/C4ConsoleGTKDlg.h
		src/platform/C4AppGTK.cpp
		src/platform/C4AppGTKImpl.h
		src/platform/C4WindowGTK.cpp
	)
elseif(USE_SDL_MAINLOOP)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4AppSDL.cpp
		src/platform/C4WindowSDL.cpp
	)
elseif(WIN32)
	list(APPEND OC_SYSTEM_SOURCES
		src/editor/C4ConsoleWin32.cpp
		src/platform/C4WindowWin32.cpp
		src/platform/StdJoystick.cpp
		src/platform/StdJoystick.h
	)
elseif(USE_COCOA)
	list(APPEND OC_SYSTEM_SOURCES
		src/editor/C4ConsoleCocoa.mm
		src/platform/C4AppMac.mm
		src/platform/C4WindowMac.mm
		src/platform/C4AppDelegate+MainMenuActions.h
		src/platform/C4AppDelegate+MainMenuActions.mm
		src/graphics/C4DrawGLMac.h
		src/graphics/C4DrawGLMac.mm
		src/platform/C4WindowController.h
		src/platform/C4WindowController.mm
		src/platform/CocoaKeycodeMap.h
		src/editor/C4EditorWindowController.h
		src/editor/C4EditorWindowController.mm
		src/platform/ObjectiveCAssociated.h
	)
endif()
if(WITH_AUTOMATIC_UPDATE)
	list(APPEND OC_CLONK_SOURCES
		src/gui/C4UpdateDlg.cpp
		src/gui/C4UpdateDlg.h
	)
endif()

if(PROJECT_FOLDERS)
  source_group("Core" src/.*)
  source_group("Platform abstraction" src/platform/.*)
  source_group("Utility" src/lib/.*)
  source_group("C4Group" src/c4group/.*)
  source_group("Graphics" src/graphics/.*)
  source_group("GUI" src/gui/.*)
  source_group("Network" src/network/.*)
  source_group("Object" src/object/.*)
  source_group("Landscape" src/landscape/.*)
  source_group("Player" src/player/.*)
  source_group("Script" REGULAR_EXPRESSION .*script/.*)
  source_group("Config" src/config/.*)
  source_group("Control" src/control/.*)
  source_group("Editing" src/editor/.*)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/c4group
	${CMAKE_CURRENT_SOURCE_DIR}/src/config
	${CMAKE_CURRENT_SOURCE_DIR}/src/control
	${CMAKE_CURRENT_SOURCE_DIR}/src/editor
	${CMAKE_CURRENT_SOURCE_DIR}/src/game
	${CMAKE_CURRENT_SOURCE_DIR}/src/gamescript
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui
	${CMAKE_CURRENT_SOURCE_DIR}/src/landscape
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib
	${CMAKE_CURRENT_SOURCE_DIR}/src/network
	${CMAKE_CURRENT_SOURCE_DIR}/src/object
	${CMAKE_CURRENT_SOURCE_DIR}/src/platform
	${CMAKE_CURRENT_SOURCE_DIR}/src/player
	${CMAKE_CURRENT_SOURCE_DIR}/src/res
	${CMAKE_CURRENT_SOURCE_DIR}/src/script
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

############################################################################
# Check for required system headers
############################################################################
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(direct.h HAVE_DIRECT_H)
CHECK_INCLUDE_FILE_CXX(io.h HAVE_IO_H)
CHECK_INCLUDE_FILE_CXX(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(share.h HAVE_SHARE_H)
CHECK_INCLUDE_FILE_CXX(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CXX(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CXX(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CXX(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CXX(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CXX(execinfo.h HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE_CXX(langinfo.h HAVE_LANGINFO_H)
CHECK_INCLUDE_FILE_CXX(poll.h HAVE_POLL_H)
CHECK_INCLUDE_FILE_CXX(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILE_CXX(sys/timerfd.h HAVE_SYS_TIMERFD_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/eventfd.h HAVE_SYS_EVENTFD_H)
CHECK_INCLUDE_FILE_CXX(sys/file.h HAVE_SYS_FILE_H)
if(USE_CONSOLE)
	CHECK_INCLUDE_FILE_CXX(readline.h HAVE_READLINE_H)
	CHECK_INCLUDE_FILE_CXX(readline/readline.h HAVE_READLINE_READLINE_H)
endif()
CHECK_INCLUDE_FILE_CXX(natupnp.h HAVE_NATUPNP_H)

# ck 09-09-20: The following headers require Xlib.h for things such as
# 'Bool' and 'Window' to be defined. Unfortunately, this doesn't exist
# as a CXX version (yet?).
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/extensions/Xrandr.h HAVE_X11_EXTENSIONS_XRANDR_H)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/keysym.h HAVE_X11_KEYSYM_H)
CHECK_INCLUDE_FILE_CXX(iconv.h HAVE_ICONV)

if(HAVE_ICONV)
	SET(ICONV_CONST ON)
endif()

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(vasprintf HAVE_VASPRINTF)

if(HAVE_ICONV)
	CHECK_FUNCTION_EXISTS(iconv HAVE_ICONV_WO_LINK)
	mark_as_advanced(HAVE_ICONV_WO_LINK)

	if(NOT HAVE_ICONV_WO_LINK)
		FIND_LIBRARY( ICONV_LIBRARY NAMES iconv )
		MARK_AS_ADVANCED( ICONV_LIBRARY )
	endif()
endif()

if(HAVE_NATUPNP_H)
	list(APPEND OC_SYSTEM_SOURCES
		src/network/C4Network2UPnPWin32.cpp
	)
else()
	FIND_PACKAGE(Upnp)
	SET(HAVE_UPNP ${UPNP_FOUND})
	if(UPNP_FOUND)
		list(APPEND OC_SYSTEM_SOURCES
			src/network/C4Network2UPnPLinux.cpp
		)
	else()
		list(APPEND OC_SYSTEM_SOURCES
			src/network/C4Network2UPnPDummy.cpp
		)
	endif()
endif()

############################################################################
# Locate necessary libraries
############################################################################
# Find win32 SDK, since a lot of users don't have the include path in their environment
if(MSVC)
	get_filename_component(WINSDK_ROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE CACHE)
	mark_as_advanced(WINSDK_ROOT)
	include_directories(SYSTEM ${WINSDK_ROOT}/Include)
	if(${FIND_LIBRARY_USE_LIB64_PATHS})
		list(APPEND CMAKE_LIBRARY_PATH ${WINSDK_ROOT}/Lib/x64)
	else()
		list(APPEND CMAKE_LIBRARY_PATH ${WINSDK_ROOT}/Lib)
	endif()
endif()

SET(JPEG_NAMES ${JPEG_NAMES} libjpeg)
include(FindJPEG)
include(FindPNG)
include(FindZLIB)
include_directories(${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

find_package("Boost" 1.40.0)
if(NOT Boost_INCLUDE_DIR)
	message(SEND_ERROR "Could not find the Boost C++ Libraries")
else()
	include_directories(SYSTEM ${Boost_INCLUDE_DIR})
endif()

include(FindThreads)
if(NOT WIN32)
	SET(HAVE_PTHREAD ${CMAKE_USE_PTHREADS_INIT} CACHE INTERNAL "libpthread available")
endif()

# isilkor 09-05-29: FindFreetype only checks for no-ver and 219
find_library(FREETYPE_LIBRARY NAMES freetype239)
include(FindFreetype)
include_directories(${FREETYPE_INCLUDE_DIRS})
SET(HAVE_FREETYPE ${FREETYPE_FOUND} CACHE INTERNAL "Rename of FREETYPE_FOUND for config.h")

# FINDLIB works the same as find_library, but also marks the resulting var as
# advanced so it doesn't show in GUIs by default
macro(FINDLIB lib)
	find_library(${ARGV})
	mark_as_advanced(${lib})
endmacro(FINDLIB)

if(USE_CONSOLE)
	find_package(Readline)
	include_directories(${READLINE_INCLUDE_DIRS})
	SET(HAVE_LIBREADLINE ${READLINE_FOUND} CACHE INTERNAL "libreadline available")
endif()

# Set GTK link directory. This needs to be done before add_executable,
# otherwise the path is not used for linking clonk
if(USE_GTK)
	include(FindPkgConfig)
	if (USE_GTK3)
		pkg_check_modules(GTK3 REQUIRED "glib-2.0 >= 2.27.5" "gtk+-3.0 >= 2.99.0")
		include_directories(${GTK3_INCLUDE_DIRS})
		link_directories(${GTK3_LIBRARY_DIRS})
	else()
		pkg_check_modules(GTK REQUIRED "glib-2.0 >= 2.8" "gtk+-2.0 >= 2.8")
		include_directories(${GTK_INCLUDE_DIRS})
		link_directories(${GTK_LIBRARY_DIRS})
	endif()
endif()

if(USE_OPEN_AL)
	if(MSVC)
		if(${FIND_LIBRARY_USE_LIB64_PATHS})
			FINDLIB(OPENAL_LIBRARY NAMES OpenAL64)
		else()
			FINDLIB(OPENAL_LIBRARY NAMES OpenAL32)
		endif()
		FINDLIB(OGG_LIBRARY NAMES libogg_static)
		FINDLIB(VORBIS_LIBRARY NAMES libvorbis_static)
		FINDLIB(VORBISFILE_LIBRARY NAMES libvorbisfile_static)
	else()
		FINDLIB(VORBIS_LIBRARY NAMES vorbis)
		FINDLIB(VORBISFILE_LIBRARY NAMES vorbisfile)
		FINDLIB(OGG_LIBRARY NAMES ogg)
	endif()
	if(NOT APPLE)
		FINDLIB(ALUT_LIBRARY NAMES alut)
	endif()
endif()

############################################################################
# Precompiled header support, part 1 (pre-target)
############################################################################
if(HAVE_PRECOMPILED_HEADERS)
	if(MSVC_VERSION)
		list(INSERT OC_SYSTEM_SOURCES 0 src/C4Include.cpp)
	endif()
endif()

############################################################################
# Mac OS bundle setup
############################################################################

if(APPLE)

	list(APPEND OC_BUNDLE_RESOURCES
		src/res/Clonk.icns
		src/res/ocd.icns src/res/ocf.icns src/res/ocg.icns
		src/res/C4P.icns src/res/ocs.icns src/res/ocu.icns
		src/res/MainMenu.xib
		src/res/FullScreen.xib
		src/res/EditorGUIWindow.xib
		src/res/EditorViewport.xib
		src/res/Editor.xib
		src/res/Mouse_Trans.png
		src/res/Cursor_Trans.png
		src/res/Brush_Trans.png
		src/res/Play_Trans.png
		src/res/Halt_Trans.png
		src/res/Dynamic_Trans.png
		src/res/Static_Trans.png
		src/res/Exact_Trans.png
		src/res/Line_Trans.png
		src/res/Rect_Trans.png
		src/res/Fill_Trans.png
		src/res/Picker_Trans.png
		src/res/Ift_Trans.png
		src/res/NoIft_Trans.png
	)

	# Add icon resources
	set_source_files_properties(
		${OC_BUNDLE_RESOURCES}
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)
	list(APPEND OC_SYSTEM_SOURCES ${OC_BUNDLE_RESOURCES})

endif()

############################################################################
# Generate output files
############################################################################
add_definitions(-DHAVE_CONFIG_H)
add_executable(clonk WIN32 MACOSX_BUNDLE
	${OC_SYSTEM_SOURCES}
	${OC_CLONK_SOURCES}
)

add_executable(c4group
	src/c4group/C4GroupMain.cpp
	src/c4group/C4Group.cpp
	src/lib/C4InputValidation.cpp
	src/c4group/C4Update.cpp
	src/lib/Standard.cpp
	src/c4group/CStdFile.cpp
	src/lib/C4Markup.cpp
	src/lib/StdBuf.cpp
	src/lib/StdCompiler.cpp
	src/platform/StdFile.cpp
	src/platform/StdRegistry.cpp
	src/lib/StdResStr2.cpp
	src/lib/C4SimpleLog.cpp
	src/zlib/gzio.c
	src/C4Include.cpp
)

add_executable(netpuncher EXCLUDE_FROM_ALL
	src/C4Include.cpp
	src/lib/C4SimpleLog.cpp
	src/lib/StdBuf.cpp
	src/lib/Standard.cpp
	src/platform/GetTime.cpp
	src/platform/StdScheduler.cpp
	src/platform/StdFile.cpp
	src/network/C4NetIO.cpp
	src/netpuncher/main.cpp
)

add_executable(c4script
	include/c4script/c4script.h
	src/c4group/C4Group.cpp
	src/c4group/C4Group.h
	src/c4group/CStdFile.cpp
	src/c4group/CStdFile.h
	src/lib/C4InputValidation.cpp
	src/lib/C4InputValidation.h
	src/lib/C4SimpleLog.cpp
	src/lib/Standard.cpp
	src/lib/Standard.h
	src/lib/StdBuf.cpp
	src/lib/StdBuf.h
	src/lib/StdCompiler.cpp
	src/lib/StdCompiler.h
	src/lib/C4Markup.cpp
	src/lib/C4Markup.h
	src/lib/StdResStr2.cpp
	src/lib/StdResStr2.h
	src/platform/GetTime.cpp
	src/platform/StdFile.cpp
	src/platform/StdFile.h
	src/lib/C4Real.cpp
	src/lib/C4Random.cpp
	src/script/C4Aul.cpp
	src/script/C4AulExec.cpp
	src/script/C4AulFunc.cpp
	src/script/C4AulLink.cpp
	src/script/C4AulParse.cpp
	src/script/C4StringTable.cpp
	src/script/C4PropList.cpp
	src/script/C4ScriptHost.cpp
	src/script/C4ScriptMain.cpp
	src/script/C4ScriptStandalone.cpp
	src/script/C4ValueArray.cpp
	src/script/C4Value.cpp
	src/script/C4ValueMap.cpp
	src/object/C4Id.cpp
	src/script/C4Script.cpp
	src/c4group/C4GroupSet.cpp
	src/c4group/C4ComponentHost.cpp
	src/c4group/C4LangStringTable.cpp
	src/zlib/gzio.c
	src/C4Include.cpp
)

target_link_libraries(clonk
	${FREETYPE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${ICONV_LIBRARY}
	${VORBIS_LIBRARY}
	${VORBISFILE_LIBRARY}
	${OGG_LIBRARY}
	${OPENAL_LIBRARY}
	${ALUT_LIBRARY}
)
target_link_libraries(c4group
	${ZLIB_LIBRARIES}
)
target_link_libraries(c4script
	${ZLIB_LIBRARIES}
)
if(HAVE_PTHREAD)
	target_link_libraries(netpuncher
		pthread
	)
	target_link_libraries(c4script
		pthread
	)
endif()
if(USE_CONSOLE)
	target_link_libraries(clonk
		pthread
		${READLINE_LIBRARIES}
	)
endif()
if(UNIX AND NOT APPLE)
	target_link_libraries(c4group rt)
	target_link_libraries(c4script rt)
	target_link_libraries(netpuncher rt)
	target_link_libraries(clonk rt)
endif()
set_property(TARGET clonk APPEND PROPERTY COMPILE_DEFINITIONS GLEW_STATIC)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG _DEBUG)

# This expands some variables in Info.plist as a side-effect. XCode might then 
# expand a second time, using the same syntax. Try not to get confused by this!
set_target_properties(clonk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/Info.plist")

if(MSVC)
	# set target output filenames to a per-configuration value
	function(oc_set_target_names target_name)
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER "${CONFIG}" CONFIG_UPPER)
			string(TOLOWER "${CONFIG}" CONFIG_LOWER)
			set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_CURRENT_BINARY_DIR}")
			if(NOT "${CONFIG}" STREQUAL "RelWithDebInfo")
				set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_NAME_${CONFIG_UPPER} "${target_name}-${CONFIG_LOWER}")
			endif()
		endforeach()
	endfunction()
	oc_set_target_names(clonk)
	oc_set_target_names(c4group)
	oc_set_target_names(c4script)
	oc_set_target_names(netpuncher)
endif()

############################################################################
# Precompiled header support, part 2 (post-target)
############################################################################
if(HAVE_PRECOMPILED_HEADERS)
	if(MSVC_VERSION)
		get_property(OC_SOURCES TARGET clonk PROPERTY SOURCES)
		foreach(FILE ${OC_SOURCES})
			get_filename_component(FILE_EXT ${FILE} EXT)
			get_filename_component(FILE_NAME_WE ${FILE} NAME_WE)
			if(FILE_EXT STREQUAL ".cpp")
				# This check is necessary since gzio.c gets compiled as C
				# code, which can't use C++ PCH.
				set_property(SOURCE ${FILE} PROPERTY COMPILE_FLAGS /YuC4Include.h)
			endif()
		endforeach()
		set_property(SOURCE src/C4Include.cpp PROPERTY COMPILE_FLAGS /YcC4Include.h)
	endif()
endif()

############################################################################
# Assemble compiler flags
############################################################################
if(UNIX)
	# Don't put this into CMAKE_CXX_FLAGS because otherwise it is cached,
	# and when the path is changed both the old and new definition appears
	# in the list of flags.
	add_definitions("-DOC_SYSTEM_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/share/openclonk\"")
endif()
if(OC_CXX_FLAGS)
	list(REMOVE_DUPLICATES OC_CXX_FLAGS)
endif()
set(CMAKE_CXX_FLAGS "" CACHE STRING "C++ compiler flags" FORCE)
foreach(FLAG ${OC_CXX_FLAGS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" CACHE STRING "C++ compiler flags" FORCE)
endforeach()
if(OC_CXX_FLAGS_DEBUG)
	list(REMOVE_DUPLICATES OC_CXX_FLAGS_DEBUG)
endif()
set(CMAKE_CXX_FLAGS_DEBUG "" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
foreach(FLAG ${OC_CXX_FLAGS_DEBUG})
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
endforeach()
if(OC_EXE_LINKER_FLAGS)
	list(REMOVE_DUPLICATES OC_EXE_LINKER_FLAGS)
endif()
set(CMAKE_EXE_LINKER_FLAGS "" CACHE STRING "Flags used by the linker." FORCE)
foreach(FLAG ${OC_EXE_LINKER_FLAGS})
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}" CACHE STRING "Flags used by the linker." FORCE)
endforeach()
if(OC_EXE_LINKER_FLAGS_DEBUG)
	list(REMOVE_DUPLICATES OC_EXE_LINKER_FLAGS_DEBUG)
endif()
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE STRING "Flags used by the linker during debug builds." FORCE)
foreach(FLAG ${OC_EXE_LINKER_FLAGS_DEBUG})
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${FLAG}" CACHE STRING "Flags used by the linker during debug builds." FORCE)
endforeach()

############################################################################
# Precompiled header support, gcc part (it needs the cxx flags)
############################################################################

if(CMAKE_COMPILER_IS_GNUCXX)
	include(GccPchSupport)
	option(USE_GCC_PCH "Use GCC precompiled headers" ON)
endif()
if(USE_GCC_PCH)
	add_precompiled_header(clonk src/C4Include.h)
	add_precompiled_header(c4group src/C4Include.h)
	add_precompiled_header(c4script src/C4Include.h)
endif()

############################################################################
# Some Xcode/OSX specific settings involving building with clang, precompiled headers...
############################################################################

if (APPLE)
	add_custom_command(TARGET clonk
		POST_BUILD COMMAND "/usr/bin/ruby" "${CMAKE_CURRENT_SOURCE_DIR}/tools/osx_bundle_libs"
	)
	
	set(CMAKE_XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
	set(CMAKE_XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
	
	SET_TARGET_PROPERTIES(clonk PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
	SET_TARGET_PROPERTIES(clonk PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++ objective-c++")
	SET_TARGET_PROPERTIES(clonk PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
	SET_TARGET_PROPERTIES(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
	SET_TARGET_PROPERTIES(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++ objective-c++")
	SET_TARGET_PROPERTIES(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
	SET_TARGET_PROPERTIES(clonk PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
	
	if (USE_APPLE_CLANG)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
		set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++0x -stdlib=libc++ -g -Wall")
		SET_TARGET_PROPERTIES(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++0x objective-c++0x")
		SET_TARGET_PROPERTIES(clonk PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++0x objective-c++0x")
				
		set(HAVE_RVALUE_REF ON)
	else()
		set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
	endif()
endif()

############################################################################
# Locate optional libraries (if so desired)
############################################################################
CHECK_INCLUDE_FILE_CXX(getopt.h HAVE_GETOPT_H)
if(NOT HAVE_GETOPT_H)
	include_directories(thirdparty/getopt)
	add_subdirectory(thirdparty/getopt)
	target_link_libraries(clonk getopt)
elseif(MINGW)
	# Link libiberty which my contain getopt with mingw
	FINDLIB(iberty_LIBRARIES iberty)
	if(iberty_LIBRARIES)
		target_link_libraries(clonk iberty)
	endif()
endif()

CHECK_INCLUDE_FILE_CXX(getopt.h HAVE_GETOPT_H)

# TinyXML
add_subdirectory(thirdparty/tinyxml)
target_link_libraries(clonk tinyxml)

if(WIN32)
	find_package(DbgHelp)
	if(DBGHELP_FOUND)
		target_link_libraries(clonk ${DBGHELP_LIBRARIES})
		include_directories(${DBGHELP_INCLUDE_DIR})
	endif()
	set(HAVE_DBGHELP ${DBGHELP_FOUND})
endif()

if(USE_GL)
	include(FindOpenGL)
	FINDLIB(GLEW_LIBRARIES NAMES GLEW glew32 glew32s)
	FIND_PATH(GLEW_INCLUDE_DIR NAMES GL/glew.h)
	target_link_libraries(clonk
		${GLEW_LIBRARIES}
		${OPENGL_LIBRARIES}
	)
	include_directories(${GLEW_INCLUDE_DIR})
endif()
if(USE_DIRECTX)
	FINDLIB(DIRECT3D_LIBRARIES d3d9)
	FINDLIB(DIRECT3DX_LIBRARIES d3dx9)
	FIND_PATH(DIRECT3D_INCLUDE_DIR NAMES d3d9.h)
	FIND_PATH(DIRECT3DX_INCLUDE_DIR NAMES d3dx9.h)
	target_link_libraries(clonk
		${DIRECT3D_LIBRARIES}
		${DIRECT3DX_LIBRARIES}
	)
	include_directories(
		${DIRECT3D_INCLUDE_DIR}
		${DIRECT3DX_INCLUDE_DIR}
	)
endif()
if(USE_GTK)
	SET(WITH_DEVELOPER_MODE ${GTK_FOUND})
	SET(WITH_GLIB ${GTK_FOUND})
	if(USE_GTK3)
		target_link_libraries(clonk
			${GTK3_LIBRARIES}
		)
	else()
		target_link_libraries(clonk
			${GTK_LIBRARIES}
		)
	endif()
endif()
if(USE_X11)
	FINDLIB(X11_LIBRARIES X11)
	FINDLIB(XRANDR_LIBRARIES Xrandr)
	target_link_libraries(clonk
		${X11_LIBRARIES}
		${XRANDR_LIBRARIES}
	)
endif()
if(USE_COCOA)
	#stupid fix: just link to iconv that way
	TARGET_LINK_LIBRARIES(clonk "-framework Cocoa -framework AppKit -framework Quartz -framework OpenAL -framework AudioToolBox -liconv")
endif()
if (WIN32)
	# CMake is too incompetent to check whether these libraries can be linked to
	# So just pretend that everything is fine
	FINDLIB(VFW32_LIBRARIES vfw32)
	FINDLIB(wavifil32_LIBRARIES wavifil32)
	if (VFW32_LIBRARIES)
		target_link_libraries(clonk vfw32)
		set(HAVE_VFW32 TRUE)
	elseif(wavifil32_LIBRARIES)
		target_link_libraries(clonk wavifil32 msvfw32)
		set(HAVE_VFW32 TRUE)
	endif()

	target_link_libraries(clonk ws2_32 winmm)
	target_link_libraries(c4group ws2_32)
	target_link_libraries(netpuncher ws2_32 winmm)
	target_link_libraries(c4script ws2_32 winmm)

	if(NOT USE_OPEN_AL)
		find_package(FMod)
		if(FMOD_FOUND)
			set(HAVE_FMOD TRUE)
			target_link_libraries(clonk
				${FMOD_LIBRARIES}
			)
			include_directories(${FMOD_INCLUDE_DIR})
		else()
			set(HAVE_FMOD FALSE)
		endif()
	endif()
endif()
if(NOT HAVE_FMOD AND NOT USE_OPEN_AL OR USE_SDL_MAINLOOP AND NOT USE_OPEN_AL)
	include(FindSDL)
	SET(HAVE_SDL ${SDL_FOUND})
	if(SDL_FOUND)
		include_directories(${SDL_INCLUDE_DIR})
		FINDLIB(SDLMIXER_LIBRARIES SDL_mixer)
		if(SDLMIXER_LIBRARIES)
			SET(HAVE_LIBSDL_MIXER ON)
		endif()
		target_link_libraries(clonk
			${SDL_LIBRARY}
			${SDLMIXER_LIBRARIES}
		)
	endif()
endif()

if(HAVE_UPNP)
	include_directories(${UPNP_INCLUDE_DIR})
	target_link_libraries(clonk ${UPNP_LIBRARIES})
endif()

# GTest
include(FindGTest)
if(GTEST_FOUND)
	include_directories(${GTEST_INCLUDE_DIRS})
	add_executable(tests EXCLUDE_FROM_ALL
		tests/UnicodeHandlingTest.cpp
		tests/main.cpp
	)
	target_link_libraries(tests ${GTEST_LIBRARIES})
	if(HAVE_PTHREAD)
		target_link_libraries(tests pthread)
	endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# CMake support for MSVC 2010 is broken. Change the .sln file to avoid
# millions of "reload X project?" dialog boxes.
# For best results, also change CMake's ReloadProjects macro to only
# call StopBuild().
if(MSVC_VERSION EQUAL 1600)
	file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln" "\n# reload me\n")
endif()

############################################################################
# installation
############################################################################
# Install the icon into share/icons/hicolor/48x48/apps/clonk.png. Do this by
# extracting the correct size from oc.ico. Currently this is layer 2 - let's
# hope that it stays this way.

# Only allow installation for WITH_AUTOMATIC_UPDATE because otherwise
# the installed binary wouldn't find its game data in the system data path.
IF(WITH_AUTOMATIC_UPDATE)
	INSTALL(CODE "MESSAGE(SEND_ERROR \"Installation is only supported for WITH_AUTOMATIC_UPDATE disabled\")")
ENDIF()

# hack to build the data on install, see
# http://public.kitware.com/Bug/view.php?id=8438
add_custom_target(data)
install(
	CODE
	"execute_process(
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target data
		)"
	)

# TODO: Check for convert at configure step?
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/clonk.png
	COMMAND convert "${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc.ico[2]" "${CMAKE_CURRENT_BINARY_DIR}/clonk.png"
	MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc.ico
	VERBATIM
	)
add_custom_target(icon DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/clonk.png)
add_dependencies(data icon)
install(
	FILES ${CMAKE_CURRENT_BINARY_DIR}/clonk.png
	DESTINATION share/icons/hicolor/48x48/apps
	)

set(OC_C4GROUPS
	Graphics.ocg
	Material.ocg
	Music.ocg
	Sound.ocg
	System.ocg
	Objects.ocd
	Arena.ocf
	Parkour.ocf
	Settlement.ocf
	Tutorial.ocf
)

get_target_property(C4GROUP_LOCATION c4group LOCATION)
get_target_property(CLONK_LOCATION clonk LOCATION)
foreach(group ${OC_C4GROUPS})
	if (APPLE)
		add_custom_command(TARGET clonk
			POST_BUILD COMMAND "/bin/sh" "${CMAKE_CURRENT_SOURCE_DIR}/tools/osx_pack_gamedata.sh"
			"${C4GROUP_LOCATION}"
			"${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}"
			DEPENDS c4group
			)
	else()
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${group}
			COMMAND c4group ARGS "${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}" -t "${CMAKE_CURRENT_BINARY_DIR}/${group}"
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}
			DEPENDS c4group
			VERBATIM
			)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${group} DESTINATION share/openclonk)
	endif()
endforeach()
if (NOT APPLE)
	add_custom_target(groups DEPENDS ${OC_C4GROUPS})
	add_dependencies(data groups)
endif()

# Install new files
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/clonk.desktop DESTINATION share/applications)

# Install binaries
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/clonk ${CMAKE_CURRENT_BINARY_DIR}/c4group DESTINATION bin/)

############################################################################
# setup_openclonk.exe
############################################################################

find_program(MAKENSIS makensis PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS])

string(REPLACE / \\ C4GROUP_LOCATION ${C4GROUP_LOCATION})
string(REPLACE / \\ CLONK_LOCATION ${CLONK_LOCATION})

add_custom_command(
	OUTPUT setup_openclonk.exe
	COMMAND ${MAKENSIS} -NOCD -DSRCDIR=${CMAKE_CURRENT_SOURCE_DIR} -DPROGRAMFILES=$PROGRAMFILES "-DPRODUCT_NAME=${C4ENGINENAME}${C4VERSIONBUILDNAME}" "-DPRODUCT_COMPANY=${C4PROJECT}" "-DCLONK=${CLONK_LOCATION}" "-DC4GROUP=${C4GROUP_LOCATION}" ${CMAKE_CURRENT_SOURCE_DIR}/tools/install/oc.nsi "-XOutFile setup_openclonk.exe"
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/oc.nsi
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/header.bmp
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/inst.ico
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/uninst.ico
		${OC_C4GROUPS} clonk c4group
	VERBATIM
)

add_custom_target(setup
	DEPENDS setup_openclonk.exe
)

############################################################################
# CPack support
############################################################################
set(CPACK_PACKAGE_NAME "openclonk")
set(CPACK_PACKAGE_VENDOR "${C4PROJECT_URL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenClonk - A game mix out of tactic, strategy and Jump & Run")
set(CPACK_PACKAGE_VERSION "${C4XVER1}.${C4XVER2}.${C4XVER3}")
set(CPACK_PACKAGE_VERSION_MAJOR "${C4XVER1}")
set(CPACK_PACKAGE_VERSION_MINOR "${C4XVER2}")
set(CPACK_PACKAGE_VERSION_PATCH "${C4XVER3}")
set(CPACK_PACKAGE_FILE_NAME "openclonk-${C4XVER1}.${C4XVER2}.${C4XVER3}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "openclonk-src-${C4XVER1}.${C4XVER2}.${C4XVER3}")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
# Somebody who uses Debian/Ubuntu should set this
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgcc1, libx11, libXrandr, libXpm, libGLEW, libGL, libpng, libSDL-1.2, libSDL_mixer-1.2, gtk2, libjpeg, zlib")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_REQUIRES "libc6, libgcc1, libx11, libXrandr, libXpm, libGLEW, libGL, libpng, libSDL-1.2, libSDL_mixer-1.2, gtk2, libjpeg, zlib")

include(CPack)
