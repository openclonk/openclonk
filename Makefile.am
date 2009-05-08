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

noinst_PROGRAMS = clonk

EXTRA_PROGRAMS = tstc4netio puncher c4group gunzip4c4group anigrab_

noinst_LIBRARIES = libstandard.a libzlib.a
if INTERNAL_JPEG
noinst_LIBRARIES += libjpeg.a
endif
if INTERNAL_PNG
noinst_LIBRARIES += libpng.a
endif
if INTERNAL_FREETYPE
noinst_LIBRARIES += libfreetype.a
endif

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
  -I$(srcdir)/standard/zlib \
  -I$(srcdir)/standard/inc \
  -I$(srcdir)/engine \
  -I$(srcdir)/engine/inc \
  -I$(srcdir)/engine/sec \
  -DC4ENGINE -DGLEW_STATIC \
  $(GTK_CFLAGS) $(FREETYPE_CFLAGS) $(SDL_CFLAGS)

if USE_FMOD
AM_CPPFLAGS += -I$(srcdir)/engine/fmod
endif

#various hacks to get dependency tracking working with a precompiled C4Include
if RECENT_GCC
BUILT_SOURCES = C4Include.h.gch
CLEANFILES = C4Include.h.gch

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
  engine/src/C4Wrappers.cpp \
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
  engine/inc/C4Wrappers.h \
  engine/inc/C4RoundResults.h \
  engine/src/C4RoundResults.cpp \
  engine/res/Brush.h \
  engine/res/c4x.h \
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
  engine/res/Static.h

if WIN32
clonk_SOURCES += engine/src/C4FileClasses.cpp
clonk_SOURCES += engine/res/engine.rc engine/res/resource.h
endif

if USE_FMOD
clonk_SOURCES += engine/fmod/fmod.h engine/fmod/fmod_errors.h engine/fmod/fmoddyn.h engine/fmod/wincompat.h
endif

clonk_LDADD = \
  libstandard.a \
  libzlib.a \
  $(LIBICONV) $(GTK_LIBS) $(FREETYPE_LIBS) $(SDL_LIBS) $(PTHREAD_LIBS) $(CLONK_LIBS) $(OPENSSL_LIBS)

if INTERNAL_JPEG
clonk_LDADD += libjpeg.a
endif

if INTERNAL_PNG
clonk_LDADD += libpng.a
endif

if INTERNAL_FREETYPE
clonk_LDADD += libfreetype.a
endif

if USE_FMOD
clonk_LDADD += $(srcdir)/engine/fmod/libfmod.a
endif

# build the resources
.rc.o:
	$(WINDRES) -I $(srcdir)/engine/res -i $< -o $@

## standard

libstandard_a_SOURCES = \
  standard/gl/glew.c \
  standard/gl/glew.h \
  standard/gl/glxew.h \
  standard/gl/wglew.h \
  standard/src/Bitmap256.cpp \
  standard/src/CStdFile.cpp \
  standard/src/Fixed.cpp \
  standard/src/Midi.cpp \
  standard/src/OpenURL.cpp \
  standard/src/PathFinder.cpp \
  standard/src/Standard.cpp \
  standard/src/StdBase64.cpp \
  standard/src/StdBitmap.cpp \
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
  standard/inc/StdBitmap.h \
  standard/inc/StdBuf.h \
  standard/inc/StdColors.h \
  standard/inc/StdConfig.h \
  standard/inc/StdCompiler.h \
  standard/inc/StdD3D.h \
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
  standard/inc/StdWindow.h

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

libpng_a_SOURCES = \
  standard/lpng121/png.c \
  standard/lpng121/pngerror.c \
  standard/lpng121/pnggccrd.c \
  standard/lpng121/pngget.c \
  standard/lpng121/pngmem.c \
  standard/lpng121/pngpread.c \
  standard/lpng121/pngread.c \
  standard/lpng121/pngrio.c \
  standard/lpng121/pngrtran.c \
  standard/lpng121/pngrutil.c \
  standard/lpng121/pngset.c \
  standard/lpng121/pngtrans.c \
  standard/lpng121/pngvcrd.c \
  standard/lpng121/pngwio.c \
  standard/lpng121/pngwrite.c \
  standard/lpng121/pngwtran.c \
  standard/lpng121/pngwutil.c \
  standard/lpng121/png.h \
  standard/lpng121/pngconf.h

