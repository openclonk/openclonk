
cmake_minimum_required (VERSION 2.6.0)
project (clonk CXX C)

include(Version.txt)

set(CMAKE_ADDITIONAL_DEPS_PATH deps CACHE PATH "Additional directory to search for libraries and headers")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_ADDITIONAL_DEPS_PATH})
set(OC_CXX_FLAGS ${CMAKE_CXX_FLAGS})
separate_arguments(OC_CXX_FLAGS)
set(OC_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
separate_arguments(OC_CXX_FLAGS_DEBUG)

############################################################################
# User selectable options
############################################################################
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
option(USE_GL "Enable OpenGL support" ON)
if(APPLE)
	SET(INITIAL_USE_SDL_MAINLOOP_VALUE ON)
else()
	SET(INITIAL_USE_SDL_MAINLOOP_VALUE OFF)
endif()
option(USE_SDL_MAINLOOP "Use SDL to create windows etc." ${INITIAL_USE_SDL_MAINLOOP_VALUE})
if(WIN32)
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
if(UNIX AND NOT USE_SDL_MAINLOOP)
	option(USE_X11 "Use X11 to create windows etc." ON)
	option(USE_GTK "Use GTK for the developer mode" ON)
	option(USE_GTK3 "Use GTK3 instead of GTK2" OFF)
else()
	SET(USE_X11 OFF)
	SET(USE_GTK OFF)
	SET(USE_GTK3 OFF)
endif()
if(NOT USE_GL AND NOT USE_DIRECTX)
	message(STATUS "No graphics display enabled; building dedicated server")
	SET(USE_CONSOLE ON CACHE INTERNAL "Build dedicated server")
endif()
option(BUILD_TO_PLANET "Put build files to ./planet/" OFF)
option(DEBUGREC "Debug records" OFF)
option(OC_BUILD_MULTIPROCESSOR "Use all processor cores to build" OFF)

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

############################################################################
# List target source files. Don't directly create a target since we condi-
# tionally add more sources depending on configuration options later.
############################################################################
# OC_CLONK_SOURCES: Sources for OpenClonk that are needed by every build.
# OC_SYSTEM_SOURCES: Sources for OpenClonk that are only needed by specific
# 	configurations.
set(OC_CLONK_SOURCES
	src/C4Application.cpp
	src/C4Application.h
	src/C4FullScreen.cpp
	src/C4FullScreen.h
	src/C4Game.cpp
	src/C4Game.h
	src/C4Globals.cpp
	src/C4GraphicsSystem.cpp
	src/C4GraphicsSystem.h
	src/c4group/C4Components.h
	src/c4group/C4ComponentHost.cpp
	src/c4group/C4ComponentHost.h
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
	src/C4WinMain.cpp
	src/config/C4Config.cpp
	src/config/C4Config.h
	src/config/C4ConfigShareware.cpp
	src/config/C4ConfigShareware.h
	src/config/C4Constants.h
	src/config/C4SecurityCertificates.cpp
	src/config/C4SecurityCertificates.h
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
	src/editor/C4Console.h
	src/editor/C4DevmodeDlg.cpp
	src/editor/C4DevmodeDlg.h
	src/editor/C4EditCursor.cpp
	src/editor/C4EditCursor.h
	src/editor/C4ObjectListDlg.cpp
	src/editor/C4ObjectListDlg.h
	src/editor/C4PropertyDlg.cpp
	src/editor/C4PropertyDlg.h
	src/editor/C4ToolsDlg.cpp
	src/editor/C4ToolsDlg.h
	src/game/C4GameVersion.h
	src/game/C4Physics.h
	src/game/landscape/C4Landscape.cpp
	src/game/landscape/C4Landscape.h
	src/game/landscape/C4Map.cpp
	src/game/landscape/C4MapCreatorS2.cpp
	src/game/landscape/C4MapCreatorS2.h
	src/game/landscape/C4Map.h
	src/game/landscape/C4MassMover.cpp
	src/game/landscape/C4MassMover.h
	src/game/landscape/C4Material.cpp
	src/game/landscape/C4Material.h
	src/game/landscape/C4MaterialList.cpp
	src/game/landscape/C4MaterialList.h
	src/game/landscape/C4Particles.cpp
	src/game/landscape/C4Particles.h
	src/game/landscape/C4PathFinder.cpp
	src/game/landscape/C4PathFinder.h
	src/game/landscape/C4PXS.cpp
	src/game/landscape/C4PXS.h
	src/game/landscape/C4Region.cpp
	src/game/landscape/C4Region.h
	src/game/landscape/C4Scenario.cpp
	src/game/landscape/C4Scenario.h
	src/game/landscape/C4Sky.cpp
	src/game/landscape/C4Sky.h
	src/game/landscape/C4SolidMask.cpp
	src/game/landscape/C4SolidMask.h
	src/game/landscape/C4Texture.cpp
	src/game/landscape/C4Texture.h
	src/game/landscape/C4Weather.cpp
	src/game/landscape/C4Weather.h
	src/game/object/C4Action.cpp
	src/game/object/C4Command.cpp
	src/game/object/C4Command.h
	src/game/object/C4Def.cpp
	src/game/object/C4DefGraphics.cpp
	src/game/object/C4DefGraphics.h
	src/game/object/C4Def.h
	src/game/object/C4GameObjects.cpp
	src/game/object/C4GameObjects.h
	src/game/object/C4Id.cpp
	src/game/object/C4Id.h
	src/game/object/C4IDList.cpp
	src/game/object/C4IDList.h
	src/game/object/C4InfoCore.cpp
	src/game/object/C4InfoCore.h
	src/game/object/C4MeshAnimation.cpp
	src/game/object/C4MeshAnimation.h
	src/game/object/C4Movement.cpp
	src/game/object/C4ObjectCom.cpp
	src/game/object/C4ObjectCom.h
	src/game/object/C4Object.cpp
	src/game/object/C4Object.h
	src/game/object/C4ObjectInfo.cpp
	src/game/object/C4ObjectInfo.h
	src/game/object/C4ObjectInfoList.cpp
	src/game/object/C4ObjectInfoList.h
	src/game/object/C4ObjectList.cpp
	src/game/object/C4ObjectList.h
	src/game/object/C4ObjectMenu.cpp
	src/game/object/C4ObjectMenu.h
	src/game/object/C4ObjectPtr.cpp
	src/game/object/C4ObjectPtr.h
	src/game/object/C4Sector.cpp
	src/game/object/C4Sector.h
	src/game/object/C4Shape.cpp
	src/game/object/C4Shape.h
	src/game/player/C4Player.cpp
	src/game/player/C4Player.h
	src/game/player/C4PlayerList.cpp
	src/game/player/C4PlayerList.h
	src/game/player/C4RankSystem.cpp
	src/game/player/C4RankSystem.h
	src/game/script/C4Effect.cpp
	src/game/script/C4Effects.h
	src/game/script/C4FindObject.cpp
	src/game/script/C4FindObject.h
	src/game/script/C4Script.cpp
	src/game/script/C4Script.h
	src/game/script/C4TransferZone.cpp
	src/game/script/C4TransferZone.h
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
	src/gui/C4UpdateDlg.cpp
	src/gui/C4UpdateDlg.h
	src/gui/C4UpperBoard.cpp
	src/gui/C4UpperBoard.h
	src/gui/C4UserMessages.h
	src/gui/C4Viewport.cpp
	src/gui/C4Viewport.h
	src/lib/C4InputValidation.cpp
	src/lib/C4InputValidation.h
	src/lib/C4LogBuf.cpp
	src/lib/C4LogBuf.h
	src/lib/C4Log.cpp
	src/lib/C4Log.h
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
	src/lib/StdMarkup.cpp
	src/lib/StdMarkup.h
	src/lib/StdResStr2.cpp
	src/lib/StdResStr2.h
	src/lib/StdResStr.h
	src/lib/texture/C4Facet.cpp
	src/lib/texture/C4FacetEx.cpp
	src/lib/texture/C4FacetEx.h
	src/lib/texture/C4Facet.h
	src/lib/texture/C4GraphicsResource.cpp
	src/lib/texture/C4GraphicsResource.h
	src/lib/texture/C4Surface.cpp
	src/lib/texture/C4SurfaceFile.cpp
	src/lib/texture/C4SurfaceFile.h
	src/lib/texture/C4Surface.h
	src/lib/texture/StdPNG.cpp
	src/lib/texture/StdPNG.h
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
	src/network/C4Packet2.cpp
	src/network/C4PacketBase.h
	src/platform/Bitmap256.cpp
	src/platform/Bitmap256.h
	src/platform/C4FileClasses.h
	src/platform/C4FileMonitor.cpp
	src/platform/C4FileMonitor.h
	src/platform/C4Fonts.cpp
	src/platform/C4Fonts.h
	src/platform/C4GamePadCon.cpp
	src/platform/C4GamePadCon.h
	src/platform/C4MusicFile.cpp
	src/platform/C4MusicFile.h
	src/platform/C4MusicSystem.cpp
	src/platform/C4MusicSystem.h
	src/platform/C4SoundSystem.cpp
	src/platform/C4SoundSystem.h
	src/platform/C4SoundLoaders.h
	src/platform/C4SoundLoaders.cpp
	src/platform/C4Video.cpp
	src/platform/C4Video.h
	src/platform/C4VideoPlayback.cpp
	src/platform/C4VideoPlayback.h
	src/platform/OpenURL.cpp
	src/platform/PlatformAbstraction.h
	src/platform/StdConfig.cpp
	src/platform/StdConfig.h
	src/platform/StdD3D.cpp
	src/platform/StdD3D.h
	src/platform/StdD3DShader.cpp
	src/platform/StdD3DShader.h
	src/platform/StdDDraw2.cpp
	src/platform/StdDDraw2.h
	src/platform/StdFacet.h
	src/platform/StdFile.cpp
	src/platform/StdFile.h
	src/platform/StdFont.cpp
	src/platform/StdFont.h
	src/platform/StdGL.cpp
	src/platform/StdGLCtx.cpp
	src/platform/StdGL.h
	src/platform/StdMesh.cpp
	src/platform/StdMesh.h
	src/platform/StdMeshLoader.h
	src/platform/StdMeshLoaderBinary.cpp
	src/platform/StdMeshLoaderBinaryChunks.cpp
	src/platform/StdMeshLoaderBinaryChunks.h
	src/platform/StdMeshLoaderDataStream.h
	src/platform/StdMeshLoaderXml.cpp
	src/platform/StdMeshMaterial.cpp
	src/platform/StdMeshMaterial.h
	src/platform/StdNoGfx.cpp
	src/platform/StdNoGfx.h
	src/platform/StdRegistry.cpp
	src/platform/StdRegistry.h
	src/platform/StdScheduler.cpp
	src/platform/StdScheduler.h
	src/platform/StdSurface2.cpp
	src/platform/StdSurface2.h
	src/platform/StdSurface8.cpp
	src/platform/StdSurface8.h
	src/platform/StdSync.h
	src/platform/StdVideo.cpp
	src/platform/StdVideo.h
	src/platform/StdWindow.h
	src/script/C4AList.cpp
	src/script/C4AList.h
	src/script/C4Aul.cpp
	src/script/C4AulExec.h
	src/script/C4AulExec.cpp
	src/script/C4AulDebug.h
	src/script/C4AulDebug.cpp
	src/script/C4Aul.h
	src/script/C4AulLink.cpp
	src/script/C4AulParse.cpp
	src/script/C4PropList.cpp
	src/script/C4PropList.h
	src/script/C4ScriptHost.cpp
	src/script/C4ScriptHost.h
	src/script/C4StringTable.cpp
	src/script/C4StringTable.h
	src/script/C4Value.cpp
	src/script/C4Value.h
	src/script/C4ValueList.cpp
	src/script/C4ValueList.h
	src/script/C4ValueMap.cpp
	src/script/C4ValueMap.h
	src/tinyxml/tinystr.cpp
	src/tinyxml/tinystr.h
	src/tinyxml/tinyxml.cpp
	src/tinyxml/tinyxml.h
	src/tinyxml/tinyxmlerror.cpp
	src/tinyxml/tinyxmlparser.cpp
	src/zlib/gzio.c
	src/zlib/zutil.h
)
mark_as_advanced(OC_CLONK_SOURCES)
mark_as_advanced(OC_SYSTEM_SOURCES)

if(WIN32)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/StdWindow.cpp
		src/platform/StdJoystick.cpp
		src/platform/StdJoystick.h
		src/platform/C4FileClasses.cpp
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
		                                                -o ${CMAKE_CURRENT_BINARY_DIR}/engine_rc.o)

		list(APPEND OC_SYSTEM_SOURCES
			${CMAKE_CURRENT_BINARY_DIR}/engine_rc.o
		)
	else(MINGW)
		list(APPEND OC_SYSTEM_SOURCES
			src/res/engine.rc
		)
	endif(MINGW)
