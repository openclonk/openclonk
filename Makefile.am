# Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de

# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.

# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
ACLOCAL_AMFLAGS = -I autotools --install

bin_PROGRAMS = clonk

EXTRA_PROGRAMS = tstc4netio puncher c4group gunzip4c4group

noinst_LIBRARIES = libstandard.a

# Some defines and warning options
if RECENT_GCC
GCC_FLAGS = -pipe
WARNING_FLAGS = -Wall -Wextra -Wno-invalid-offsetof \
-Wredundant-decls -Wendif-labels -Wpointer-arith \
-Wcast-qual -Wcast-align -Wwrite-strings -Winit-self \
-Wsign-promo -Wunsafe-loop-optimizations \
-Wsign-promo \
-Wno-parentheses -Wno-unused-parameter -Wno-inline \
-Wno-reorder -Wno-float-equal -Wno-switch
##-Wmissing-format-attribute -Wdisabled-optimization -Wlogical-op
##-Weffc++ -Wold-style-cast -Woverloaded-virtual
else
GCC_FLAGS =
WARNING_FLAGS = -Wall
endif

AM_CXXFLAGS = $(PTHREAD_CFLAGS) $(WINDOWS_CFLAGS) $(WARNING_FLAGS) $(GCC_FLAGS)

AM_CFLAGS = -Wall $(GCC_FLAGS)

AM_CPPFLAGS = \
  -I$(builddir) \
  -I$(srcdir)/standard \
  -I$(srcdir)/standard/inc \
  -I$(srcdir)/engine \
  -I$(srcdir)/engine/inc \
  -I$(srcdir)/engine/sec \
  -DC4ENGINE \
  $(GLEW_CFLAGS) $(GTK_CFLAGS) $(FREETYPE_CFLAGS) $(SDL_CFLAGS)

#various hacks to get dependency tracking working with a precompiled C4Include
if RECENT_GCC
BUILT_SOURCES = C4Include.h.gch
CLEANFILES = C4Include.h.gch
DISTCLEANFILES = @am__quote@$(DEPDIR)/C4Include.Po@am__quote@

# Adapted from the automake compile-command for normal source files.
# Has to be updated after certain changes in this file.
C4Include.h.gch: engine/inc/C4Include.h
@am__fastdepCXX_TRUE@	depbase=`echo $@ | sed 's|[^/]*$$|$(DEPDIR)/&|;s|\.h.gch$$||'`;\
@am__fastdepCXX_TRUE@	$(CXXCOMPILE) -x c++-header -MT $@ -MD -MP -MF $$depbase.Tpo -c -o $@ $< &&\
@am__fastdepCXX_TRUE@	mv -f $$depbase.Tpo $$depbase.Po
@am__fastdepCXX_FALSE@	$(CXXCOMPILE) -x c++-header -c -o $@ $<

@AMDEP_TRUE@@am__include@ @am__quote@$(DEPDIR)/C4Include.Po@am__quote@

if am__fastdepCXX
AM_CXXFLAGS += -fpch-deps
endif

WARNING_FLAGS += -Winvalid-pch

endif