libjpeg_a_SOURCES = \
  standard/jpeglib/jcapimin.c \
  standard/jpeglib/jconfig.h \
  standard/jpeglib/jdct.h \
  standard/jpeglib/jdtrans.c \
  standard/jpeglib/jmemmgr.c \
  standard/jpeglib/jcapistd.c \
  standard/jpeglib/jcparam.c \
  standard/jpeglib/jddctmgr.c \
  standard/jpeglib/jerror.c \
  standard/jpeglib/jmemsys.h \
  standard/jpeglib/jccoefct.c \
  standard/jpeglib/jcphuff.c \
  standard/jpeglib/jdhuff.c \
  standard/jpeglib/jerror.h \
  standard/jpeglib/jmorecfg.h \
  standard/jpeglib/jccolor.c \
  standard/jpeglib/jcprepct.c \
  standard/jpeglib/jdhuff.h \
  standard/jpeglib/jfdctflt.c \
  standard/jpeglib/jpegint.h \
  standard/jpeglib/jcdctmgr.c \
  standard/jpeglib/jcsample.c \
  standard/jpeglib/jdinput.c \
  standard/jpeglib/jfdctfst.c \
  standard/jpeglib/jpeglib.h \
  standard/jpeglib/jchuff.c \
  standard/jpeglib/jctrans.c \
  standard/jpeglib/jdmainct.c \
  standard/jpeglib/jfdctint.c \
  standard/jpeglib/jquant1.c \
  standard/jpeglib/jchuff.h \
  standard/jpeglib/jdapimin.c \
  standard/jpeglib/jdmarker.c \
  standard/jpeglib/jidctflt.c \
  standard/jpeglib/jquant2.c \
  standard/jpeglib/jcinit.c \
  standard/jpeglib/jdapistd.c \
  standard/jpeglib/jdmaster.c \
  standard/jpeglib/jidctfst.c \
  standard/jpeglib/jutils.c \
  standard/jpeglib/jcmainct.c \
  standard/jpeglib/jdatadst.c \
  standard/jpeglib/jdmerge.c \
  standard/jpeglib/jidctint.c \
  standard/jpeglib/jversion.h \
  standard/jpeglib/jcmarker.c \
  standard/jpeglib/jdatasrc.c \
  standard/jpeglib/jdphuff.c \
  standard/jpeglib/jidctred.c \
  standard/jpeglib/jcmaster.c \
  standard/jpeglib/jdcoefct.c \
  standard/jpeglib/jdpostct.c \
  standard/jpeglib/jinclude.h \
  standard/jpeglib/jcomapi.c \
  standard/jpeglib/jdcolor.c \
  standard/jpeglib/jdsample.c \
  standard/jpeglib/jmemansi.c

libzlib_a_SOURCES = \
  standard/zlib/adler32.c \
  standard/zlib/compress.c \
  standard/zlib/crc32.c \
  standard/zlib/deflate.c \
  standard/zlib/gzio.c \
  standard/zlib/infback.c \
  standard/zlib/inffast.c \
  standard/zlib/inflate.c \
  standard/zlib/inftrees.c \
  standard/zlib/trees.c \
  standard/zlib/uncompr.c \
  standard/zlib/zutil.c \
  standard/zlib/crc32.h \
  standard/zlib/inffast.h \
  standard/zlib/inflate.h \
  standard/zlib/trees.h \
  standard/zlib/zlib.h \
  standard/zlib/deflate.h \
  standard/zlib/inffixed.h \
  standard/zlib/inftrees.h \
  standard/zlib/zconf.h \
  standard/zlib/zutil.h

