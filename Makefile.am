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

bin_PROGRAMS = clonk c4group

EXTRA_PROGRAMS = tstc4netio puncher gunzip4c4group

noinst_LIBRARIES = lib.a

# Some defines and warning options
if RECENT_GCC
GCC_FLAGS = -pipe
WARNING_FLAGS = -Wall -Wextra -Wno-invalid-offsetof \
-Wredundant-decls -Wendif-labels -Wpointer-arith \
-Wcast-qual -Wcast-align -Wwrite-strings -Winit-self \
-Wsign-promo \
-Wno-parentheses -Wno-unused-parameter -Wno-inline \
-Wno-reorder -Wno-float-equal -Wno-switch
##-Wmissing-format-attribute -Wdisabled-optimization -Wlogical-op
##-Weffc++ -Wold-style-cast -Woverloaded-virtual -Wunsafe-loop-optimizations
else
GCC_FLAGS =
WARNING_FLAGS = -Wall
endif

AM_CXXFLAGS = $(PTHREAD_CFLAGS) $(WINDOWS_CFLAGS) $(WARNING_FLAGS) $(GCC_FLAGS)

AM_CFLAGS = -Wall $(GCC_FLAGS)

AM_CPPFLAGS = \
  -I$(builddir) \
  -I$(srcdir)/src \
  -I$(srcdir)/src/c4group \
  -I$(srcdir)/src/network \
  -I$(srcdir)/src/lib \
  -I$(srcdir)/src/platform \
  -I$(srcdir)/src/config \
  -I$(srcdir)/src/res \
  -I$(srcdir)/src/control \
  -I$(srcdir)/src/gui \
  -I$(srcdir)/src/editor \
  -I$(srcdir)/src/game/landscape \
  -I$(srcdir)/src/game/player \
  -I$(srcdir)/src/game/script \
  -I$(srcdir)/src/game \
  -I$(srcdir)/src/game/object \
  -I$(srcdir)/src/lib/texture \
  -I$(srcdir)/src/script \
  $(GLEW_CFLAGS) $(GTK_CFLAGS) $(FREETYPE_CFLAGS) $(SDL_CFLAGS) $(BOOST_CPPFLAGS)

##BUILT_SOURCES = hgrevision.h
##hgrevision.h: $(srcdir)/.hg/dirstate
##	$(srcdir)/tools/get_hg_revision.sh > hgrevision.h
##$(srcdir)/.hg/dirstate:

