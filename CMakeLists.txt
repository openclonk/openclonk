cmake_minimum_required (VERSION 2.6)
project (clonk CXX C)

list(APPEND CMAKE_PREFIX_PATH deps)
set(OC_CXX_FLAGS ${CMAKE_CXX_FLAGS})
separate_arguments(OC_CXX_FLAGS)
set(OC_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
separate_arguments(OC_CXX_FLAGS_DEBUG)

if(WIN32)
	SET(additional_clonk_sources engine/src/C4FileClasses.cpp engine/res/engine.rc engine/res/resource.h)
endif()
add_executable(clonk WIN32 MACOSX_BUNDLE
	${additional_clonk_sources}
	engine/src/C4AList.cpp
	engine/src/C4Action.cpp
	engine/src/C4Application.cpp
	engine/src/C4Aul.cpp
	engine/src/C4AulExec.cpp
	engine/src/C4AulLink.cpp
	engine/src/C4AulParse.cpp
	engine/src/C4ChatDlg.cpp
	engine/src/C4Client.cpp
	engine/src/C4Command.cpp
	engine/src/C4ComponentHost.cpp
	engine/src/C4Config.cpp
	engine/src/C4Console.cpp
	engine/src/C4Control.cpp
	engine/src/C4Def.cpp
	engine/src/C4DefGraphics.cpp
	engine/src/C4DevmodeDlg.cpp
	engine/src/C4DownloadDlg.cpp
	engine/src/C4EditCursor.cpp
	engine/src/C4Effect.cpp
	engine/src/C4Extra.cpp
	engine/src/C4Facet.cpp
	engine/src/C4FacetEx.cpp
	engine/src/C4FindObject.cpp
	engine/src/C4FileMonitor.cpp
	engine/src/C4FileSelDlg.cpp
	engine/src/C4Folder.cpp
	engine/src/C4Fonts.cpp
	engine/src/C4FullScreen.cpp
	engine/src/C4Game.cpp
	engine/src/C4GameControl.cpp
	engine/src/C4GameControlNetwork.cpp
	engine/src/C4GameDialogs.cpp
	engine/src/C4GameLobby.cpp
	engine/src/C4GameMessage.cpp
	engine/src/C4GameObjects.cpp
	engine/src/C4GameOptions.cpp
	engine/src/C4GameOverDlg.cpp
	engine/src/C4GamePadCon.cpp
	engine/src/C4GameParameters.cpp
	engine/src/C4GameSave.cpp
	engine/src/C4GraphicsResource.cpp
	engine/src/C4GraphicsSystem.cpp
	engine/src/C4Group.cpp
	engine/src/C4GroupSet.cpp
	engine/src/C4Gui.cpp
	engine/src/C4GuiButton.cpp
	engine/src/C4GuiCheckBox.cpp
	engine/src/C4GuiComboBox.cpp
	engine/src/C4GuiContainers.cpp
	engine/src/C4GuiDialogs.cpp
	engine/src/C4GuiEdit.cpp
	engine/src/C4GuiLabels.cpp
	engine/src/C4GuiListBox.cpp
	engine/src/C4GuiMenu.cpp
	engine/src/C4GuiTabular.cpp
	engine/src/C4IDList.cpp
	engine/src/C4Id.cpp
	engine/src/C4InfoCore.cpp
	engine/src/C4InputValidation.cpp
	engine/src/C4InteractiveThread.cpp
	engine/src/C4KeyboardInput.cpp
	engine/src/C4Landscape.cpp
	engine/src/C4LangStringTable.cpp
	engine/src/C4Language.cpp
	engine/src/C4League.cpp
	engine/src/C4LoaderScreen.cpp
	engine/src/C4Log.cpp
	engine/src/C4LogBuf.cpp
	engine/src/C4MainMenu.cpp
	engine/src/C4Map.cpp
	engine/src/C4MapCreatorS2.cpp
	engine/src/C4MassMover.cpp
	engine/src/C4Material.cpp
	engine/src/C4MaterialList.cpp
	engine/src/C4Menu.cpp
	engine/src/C4MessageBoard.cpp
	engine/src/C4MessageInput.cpp
	engine/src/C4MouseControl.cpp
	engine/src/C4Movement.cpp
	engine/src/C4MusicFile.cpp
	engine/src/C4MusicSystem.cpp
	engine/src/C4NameList.cpp
	engine/src/C4NetIO.cpp
	engine/src/C4Network2.cpp
	engine/src/C4Network2Client.cpp
	engine/src/C4Network2Dialogs.cpp
	engine/src/C4Network2Discover.cpp
	engine/src/C4Network2IO.cpp
	engine/src/C4Network2IRC.cpp
	engine/src/C4Network2Players.cpp
	engine/src/C4Network2Reference.cpp
	engine/src/C4Network2Res.cpp
	engine/src/C4Network2ResDlg.cpp
	engine/src/C4Network2Stats.cpp
	engine/src/C4Object.cpp
	engine/src/C4ObjectCom.cpp
	engine/src/C4ObjectMenu.cpp
	engine/src/C4ObjectInfo.cpp
	engine/src/C4ObjectInfoList.cpp
	engine/src/C4ObjectList.cpp
	engine/src/C4ObjectListDlg.cpp
	engine/src/C4PXS.cpp
	engine/src/C4Packet2.cpp
	engine/src/C4Particles.cpp
	engine/src/C4PathFinder.cpp
	engine/src/C4Player.cpp
	engine/src/C4PlayerInfo.cpp
	engine/src/C4PlayerInfoConflicts.cpp
	engine/src/C4PlayerInfoListBox.cpp
	engine/src/C4PlayerList.cpp
	engine/src/C4PropertyDlg.cpp
	engine/src/C4Random.cpp
	engine/src/C4RankSystem.cpp
	engine/src/C4Record.cpp
	engine/src/C4Region.cpp
	engine/src/C4RTF.cpp
	engine/src/C4Scenario.cpp
	engine/src/C4Scoreboard.cpp
	engine/src/C4Script.cpp
	engine/src/C4ScriptHost.cpp
	engine/src/C4Sector.cpp
	engine/src/C4Shape.cpp
	engine/src/C4Sky.cpp
	engine/src/C4SolidMask.cpp
	engine/src/C4SoundSystem.cpp
	engine/src/C4Startup.cpp
	engine/src/C4StartupAboutDlg.cpp
	engine/src/C4StartupMainDlg.cpp
	engine/src/C4StartupNetDlg.cpp
	engine/src/C4StartupOptionsDlg.cpp
	engine/src/C4StartupPlrSelDlg.cpp
	engine/src/C4StartupScenSelDlg.cpp
	engine/src/C4Stat.cpp
	engine/src/C4StringTable.cpp
	engine/src/C4Surface.cpp
	engine/src/C4SurfaceFile.cpp
	engine/src/C4Teams.cpp
	engine/src/C4Texture.cpp
	engine/src/C4ToolsDlg.cpp
	engine/src/C4TransferZone.cpp
	engine/src/C4UpdateDlg.cpp
	engine/src/C4UpperBoard.cpp
	engine/src/C4Value.cpp
	engine/src/C4ValueList.cpp
	engine/src/C4ValueMap.cpp
	engine/src/C4Video.cpp
	engine/src/C4VideoPlayback.cpp
	engine/src/C4Viewport.cpp
	engine/src/C4Weather.cpp
	engine/src/C4WinMain.cpp
	engine/sec/C4ConfigShareware.cpp
	engine/sec/C4ConfigShareware.h
	engine/sec/C4SecurityCertificates.cpp
	engine/sec/C4SecurityCertificates.h
	engine/inc/C4AList.h
	engine/inc/C4Application.h
	engine/inc/C4Aul.h
	engine/inc/C4ChatDlg.h
	engine/inc/C4Command.h
	engine/inc/C4Client.h
	engine/inc/C4ComponentHost.h
	engine/inc/C4Components.h
	engine/inc/C4Config.h
	engine/inc/C4Console.h
	engine/inc/C4Constants.h
	engine/inc/C4Control.h
	engine/inc/C4DefGraphics.h
	engine/inc/C4Def.h
	engine/inc/C4DevmodeDlg.h
	engine/inc/C4DownloadDlg.h
	engine/inc/C4EditCursor.h
	engine/inc/C4Effects.h
	engine/inc/C4Extra.h
	engine/inc/C4FacetEx.h
	engine/inc/C4Facet.h
	engine/inc/C4FileClasses.h
	engine/inc/C4FileMonitor.h
	engine/inc/C4FileSelDlg.h
	engine/inc/C4FindObject.h
	engine/inc/C4Folder.h
	engine/inc/C4Fonts.h
	engine/inc/C4FullScreen.h
	engine/inc/C4Game.h
	engine/inc/C4GameControl.h
	engine/inc/C4GameControlNetwork.h
	engine/inc/C4GameDialogs.h
	engine/inc/C4GameLobby.h
	engine/inc/C4GameMessage.h
	engine/inc/C4GameObjects.h
	engine/inc/C4GameOptions.h
	engine/inc/C4GameOverDlg.h
	engine/inc/C4GamePadCon.h
	engine/inc/C4GameParameters.h
	engine/inc/C4GameSave.h
	engine/inc/C4GameVersion.h
	engine/inc/C4GraphicsResource.h
	engine/inc/C4GraphicsSystem.h
	engine/inc/C4Group.h
	engine/inc/C4GroupSet.h
	engine/inc/C4Gui.h
	engine/inc/C4Id.h
	engine/inc/C4IDList.h
	engine/inc/C4Include.h
	engine/inc/C4InfoCore.h
	engine/inc/C4InputValidation.h
	engine/inc/C4InteractiveThread.h
	engine/inc/C4KeyboardInput.h
	engine/inc/C4Landscape.h
	engine/inc/C4LangStringTable.h
	engine/inc/C4Language.h
	engine/inc/C4League.h
	engine/inc/C4LoaderScreen.h
	engine/inc/C4LogBuf.h
	engine/inc/C4Log.h
	engine/inc/C4MainMenu.h
	engine/inc/C4MapCreatorS2.h
	engine/inc/C4Map.h
	engine/inc/C4MassMover.h
	engine/inc/C4Material.h
	engine/inc/C4MaterialList.h
	engine/inc/C4Menu.h
	engine/inc/C4MessageBoard.h
	engine/inc/C4MessageInput.h
	engine/inc/C4MouseControl.h
	engine/inc/C4MusicFile.h
	engine/inc/C4MusicSystem.h
	engine/inc/C4NameList.h
	engine/inc/C4NetIO.h
	engine/inc/C4Network2Client.h
	engine/inc/C4Network2Dialogs.h
	engine/inc/C4Network2Discover.h
	engine/inc/C4Network2.h
	engine/inc/C4Network2IO.h
	engine/inc/C4Network2IRC.h
	engine/inc/C4Network2Players.h
	engine/inc/C4Network2Reference.h
	engine/inc/C4Network2Res.h
	engine/inc/C4Network2Stats.h
	engine/inc/C4ObjectCom.h
	engine/inc/C4Object.h
	engine/inc/C4ObjectInfo.h
	engine/inc/C4ObjectInfoList.h
	engine/inc/C4ObjectMenu.h
	engine/inc/C4ObjectList.h
	engine/inc/C4ObjectListDlg.h
	engine/inc/C4PacketBase.h
	engine/inc/C4Particles.h
	engine/inc/C4PathFinder.h
	engine/inc/C4Physics.h
	engine/inc/C4Player.h
	engine/inc/C4PlayerInfo.h
	engine/inc/C4PlayerInfoListBox.h
	engine/inc/C4PlayerList.h
	engine/inc/C4PropertyDlg.h
	engine/inc/C4Prototypes.h
	engine/inc/C4PXS.h
	engine/inc/C4Random.h
	engine/inc/C4RankSystem.h
	engine/inc/C4Record.h
	engine/inc/C4Region.h
	engine/inc/C4RTF.H
	engine/inc/C4Scenario.h
	engine/inc/C4Scoreboard.h
	engine/inc/C4Script.h
	engine/inc/C4ScriptHost.h
	engine/inc/C4Sector.h
	engine/inc/C4Shape.h
	engine/inc/C4Sky.h
	engine/inc/C4SolidMask.h
	engine/inc/C4SoundSystem.h
	engine/inc/C4Startup.h
	engine/inc/C4StartupAboutDlg.h
	engine/inc/C4StartupMainDlg.h
	engine/inc/C4StartupNetDlg.h
	engine/inc/C4StartupOptionsDlg.h
	engine/inc/C4StartupPlrSelDlg.h
	engine/inc/C4StartupScenSelDlg.h
	engine/inc/C4Stat.h
	engine/inc/C4StringTable.h
	engine/inc/C4SurfaceFile.h
	engine/inc/C4Surface.h
	engine/inc/C4Teams.h
	engine/inc/C4Texture.h
	engine/inc/C4ToolsDlg.h
	engine/inc/C4TransferZone.h
	engine/inc/C4UpdateDlg.h
	engine/inc/C4UpperBoard.h
	engine/inc/C4UserMessages.h
	engine/inc/C4Value.h
	engine/inc/C4ValueList.h
	engine/inc/C4ValueMap.h
	engine/inc/C4Version.h
	engine/inc/C4Video.h
	engine/inc/C4VideoPlayback.h
	engine/inc/C4Viewport.h
	engine/inc/C4Weather.h
	engine/inc/C4RoundResults.h
	engine/src/C4RoundResults.cpp
	engine/res/Brush.h
	engine/res/c4x.xpm
	engine/res/Cursor.h
	engine/res/Dynamic.h
	engine/res/Exact.h
	engine/res/Fill.h
	engine/res/Halt.h
	engine/res/Ift.h
	engine/res/Line.h
	engine/res/Mouse.h
	engine/res/NoIft.h
	engine/res/Picker.h
	engine/res/Play.h
	engine/res/Rect.h
	engine/res/Static.h
	engine/src/C4PropList.cpp
	engine/inc/C4PropList.h
)

add_executable(c4group
	group/c4group_ng.cpp
	engine/src/C4Group.cpp
	engine/src/C4InputValidation.cpp
	engine/sec/C4SecurityCertificates.cpp
	engine/src/C4Update.cpp
	engine/inc/C4Update.h
	engine/inc/C4Group.h
	engine/inc/C4Version.h
	standard/src/Standard.cpp
	standard/src/CStdFile.cpp
	standard/src/StdMarkup.cpp
	standard/src/StdBuf.cpp
	standard/src/StdCompiler.cpp
	standard/src/StdFile.cpp
	standard/src/StdRegistry.cpp
	standard/src/StdResStr2.cpp
	standard/src/StdConfig.cpp
	standard/zlib/gzio.c
	standard/zlib/zutil.h
)

if(WIN32)
	SET(additional_system_sources
		standard/src/StdWindow.cpp
		standard/src/StdJoystick.cpp
		standard/inc/StdJoystick.h
	)
elseif(USE_SDL)
	SET(additional_system_sources
		standard/src/StdSDLApp.cpp
		standard/src/StdSDLWindow.cpp
	)
elseif(USE_X11)
	SET(additional_system_sources
		standard/src/StdXApp.cpp
		standard/src/StdXWindow.cpp
		standard/src/StdXPrivate.h
	)
endif()
if(USE_GTK)
	SET(additional_system_sources
		${additional_system_sources}
		standard/src/StdGtkWindow.cpp
		standard/inc/StdGtkWindow.h
	)
endif()
add_library(standard STATIC
	${additional_system_sources}
	standard/src/Bitmap256.cpp
	standard/src/CStdFile.cpp
	standard/src/Fixed.cpp
	standard/src/Midi.cpp
	standard/src/OpenURL.cpp
	standard/src/PathFinder.cpp
	standard/src/Standard.cpp
	standard/src/StdBase64.cpp
	standard/src/StdBuf.cpp
	standard/src/StdConfig.cpp
	standard/src/StdCompiler.cpp
	standard/src/StdD3D.cpp
	standard/src/StdD3DShader.cpp
	standard/src/StdDDraw2.cpp
	standard/src/StdFile.cpp
	standard/src/StdFont.cpp
	standard/src/StdGL.cpp
	standard/src/StdGLCtx.cpp
	standard/src/StdMarkup.cpp
	standard/src/StdNoGfx.cpp
	standard/src/StdPNG.cpp
	standard/src/StdRegistry.cpp
	standard/src/StdResStr2.cpp
	standard/src/StdScheduler.cpp
	standard/src/StdSurface2.cpp
	standard/src/StdSurface8.cpp
	standard/src/StdVideo.cpp
	standard/inc/Bitmap256.h
	standard/inc/CStdFile.h
	standard/inc/DInputX.h
	standard/inc/DSoundX.h
	standard/inc/Fixed.h
	standard/inc/Midi.h
	standard/inc/PathFinder.h
	standard/inc/Standard.h
	standard/inc/StdAdaptors.h
	standard/inc/StdBase64.h
	standard/inc/StdBuf.h
	standard/inc/StdColors.h
	standard/inc/StdConfig.h
	standard/inc/StdCompiler.h
	standard/inc/StdD3D.h
	standard/inc/StdD3DShader.h
	standard/inc/StdDDraw2.h
	standard/inc/StdFacet.h
	standard/inc/StdFile.h
	standard/inc/StdFont.h
	standard/inc/StdGL.h
	standard/inc/StdMarkup.h
	standard/inc/StdNoGfx.h
	standard/inc/StdPNG.h
	standard/inc/StdRegistry.h
	standard/inc/StdResStr2.h
	standard/inc/StdResStr.h
	standard/inc/StdScheduler.h
	standard/inc/StdSurface2.h
	standard/inc/StdSurface8.h
	standard/inc/StdSync.h
	standard/inc/StdVideo.h
	standard/inc/StdWindow.h
	standard/zlib/gzio.c
	standard/zlib/zutil.h
)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/standard
	${CMAKE_CURRENT_SOURCE_DIR}/standard/inc
	${CMAKE_CURRENT_SOURCE_DIR}/engine/inc
	${CMAKE_CURRENT_SOURCE_DIR}/engine/sec
	${CMAKE_CURRENT_SOURCE_DIR}/engine
)
target_link_libraries(clonk standard)