clonk_SOURCES = \
  engine/src/C4AList.cpp \
  engine/src/C4Action.cpp \
  engine/src/C4Application.cpp \
  engine/src/C4Aul.cpp \
  engine/src/C4AulExec.cpp \
  engine/src/C4AulLink.cpp \
  engine/src/C4AulParse.cpp \
  engine/src/C4ChatDlg.cpp \
  engine/src/C4Client.cpp \
  engine/src/C4Command.cpp \
  engine/src/C4ComponentHost.cpp \
  engine/src/C4Config.cpp \
  engine/src/C4Console.cpp \
  engine/src/C4Control.cpp \
  engine/src/C4Def.cpp \
  engine/src/C4DefGraphics.cpp \
  engine/src/C4DevmodeDlg.cpp \
  engine/src/C4DownloadDlg.cpp \
  engine/src/C4EditCursor.cpp \
  engine/src/C4Effect.cpp \
  engine/src/C4Extra.cpp \
  engine/src/C4Facet.cpp \
  engine/src/C4FacetEx.cpp \
  engine/src/C4FindObject.cpp \
  engine/src/C4FileMonitor.cpp \
  engine/src/C4FileSelDlg.cpp \
  engine/src/C4Folder.cpp \
  engine/src/C4Fonts.cpp \
  engine/src/C4FullScreen.cpp \
  engine/src/C4Game.cpp \
  engine/src/C4GameControl.cpp \
  engine/src/C4GameControlNetwork.cpp \
  engine/src/C4GameDialogs.cpp \
  engine/src/C4GameLobby.cpp \
  engine/src/C4GameMessage.cpp \
  engine/src/C4GameObjects.cpp \
  engine/src/C4GameOptions.cpp \
  engine/src/C4GameOverDlg.cpp \
  engine/src/C4GamePadCon.cpp \
  engine/src/C4GameParameters.cpp \
  engine/src/C4GameSave.cpp \
  engine/src/C4GraphicsResource.cpp \
  engine/src/C4GraphicsSystem.cpp \
  engine/src/C4Group.cpp \
  engine/src/C4GroupSet.cpp \
  engine/src/C4Gui.cpp \
  engine/src/C4GuiButton.cpp \
  engine/src/C4GuiCheckBox.cpp \
  engine/src/C4GuiComboBox.cpp \
  engine/src/C4GuiContainers.cpp \
  engine/src/C4GuiDialogs.cpp \
  engine/src/C4GuiEdit.cpp \
  engine/src/C4GuiLabels.cpp \
  engine/src/C4GuiListBox.cpp \
  engine/src/C4GuiMenu.cpp \
  engine/src/C4GuiTabular.cpp \
  engine/src/C4IDList.cpp \
  engine/src/C4Id.cpp \
  engine/src/C4InfoCore.cpp \
  engine/src/C4InputValidation.cpp \
  engine/src/C4InteractiveThread.cpp \
  engine/src/C4KeyboardInput.cpp \
  engine/src/C4Landscape.cpp \
  engine/src/C4LangStringTable.cpp \
  engine/src/C4Language.cpp \
  engine/src/C4League.cpp \
  engine/src/C4LoaderScreen.cpp \
  engine/src/C4Log.cpp \
  engine/src/C4LogBuf.cpp \
  engine/src/C4MainMenu.cpp \
  engine/src/C4Map.cpp \
  engine/src/C4MapCreatorS2.cpp \
  engine/src/C4MassMover.cpp \
  engine/src/C4Material.cpp \
  engine/src/C4MaterialList.cpp \
  engine/src/C4Menu.cpp \
  engine/src/C4MessageBoard.cpp \
  engine/src/C4MessageInput.cpp \
  engine/src/C4MouseControl.cpp \
  engine/src/C4Movement.cpp \
  engine/src/C4MusicFile.cpp \
  engine/src/C4MusicSystem.cpp \
  engine/src/C4NameList.cpp \
  engine/src/C4NetIO.cpp \
  engine/src/C4Network2.cpp \
  engine/src/C4Network2Client.cpp \
  engine/src/C4Network2Dialogs.cpp \
  engine/src/C4Network2Discover.cpp \
  engine/src/C4Network2IO.cpp \
  engine/src/C4Network2IRC.cpp \
  engine/src/C4Network2Players.cpp \
  engine/src/C4Network2Reference.cpp \
  engine/src/C4Network2Res.cpp \
  engine/src/C4Network2ResDlg.cpp \
  engine/src/C4Network2Stats.cpp \
  engine/src/C4Object.cpp \
  engine/src/C4ObjectCom.cpp \
  engine/src/C4ObjectMenu.cpp \
  engine/src/C4ObjectInfo.cpp \
  engine/src/C4ObjectInfoList.cpp \
  engine/src/C4ObjectList.cpp \
  engine/src/C4ObjectListDlg.cpp \
  engine/src/C4PXS.cpp \
  engine/src/C4Packet2.cpp \
  engine/src/C4Particles.cpp \
  engine/src/C4PathFinder.cpp \
  engine/src/C4Player.cpp \
  engine/src/C4PlayerInfo.cpp \
  engine/src/C4PlayerInfoConflicts.cpp \
  engine/src/C4PlayerInfoListBox.cpp \
  engine/src/C4PlayerList.cpp \
  engine/src/C4PropertyDlg.cpp \
  engine/src/C4Random.cpp \
  engine/src/C4RankSystem.cpp \
  engine/src/C4Record.cpp \
  engine/src/C4Region.cpp \
  engine/src/C4RTF.cpp \
  engine/src/C4Scenario.cpp \
  engine/src/C4Scoreboard.cpp \
  engine/src/C4Script.cpp \
  engine/src/C4ScriptHost.cpp \
  engine/src/C4Sector.cpp \
  engine/src/C4Shape.cpp \
  engine/src/C4Sky.cpp \
  engine/src/C4SolidMask.cpp \
  engine/src/C4SoundSystem.cpp \
  engine/src/C4Startup.cpp \
  engine/src/C4StartupAboutDlg.cpp \
  engine/src/C4StartupMainDlg.cpp \
  engine/src/C4StartupNetDlg.cpp \
  engine/src/C4StartupOptionsDlg.cpp \
  engine/src/C4StartupPlrSelDlg.cpp \
  engine/src/C4StartupScenSelDlg.cpp \
  engine/src/C4Stat.cpp \
  engine/src/C4StringTable.cpp \
  engine/src/C4Surface.cpp \
  engine/src/C4SurfaceFile.cpp \
  engine/src/C4Teams.cpp \
  engine/src/C4Texture.cpp \
  engine/src/C4ToolsDlg.cpp \
  engine/src/C4TransferZone.cpp \
  engine/src/C4UpdateDlg.cpp \
  engine/src/C4UpperBoard.cpp \
  engine/src/C4Value.cpp \
  engine/src/C4ValueList.cpp \
  engine/src/C4ValueMap.cpp \
  engine/src/C4Video.cpp \
  engine/src/C4VideoPlayback.cpp \
  engine/src/C4Viewport.cpp \
  engine/src/C4Weather.cpp \
  engine/src/C4WinMain.cpp \
  engine/sec/C4ConfigShareware.cpp \
  engine/sec/C4ConfigShareware.h \
  engine/sec/C4SecurityCertificates.cpp \
  engine/sec/C4SecurityCertificates.h \
  engine/inc/C4AList.h \
  engine/inc/C4Application.h \
  engine/inc/C4Aul.h \
  engine/inc/C4ChatDlg.h \
  engine/inc/C4Command.h \
  engine/inc/C4Client.h \
  engine/inc/C4ComponentHost.h \
  engine/inc/C4Components.h \
  engine/inc/C4Config.h \
  engine/inc/C4Console.h \
  engine/inc/C4Constants.h \
  engine/inc/C4Control.h \
  engine/inc/C4DefGraphics.h \
  engine/inc/C4Def.h \
  engine/inc/C4DevmodeDlg.h \
  engine/inc/C4DownloadDlg.h \
  engine/inc/C4EditCursor.h \
  engine/inc/C4Effects.h \
  engine/inc/C4Extra.h \
  engine/inc/C4FacetEx.h \
  engine/inc/C4Facet.h \
  engine/inc/C4FileClasses.h \
  engine/inc/C4FileMonitor.h \
  engine/inc/C4FileSelDlg.h \
  engine/inc/C4FindObject.h \
  engine/inc/C4Folder.h \
  engine/inc/C4Fonts.h \
  engine/inc/C4FullScreen.h \
  engine/inc/C4Game.h \
  engine/inc/C4GameControl.h \
  engine/inc/C4GameControlNetwork.h \
  engine/inc/C4GameDialogs.h \
  engine/inc/C4GameLobby.h \
  engine/inc/C4GameMessage.h \
  engine/inc/C4GameObjects.h \
  engine/inc/C4GameOptions.h \
  engine/inc/C4GameOverDlg.h \
  engine/inc/C4GamePadCon.h \
  engine/inc/C4GameParameters.h \
  engine/inc/C4GameSave.h \
  engine/inc/C4GameVersion.h \
  engine/inc/C4GraphicsResource.h \
  engine/inc/C4GraphicsSystem.h \
  engine/inc/C4Group.h \
  engine/inc/C4GroupSet.h \
  engine/inc/C4Gui.h \
  engine/inc/C4Id.h \
  engine/inc/C4IDList.h \
  engine/inc/C4Include.h \
  engine/inc/C4InfoCore.h \
  engine/inc/C4InputValidation.h \
  engine/inc/C4InteractiveThread.h \
  engine/inc/C4KeyboardInput.h \
  engine/inc/C4Landscape.h \
  engine/inc/C4LangStringTable.h \
  engine/inc/C4Language.h \
  engine/inc/C4League.h \
  engine/inc/C4LoaderScreen.h \
  engine/inc/C4LogBuf.h \
  engine/inc/C4Log.h \
  engine/inc/C4MainMenu.h \
  engine/inc/C4MapCreatorS2.h \
  engine/inc/C4Map.h \
  engine/inc/C4MassMover.h \
  engine/inc/C4Material.h \
  engine/inc/C4MaterialList.h \
  engine/inc/C4Menu.h \
  engine/inc/C4MessageBoard.h \
  engine/inc/C4MessageInput.h \
  engine/inc/C4MouseControl.h \
  engine/inc/C4MusicFile.h \
  engine/inc/C4MusicSystem.h \
  engine/inc/C4NameList.h \
  engine/inc/C4NetIO.h \
  engine/inc/C4Network2Client.h \
  engine/inc/C4Network2Dialogs.h \
  engine/inc/C4Network2Discover.h \
  engine/inc/C4Network2.h \
  engine/inc/C4Network2IO.h \
  engine/inc/C4Network2IRC.h \
  engine/inc/C4Network2Players.h \
  engine/inc/C4Network2Reference.h \
  engine/inc/C4Network2Res.h \
  engine/inc/C4Network2Stats.h \
  engine/inc/C4ObjectCom.h \
  engine/inc/C4Object.h \
  engine/inc/C4ObjectInfo.h \
  engine/inc/C4ObjectInfoList.h \
  engine/inc/C4ObjectMenu.h \
  engine/inc/C4ObjectList.h \
  engine/inc/C4ObjectListDlg.h \
  engine/inc/C4PacketBase.h \
  engine/inc/C4Particles.h \
  engine/inc/C4PathFinder.h \
  engine/inc/C4Physics.h \
  engine/inc/C4Player.h \
  engine/inc/C4PlayerInfo.h \
  engine/inc/C4PlayerInfoListBox.h \
  engine/inc/C4PlayerList.h \
  engine/inc/C4PropertyDlg.h \
  engine/inc/C4Prototypes.h \
  engine/inc/C4PXS.h \
  engine/inc/C4Random.h \
  engine/inc/C4RankSystem.h \
  engine/inc/C4Record.h \
  engine/inc/C4Region.h \
  engine/inc/C4RTF.H \
  engine/inc/C4Scenario.h \
  engine/inc/C4Scoreboard.h \
  engine/inc/C4Script.h \
  engine/inc/C4ScriptHost.h \
  engine/inc/C4Sector.h \
  engine/inc/C4Shape.h \
  engine/inc/C4Sky.h \
  engine/inc/C4SolidMask.h \
  engine/inc/C4SoundSystem.h \
  engine/inc/C4Startup.h \
  engine/inc/C4StartupAboutDlg.h \
  engine/inc/C4StartupMainDlg.h \
  engine/inc/C4StartupNetDlg.h \
  engine/inc/C4StartupOptionsDlg.h \
  engine/inc/C4StartupPlrSelDlg.h \
  engine/inc/C4StartupScenSelDlg.h \
  engine/inc/C4Stat.h \
  engine/inc/C4StringTable.h \
  engine/inc/C4SurfaceFile.h \
  engine/inc/C4Surface.h \
  engine/inc/C4Teams.h \
  engine/inc/C4Texture.h \
  engine/inc/C4ToolsDlg.h \
  engine/inc/C4TransferZone.h \
  engine/inc/C4UpdateDlg.h \
  engine/inc/C4UpperBoard.h \
  engine/inc/C4UserMessages.h \
  engine/inc/C4Value.h \
  engine/inc/C4ValueList.h \
  engine/inc/C4ValueMap.h \
  engine/inc/C4Version.h \
  engine/inc/C4Video.h \
  engine/inc/C4VideoPlayback.h \
  engine/inc/C4Viewport.h \
  engine/inc/C4Weather.h \
  engine/inc/C4RoundResults.h \
  engine/src/C4RoundResults.cpp \
  engine/res/Brush.h \
  engine/res/c4x.xpm \
  engine/res/Cursor.h \
  engine/res/Dynamic.h \
  engine/res/Exact.h \
  engine/res/Fill.h \
  engine/res/Halt.h \
  engine/res/Ift.h \
  engine/res/Line.h \
  engine/res/Mouse.h \
  engine/res/NoIft.h \
  engine/res/Picker.h \
  engine/res/Play.h \
  engine/res/Rect.h \
  engine/res/Static.h \
  engine/src/C4PropList.cpp \
  engine/inc/C4PropList.h