libfreetype_a_SOURCES = \
  standard/freetype2/src/autofit/autofit.c \
  standard/freetype2/src/base/ftbase.c \
  standard/freetype2/src/base/ftbitmap.c \
  standard/freetype2/src/base/ftglyph.c \
  standard/freetype2/src/base/ftinit.c \
  standard/freetype2/src/base/ftmm.c \
  standard/freetype2/src/base/ftsystem.c \
  standard/freetype2/src/base/fttrigon.c \
  standard/freetype2/src/base/fttype1.c \
  standard/freetype2/src/base/ftutil.c \
  standard/freetype2/src/base/ftwinfnt.c \
  standard/freetype2/src/base/ftxf86.c \
  standard/freetype2/src/bdf/bdf.c \
  standard/freetype2/src/cache/ftcache.c \
  standard/freetype2/src/cff/cff.c \
  standard/freetype2/src/cid/type1cid.c \
  standard/freetype2/src/gzip/ftgzip.c \
  standard/freetype2/src/lzw/ftlzw.c \
  standard/freetype2/src/otvalid/otvalid.c \
  standard/freetype2/src/pcf/pcf.c \
  standard/freetype2/src/pfr/pfr.c \
  standard/freetype2/src/psaux/psaux.c \
  standard/freetype2/src/pshinter/pshinter.c \
  standard/freetype2/src/psnames/psnames.c \
  standard/freetype2/src/raster/raster.c \
  standard/freetype2/src/sfnt/sfnt.c \
  standard/freetype2/src/smooth/smooth.c \
  standard/freetype2/src/truetype/truetype.c \
  standard/freetype2/src/type1/type1.c \
  standard/freetype2/src/type42/type42.c \
  standard/freetype2/src/winfonts/winfnt.c

libfreetype_a_CPPFLAGS = $(AM_CPPFLAGS) -DFT2_BUILD_LIBRARY