set_property(TARGET clonk standard APPEND PROPERTY COMPILE_DEFINITIONS C4ENGINE GLEW_STATIC)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG _DEBUG)
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
CHECK_INCLUDE_FILE_CXX(X11/extensions/xf86vmode.h HAVE_X11_EXTENSIONS_XF86VMODE_H)
CHECK_INCLUDE_FILE_CXX(X11/extensions/Xrandr.h HAVE_X11_EXTENSIONS_XRANDR_H)
CHECK_INCLUDE_FILE_CXX(X11/keysym.h HAVE_X11_KEYSYM_H)
CHECK_INCLUDE_FILE_CXX(X11/xpm.h HAVE_X11_XPM_H)
CHECK_INCLUDE_FILE_CXX(iconv.h HAVE_ICONV)

if(HAVE_ICONV)
	SET(ICONV_CONST ON)
endif()

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(vasprintf HAVE_VASPRINTF)

# User selectable options
option(USE_GL "Enable OpenGL support" ON)
if(APPLE)
	SET(INITIAL_USE_SDL_VALUE ON)
else()
	SET(INITIAL_USE_SDL_VALUE OFF)
endif()
option(USE_SDL "Use SDL to create windows etc." ${INITIAL_USE_SDL_VALUE})
if(WIN32)
	option(USE_DIRECTX "Enable DirectX support" ON)
	if(EXISTS $ENV{DXSDK_DIR})
		include_directories(SYSTEM $ENV{DXSDK_DIR}/Include)
		if(${FIND_LIBRARY_USE_LIB64_PATHS})
			list(APPEND CMAKE_LIBRARY_PATH $ENV{DXSDK_DIR}/Lib/x64)
		else()
			list(APPEND CMAKE_LIBRARY_PATH $ENV{DXSDK_DIR}/Lib/x86)
		endif()
	endif()