if WIN32
clonk_SOURCES += engine/src/C4FileClasses.cpp
clonk_SOURCES += engine/res/engine.rc engine/res/resource.h
endif

clonk_LDADD = \
  libstandard.a \
  $(LIBICONV) \
  $(GTK_LIBS) \
  $(FREETYPE_LIBS) \
  $(SDL_LIBS) \
  $(PTHREAD_LIBS) \
  $(CLONK_LIBS) \
  $(OPENSSL_LIBS)

# build the resources
.rc.o:
	$(WINDRES) -I $(srcdir)/engine/res -i $< -o $@

## standard

#  standard/src/StdBitmap.cpp \
#
libstandard_a_SOURCES = \
  standard/src/Bitmap256.cpp \
  standard/src/CStdFile.cpp \
  standard/src/Fixed.cpp \
  standard/src/Midi.cpp \
  standard/src/OpenURL.cpp \
  standard/src/PathFinder.cpp \
  standard/src/Standard.cpp \
  standard/src/StdBase64.cpp \
  standard/src/StdBuf.cpp \
  standard/src/StdConfig.cpp \
  standard/src/StdCompiler.cpp \
  standard/src/StdD3D.cpp \
  standard/src/StdDDraw2.cpp \
  standard/src/StdFile.cpp \
  standard/src/StdFont.cpp \
  standard/src/StdGL.cpp \
  standard/src/StdGLCtx.cpp \
  standard/src/StdMarkup.cpp \
  standard/src/StdNoGfx.cpp \
  standard/src/StdPNG.cpp \
  standard/src/StdRegistry.cpp \
  standard/src/StdResStr2.cpp \
  standard/src/StdScheduler.cpp \
  standard/src/StdSurface2.cpp \
  standard/src/StdSurface8.cpp \
  standard/src/StdVideo.cpp \
  standard/inc/Bitmap256.h \
  standard/inc/CStdFile.h \
  standard/inc/DInputX.h \
  standard/inc/DSoundX.h \
  standard/inc/Fixed.h \
  standard/inc/Midi.h \
  standard/inc/PathFinder.h \
  standard/inc/Standard.h \
  standard/inc/StdAdaptors.h \
  standard/inc/StdBase64.h \
  standard/inc/StdBuf.h \
  standard/inc/StdColors.h \
  standard/inc/StdConfig.h \
  standard/inc/StdCompiler.h \
  standard/inc/StdD3D.h \
  standard/inc/StdD3DShader.h \
  standard/inc/StdDDraw2.h \
  standard/inc/StdFacet.h \
  standard/inc/StdFile.h \
  standard/inc/StdFont.h \
  standard/inc/StdGL.h \
  standard/inc/StdMarkup.h \
  standard/inc/StdNoGfx.h \
  standard/inc/StdPNG.h \
  standard/inc/StdRandom.h \
  standard/inc/StdRegistry.h \
  standard/inc/StdResStr2.h \
  standard/inc/StdResStr.h \
  standard/inc/StdScheduler.h \
  standard/inc/StdSurface2.h \
  standard/inc/StdSurface8.h \
  standard/inc/StdSync.h \
  standard/inc/StdVideo.h \
  standard/inc/StdWindow.h \
  standard/zlib/gzio.c \
  standard/zlib/zutil.h