elseif(USE_SDL_MAINLOOP)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/StdSDLApp.cpp
		src/platform/StdSDLWindow.cpp
	)
elseif(USE_X11)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/StdXApp.cpp
		src/platform/StdXWindow.cpp
		src/platform/StdXPrivate.h
	)
endif()
if(APPLE)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/MacUtility.mm
		src/platform/StdMacApp.mm
		src/platform/C4FileMonitorMac.mm
	)
endif()
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
		src/platform/StdGtkWindow.cpp
		src/platform/StdGtkWindow.h
	)
endif()

if(PROJECT_FOLDERS)
  source_group("Unsorted" .*)
  source_group("Library\\Platform abstraction" src/platform/.*)
  source_group("Library\\Utility" src/lib/.*)
  source_group("Library\\C4Group" src/c4group/.*)
  source_group("Library\\Graphics" src/lib/texture/.*)
  source_group("GUI" src/gui/.*)
  source_group("Network" src/network/.*)
  source_group("Game\\Objects" src/game/object/.*)
  source_group("Game\\Scenario" src/game/landscape/.*)
  source_group("Game\\Player" src/game/player/.*)
  source_group("Game\\Script" src/game/script/.*)
  source_group("Scripting" src/script/.*)
  source_group("Config" src/config/.*)
  source_group("Control" src/control/.*)
  source_group("Editing" src/editor/.*)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/c4group
	${CMAKE_CURRENT_SOURCE_DIR}/src/network
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib
	${CMAKE_CURRENT_SOURCE_DIR}/src/platform
	${CMAKE_CURRENT_SOURCE_DIR}/src/config
	${CMAKE_CURRENT_SOURCE_DIR}/src/res
	${CMAKE_CURRENT_SOURCE_DIR}/src/control
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui
	${CMAKE_CURRENT_SOURCE_DIR}/src/editor
	${CMAKE_CURRENT_SOURCE_DIR}/src/game/landscape
	${CMAKE_CURRENT_SOURCE_DIR}/src/game/player
	${CMAKE_CURRENT_SOURCE_DIR}/src/game/script
	${CMAKE_CURRENT_SOURCE_DIR}/src/game
	${CMAKE_CURRENT_SOURCE_DIR}/src/game/object
	${CMAKE_CURRENT_SOURCE_DIR}/src/lib/texture
	${CMAKE_CURRENT_SOURCE_DIR}/src/script
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
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)