else()
	SET(USE_DIRECTX OFF)
endif()
if(UNIX)
	option(USE_X11 "Use X11 to create windows etc." ON)
	option(USE_GTK "Use GTK for the developer mode" ON)
else()
	SET(USE_X11 OFF)
	SET(USE_GTK OFF)
endif()
if(NOT USE_GL AND NOT USE_DIRECTX)
	message(STATUS "No graphics display enabled; building dedicated server")
	SET(USE_CONSOLE ON CACHE INTERNAL "Build dedicated server")
endif()

# Find win32 SDK, since a lot of users don't have the include path in their environment
if(WIN32)
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
FINDLIB(OPENSSL NAMES crypto libeay32)

# Optional libraries
if(USE_GL)
	include(FindOpenGL)
	FINDLIB(GLEW NAMES GLEW glew32 glew32s)
endif()
if(USE_DIRECTX)
	FINDLIB(DIRECT3D d3d9)
	FINDLIB(DIRECT3DX d3dx9)
endif()
if(USE_GTK)
	include(FindPkgConfig)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(GTK glib-2.0>=2.4 gtk+-2.0>=2.4)
	endif()
	if(GTK_FOUND)
		include_directories(${GTK_INCLUDE_DIRS})
		link_directories(${GTK_LIBRARY_DIRS})
		SET(WITH_DEVELOPER_MODE ${GTK_FOUND})
		SET(WITH_GLIB ${GTK_FOUND})
	endif()