if WIN32
libstandard_a_SOURCES += standard/src/StdWindow.cpp standard/src/StdJoystick.cpp standard/inc/StdJoystick.h
else
if SDL_MAIN_LOOP
libstandard_a_SOURCES += standard/src/StdSDLApp.cpp standard/src/StdSDLWindow.cpp
else
libstandard_a_SOURCES += standard/src/StdXApp.cpp standard/src/StdXWindow.cpp standard/src/StdXPrivate.h standard/src/StdTApp.cpp
endif
endif

if DEVELOPER_MODE
libstandard_a_SOURCES += standard/src/StdGtkWindow.cpp standard/inc/StdGtkWindow.h
endif

c4group_CPPFLAGS = \
  -I$(builddir) \
  -I$(srcdir)/standard \
  -I$(srcdir)/standard/inc \
  -I$(srcdir)/engine/inc \
  -I$(srcdir)/engine/sec \
  -DC4GROUP -DUSE_CONSOLE

c4group_SOURCES = \
  group/c4group_ng.cpp \
  engine/src/C4Group.cpp \
  engine/src/C4InputValidation.cpp \
  engine/sec/C4SecurityCertificates.cpp \
  engine/src/C4Update.cpp \
  engine/inc/C4Update.h \
  engine/inc/C4Group.h \
  engine/inc/C4Version.h \
  standard/src/Standard.cpp \
  standard/src/CStdFile.cpp \
  standard/src/StdMarkup.cpp \
  standard/src/StdBuf.cpp \
  standard/src/StdCompiler.cpp \
  standard/src/StdFile.cpp \
  standard/src/StdRegistry.cpp \
  standard/src/StdResStr2.cpp \
  standard/src/StdConfig.cpp