# ck 09-09-20: The following headers require Xlib.h for things such as
# 'Bool' and 'Window' to be defined. Unfortunately, this doesn't exist
# as a CXX version (yet?).
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/extensions/xf86vmode.h HAVE_X11_EXTENSIONS_XF86VMODE_H)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/extensions/Xrandr.h HAVE_X11_EXTENSIONS_XRANDR_H)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/keysym.h HAVE_X11_KEYSYM_H)
CHECK_INCLUDE_FILES(X11/Xlib.h X11/xpm.h HAVE_X11_XPM_H)
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

# isilkor 09-05-29: Don't use FindOpenSSL, because that wants to link to ssleay
FINDLIB(OPENSSL_LIBRARIES NAMES crypto libeay32)
FIND_PATH(OPENSSL_INCLUDE_DIR NAMES openssl/sha.h)
include_directories(${OPENSSL_INCLUDE_DIR})

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
		src/res/C4D.icns src/res/C4F.icns src/res/C4G.icns src/res/C4K.icns
		src/res/C4P.icns src/res/C4S.icns src/res/C4U.icns
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

# Set GTK link directory. This needs to be done before add_executable,
# otherwise the path is not used for linking clonk
if(USE_GTK)
	include(FindPkgConfig)
	if (PKG_CONFIG_FOUND)
		if (USE_GTK3)
			pkg_check_modules(GTK glib-2.0>=2.4 gtk+-3.0)
		else()
			pkg_check_modules(GTK glib-2.0>=2.4 gtk+-2.0>=2.4)
		endif()
	endif()
	if(GTK_FOUND)
		link_directories(${GTK_LIBRARY_DIRS})
	endif()