endif()
if(USE_X11)
	FINDLIB(X11 X11)
	FINDLIB(XPM Xpm)
	FINDLIB(XXF86VM Xxf86vm)
	FINDLIB(XRANDR Xrandr)
endif()
if (WIN32)
	FINDLIB(VFW vfw32)
	FINDLIB(WS2 ws2_32)
	FINDLIB(WINMM winmm)
endif()
if (UNIX)
	include(FindSDL)
	SET(HAVE_SDL ${SDL_FOUND})
	if(SDL_FOUND)
		include_directories(${SDL_INCLUDE_DIR})
		FINDLIB(SDLMIXER SDL_mixer)
		SET(HAVE_LIBSDL_MIXER ${SDLMIXER})
	endif()
endif()

target_link_libraries(clonk
	${FREETYPE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${VFW}
	${WS2}
	${WINMM}
	${GLEW}
	${OPENGL_LIBRARIES}
	${DIRECT3D} ${DIRECT3DX}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${OPENSSL}
	${GTK_LIBRARIES}
	${X11}
	${XPM}
	${XXF86VM}
	${XRANDR}
	${SDL_LIBRARY}
	${SDLMIXER}
)
target_link_libraries(c4group
	${ZLIB_LIBRARIES}
	${WS2}
	${OPENSSL}
)

include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX)
	CHECK_CXX_COMPILER_FLAG("-std=gnu++0x" USE_GCC_STD_0X)
	if(USE_GCC_STD_0X)
		list(APPEND OC_CXX_FLAGS "-std=gnu++0x")
	endif()