c4group_LDADD = \
  $(LIBICONV) \
  $(OPENSSL_LIBS) \
  $(PTHREAD_LIBS)

## gunzip4c4group

gunzip4c4group_SOURCES = \
  group/gunzip4c4group.cpp

gunzip4c4group_LDADD = \
  libstandard.a \
  $(PTHREAD_LIBS)

## puncher

puncher_SOURCES = \
  netpuncher/main.cpp \
  engine/src/C4NetIO.cpp \
  engine/inc/C4NetIO.h

puncher_LDADD = \
  libstandard.a \
  $(PTHREAD_LIBS)

if WIN32
puncher_LDADD += -lwinmm -lws2_32
endif

## tstc4netio

tstc4netio_SOURCES = \
  engine/netio/TstC4NetIO.cpp \
  engine/src/C4NetIO.cpp \
  engine/inc/C4NetIO.h

tstc4netio_LDADD = \
  libstandard.a \
  libzlib.a \
  $(PTHREAD_LIBS)

if WIN32
tstc4netio_LDADD += -lwinmm -lws2_32
endif

## other stuff

EXTRA_DIST = \
  clonk.anjuta \
  licenses \
  xcode \
  engine/netio/NetIO.dsp \
  netpuncher/Puncher.dsp \
  engine/res/Brush_Trans.png \
  engine/res/Cursor_Trans.png \
  engine/res/cz.ico \
  engine/res/Dynamic_Trans.png \
  engine/res/Exact_Trans.png \
  engine/res/Fill_Trans.png \
  engine/res/Halt_Trans.png \
  engine/res/Ift_Trans.png \
  engine/res/Line_Trans.png \
  engine/res/Mouse_Trans.png \
  engine/res/NoIft_Trans.png \
  engine/res/Picker_Trans.png \
  engine/res/Play_Trans.png \
  engine/res/Rect_Trans.png \
  engine/res/Static_Trans.png \
  group/c4group_cmdl.cpp \
  group/resource.h \
  group/Resource.rc \
  engine/res/brush1.bmp \
  engine/res/c4g.ico \
  engine/res/c4s.ico \
  engine/res/Cursor2.bmp \
  engine/res/Game.pal \
  engine/res/line1.bmp \
  engine/res/Play2.bmp \
  engine/res/Brush2.bmp \
  engine/res/c4i.ico \
  engine/res/c4u.ico \
  engine/res/Cursor.bmp \
  engine/res/Grab.bmp \
  engine/res/Line.bmp \
  engine/res/Play.bmp \
  engine/res/Brush.bmp \
  engine/res/c4k.ico \
  engine/res/c4v.ico \
  engine/res/dynamic1.bmp \
  engine/res/Halt2.bmp \
  engine/res/mouse1.bmp \
  engine/res/rect1.bmp \
  engine/res/c4b.ico \
  engine/res/c4l.ico \
  engine/res/c4x.ico \
  engine/res/Halt.bmp \
  engine/res/mouse.bmp \
  engine/res/Rectangle.bmp \
  engine/res/c4d.ico \
  engine/res/c4m.ico \
  engine/res/c4x.xpm \
  engine/res/fill1.bmp \
  engine/res/ift1.bmp \
  engine/res/NoIFT.bmp \
  engine/res/c4f.ico \
  engine/res/c4p.ico \
  engine/res/Fill.bmp \
  engine/res/IFT.bmp \
  engine/res/picker1.bmp \
  engine/res/static1.bmp