EXTRA_DIST = \
  standard/freetype2/freetype/config/ftconfig.h \
  standard/freetype2/freetype/config/ftheader.h \
  standard/freetype2/freetype/config/ftmodule.h \
  standard/freetype2/freetype/config/ftoption.h \
  standard/freetype2/freetype/config/ftstdlib.h \
  standard/freetype2/freetype/freetype.h \
  standard/freetype2/freetype/ftbbox.h \
  standard/freetype2/freetype/ftbdf.h \
  standard/freetype2/freetype/ftbitmap.h \
  standard/freetype2/freetype/ftcache.h \
  standard/freetype2/freetype/ftchapters.h \
  standard/freetype2/freetype/fterrdef.h \
  standard/freetype2/freetype/fterrors.h \
  standard/freetype2/freetype/ftgasp.h \
  standard/freetype2/freetype/ftglyph.h \
  standard/freetype2/freetype/ftgxval.h \
  standard/freetype2/freetype/ftgzip.h \
  standard/freetype2/freetype/ftimage.h \
  standard/freetype2/freetype/ftincrem.h \
  standard/freetype2/freetype/ftlcdfil.h \
  standard/freetype2/freetype/ftlist.h \
  standard/freetype2/freetype/ftlzw.h \
  standard/freetype2/freetype/ftmac.h \
  standard/freetype2/freetype/ftmm.h \
  standard/freetype2/freetype/ftmodapi.h \
  standard/freetype2/freetype/ftmoderr.h \
  standard/freetype2/freetype/ftotval.h \
  standard/freetype2/freetype/ftoutln.h \
  standard/freetype2/freetype/ftpfr.h \
  standard/freetype2/freetype/ftrender.h \
  standard/freetype2/freetype/ftsizes.h \
  standard/freetype2/freetype/ftsnames.h \
  standard/freetype2/freetype/ftstroke.h \
  standard/freetype2/freetype/ftsynth.h \
  standard/freetype2/freetype/ftsystem.h \
  standard/freetype2/freetype/fttrigon.h \
  standard/freetype2/freetype/fttypes.h \
  standard/freetype2/freetype/ftwinfnt.h \
  standard/freetype2/freetype/ftxf86.h \
  standard/freetype2/freetype/internal/autohint.h \
  standard/freetype2/freetype/internal/ftcalc.h \
  standard/freetype2/freetype/internal/ftdebug.h \
  standard/freetype2/freetype/internal/ftdriver.h \
  standard/freetype2/freetype/internal/ftgloadr.h \
  standard/freetype2/freetype/internal/ftmemory.h \
  standard/freetype2/freetype/internal/ftobjs.h \
  standard/freetype2/freetype/internal/ftrfork.h \
  standard/freetype2/freetype/internal/ftserv.h \
  standard/freetype2/freetype/internal/ftstream.h \
  standard/freetype2/freetype/internal/fttrace.h \
  standard/freetype2/freetype/internal/ftvalid.h \
  standard/freetype2/freetype/internal/internal.h \
  standard/freetype2/freetype/internal/pcftypes.h \
  standard/freetype2/freetype/internal/psaux.h \
  standard/freetype2/freetype/internal/pshints.h \
  standard/freetype2/freetype/internal/services/svbdf.h \
  standard/freetype2/freetype/internal/services/svgldict.h \
  standard/freetype2/freetype/internal/services/svgxval.h \
  standard/freetype2/freetype/internal/services/svkern.h \
  standard/freetype2/freetype/internal/services/svmm.h \
  standard/freetype2/freetype/internal/services/svotval.h \
  standard/freetype2/freetype/internal/services/svpfr.h \
  standard/freetype2/freetype/internal/services/svpostnm.h \
  standard/freetype2/freetype/internal/services/svpscmap.h \
  standard/freetype2/freetype/internal/services/svpsinfo.h \
  standard/freetype2/freetype/internal/services/svsfnt.h \
  standard/freetype2/freetype/internal/services/svttcmap.h \
  standard/freetype2/freetype/internal/services/svtteng.h \
  standard/freetype2/freetype/internal/services/svwinfnt.h \
  standard/freetype2/freetype/internal/services/svxf86nm.h \
  standard/freetype2/freetype/internal/sfnt.h \
  standard/freetype2/freetype/internal/t1types.h \
  standard/freetype2/freetype/internal/tttypes.h \
  standard/freetype2/freetype/t1tables.h \
  standard/freetype2/freetype/ttnameid.h \
  standard/freetype2/freetype/tttables.h \
  standard/freetype2/freetype/tttags.h \
  standard/freetype2/freetype/ttunpat.h \
  standard/freetype2/ft2build.h \
  standard/freetype2/src/autofit/afangles.c \
  standard/freetype2/src/autofit/afangles.h \
  standard/freetype2/src/autofit/afcjk.c \
  standard/freetype2/src/autofit/afcjk.h \
  standard/freetype2/src/autofit/afdummy.c \
  standard/freetype2/src/autofit/afdummy.h \
  standard/freetype2/src/autofit/aferrors.h \
  standard/freetype2/src/autofit/afglobal.c \
  standard/freetype2/src/autofit/afglobal.h \
  standard/freetype2/src/autofit/afhints.c \
  standard/freetype2/src/autofit/afhints.h \
  standard/freetype2/src/autofit/aflatin.c \
  standard/freetype2/src/autofit/aflatin.h \
  standard/freetype2/src/autofit/afloader.c \
  standard/freetype2/src/autofit/afloader.h \
  standard/freetype2/src/autofit/afmodule.c \
  standard/freetype2/src/autofit/afmodule.h \
  standard/freetype2/src/autofit/aftypes.h \
  standard/freetype2/src/autofit/afwarp.c \
  standard/freetype2/src/autofit/afwarp.h \
  standard/freetype2/src/base/ftapi.c \
  standard/freetype2/src/base/ftbbox.c \
  standard/freetype2/src/base/ftbdf.c \
  standard/freetype2/src/base/ftcalc.c \
  standard/freetype2/src/base/ftdbgmem.c \
  standard/freetype2/src/base/ftdebug.c \
  standard/freetype2/src/base/ftgasp.c \
  standard/freetype2/src/base/ftgloadr.c \
  standard/freetype2/src/base/ftgxval.c \
  standard/freetype2/src/base/ftlcdfil.c \
  standard/freetype2/src/base/ftmac.c \
  standard/freetype2/src/base/ftnames.c \
  standard/freetype2/src/base/ftobjs.c \
  standard/freetype2/src/base/ftotval.c \
  standard/freetype2/src/base/ftoutln.c \
  standard/freetype2/src/base/ftpfr.c \
  standard/freetype2/src/base/ftrfork.c \
  standard/freetype2/src/base/ftstream.c \
  standard/freetype2/src/base/ftstroke.c \
  standard/freetype2/src/base/ftsynth.c \
  standard/freetype2/src/bdf/bdf.h \
  standard/freetype2/src/bdf/bdfdrivr.c \
  standard/freetype2/src/bdf/bdfdrivr.h \
  standard/freetype2/src/bdf/bdferror.h \
  standard/freetype2/src/bdf/bdflib.c \
  standard/freetype2/src/cache/ftcbasic.c \
  standard/freetype2/src/cache/ftccache.c \
  standard/freetype2/src/cache/ftccache.h \
  standard/freetype2/src/cache/ftccback.h \
  standard/freetype2/src/cache/ftccmap.c \
  standard/freetype2/src/cache/ftcerror.h \
  standard/freetype2/src/cache/ftcglyph.c \
  standard/freetype2/src/cache/ftcglyph.h \
  standard/freetype2/src/cache/ftcimage.c \
  standard/freetype2/src/cache/ftcimage.h \
  standard/freetype2/src/cache/ftcmanag.c \
  standard/freetype2/src/cache/ftcmanag.h \
  standard/freetype2/src/cache/ftcmru.c \
  standard/freetype2/src/cache/ftcmru.h \
  standard/freetype2/src/cache/ftcsbits.c \
  standard/freetype2/src/cache/ftcsbits.h \
  standard/freetype2/src/cff/cffcmap.c \
  standard/freetype2/src/cff/cffcmap.h \
  standard/freetype2/src/cff/cffdrivr.c \
  standard/freetype2/src/cff/cffdrivr.h \
  standard/freetype2/src/cff/cfferrs.h \
  standard/freetype2/src/cff/cffgload.c \
  standard/freetype2/src/cff/cffgload.h \
  standard/freetype2/src/cff/cffload.c \
  standard/freetype2/src/cff/cffload.h \
  standard/freetype2/src/cff/cffobjs.c \
  standard/freetype2/src/cff/cffobjs.h \
  standard/freetype2/src/cff/cffparse.c \
  standard/freetype2/src/cff/cffparse.h \
  standard/freetype2/src/cff/cfftoken.h \
  standard/freetype2/src/cff/cfftypes.h \
  standard/freetype2/src/cid/ciderrs.h \
  standard/freetype2/src/cid/cidgload.c \
  standard/freetype2/src/cid/cidgload.h \
  standard/freetype2/src/cid/cidload.c \
  standard/freetype2/src/cid/cidload.h \
  standard/freetype2/src/cid/cidobjs.c \
  standard/freetype2/src/cid/cidobjs.h \
  standard/freetype2/src/cid/cidparse.c \
  standard/freetype2/src/cid/cidparse.h \
  standard/freetype2/src/cid/cidriver.c \
  standard/freetype2/src/cid/cidriver.h \
  standard/freetype2/src/cid/cidtoken.h \
  standard/freetype2/src/gxvalid/gxvalid.c \
  standard/freetype2/src/gxvalid/gxvalid.h \
  standard/freetype2/src/gxvalid/gxvbsln.c \
  standard/freetype2/src/gxvalid/gxvcommn.c \
  standard/freetype2/src/gxvalid/gxvcommn.h \
  standard/freetype2/src/gxvalid/gxverror.h \
  standard/freetype2/src/gxvalid/gxvfeat.c \
  standard/freetype2/src/gxvalid/gxvfeat.h \
  standard/freetype2/src/gxvalid/gxvfgen.c \
  standard/freetype2/src/gxvalid/gxvjust.c \
  standard/freetype2/src/gxvalid/gxvkern.c \
  standard/freetype2/src/gxvalid/gxvlcar.c \
  standard/freetype2/src/gxvalid/gxvmod.c \
  standard/freetype2/src/gxvalid/gxvmod.h \
  standard/freetype2/src/gxvalid/gxvmort.c \
  standard/freetype2/src/gxvalid/gxvmort.h \
  standard/freetype2/src/gxvalid/gxvmort0.c \
  standard/freetype2/src/gxvalid/gxvmort1.c \
  standard/freetype2/src/gxvalid/gxvmort2.c \
  standard/freetype2/src/gxvalid/gxvmort4.c \
  standard/freetype2/src/gxvalid/gxvmort5.c \
  standard/freetype2/src/gxvalid/gxvmorx.c \
  standard/freetype2/src/gxvalid/gxvmorx.h \
  standard/freetype2/src/gxvalid/gxvmorx0.c \
  standard/freetype2/src/gxvalid/gxvmorx1.c \
  standard/freetype2/src/gxvalid/gxvmorx2.c \
  standard/freetype2/src/gxvalid/gxvmorx4.c \
  standard/freetype2/src/gxvalid/gxvmorx5.c \
  standard/freetype2/src/gxvalid/gxvopbd.c \
  standard/freetype2/src/gxvalid/gxvprop.c \
  standard/freetype2/src/gxvalid/gxvtrak.c \
  standard/freetype2/src/lzw/ftzopen.c \
  standard/freetype2/src/lzw/ftzopen.h \
  standard/freetype2/src/lzw/zopen.c \
  standard/freetype2/src/lzw/zopen.h \
  standard/freetype2/src/otvalid/otvalid.h \
  standard/freetype2/src/otvalid/otvbase.c \
  standard/freetype2/src/otvalid/otvcommn.c \
  standard/freetype2/src/otvalid/otvcommn.h \
  standard/freetype2/src/otvalid/otverror.h \
  standard/freetype2/src/otvalid/otvgdef.c \
  standard/freetype2/src/otvalid/otvgpos.c \
  standard/freetype2/src/otvalid/otvgpos.h \
  standard/freetype2/src/otvalid/otvgsub.c \
  standard/freetype2/src/otvalid/otvjstf.c \
  standard/freetype2/src/otvalid/otvmod.c \
  standard/freetype2/src/otvalid/otvmod.h \
  standard/freetype2/src/pcf/pcf.h \
  standard/freetype2/src/pcf/pcfdrivr.c \
  standard/freetype2/src/pcf/pcfdrivr.h \
  standard/freetype2/src/pcf/pcferror.h \
  standard/freetype2/src/pcf/pcfread.c \
  standard/freetype2/src/pcf/pcfread.h \
  standard/freetype2/src/pcf/pcfutil.c \
  standard/freetype2/src/pcf/pcfutil.h \
  standard/freetype2/src/pfr/pfrcmap.c \
  standard/freetype2/src/pfr/pfrcmap.h \
  standard/freetype2/src/pfr/pfrdrivr.c \
  standard/freetype2/src/pfr/pfrdrivr.h \
  standard/freetype2/src/pfr/pfrerror.h \
  standard/freetype2/src/pfr/pfrgload.c \
  standard/freetype2/src/pfr/pfrgload.h \
  standard/freetype2/src/pfr/pfrload.c \
  standard/freetype2/src/pfr/pfrload.h \
  standard/freetype2/src/pfr/pfrobjs.c \
  standard/freetype2/src/pfr/pfrobjs.h \
  standard/freetype2/src/pfr/pfrsbit.c \
  standard/freetype2/src/pfr/pfrsbit.h \
  standard/freetype2/src/pfr/pfrtypes.h \
  standard/freetype2/src/psaux/afmparse.c \
  standard/freetype2/src/psaux/afmparse.h \
  standard/freetype2/src/psaux/psauxerr.h \
  standard/freetype2/src/psaux/psauxmod.c \
  standard/freetype2/src/psaux/psauxmod.h \
  standard/freetype2/src/psaux/psconv.c \
  standard/freetype2/src/psaux/psconv.h \
  standard/freetype2/src/psaux/psobjs.c \
  standard/freetype2/src/psaux/psobjs.h \
  standard/freetype2/src/psaux/t1cmap.c \
  standard/freetype2/src/psaux/t1cmap.h \
  standard/freetype2/src/psaux/t1decode.c \
  standard/freetype2/src/psaux/t1decode.h \
  standard/freetype2/src/pshinter/pshalgo.c \
  standard/freetype2/src/pshinter/pshalgo.h \
  standard/freetype2/src/pshinter/pshglob.c \
  standard/freetype2/src/pshinter/pshglob.h \
  standard/freetype2/src/pshinter/pshmod.c \
  standard/freetype2/src/pshinter/pshmod.h \
  standard/freetype2/src/pshinter/pshnterr.h \
  standard/freetype2/src/pshinter/pshrec.c \
  standard/freetype2/src/pshinter/pshrec.h \
  standard/freetype2/src/psnames/psmodule.c \
  standard/freetype2/src/psnames/psmodule.h \
  standard/freetype2/src/psnames/psnamerr.h \
  standard/freetype2/src/psnames/pstables.h \
  standard/freetype2/src/raster/ftmisc.h \
  standard/freetype2/src/raster/ftraster.c \
  standard/freetype2/src/raster/ftraster.h \
  standard/freetype2/src/raster/ftrend1.c \
  standard/freetype2/src/raster/ftrend1.h \
  standard/freetype2/src/raster/rasterrs.h \
  standard/freetype2/src/sfnt/sfdriver.c \
  standard/freetype2/src/sfnt/sfdriver.h \
  standard/freetype2/src/sfnt/sferrors.h \
  standard/freetype2/src/sfnt/sfobjs.c \
  standard/freetype2/src/sfnt/sfobjs.h \
  standard/freetype2/src/sfnt/ttbdf.c \
  standard/freetype2/src/sfnt/ttbdf.h \
  standard/freetype2/src/sfnt/ttcmap.c \
  standard/freetype2/src/sfnt/ttcmap.h \
  standard/freetype2/src/sfnt/ttkern.c \
  standard/freetype2/src/sfnt/ttkern.h \
  standard/freetype2/src/sfnt/ttload.c \
  standard/freetype2/src/sfnt/ttload.h \
  standard/freetype2/src/sfnt/ttmtx.c \
  standard/freetype2/src/sfnt/ttmtx.h \
  standard/freetype2/src/sfnt/ttpost.c \
  standard/freetype2/src/sfnt/ttpost.h \
  standard/freetype2/src/sfnt/ttsbit.c \
  standard/freetype2/src/sfnt/ttsbit.h \
  standard/freetype2/src/sfnt/ttsbit0.c \
  standard/freetype2/src/sfnt/ttsbit0.h \
  standard/freetype2/src/smooth/ftgrays.c \
  standard/freetype2/src/smooth/ftgrays.h \
  standard/freetype2/src/smooth/ftsmerrs.h \
  standard/freetype2/src/smooth/ftsmooth.c \
  standard/freetype2/src/smooth/ftsmooth.h \
  standard/freetype2/src/truetype/ttdriver.c \
  standard/freetype2/src/truetype/ttdriver.h \
  standard/freetype2/src/truetype/tterrors.h \
  standard/freetype2/src/truetype/ttgload.c \
  standard/freetype2/src/truetype/ttgload.h \
  standard/freetype2/src/truetype/ttgxvar.c \
  standard/freetype2/src/truetype/ttgxvar.h \
  standard/freetype2/src/truetype/ttinterp.c \
  standard/freetype2/src/truetype/ttinterp.h \
  standard/freetype2/src/truetype/ttobjs.c \
  standard/freetype2/src/truetype/ttobjs.h \
  standard/freetype2/src/truetype/ttpload.c \
  standard/freetype2/src/truetype/ttpload.h \
  standard/freetype2/src/type1/t1afm.c \
  standard/freetype2/src/type1/t1afm.h \
  standard/freetype2/src/type1/t1driver.c \
  standard/freetype2/src/type1/t1driver.h \
  standard/freetype2/src/type1/t1errors.h \
  standard/freetype2/src/type1/t1gload.c \
  standard/freetype2/src/type1/t1gload.h \
  standard/freetype2/src/type1/t1load.c \
  standard/freetype2/src/type1/t1load.h \
  standard/freetype2/src/type1/t1objs.c \
  standard/freetype2/src/type1/t1objs.h \
  standard/freetype2/src/type1/t1parse.c \
  standard/freetype2/src/type1/t1parse.h \
  standard/freetype2/src/type1/t1tokens.h \
  standard/freetype2/src/type42/t42drivr.c \
  standard/freetype2/src/type42/t42drivr.h \
  standard/freetype2/src/type42/t42error.h \
  standard/freetype2/src/type42/t42objs.c \
  standard/freetype2/src/type42/t42objs.h \
  standard/freetype2/src/type42/t42parse.c \
  standard/freetype2/src/type42/t42parse.h \
  standard/freetype2/src/type42/t42types.h \
  standard/freetype2/src/winfonts/fnterrs.h \
  standard/freetype2/src/winfonts/winfnt.h
