/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

/* Forward declarations */

#ifndef INC_C4Prototypes
#define INC_C4Prototypes

// class declarations
class C4AbstractApp;
class C4Action;
class C4AulBCC;
class C4AulDefFunc;
class C4AulExec;
class C4AulFunc;
struct C4AulParSet;
struct C4AulScriptContext;
class C4AulScriptEngine;
class C4AulScriptFunc;
class C4BltTransform;
class C4Client;
class C4ClientCore;
class C4ClientList;
class C4ClientPlayerInfos;
class C4Command;
class C4Config;
class C4Console;
class C4Control;
class C4Def;
class C4DefGraphics;
class C4DefList;
class C4Draw;
class C4Effect;
class C4Facet;
class C4FacetSurface;
class C4FileMonitor;
class C4Game;
class C4GameMessage;
class C4GameMessageList;
class C4GameOptionButtons;
class C4GameOptionsList;
class C4GamePadControl;
class C4GamePadOpener;
class C4GameParameters;
class C4GameResList;
class C4GameSec1Timer;
class C4Graph;
class C4GraphicsSystem;
class C4Group;
class C4GroupSet;
class C4IDList;
class C4KeyboardInput;
class C4LangStringTable;
class C4LChunk;
class C4League;
class C4LoaderScreen;
class C4LSector;
class C4LSectors;
class C4MainMenu;
class C4MapCreatorS2;
class C4Markup;
class C4MassMover;
class C4MassMoverSet;
class C4Material;
class C4MaterialList;
class C4MaterialMap;
class C4Menu;
class C4MenuItem;
class C4MeshDenumerator;
class C4MouseControl;
class C4NameList;
class C4Network2Client;
class C4Network2IOConnection;
class C4Network2IRCClient;
class C4Network2RefClient;
class C4Network2Reference;
class C4Network2ResDlg;
class C4Network2Stats;
class C4Object;
class C4ObjectInfo;
class C4ObjectInfoCore;
class C4ObjectLink;
class C4ObjectList;
class C4PacketJoinData;
class C4PathFinder;
class C4Pattern;
class C4Playback;
class C4Player;
class C4PlayerInfo;
class C4PlayerInfoCore;
class C4PlayerInfoList;
class C4PlayerInfoListBox;
class C4PlayerList;
class C4PropList;
class C4PropListStatic;
class C4PXS;
class C4PXSSystem;
class C4RankSystem;
class C4Record;
class C4Rect;
class C4RoundResult;
class C4RoundResults;
class C4Scenario;
class C4ScriptHost;
class C4SolidMask;
class C4SoundSystem;
class C4Stream;
class C4String;
class C4Surface;
class C4SVal;
class C4TargetFacet;
class C4TargetRect;
class C4Team;
class C4TeamList;
class C4TexMapEntry;
class C4TexMgr;
class C4TexRef;
class C4TextureMap;
class C4TransferZone;
class C4TransferZones;
class C4Value;
class C4ValueArray;
class C4ValueNumbers;
class C4Viewport;
class C4ViewportList;
class C4ViewportWindow;
class C4Window;
class CStdFont;
class CStdGLCtx;
struct CStdPalette;
class CStdStream;
class CStdVectorFont;
class CSurface8;
class StdCompiler;
class StdMesh;
class StdMeshAnimation;
class StdMeshAnimationUpdate;
class StdMeshBone;
class StdMeshInstance;
class StdMeshInstanceAnimationNode;
class StdMeshMaterial;
class StdMeshMaterialUpdate;
class StdMeshMatManager;
class StdMeshMatrix;
class StdMeshSkeletonLoader;
class StdMeshUpdate;

namespace C4GameLobby
{
	class Countdown;
	class MainDlg;
}
namespace C4GUI
{
	class ComboBox_FillCB;
	class Dialog;
	class FrameDecoration;
	class Screen;
}
typedef C4GUI::Screen C4GUIScreen;

#endif // INC_C4Prototypes