BUILT_SOURCES = C4Version.h
EXTRA_DIST = automake.in
do_subst = sed -e 's,[@]C4PROJECT[@],OpenClonk Project,g' \
  -e 's,[@]C4PROJECT_DOMAIN[@],openclonk,g' \
  -e 's,[@]C4PROJECT_TLD[@],org,g' \
  -e 's,[@]C4PROJECT_URL[@],http://wiki.openclonk.org,g' \
  -e 's,[@]C4COPYRIGHT_YEAR[@],'$$(sed -n 's/SET(C4COPYRIGHT_YEAR\s\+\(.\+\)).\+/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4ENGINENAME[@],'"$$(sed -n 's/SET(C4ENGINENAME\s\+"\(.\+\)")/\1/ p' $(srcdir)/Version.txt)"',g' \
  -e 's,[@]C4ENGINENICK[@],'$$(sed -n 's/SET(C4ENGINENICK\s\+"\(.\+\)")/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4ENGINEID[@],org.openclonk." C4ENGINENICK ",g' \
  -e 's,[@]C4XVER1[@],'$$(sed -n 's/SET(C4XVER1\s\+\(.\+\))/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4XVER2[@],'$$(sed -n 's/SET(C4XVER2\s\+\(.\+\))/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4XVER3[@],'$$(sed -n 's/SET(C4XVER3\s\+\(.\+\))/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4XVER4[@],'$$(sed -n 's/SET(C4XVER4\s\+\(.\+\))/\1/ p' $(srcdir)/Version.txt)',g' \
  -e 's,[@]C4VERSIONBUILDNAME[@],'"$$(sed -n 's/SET(C4VERSIONBUILDNAME\s\+"\(.\+\)")/\1/ p' $(srcdir)/Version.txt)"',g' \
  -e 's,[@]C4VERSIONEXTRA[@],'$$(sed -n 's/SET(C4VERSIONEXTRA\s\+"\(.\+\)")/\1/ p' $(srcdir)/Version.txt)',g'

C4Version.h: $(srcdir)/src/C4Version.h.in
	$(do_subst) < $< > $@



#various hacks to get dependency tracking working with a precompiled C4Include
if RECENT_GCC
BUILT_SOURCES += C4Include.h.gch
CLEANFILES = C4Include.h.gch
DISTCLEANFILES = @am__quote@$(DEPDIR)/C4Include.Po@am__quote@

# Adapted from the automake compile-command for normal source files.
# Has to be updated after certain changes in this file.
C4Include.h.gch: src/C4Include.h
@am__fastdepCXX_TRUE@	$(CXXCOMPILE) -x c++-header -MT C4Include.h.gch -MD -MP -MF $(DEPDIR)/C4Include.Tpo -c -o $@ $(srcdir)/src/C4Include.h
@am__fastdepCXX_TRUE@	mv -f $(DEPDIR)/C4Include.Tpo $(DEPDIR)/C4Include.Po
@am__fastdepCXX_FALSE@	$(CXXCOMPILE) -x c++-header -c -o $@ $(srcdir)/src/C4Include.h

@AMDEP_TRUE@@am__include@ @am__quote@$(DEPDIR)/C4Include.Po@am__quote@

if am__fastdepCXX
AM_CXXFLAGS += -fpch-deps
endif

WARNING_FLAGS += -Winvalid-pch

endif

lib_a_SOURCES = \
src/lib/C4InputValidation.cpp \
src/lib/C4InputValidation.h \
src/lib/C4LogBuf.cpp \
src/lib/C4LogBuf.h \
src/lib/C4Random.cpp \
src/lib/C4Random.h \
src/lib/C4Real.cpp \
src/lib/C4Real.h \
src/lib/C4Rect.cpp \
src/lib/C4Rect.h \
src/lib/C4RTF.cpp \
src/lib/C4RTF.h \
src/lib/C4Stat.cpp \
src/lib/C4Stat.h \
src/lib/PathFinder.cpp \
src/lib/PathFinder.h \
src/lib/Standard.cpp \
src/lib/Standard.h \
src/lib/StdAdaptors.h \
src/lib/StdBase64.cpp \
src/lib/StdBase64.h \
src/lib/StdBuf.cpp \
src/lib/StdBuf.h \
src/lib/StdColors.h \
src/lib/StdCompiler.cpp \
src/lib/StdCompiler.h \
src/lib/StdMarkup.cpp \
src/lib/StdMarkup.h \
src/lib/StdResStr2.cpp \
src/lib/StdResStr2.h \
src/lib/StdResStr.h \
src/lib/texture/C4Facet.cpp \
src/lib/texture/C4FacetEx.cpp \
src/lib/texture/C4FacetEx.h \
src/lib/texture/C4Facet.h \
src/lib/texture/C4Surface.cpp \
src/lib/texture/C4SurfaceFile.cpp \
src/lib/texture/C4SurfaceFile.h \
src/lib/texture/C4Surface.h \
src/lib/texture/StdPNG.cpp \
src/lib/texture/StdPNG.h

clonk_SOURCES = \
src/lib/C4Log.cpp \
src/lib/C4Log.h \
src/lib/C4NameList.cpp \
src/lib/C4NameList.h \
src/lib/texture/C4GraphicsResource.cpp \
src/lib/texture/C4GraphicsResource.h \
src/C4Application.cpp \
src/C4Application.h \
src/C4FullScreen.cpp \
src/C4FullScreen.h \
src/C4Game.cpp \
src/C4Game.h \
src/C4Globals.cpp \
src/C4GraphicsSystem.cpp \
src/C4GraphicsSystem.h \
src/c4group/C4LangStringTable.cpp \
src/c4group/C4LangStringTable.h \
src/c4group/C4Components.h \
src/c4group/C4ComponentHost.cpp \
src/c4group/C4ComponentHost.h \
src/c4group/C4Extra.cpp \
src/c4group/C4Extra.h \
src/c4group/C4Group.cpp \
src/c4group/C4Group.h \
src/c4group/C4GroupSet.cpp \
src/c4group/C4GroupSet.h \
src/c4group/C4Language.cpp \
src/c4group/C4Language.h \
src/c4group/C4Update.cpp \
src/c4group/C4Update.h \
src/c4group/CStdFile.cpp \
src/c4group/CStdFile.h \
src/C4Include.h \
src/C4Prototypes.h \
src/C4Version.h \
src/C4WinMain.cpp \
src/config/C4Config.cpp \
src/config/C4Config.h \
src/config/C4ConfigShareware.cpp \
src/config/C4ConfigShareware.h \
src/config/C4Constants.h \
src/config/C4SecurityCertificates.cpp \
src/config/C4SecurityCertificates.h \
src/control/C4Control.cpp \
src/control/C4Control.h \
src/control/C4GameControl.cpp \
src/control/C4GameControl.h \
src/control/C4GameParameters.cpp \
src/control/C4GameParameters.h \
src/control/C4GameSave.cpp \
src/control/C4GameSave.h \
src/control/C4PlayerControl.cpp \
src/control/C4PlayerControl.h \
src/control/C4PlayerInfoConflicts.cpp \
src/control/C4PlayerInfo.cpp \
src/control/C4PlayerInfo.h \
src/control/C4Record.cpp \
src/control/C4Record.h \
src/control/C4RoundResults.cpp \
src/control/C4RoundResults.h \
src/control/C4Teams.cpp \
src/control/C4Teams.h \
src/editor/C4Console.cpp \
src/editor/C4Console.h \
src/editor/C4DevmodeDlg.cpp \
src/editor/C4DevmodeDlg.h \
src/editor/C4EditCursor.cpp \
src/editor/C4EditCursor.h \
src/editor/C4ObjectListDlg.cpp \
src/editor/C4ObjectListDlg.h \
src/editor/C4PropertyDlg.cpp \
src/editor/C4PropertyDlg.h \
src/editor/C4ToolsDlg.cpp \
src/editor/C4ToolsDlg.h \
src/game/C4GameVersion.h \
src/game/C4Physics.h \
src/game/landscape/C4Landscape.cpp \
src/game/landscape/C4Landscape.h \
src/game/landscape/C4Map.cpp \
src/game/landscape/C4MapCreatorS2.cpp \
src/game/landscape/C4MapCreatorS2.h \
src/game/landscape/C4Map.h \
src/game/landscape/C4MassMover.cpp \
src/game/landscape/C4MassMover.h \
src/game/landscape/C4Material.cpp \
src/game/landscape/C4Material.h \
src/game/landscape/C4MaterialList.cpp \
src/game/landscape/C4MaterialList.h \
src/game/landscape/C4Particles.cpp \
src/game/landscape/C4Particles.h \
src/game/landscape/C4PathFinder.cpp \
src/game/landscape/C4PathFinder.h \
src/game/landscape/C4PXS.cpp \
src/game/landscape/C4PXS.h \
src/game/landscape/C4Region.cpp \
src/game/landscape/C4Region.h \
src/game/landscape/C4Scenario.cpp \
src/game/landscape/C4Scenario.h \
src/game/landscape/C4Sky.cpp \
src/game/landscape/C4Sky.h \
src/game/landscape/C4SolidMask.cpp \
src/game/landscape/C4SolidMask.h \
src/game/landscape/C4Texture.cpp \
src/game/landscape/C4Texture.h \
src/game/landscape/C4Weather.cpp \
src/game/landscape/C4Weather.h \
src/game/object/C4Action.cpp \
src/game/object/C4Command.cpp \
src/game/object/C4Command.h \
src/game/object/C4Def.cpp \
src/game/object/C4DefGraphics.cpp \
src/game/object/C4DefGraphics.h \
src/game/object/C4Def.h \
src/game/object/C4GameObjects.cpp \
src/game/object/C4GameObjects.h \
src/game/object/C4Id.cpp \
src/game/object/C4Id.h \
src/game/object/C4IDList.cpp \
src/game/object/C4IDList.h \
src/game/object/C4InfoCore.cpp \
src/game/object/C4InfoCore.h \
src/game/object/C4MeshAnimation.cpp \
src/game/object/C4MeshAnimation.h \
src/game/object/C4Movement.cpp \
src/game/object/C4ObjectCom.cpp \
src/game/object/C4ObjectCom.h \
src/game/object/C4Object.cpp \
src/game/object/C4Object.h \
src/game/object/C4ObjectInfo.cpp \
src/game/object/C4ObjectInfo.h \
src/game/object/C4ObjectInfoList.cpp \
src/game/object/C4ObjectInfoList.h \
src/game/object/C4ObjectList.cpp \
src/game/object/C4ObjectList.h \
src/game/object/C4ObjectMenu.cpp \
src/game/object/C4ObjectMenu.h \
src/game/object/C4ObjectPtr.cpp \
src/game/object/C4ObjectPtr.h \
src/game/object/C4Sector.cpp \
src/game/object/C4Sector.h \
src/game/object/C4Shape.cpp \
src/game/object/C4Shape.h \
src/game/player/C4Player.cpp \
src/game/player/C4Player.h \
src/game/player/C4PlayerList.cpp \
src/game/player/C4PlayerList.h \
src/game/player/C4RankSystem.cpp \
src/game/player/C4RankSystem.h \
src/game/script/C4Effect.cpp \
src/game/script/C4Effects.h \
src/game/script/C4FindObject.cpp \
src/game/script/C4FindObject.h \
src/game/script/C4Script.cpp \
src/game/script/C4Script.h \
src/game/script/C4TransferZone.cpp \
src/game/script/C4TransferZone.h \
src/gui/C4ChatDlg.cpp \
src/gui/C4ChatDlg.h \
src/gui/C4DownloadDlg.cpp \
src/gui/C4DownloadDlg.h \
src/gui/C4FileSelDlg.cpp \
src/gui/C4FileSelDlg.h \
src/gui/C4Folder.cpp \
src/gui/C4Folder.h \
src/gui/C4GameDialogs.cpp \
src/gui/C4GameDialogs.h \
src/gui/C4GameLobby.cpp \
src/gui/C4GameLobby.h \
src/gui/C4GameMessage.cpp \
src/gui/C4GameMessage.h \
src/gui/C4GameOptions.cpp \
src/gui/C4GameOptions.h \
src/gui/C4GameOverDlg.cpp \
src/gui/C4GameOverDlg.h \
src/gui/C4GuiButton.cpp \
src/gui/C4GuiCheckBox.cpp \
src/gui/C4GuiComboBox.cpp \
src/gui/C4GuiContainers.cpp \
src/gui/C4Gui.cpp \
src/gui/C4GuiDialogs.cpp \
src/gui/C4GuiEdit.cpp \
src/gui/C4Gui.h \
src/gui/C4GuiLabels.cpp \
src/gui/C4GuiListBox.cpp \
src/gui/C4GuiMenu.cpp \
src/gui/C4GuiTabular.cpp \
src/gui/C4KeyboardInput.cpp \
src/gui/C4KeyboardInput.h \
src/gui/C4LoaderScreen.cpp \
src/gui/C4LoaderScreen.h \
src/gui/C4MainMenu.cpp \
src/gui/C4MainMenu.h \
src/gui/C4Menu.cpp \
src/gui/C4Menu.h \
src/gui/C4MessageBoard.cpp \
src/gui/C4MessageBoard.h \
src/gui/C4MessageInput.cpp \
src/gui/C4MessageInput.h \
src/gui/C4MouseControl.cpp \
src/gui/C4MouseControl.h \
src/gui/C4PlayerInfoListBox.cpp \
src/gui/C4PlayerInfoListBox.h \
src/gui/C4Scoreboard.cpp \
src/gui/C4Scoreboard.h \
src/gui/C4StartupAboutDlg.cpp \
src/gui/C4StartupAboutDlg.h \
src/gui/C4Startup.cpp \
src/gui/C4Startup.h \
src/gui/C4StartupMainDlg.cpp \
src/gui/C4StartupMainDlg.h \
src/gui/C4StartupNetDlg.cpp \
src/gui/C4StartupNetDlg.h \
src/gui/C4StartupOptionsDlg.cpp \
src/gui/C4StartupOptionsDlg.h \
src/gui/C4StartupPlrSelDlg.cpp \
src/gui/C4StartupPlrSelDlg.h \
src/gui/C4StartupScenSelDlg.cpp \
src/gui/C4StartupScenSelDlg.h \
src/gui/C4UpdateDlg.cpp \
src/gui/C4UpdateDlg.h \
src/gui/C4UpperBoard.cpp \
src/gui/C4UpperBoard.h \
src/gui/C4UserMessages.h \
src/gui/C4Viewport.cpp \
src/gui/C4Viewport.h \
src/network/C4Client.cpp \
src/network/C4Client.h \
src/network/C4GameControlNetwork.cpp \
src/network/C4GameControlNetwork.h \
src/network/C4InteractiveThread.cpp \
src/network/C4InteractiveThread.h \
src/network/C4League.cpp \
src/network/C4League.h \
src/network/C4NetIO.cpp \
src/network/C4NetIO.h \
src/network/C4Network2Client.cpp \
src/network/C4Network2Client.h \
src/network/C4Network2.cpp \
src/network/C4Network2Dialogs.cpp \
src/network/C4Network2Dialogs.h \
src/network/C4Network2Discover.cpp \
src/network/C4Network2Discover.h \
src/network/C4Network2.h \
src/network/C4Network2IO.cpp \
src/network/C4Network2IO.h \
src/network/C4Network2IRC.cpp \
src/network/C4Network2IRC.h \
src/network/C4Network2Players.cpp \
src/network/C4Network2Players.h \
src/network/C4Network2Reference.cpp \
src/network/C4Network2Reference.h \
src/network/C4Network2Res.cpp \
src/network/C4Network2ResDlg.cpp \
src/network/C4Network2Res.h \
src/network/C4Network2Stats.cpp \
src/network/C4Network2Stats.h \
src/network/C4Packet2.cpp \
src/network/C4PacketBase.h \
src/platform/Bitmap256.cpp \
src/platform/Bitmap256.h \
src/platform/C4FileClasses.h \
src/platform/C4FileMonitor.cpp \
src/platform/C4FileMonitor.h \
src/platform/C4Fonts.cpp \
src/platform/C4Fonts.h \
src/platform/C4GamePadCon.cpp \
src/platform/C4GamePadCon.h \
src/platform/C4MusicFile.cpp \
src/platform/C4MusicFile.h \
src/platform/C4MusicSystem.cpp \
src/platform/C4MusicSystem.h \
src/platform/C4SoundSystem.cpp \
src/platform/C4SoundSystem.h \
src/platform/C4Video.cpp \
src/platform/C4Video.h \
src/platform/C4VideoPlayback.cpp \
src/platform/C4VideoPlayback.h \
src/platform/OpenURL.cpp \
src/platform/PlatformAbstraction.h \
src/platform/StdConfig.cpp \
src/platform/StdConfig.h \
src/platform/StdD3D.cpp \
src/platform/StdD3D.h \
src/platform/StdD3DShader.cpp \
src/platform/StdD3DShader.h \
src/platform/StdDDraw2.cpp \
src/platform/StdDDraw2.h \
src/platform/StdFacet.h \
src/platform/StdFile.cpp \
src/platform/StdFile.h \
src/platform/StdFont.cpp \
src/platform/StdFont.h \
src/platform/StdGL.cpp \
src/platform/StdGLCtx.cpp \
src/platform/StdGL.h \
src/platform/StdMesh.cpp \
src/platform/StdMesh.h \
src/platform/StdMeshLoader.h \
src/platform/StdMeshLoaderBinary.cpp \
src/platform/StdMeshLoaderBinaryChunks.cpp \
src/platform/StdMeshLoaderBinaryChunks.h \
src/platform/StdMeshLoaderDataStream.h \
src/platform/StdMeshLoaderXml.cpp \
src/platform/StdMeshMaterial.cpp \
src/platform/StdMeshMaterial.h \
src/platform/StdNoGfx.cpp \
src/platform/StdNoGfx.h \
src/platform/StdRegistry.cpp \
src/platform/StdRegistry.h \
src/platform/StdScheduler.cpp \
src/platform/StdScheduler.h \
src/platform/StdSurface2.cpp \
src/platform/StdSurface2.h \
src/platform/StdSurface8.cpp \
src/platform/StdSurface8.h \
src/platform/StdSync.h \
src/platform/StdVideo.cpp \
src/platform/StdVideo.h \
src/platform/StdWindow.h \
src/res/Brush.h \
src/res/Cursor.h \
src/res/Dynamic.h \
src/res/Exact.h \
src/res/Fill.h \
src/res/Halt.h \
src/res/Ift.h \
src/res/Line.h \
src/res/Mouse.h \
src/res/NoIft.h \
src/res/Picker.h \
src/res/Play.h \
src/res/Rect.h \
src/res/resource.h \
src/res/Static.h \
src/script/C4AList.cpp \
src/script/C4AList.h \
src/script/C4Aul.cpp \
src/script/C4AulDebug.cpp \
src/script/C4AulExec.cpp \
src/script/C4Aul.h \
src/script/C4AulLink.cpp \
src/script/C4AulParse.cpp \
src/script/C4PropList.cpp \
src/script/C4PropList.h \
src/script/C4ScriptHost.cpp \
src/script/C4ScriptHost.h \
src/script/C4StringTable.cpp \
src/script/C4StringTable.h \
src/script/C4Value.cpp \
src/script/C4Value.h \
src/script/C4ValueList.cpp \
src/script/C4ValueList.h \
src/script/C4ValueMap.cpp \
src/script/C4ValueMap.h \
src/tinyxml/tinystr.cpp \
src/tinyxml/tinystr.h \
src/tinyxml/tinyxml.cpp \
src/tinyxml/tinyxml.h \
src/tinyxml/tinyxmlerror.cpp \
src/tinyxml/tinyxmlparser.cpp \
src/zlib/gzio.c \
src/zlib/zutil.h \
src/script/C4AulDebug.h \
src/script/C4AulExec.h \
src/C4Version.h.in
##hgrevision.h

if WIN32
clonk_SOURCES += src/platform/C4FileClasses.cpp
clonk_SOURCES += src/res/engine.rc src/res/resource.h
clonk_SOURCES += src/platform/StdJoystick.cpp src/platform/StdJoystick.h
endif
if MACOSX
clonk_SOURCES += src/platform/MacUtility.h src/platform/MacUtility.mm src/platform/StdMacApp.mm
endif
if SDL_MAIN_LOOP
clonk_SOURCES += src/platform/StdSDLApp.cpp src/platform/StdSDLWindow.cpp
else
if WIN32
clonk_SOURCES += src/platform/StdWindow.cpp
else
clonk_SOURCES += src/platform/StdXApp.cpp src/platform/StdXWindow.cpp src/platform/StdXPrivate.h src/platform/StdTApp.cpp
endif
endif
if DEVELOPER_MODE
clonk_SOURCES += src/platform/StdGtkWindow.cpp src/platform/StdGtkWindow.h
endif

clonk_LDADD = \
  lib.a \
  $(LIBICONV) \
  $(GTK_LIBS) \
  $(FREETYPE_LIBS) \
  $(SDL_LIBS) \
  $(PTHREAD_LIBS) \
  $(Z_LIBS) \
  $(CLONK_LIBS) \
  $(OPENSSL_LIBS)

# build the resources
.rc.o:
	$(WINDRES) -I $(srcdir)/src/res -I $(builddir) -i $< -o $@

c4group_CPPFLAGS = \
  -I$(builddir) \
  -I$(srcdir)/src \
  -I$(srcdir)/src/c4group \
  -I$(srcdir)/src/network \
  -I$(srcdir)/src/lib \
  -I$(srcdir)/src/platform \
  -I$(srcdir)/src/config \
  -I$(srcdir)/src/res \
  -I$(srcdir)/src/control \
  -I$(srcdir)/src/gui \
  -I$(srcdir)/src/editor \
  -I$(srcdir)/src/game/landscape \
  -I$(srcdir)/src/game/player \
  -I$(srcdir)/src/game/script \
  -I$(srcdir)/src/game \
  -I$(srcdir)/src/game/object \
  -I$(srcdir)/src/lib/texture \
  -I$(srcdir)/src/script

c4group_SOURCES = \
src/c4group/c4group_ng.cpp \
src/c4group/C4Group.cpp \
src/lib/C4InputValidation.cpp \
src/config/C4SecurityCertificates.cpp \
src/c4group/C4Update.cpp \
src/lib/Standard.cpp \
src/c4group/CStdFile.cpp \
src/lib/StdMarkup.cpp \
src/lib/StdBuf.cpp \
src/lib/StdCompiler.cpp \
src/platform/StdFile.cpp \
src/platform/StdRegistry.cpp \
src/lib/StdResStr2.cpp \
src/platform/StdConfig.cpp \
src/zlib/gzio.c

if WIN32
c4group_SOURCES += src/c4group/Resource.rc
endif

c4group_LDADD = \
  $(LIBICONV) \
  $(Z_LIBS) \
  $(OPENSSL_LIBS) \
  $(PTHREAD_LIBS)

## gunzip4c4group

gunzip4c4group_SOURCES = \
src/c4group/gunzip4c4group.cpp \
src/c4group/C4Group.cpp \
src/lib/C4InputValidation.cpp \
src/config/C4SecurityCertificates.cpp \
src/c4group/C4Update.cpp \
src/lib/Standard.cpp \
src/c4group/CStdFile.cpp \
src/lib/StdMarkup.cpp \
src/lib/StdBuf.cpp \
src/lib/StdCompiler.cpp \
src/platform/StdFile.cpp \
src/platform/StdRegistry.cpp \
src/lib/StdResStr2.cpp \
src/platform/StdConfig.cpp \
src/zlib/gzio.c

gunzip4c4group_LDADD = \
  $(PTHREAD_LIBS)

## puncher

puncher_SOURCES = \
  src/netpuncher/main.cpp \
  src/network/C4NetIO.cpp

puncher_LDADD = \
  $(PTHREAD_LIBS)

if WIN32
puncher_LDADD += -lwinmm -lws2_32
endif

## tstc4netio

tstc4netio_SOURCES = \
  src/netio/TstC4NetIO.cpp \
  src/network/C4NetIO.cpp

tstc4netio_LDADD = \
  $(PTHREAD_LIBS) -lz

if WIN32
tstc4netio_LDADD += -lwinmm -lws2_32
endif

## other stuff

EXTRA_DIST += \
  clonk.anjuta \
  licenses \
  xcode \
  src/netio/NetIO.dsp \
  src/netpuncher/Puncher.dsp \
  src/res/Brush_Trans.png \
  src/res/Cursor_Trans.png \
  src/res/cz.ico \
  src/res/Dynamic_Trans.png \
  src/res/Exact_Trans.png \
  src/res/Fill_Trans.png \
  src/res/Halt_Trans.png \
  src/res/Ift_Trans.png \
  src/res/Line_Trans.png \
  src/res/Mouse_Trans.png \
  src/res/NoIft_Trans.png \
  src/res/Picker_Trans.png \
  src/res/Play_Trans.png \
  src/res/Rect_Trans.png \
  src/res/Static_Trans.png \
  src/c4group/c4group_cmdl.cpp \
  src/c4group/resource.h \
  src/c4group/Resource.rc \
  src/res/brush1.bmp \
  src/res/c4g.ico \
  src/res/c4s.ico \
  src/res/Cursor2.bmp \
  src/res/Game.pal \
  src/res/line1.bmp \
  src/res/Play2.bmp \
  src/res/Brush2.bmp \
  src/res/c4i.ico \
  src/res/c4u.ico \
  src/res/Cursor.bmp \
  src/res/Grab.bmp \
  src/res/Line.bmp \
  src/res/Play.bmp \
  src/res/Brush.bmp \
  src/res/c4k.ico \
  src/res/c4v.ico \
  src/res/dynamic1.bmp \
  src/res/Halt2.bmp \
  src/res/mouse1.bmp \
  src/res/rect1.bmp \
  src/res/c4b.ico \
  src/res/c4l.ico \
  src/res/c4x.ico \
  src/res/Halt.bmp \
  src/res/mouse.bmp \
  src/res/Rectangle.bmp \
  src/res/c4d.ico \
  src/res/c4m.ico \
  src/res/c4x.xpm \
  src/res/fill1.bmp \
  src/res/ift1.bmp \
  src/res/NoIFT.bmp \
  src/res/c4f.ico \
  src/res/c4p.ico \
  src/res/Fill.bmp \
  src/res/IFT.bmp \
  src/res/picker1.bmp \
  src/res/static1.bmp