endif()
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_FLAGS ${OC_CXX_FLAGS})
CHECK_CXX_SOURCE_COMPILES("void f(struct D&&); int main() { return 0; }" HAVE_RVALUE_REF)

if(MSVC_VERSION GREATER 1499)
	# Activate minimal rebuild
	list(APPEND OC_CXX_FLAGS_DEBUG /Gm)
	if (NOT FIND_LIBRARY_USE_LIB64_PATHS)
		# Activate edit-and-continue
		list(REMOVE_ITEM OC_CXX_FLAGS_DEBUG /Zi)
		list(APPEND OC_CXX_FLAGS_DEBUG /ZI /Gy)
	endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)

list(REMOVE_DUPLICATES OC_CXX_FLAGS)
set(CMAKE_CXX_FLAGS "" CACHE STRING "C++ compiler flags" FORCE)
foreach(FLAG ${OC_CXX_FLAGS})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" CACHE STRING "C++ compiler flags" FORCE)
endforeach()
list(REMOVE_DUPLICATES OC_CXX_FLAGS_DEBUG)
set(CMAKE_CXX_FLAGS_DEBUG "" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
foreach(FLAG ${OC_CXX_FLAGS_DEBUG})
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAG}" CACHE STRING "Flags used by the compiler during debug builds." FORCE)
endforeach()