endif()

add_definitions(-DHAVE_CONFIG_H)
add_executable(clonk WIN32 MACOSX_BUNDLE
	${OC_SYSTEM_SOURCES}
	${OC_CLONK_SOURCES}
)

add_executable(c4group
	src/c4group/c4group_ng.cpp
	src/c4group/C4Group.cpp
	src/lib/C4InputValidation.cpp
	src/config/C4SecurityCertificates.cpp
	src/c4group/C4Update.cpp
	src/lib/Standard.cpp
	src/c4group/CStdFile.cpp
	src/lib/StdMarkup.cpp
	src/lib/StdBuf.cpp
	src/lib/StdCompiler.cpp
	src/platform/StdFile.cpp
	src/platform/StdRegistry.cpp
	src/lib/StdResStr2.cpp
	src/platform/StdConfig.cpp
	src/zlib/gzio.c
	src/C4Include.cpp
)
target_link_libraries(clonk
	${FREETYPE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${ICONV_LIBRARY}
)
target_link_libraries(c4group
	${ZLIB_LIBRARIES}
	${OPENSSL_LIBRARIES}
)
set_property(TARGET clonk APPEND PROPERTY COMPILE_DEFINITIONS GLEW_STATIC)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG _DEBUG)

# This expands some variables in Info.plist as a side-effect. XCode might then 
# expand a second time, using the same syntax. Try not to get confused by this!
set_target_properties(clonk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "src/res/Info.plist")