## c4group

c4group_CPPFLAGS = \
  -I$(builddir) \
  -I$(srcdir)/standard \
  -I$(srcdir)/standard/inc \
  -I$(srcdir)/standard/zlib \
  -I$(srcdir)/engine/inc \
  -I$(srcdir)/engine/sec \
  -DC4GROUP -DUSE_CONSOLE

c4group_SOURCES = \
  group/c4group_ng.cpp \
  engine/src/C4Config.cpp \
  engine/sec/C4ConfigShareware.cpp \
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
  libzlib.a \
  $(LIBICONV) $(OPENSSL_LIBS) $(PTHREAD_LIBS)

## gunzip4c4group

gunzip4c4group_SOURCES = \
  group/gunzip4c4group.cpp

gunzip4c4group_LDADD = \
  libstandard.a \
  libzlib.a \
  $(PTHREAD_LIBS)

## puncher

puncher_SOURCES = \
  netpuncher/main.cpp \
  engine/src/C4NetIO.cpp \
  engine/inc/C4NetIO.h

puncher_LDADD = \
  libstandard.a \
  libzlib.a \
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

## anigrab

anigrab__SOURCES = \
  anigrab/anigrab.cpp \
  anigrab/loadpng/common.cpp \
  anigrab/loadpng/loadpng.cpp \
  anigrab/loadpng/savepng.cpp \
  anigrab/loadpng/bmphed.h \
  anigrab/loadpng/common.h \
  anigrab/loadpng/loadpng.h \
  anigrab/loadpng/savepng.h

anigrab__LDADD = \
  libstandard.a \
  libzlib.a

if INTERNAL_PNG
anigrab__LDADD += libpng.a
else
anigrab__LDADD += -lpng
endif

## other stuff

EXTRA_DIST += \
  clonk.anjuta \
  BUILD \
  clonk.dsw \
  clonk.vc7.sln \
  credits.txt \
  editor \
  licenses \
  README.linux \
  xcode \
  anigrab/anigrab.dsp \
  anigrab/anigrab.dsw \
  engine/engine_con.dsp \
  engine/engine.dsp \
  engine/engine.vc7.vcproj \
  engine/fmod/fmod.dll \
  engine/fmod/fmodvc.lib \
  engine/netio/NetIO.dsp \
  netpuncher/Puncher.dsp \
  engine/res/Brush_Trans.png \
  engine/res/c4x.png \
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
  group/c4group.dsp \
  group/c4group.vc7.vcproj \
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
  engine/res/static1.bmp \
  standard/lpng121/LICENSE \
  standard/lpng121/pngtest.c \
  standard/openssl

binary-tarball: all clonk$(EXEEXT) c4group$(EXEEXT)
	linuxdistname=clonk-linux-$$(sed -e 's/.define C4XVERBUILD \([1-9][0-9][0-9]*\)/\1/p;d' $(srcdir)/engine/inc/C4Version.h) && \
	rm -rf $${linuxdistname} && \
	mkdir $${linuxdistname} && \
	cp ObjectsNET2.c4d GraphicsNET2.c4g SystemNET2.c4g $${linuxdistname} && \
	cp clonk$(EXEEXT) c4group$(EXEEXT) $${linuxdistname} && \
	cp ../binaries/linux/LGPL.txt ../binaries/linux/cr.png ../binaries/linux/install.sh $${linuxdistname} && \
	strip $${linuxdistname}/clonk$(EXEEXT) && \
	strip $${linuxdistname}/c4group$(EXEEXT) && \
	cp $(srcdir)/README.linux $${linuxdistname}/README && \
	tar -cvzf $${linuxdistname}.tar.gz $${linuxdistname} && \
	rm -rf $${linuxdistname}

binary-zip: all clonk$(EXEEXT)
	windowsdistname=$$(sed -e 's/.define C4VERSIONBUILDNAME "\(..*\)"/\1/p;d' $(srcdir)/engine/inc/C4Version.h) && \
	rm -rf $${windowsdistname} && \
	mkdir $${windowsdistname} && \
	cp clonk$(EXEEXT) $${windowsdistname} && \
	strip $${windowsdistname}/clonk$(EXEEXT) && \
	zip -r $${windowsdistname}.zip $${windowsdistname} && \
	rm -rf $${windowsdistname}