############################################################################
# build to planet
############################################################################
if(BUILD_TO_PLANET) 
	if(MSVC) #get the files out of the Build folders
		if(CMAKE_VERSION VERSION_LESS "2.8")
			message(SEND_ERROR "You need CMake 2.8 or higher for BUILD_TO_PLANET to get the builds out of MSVCs folders. You have ${CMAKE_VERSION}")
		else()
			set_target_properties(clonk PROPERTIES RUNTIME_OUTPUT_NAME_RELEASE "../Clonk")
			set_target_properties(clonk PROPERTIES RUNTIME_OUTPUT_NAME_DEBUG "../ClonkD")
			set_target_properties(clonk PROPERTIES RUNTIME_OUTPUT_NAME_MINSIZEREL "../ClonkMSR")
			set_target_properties(clonk PROPERTIES RUNTIME_OUTPUT_NAME_RELWITHDEBINFO "../ClonkRD")
		endif()
	endif()
	set_target_properties(clonk PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/planet)
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
			if(FILE_EXT STREQUAL ".cpp" AND NOT FILE_NAME_WE MATCHES "tiny.*")
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
if(OC_CXX_FLAGS)
	list(REMOVE_DUPLICATES OC_CXX_FLAGS)
endif()
set(CMAKE_CXX_FLAGS "" CACHE STRING "C++ compiler flags" FORCE)
foreach(FLAG ${OC_CXX_FLAGS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" CACHE STRING "C++ compiler flags" FORCE)
endforeach()
list(REMOVE_DUPLICATES OC_CXX_FLAGS_DEBUG)
set(CMAKE_CXX_FLAGS_DEBUG "" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
foreach(FLAG ${OC_CXX_FLAGS_DEBUG})
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
endforeach()

############################################################################
# Locate optional libraries (if so desired)
############################################################################

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
	if(GTK_FOUND)
		include_directories(${GTK_INCLUDE_DIRS})
		SET(WITH_DEVELOPER_MODE ${GTK_FOUND})
		SET(WITH_GLIB ${GTK_FOUND})
		SET(WITH_GTK3 ${USE_GTK3})
		target_link_libraries(clonk
			${GTK_LIBRARIES}
		)
	endif()
endif()
if(USE_X11)
	FINDLIB(X11_LIBRARIES X11)
	FINDLIB(XPM_LIBRARIES Xpm)
	FINDLIB(XXF86VM_LIBRARIES Xxf86vm)
	FINDLIB(XRANDR_LIBRARIES Xrandr)
	target_link_libraries(clonk
		${X11_LIBRARIES}
		${XPM_LIBRARIES}
		${XXF86VM_LIBRARIES}
		${XRANDR_LIBRARIES}
	)
endif()
if (WIN32)
	# CMake is too incompetent to check whether these libraries can be linked to
	# So just pretend that everything is fine
	FINDLIB(VFW32_LIBRARIES vfw32)
	if (VFW32_LIBRARIES)
		target_link_libraries(clonk vfw32)
	else()
		# Mingw64 has the AviStream* and _DrawDibDraw* symbols in
		# other library files
		target_link_libraries(clonk wavifil32 msvfw32)
	endif()

	target_link_libraries(clonk ws2_32 winmm)
	target_link_libraries(c4group ws2_32)

	if(MSVC_VERSION)
		if(CMAKE_CL_64)
			FINDLIB(FMOD_LIBRARIES fmod64vc)
		else()
			FINDLIB(FMOD_LIBRARIES fmodvc)
		endif()
	elseif(CMAKE_COMPILER_IS_GNUCXX)
		FINDLIB(FMOD_LIBRARIES fmod)
	endif()
	if(FMOD_LIBRARIES)
		set(HAVE_FMOD TRUE)
		target_link_libraries(clonk
			${FMOD_LIBRARIES}
		)
	else()
		set(HAVE_FMOD FALSE)
	endif()
endif()
if(NOT HAVE_FMOD OR USE_SDL_MAINLOOP)
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


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
