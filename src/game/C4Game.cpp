/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

/* Main class to run the game */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "game/C4Game.h"

#include "C4Version.h"
#include "control/C4GameControl.h"
#include "control/C4GameParameters.h"
#include "control/C4GameSave.h"
#include "control/C4PlayerControl.h"
#include "control/C4PlayerInfo.h"
#include "control/C4Record.h"
#include "control/C4RoundResults.h"
#include "editor/C4Console.h"
#include "game/C4Application.h"
#include "game/C4FullScreen.h"
#include "game/C4GraphicsSystem.h"
#include "game/C4Viewport.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4ChatDlg.h"
#include "gui/C4GameLobby.h"
#include "gui/C4GameMessage.h"
#include "gui/C4GameOverDlg.h"
#include "gui/C4LoaderScreen.h"
#include "gui/C4MessageInput.h"
#include "gui/C4MouseControl.h"
#include "gui/C4ScriptGuiWindow.h"
#include "gui/C4Startup.h"
#include "landscape/C4Landscape.h"
#include "landscape/C4MapScript.h"
#include "landscape/C4MassMover.h"
#include "landscape/C4Material.h"
#include "landscape/C4PXS.h"
#include "landscape/C4Particles.h"
#include "landscape/C4Sky.h"
#include "landscape/C4SolidMask.h"
#include "landscape/C4Texture.h"
#include "landscape/C4Weather.h"
#include "landscape/fow/C4FoW.h"
#include "lib/C4Random.h"
#include "lib/C4Stat.h"
#include "lib/StdMesh.h"
#include "network/C4League.h"
#include "network/C4Network2Dialogs.h"
#include "network/C4Network2Reference.h"
#include "network/C4Network2Stats.h"
#include "object/C4Command.h"
#include "object/C4Def.h"
#include "object/C4DefList.h"
#include "object/C4GameObjects.h"
#include "object/C4Object.h"
#include "object/C4ObjectCom.h"
#include "object/C4ObjectInfo.h"
#include "object/C4ObjectMenu.h"
#include "platform/C4FileMonitor.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "player/C4RankSystem.h"
#include "script/C4AulDebug.h"
#include "script/C4AulExec.h"
#include "script/C4Effect.h"

#include <unordered_map>

class C4GameSec1Timer : public C4ApplicationSec1Timer
{
public:
	C4GameSec1Timer() { Application.Add(this); }
	~C4GameSec1Timer() override { Application.Remove(this); }
	void OnSec1Timer() override;
};

static C4GameParameters GameParameters;
static C4ScenarioParameterDefs GameScenarioParameterDefs;
static C4ScenarioParameters GameStartupScenarioParameters;
static C4RoundResults GameRoundResults;
static C4Value GameGlobalSoundModifier;

C4Game::C4Game():
		ScenarioParameterDefs(GameScenarioParameterDefs),
		Parameters(GameParameters),
		StartupScenarioParameters(GameStartupScenarioParameters),
		Clients(Parameters.Clients),
		Teams(Parameters.Teams),
		PlayerInfos(Parameters.PlayerInfos),
		RestorePlayerInfos(Parameters.RestorePlayerInfos),
		RoundResults(GameRoundResults),
		Input(Control.Input),
		KeyboardInput(C4KeyboardInput_Init()),
		pSec1Timer(new C4GameSec1Timer()),
		GlobalSoundModifier(GameGlobalSoundModifier)
{
	Default();
}

C4Game::~C4Game()
{
	// make sure no startup gfx remain loaded
	C4Startup::Unload();
}

bool C4Game::InitDefs()
{
	int32_t iDefs=0;
	Log(LoadResStr("IDS_PRC_INITDEFS"));
	int iDefResCount = 0;
	C4GameRes *pDef;
	for (pDef = Parameters.GameRes.iterRes(nullptr, NRT_Definitions); pDef; pDef = Parameters.GameRes.iterRes(pDef, NRT_Definitions))
		++iDefResCount;
	int i = 0;
	// Load specified defs
	for (pDef = Parameters.GameRes.iterRes(nullptr, NRT_Definitions); pDef; pDef = Parameters.GameRes.iterRes(pDef, NRT_Definitions))
	{
		int iMinProgress = 25 + (25 * i) / iDefResCount;
		int iMaxProgress = 25 + (25 * (i + 1)) / iDefResCount;
		++i;
		iDefs+=::Definitions.Load(pDef->getFile(),C4D_Load_RX,Config.General.LanguageEx,&Application.SoundSystem,true,iMinProgress,iMaxProgress);

		// Def load failure
		if (::Definitions.LoadFailure) return false;
	}

	// Load for scenario file - ignore sys group here, because it has been loaded already
	iDefs+=::Definitions.Load(ScenarioFile,C4D_Load_RX,Config.General.LanguageEx,&Application.SoundSystem,true,true,35,40, false);

	// Absolutely no defs: we don't like that
	if (!iDefs) { LogFatal(LoadResStr("IDS_PRC_NODEFS")); return false; }

	// Check def engine version (should be done immediately on def load)
	iDefs=::Definitions.CheckEngineVersion(C4XVER1,C4XVER2);
	if (iDefs>0) { LogF(LoadResStr("IDS_PRC_DEFSINVC4X"),iDefs); }

	// Check for unmet requirements
	::Definitions.CheckRequireDef();

	// build quick access table
	::Definitions.BuildTable();

	// handle skeleton appends and includes
	::Definitions.AppendAndIncludeSkeletons();

	// Done
	return true;
}


bool C4Game::OpenScenario()
{

	// Scenario from record stream
	if (RecordStream.getSize())
	{
		StdStrBuf RecordFile;
		if (!C4Playback::StreamToRecord(RecordStream.getData(), &RecordFile))
			{ LogFatal("[!] Could not process record stream data!"); return false; }
		SCopy(RecordFile.getData(), ScenarioFilename, _MAX_PATH);
	}

	// Scenario filename check & log
	if (!ScenarioFilename[0]) { LogFatal(LoadResStr("IDS_PRC_NOC4S")); return false; }
	LogF(LoadResStr("IDS_PRC_LOADC4S"),ScenarioFilename);

	// get parent folder, if it's ocf
	pParentGroup = GroupSet.RegisterParentFolders(ScenarioFilename);

	// open scenario
	if (pParentGroup)
	{
		// open from parent group
		if (!ScenarioFile.OpenAsChild(pParentGroup, GetFilename(ScenarioFilename)))
			{ LogF("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), (const char *)ScenarioFilename); return false; }
	}
	else
	{
		// open directly
		if (!Reloc.Open(ScenarioFile, ScenarioFilename))
			{ LogF("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), (const char *)ScenarioFilename); return false; }
	}

	// Remember full (absolute) path
	SCopy(ScenarioFile.GetFullName().getData(), ScenarioFilename, _MAX_PATH);

	// add scenario to group
	GroupSet.RegisterGroup(ScenarioFile, false, C4GSPrio_Scenario, C4GSCnt_Scenario);

	// Read scenario core
	if (!C4S.Load(ScenarioFile))
		{ LogFatal(LoadResStr("IDS_PRC_FILEINVALID")); return false; }

	// Check minimum engine version
	if (CompareVersion(C4S.Head.C4XVer[0],C4S.Head.C4XVer[1]) > 0)
	{
		LogFatal(FormatString(LoadResStr("IDS_PRC_NOREQC4X"), C4S.Head.C4XVer[0],C4S.Head.C4XVer[1]).getData());
		return false;
	}

	// Add scenario origin to group set
	if (C4S.Head.Origin.getLength() && !ItemIdentical(C4S.Head.Origin.getData(), ScenarioFilename))
		GroupSet.RegisterParentFolders(C4S.Head.Origin.getData());

	// Scenario definition preset
	StdStrBuf sDefinitionFilenames;
	if (!C4S.Definitions.AllowUserChange && C4S.Definitions.GetModules(&sDefinitionFilenames))
	{
		SCopy(sDefinitionFilenames.getData(), DefinitionFilenames, (sizeof DefinitionFilenames)-1);
		if (DefinitionFilenames[0]) Log(LoadResStr("IDS_PRC_SCEOWNDEFS"));
		else Log(LoadResStr("IDS_PRC_LOCALONLY"));
	}

	// Check mission access
#ifndef USE_CONSOLE
#ifndef _DEBUG
	if (C4S.Head.MissionAccess[0])
		if (!Application.isEditor)
			if (!SIsModule(Config.General.MissionAccess, C4S.Head.MissionAccess.c_str()))
				{ LogFatal(LoadResStr("IDS_PRC_NOMISSIONACCESS")); return false; }
#endif
#endif

	// Title
	C4Language::LoadComponentHost(&Title, ScenarioFile, C4CFN_Title, Config.General.LanguageEx);
	if (!Title.GetLanguageString(Config.General.LanguageEx, ScenarioTitle))
		ScenarioTitle = C4S.Head.Title;

	// String tables
	C4Language::LoadComponentHost(&ScenarioLangStringTable, ScenarioFile, C4CFN_ScriptStringTbl, Config.General.LanguageEx);

	// Custom scenario parameter definitions. Load even as network client to get localized option names
	ScenarioParameterDefs.Load(ScenarioFile, &ScenarioLangStringTable);

	// Load parameters (not as network client, because then team info has already been sent by host)
	if (!Network.isEnabled() || Network.isHost())
		if (!Parameters.Load(ScenarioFile, &C4S, GameText.GetData(), &ScenarioLangStringTable, DefinitionFilenames, &StartupScenarioParameters))
			return false;

	SetInitProgress(4);

	// If scenario is a directory: Watch for changes
	if (!ScenarioFile.IsPacked() && pFileMonitor)
		Game.pFileMonitor->AddDirectory(ScenarioFile.GetFullName().getData());

	return true;
}

void C4Game::CloseScenario()
{
	// close scenario
	ScenarioFile.Close();
	GroupSet.CloseFolders();
	pParentGroup = nullptr;
	// remove if temporary
	if (TempScenarioFile)
	{
		EraseItem(TempScenarioFile.getData());
		TempScenarioFile.Clear();
	}
	// clear scenario section
	// this removes any temp files, which may yet need to be used by any future features
	// so better don't do this too early (like, in C4Game::Clear)
	if (pScenarioSections) { delete pScenarioSections; pScenarioSections=pCurrentScenarioSection=nullptr;}
}


bool C4Game::PreInit()
{
	// init extra root group
	// this loads font definitions in this group as well
	// the function may return false, if no extra group is present - that is OK
	Extra.InitGroup();

	RandomSeed = time(nullptr);
	// Randomize
	FixRandom(RandomSeed);
	// Timer flags
	GameGo=false;
	// init message input (default commands)
	MessageInput.Init();
	Game.SetInitProgress(31.0f);
	// init keyboard input (default keys, plus overloads)
	if (!InitKeyboard())
		{ LogFatal(LoadResStr("IDS_ERR_NOKEYBOARD")); return false; }
	// Load string table
	UpdateLanguage();
	// Player keyboard input: Key definitions and default sets
	if (!InitPlayerControlSettings()) return false;
	Game.SetInitProgress(32.0f);
	// Rank system
	::DefaultRanks.Init(Config.GetSubkeyPath("ClonkRanks"), LoadResStr("IDS_GAME_DEFRANKS"), 1000);
	Game.SetInitProgress(33.0f);

	// Graphics system (required for GUI)
	if (!GraphicsSystem.Init())
		{ LogFatal(LoadResStr("IDS_ERR_NOGFXSYS")); return false; }

	// load GUI
#ifndef USE_CONSOLE
	C4Rect r;
	if (Application.isEditor)
		Console.GetSize(&r);
	else
		FullScreen.GetSize(&r);
	pGUI->Init(0, 0, r.Wdt, r.Hgt);
#endif

	fPreinited = true;

	return true;
}

bool C4Game::Init()
{
	C4ValueNumbers numbers;
	IsRunning = false;

	InitProgress=0; LastInitProgress=0;
	SetInitProgress(0);

	// reinit keyboard to reflect any config changes that might have been done
	// this is a good time to do it, because no GUI dialogs are opened
	if (!InitKeyboard()) LogFatal(LoadResStr("IDS_ERR_NOKEYBOARD"));

	// start log pos (used by startup)
	StartupLogPos=GetLogPos();
	fQuitWithError = false;
	C4GameLobby::UserAbort = false;

	// Store a start time that identifies this game on this host
	StartTime = time(nullptr);

	// Get PlayerFilenames from Config, if ParseCommandLine did not fill some in
	// Must be done here, because InitGame calls PlayerInfos.InitLocal
	if (!*PlayerFilenames)
	{
		SCopy(Config.General.Participants, PlayerFilenames, std::min(sizeof(PlayerFilenames), sizeof(Config.General.Participants)) - 1);
	}

	// Join a game?
	if (pJoinReference || *DirectJoinAddress)
	{

		if (!GraphicsSystem.pLoaderScreen)
		{
			// init extra; needed for loader screen
			Log(LoadResStr("IDS_PRC_INITEXTRA"));
			if (!Extra.Init())
				{ LogFatal(LoadResStr("IDS_PRC_ERREXTRA")); return false; }

			// init loader
			if (!Application.isEditor && !GraphicsSystem.InitLoaderScreen(C4S.Head.Loader.c_str()))
				{ LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false; }
		}

		SetInitProgress(5);

		// Initialize network
		if (pJoinReference)
		{
			// By reference
			bool fSuccess = InitNetworkFromReference(*pJoinReference);
			pJoinReference.reset();
			if (!fSuccess)
				return false;
		}
		else if (SEqual2(DirectJoinAddress, DirectJoinFilePrefix))
		{
			// By reference serialized to temp file
			if (!InitNetworkFromReferenceFile(DirectJoinAddress + strlen(DirectJoinFilePrefix)))
				return false;
		}
		else
		{
			// By address
			if (!InitNetworkFromAddress(DirectJoinAddress))
				return false;
		}

		// check wether console mode is allowed
		if (Application.isEditor && !Parameters.AllowDebug)
			{ LogFatal(LoadResStr("IDS_TEXT_JOININCONSOLEMODENOTALLOW")); return false; }

		// do lobby (if desired)
		if (Network.isLobbyActive())
			if (!Network.DoLobby())
				return false;

		// get scenario
		char szScenario[_MAX_PATH+1];
		SetInitProgress(6);
		if (!Network.RetrieveScenario(szScenario)) return false;

		// open new scenario
		SCopy(szScenario, ScenarioFilename, _MAX_PATH);
		if (!OpenScenario()) return false;
		TempScenarioFile.Copy(ScenarioFilename);

		// get everything else
		if (!Parameters.GameRes.RetrieveFiles()) return false;

		// Check network game data scenario type (safety)
		if (!C4S.Head.NetworkGame)
			{ LogFatal(LoadResStr("IDS_NET_NONETGAME")); return false; }

		SetInitProgress(7);

	}

	// Local game or host?
	else
	{

		// Open scenario
		if (!OpenScenario())
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }

		// init extra; needed for loader screen
		Log(LoadResStr("IDS_PRC_INITEXTRA"));
		if (!Extra.Init())
			{ LogFatal(LoadResStr("IDS_PRC_ERREXTRA")); return false; }

		// init loader
		if (!Application.isEditor && !GraphicsSystem.InitLoaderScreen(C4S.Head.Loader.c_str()))
			{ LogFatal(LoadResStr("IDS_PRC_ERRLOADER")); return false; }

		// Init network
		if (!InitNetworkHost()) return false;
		SetInitProgress(7);

	}

	// now free all startup gfx to make room for game gfx
	C4Startup::Unload();

	// Init debugmode
	DebugMode = !!Application.isEditor;
	if (Config.General.AlwaysDebug)
		DebugMode = true;
	if (!Parameters.AllowDebug)
		DebugMode = false;

	// Init game
	if (!InitGame(ScenarioFile, IM_Normal, true, &numbers)) return false;

	// Network final init
	if (Network.isEnabled())
	{
		if (!Network.FinalInit()) return false;
	}
	// non-net may have to synchronize now to keep in sync with replays
	// also needs to synchronize to update transfer zones
	else
	{
		// - would kill DebugRec-sync for runtime debugrec starts
		C4DebugRecOff DBGRECOFF(!!C4S.Head.SaveGame);
		SyncClearance();
		Synchronize(false);
	}

	// Init players
	if (!InitPlayers(&numbers)) return false;
	SetInitProgress(98);

	// Final init
	if (!InitGameFinal(IM_Normal)) return false;
	SetInitProgress(99);

	// Sound modifier from savegames
	if (GlobalSoundModifier) SetGlobalSoundModifier(GlobalSoundModifier._getPropList());

	// Message board and upper board
	if (!Application.isEditor)
	{
		InitFullscreenComponents(true);
	}

	// Default fullscreen menu, in case any old surfaces are left (extra safety)
	FullScreen.CloseMenu();

	// start statistics (always for now. Make this a config?)
	pNetworkStatistics = std::make_unique<C4Network2Stats>();

	// clear loader screen
	if (GraphicsSystem.pLoaderScreen)
	{
		delete GraphicsSystem.pLoaderScreen;
		GraphicsSystem.pLoaderScreen=nullptr;
	}

	// game running now!
	IsRunning = true;

	// Start message
	Log(LoadResStr(C4S.Head.NetworkGame ? "IDS_PRC_JOIN" : C4S.Head.SaveGame ? "IDS_PRC_RESUME" : "IDS_PRC_START"));

	// set non-exclusive GUI
	pGUI->SetExclusive(false);

	// after GUI is made non-exclusive, recheck the scoreboard
	Scoreboard.DoDlgShow(0, false);
	SetInitProgress(100);

	// and redraw background
	GraphicsSystem.InvalidateBg();

	// Notify editor
	Console.InitGame();

	return true;
}

void C4Game::SetScenarioFilename(const char * c4sfile)
{
	SCopy(c4sfile,ScenarioFilename,_MAX_PATH);
	if (SEqualNoCase(GetFilename(c4sfile),"scenario.txt"))
	{
		if (GetFilename(ScenarioFilename) != ScenarioFilename) *(GetFilename(ScenarioFilename) - 1) = 0;
	}
}

void C4Game::Clear()
{
	pFileMonitor.reset();
	// fade out music
	Application.MusicSystem.FadeOut(2000);
	// game no longer running
	IsRunning = false;
	PointersDenumerated = false;

	C4ST_SHOWSTAT
	// C4ST_RESET

	// Evaluation
	if (GameOver)
	{
		if (!Evaluated) Evaluate();
	}

	// stop statistics
	pNetworkStatistics.reset();
	C4AulProfiler::Abort();

	// next mission (shoud have been transferred to C4Application now if next mission was desired)
	NextMission.Clear(); NextMissionText.Clear(); NextMissionDesc.Clear();

	// Clear control
	Network.Clear();
	Control.Clear();
	MouseControl.Clear();
	KeyboardInput.Clear();
	PlayerControlUserAssignmentSets.Clear();
	PlayerControlDefaultAssignmentSets.Clear();
	PlayerControlDefs.Clear();

	// Clear game info
	Scoreboard.Clear();
	Players.Clear();
	Parameters.Clear();
	RoundResults.Clear();
	C4S.Clear();
	ScenarioParameterDefs.Clear();
	StartupScenarioParameters.Clear();
	Info.Clear();
	Title.Clear();
	Names.Clear();
	GameText.Clear();
	RecordDumpFile.Clear();
	RecordStream.Clear();

#ifdef WITH_QT_EDITOR
	// clear console pointers held into script engine
	::Console.EditCursor.Clear();
	::Console.ClearGamePointers();
#endif
	// notify editor
	Console.CloseGame();

	// Clear the particles before cleaning up the objects.
	Particles.Clear();
	DeleteObjects(true);

	// exit gui
	pGUI->Clear();
	ScriptGuiRoot.reset();

	// Clear landscape
	Weather.Clear();
	Landscape.Clear();
	PXS.Clear();
	::MaterialMap.Clear();
	TextureMap.Clear(); // texture map *MUST* be cleared after the materials, because of the patterns!
	PathFinder.Clear();
	TransferZones.Clear();

	::Messages.Clear();
	MessageInput.Clear();
	SetGlobalSoundModifier(nullptr); // must be called before script engine clear
	Application.SoundSystem.Modifiers.Clear(); // free some prop list pointers

	// Clear script engine
	::MapScript.Clear();
	::Definitions.Clear();
	::GameScript.Clear();
	C4PropListNumbered::ClearShelve(); // may be nonempty if there was a fatal error during section load
	ScriptEngine.Clear();
	pScenarioObjectsScript = nullptr;
	// delete any remaining prop lists from circular chains
	C4PropListNumbered::ClearNumberedPropLists(); 
	C4PropListScript::ClearScriptPropLists();

	// Clear translation tables
	MainSysLangStringTable.Clear();
	ScenarioLangStringTable.Clear();

	// Cleanup remaining open scenario files
	CloseScenario();
	GroupSet.Clear();
	::Application.MusicSystem.ClearGame();
	Application.SoundSystem.Clear(); // will be re-inited by application pre-init if running from startup system

	// Clear remaining graphics
#ifndef USE_CONSOLE
	::FontLoader.Clear();
#endif
	::MeshMaterialManager.Clear();
	GraphicsSystem.Clear();

	// global fullscreen class is not cleared, because it holds the carrier window
	// but the menu must be cleared (maybe move Fullscreen.Menu somewhere else?)
	FullScreen.CloseMenu();

	// Message
	// avoid double message by not printing it if no restbl is loaded
	// this would log an "[Undefined]" only, anyway
	// (could abort the whole clear-procedure here, btw?)
	if (::Languages.HasStringTable()) Log(LoadResStr("IDS_CNS_GAMECLOSED"));

	// clear game starting parameters
	*DefinitionFilenames = *DirectJoinAddress = *ScenarioFilename = *PlayerFilenames = 0;

	// join reference
	pJoinReference.reset();

	// okay, game cleared now. Remember log section
	QuitLogPos = GetLogPos();

	fPreinited = false;
	C4PropListNumbered::ResetEnumerationIndex();

	// FIXME: remove this
	Default();
}

bool C4Game::GameOverCheck()
{
	bool fDoGameOver = false;

	// Only every 35 ticks
	if (::Game.iTick35) return false;

	// do not GameOver in replay
	if (Control.isReplay()) return false;

	// All players eliminated: game over
	if (!Players.GetCountNotEliminated())
		fDoGameOver=true;

	// Message
	if (fDoGameOver) DoGameOver();

	return GameOver;
}

C4ST_NEW(ControlRcvStat,    "C4Game::Execute ReceiveControl")
C4ST_NEW(ControlStat,       "C4Game::Execute ExecuteControl")
C4ST_NEW(ExecObjectsStat,   "C4Game::Execute ExecObjects")
C4ST_NEW(GEStats,           "C4Game::Execute pGlobalEffects->Execute")
C4ST_NEW(PXSStat,           "C4Game::Execute PXS.Execute")
C4ST_NEW(DynPartStat,       "C4Game::Execute Particles.Execute")
C4ST_NEW(MassMoverStat,     "C4Game::Execute MassMover.Execute")
C4ST_NEW(WeatherStat,       "C4Game::Execute Weather.Execute")
C4ST_NEW(PlayersStat,       "C4Game::Execute Players.Execute")
C4ST_NEW(LandscapeStat,     "C4Game::Execute Landscape.Execute")
C4ST_NEW(MusicSystemStat,   "C4Game::Execute MusicSystem.Execute")
C4ST_NEW(MessagesStat,      "C4Game::Execute Messages.Execute")

#define EXEC_S(Expressions, Stat) \
  { C4ST_START(Stat) Expressions C4ST_STOP(Stat) }

#define EXEC_S_DR(Expressions, Stat, DebugRecName) { if (Config.General.DebugRec) AddDbgRec(RCT_Block, DebugRecName, 6); EXEC_S(Expressions, Stat) }
#define EXEC_DR(Expressions, DebugRecName) { if (Config.General.DebugRec) AddDbgRec(RCT_Block, DebugRecName, 6); Expressions }

bool C4Game::Execute() // Returns true if the game is over
{

	// Let's go
	GameGo = true;

	// Network
	Network.Execute();

	// Prepare control
	bool fControl;
	EXEC_S(     fControl = Control.Prepare();     , ControlStat )
	if (!fControl) return false; // not ready yet: wait

	// Halt
	if (HaltCount) return false;

	if (Config.General.DebugRec)
		Landscape.DoRelights();

	// Execute the control
	Control.Execute();
	if (!IsRunning) return false;

	// Ticks
	EXEC_DR(    Ticks();                                                , "Ticks")

	if (Config.General.DebugRec)
		// debugrec
		AddDbgRec(RCT_DbgFrame, &FrameCounter, sizeof(int32_t));

	// allow the particle system to execute the next frame BEFORE the other game stuff is calculated since it will run in parallel to the main thread
	Particles.CalculateNextStep();

	// Game

	EXEC_S(     ExecObjects();                    , ExecObjectsStat )
	EXEC_S_DR(  C4Effect::Execute(&ScriptEngine.pGlobalEffects);
	            C4Effect::Execute(&GameScript.pScenarioEffects);
	                                              , GEStats             , "GEEx\0");
	EXEC_S_DR(  PXS.Execute();                    , PXSStat             , "PXSEx")
	EXEC_S_DR(  MassMover.Execute();              , MassMoverStat       , "MMvEx")
	EXEC_S_DR(  Weather.Execute();                , WeatherStat         , "WtrEx")
	EXEC_S_DR(  Landscape.Execute();              , LandscapeStat       , "LdsEx")
	EXEC_S_DR(  Players.Execute();                , PlayersStat         , "PlrEx")
	EXEC_S_DR(  ::Messages.Execute();             , MessagesStat        , "MsgEx")

	EXEC_DR(    MouseControl.Execute();                                 , "Input")

	EXEC_DR(    GameOverCheck();                                        , "Misc\0")

	Control.DoSyncCheck();

	// Evaluation; Game over dlg
	if (GameOver)
	{
		if (!Evaluated) Evaluate();
		if (!GameOverDlgShown) ShowGameOverDlg();
	}

	// show stat each 1000 ticks
	if (!(FrameCounter % 1000))
	{
		C4ST_SHOWPARTSTAT(FrameCounter)
		C4ST_RESETPART
	}

	if (Config.General.DebugRec)
	{
		AddDbgRec(RCT_Block, "eGame", 6);
		Landscape.DoRelights();
	}

	return true;
}

void C4Game::InitFullscreenComponents(bool fRunning)
{
	// It can happen that this is called before graphics are loaded due to
	// an early OnResolutionChanged() call. Ignore it, the message board,
	// upper board and viewports will be initialized within the regular
	// startup sequence then.
	if(!GraphicsResource.IsInitialized()) return;

	// fullscreen message board
	C4Facet cgo;
	cgo.Set(FullScreen.pSurface, 0, 0, C4GUI::GetScreenWdt(), C4GUI::GetScreenHgt());
	GraphicsSystem.MessageBoard->Init(cgo, !fRunning);
	if (fRunning)
	{
		// running game: Message board upper board and viewports
		C4Facet cgo2;
		cgo2.Set(FullScreen.pSurface, 0, 0, C4GUI::GetScreenWdt(), C4UpperBoardHeight);
		GraphicsSystem.UpperBoard.Init(cgo2);
		::Viewports.RecalculateViewports();
	}
}

bool C4Game::InitMaterialTexture()
{

	// Clear old data
	TextureMap.Clear();
	::MaterialMap.Clear();

	// Check for scenario local materials
	bool fHaveScenMaterials = Game.ScenarioFile.FindEntry(C4CFN_Material);

	// Load all materials
	C4GameRes *pMatRes = nullptr;
	bool fFirst = true, fOverloadMaterials = true, fOverloadTextures = true;
	long tex_count = 0, mat_count = 0;
	while (fOverloadMaterials || fOverloadTextures)
	{

		// Are there any scenario local materials that need to be looked at firs?
		C4Group Mats;
		if (fHaveScenMaterials)
		{
			if (!Mats.OpenAsChild(&Game.ScenarioFile, C4CFN_Material))
				return false;
			// Once only
			fHaveScenMaterials = false;
		}
		else
		{
			// Find next external material source
			pMatRes = Game.Parameters.GameRes.iterRes(pMatRes, NRT_Material);
			if (!pMatRes) break;
			if (!Reloc.Open(Mats, pMatRes->getFile()))
				return false;
		}

		// Texture loader will access out of order. Pre-cache the small text-files to prevent rewind.
		Mats.PreCacheEntries(C4CFN_TexMap);
		Mats.PreCacheEntries(C4CFN_MaterialFiles, true);

		// First material file? Load texture map.
		bool fNewOverloadMaterials = false, fNewOverloadTextures = false;
		if (fFirst)
		{
			long tme_count = TextureMap.LoadMap(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures);
			LogF(LoadResStr("IDS_PRC_TEXMAPENTRIES"),tme_count);
			// Only once
			fFirst = false;
		}
		else
		{
			// Check overload-flags only
			if (!C4TextureMap::LoadFlags(Mats, C4CFN_TexMap, &fNewOverloadMaterials, &fNewOverloadTextures))
				fOverloadMaterials = fOverloadTextures = false;
		}

		// Load textures
		if (fOverloadTextures)
		{
			int iTexs = TextureMap.LoadTextures(Mats);
			// Automatically continue search if no texture was found
			if (!iTexs) fNewOverloadTextures = true;
			tex_count += iTexs;
		}

		// Load materials
		if (fOverloadMaterials)
		{
			int iMats = ::MaterialMap.Load(Mats);
			// Automatically continue search if no material was found
			if (!iMats) fNewOverloadMaterials = true;
			mat_count += iMats;
		}

		// Set flags
		fOverloadTextures = fNewOverloadTextures;
		fOverloadMaterials = fNewOverloadMaterials;
	}

	// Logs
	LogF(LoadResStr("IDS_PRC_TEXTURES"),tex_count);
	LogF(LoadResStr("IDS_PRC_MATERIALS"),mat_count);

	// Load material enumeration
	if (!::MaterialMap.LoadEnumeration(ScenarioFile))
		{ LogFatal(LoadResStr("IDS_PRC_NOMATENUM"));  return false; }

	// Initialize texture map
	TextureMap.Init();

	// Cross map mats (after texture init, because Material-Texture-combinations are used)
	if (!::MaterialMap.CrossMapMaterials(C4S.Landscape.Material.c_str())) return false;

	// get material script funcs
	::MaterialMap.UpdateScriptPointers();

	return true;
}

void C4Game::ClearObjectPtrs(C4Object *pObj)
{
	// May not call Objects.ClearPointers() because that would
	// remove pObj from primary list and pObj is to be kept
	// until CheckObjectRemoval().
	for (C4Object *cObj : Objects)
	{
		cObj->ClearPointers(pObj);
	}
	// check in inactive objects as well
	for (C4Object *cObj : Objects.InactiveObjects)
	{
		cObj->ClearPointers(pObj);
	}
}

void C4Game::ClearPointers(C4Object * pObj)
{
	::AulExec.ClearPointers(pObj);
	::Objects.ForeObjects.ClearPointers(pObj);
	::Messages.ClearPointers(pObj);
	ClearObjectPtrs(pObj);
	Application.SoundSystem.ClearPointers(pObj);
	::Players.ClearPointers(pObj);
	::Viewports.ClearPointers(pObj);
	::MessageInput.ClearPointers(pObj);
	::Console.ClearPointers(pObj);
	::MouseControl.ClearPointers(pObj);
	ScriptGuiRoot->ClearPointers(pObj);
	TransferZones.ClearPointers(pObj);
	if (::ScriptEngine.pGlobalEffects)
		::ScriptEngine.pGlobalEffects->ClearPointers(pObj);
	if (::GameScript.pScenarioEffects)
		::GameScript.pScenarioEffects->ClearPointers(pObj);
	::Landscape.ClearPointers(pObj);
}

bool C4Game::TogglePause()
{
	// pause toggling disabled during round evaluation
	if (C4GameOverDlg::IsShown()) return false;
	// otherwise, toggle
	if (IsPaused()) return Unpause(); else return Pause();
}

bool C4Game::Pause()
{
	// already paused?
	if (IsPaused()) return false;
	// pause by net?
	if (::Network.isEnabled())
	{
		// league? Vote...
		if (Parameters.isLeague() && !Game.Evaluated)
		{
			::Network.Vote(VT_Pause, true, true);
			return false;
		}
		// host only
		if (!::Network.isHost()) return true;
		::Network.Pause();
	}
	else
	{
		// pause game directly
		Game.HaltCount = true;
	}
	Console.UpdateHaltCtrls(IsPaused());
	return true;
}

bool C4Game::Unpause()
{
	// already paused?
	if (!IsPaused()) return false;
	// pause by net?
	if (::Network.isEnabled())
	{
		// league? Vote...
		if (Parameters.isLeague() && !Game.Evaluated)
		{
			::Network.Vote(VT_Pause, true, false);
			return false;
		}
		// host only
		if (!::Network.isHost()) return true;
		::Network.Start();
	}
	else
	{
		// unpause game directly
		Game.HaltCount = false;
	}
	Console.UpdateHaltCtrls(IsPaused());
	return true;
}

bool C4Game::IsPaused()
{
	// pause state defined either by network or by game halt count
	if (::Network.isEnabled())
		return !::Network.isRunning();
	return !!HaltCount;
}


C4Object* C4Game::NewObject( C4PropList *pDef, C4Object *pCreator,
                             int32_t iOwner, C4ObjectInfo *pInfo,
                             int32_t iX, int32_t iY, int32_t iR,
                             C4Real xdir, C4Real ydir, C4Real rdir,
                             int32_t iCon, int32_t iController, bool grow_from_center)
{
	// Safety
	if (!pDef) return nullptr;
	if (Config.General.DebugRec)
	{
		C4RCCreateObj rc;
		memset(&rc, '\0', sizeof(rc));
		strncpy(rc.id, pDef->GetName(), 32+1);
		rc.oei=C4PropListNumbered::GetEnumerationIndex()+1;
		rc.x=iX; rc.y=iY; rc.ownr=iOwner;
		AddDbgRec(RCT_CrObj, &rc, sizeof(rc));
	}
	// Create object
	C4Object *pObj;
	if (!(pObj=new C4Object)) return nullptr;
	// Initialize object
	pObj->Init( pDef,pCreator,iOwner,pInfo,iX,iY,iR,xdir,ydir,rdir, iController );
	// Add to object list
	if (!Objects.Add(pObj)) { delete pObj; return nullptr; }
	// ---- From now on, object is ready to be used in scripts!
	// Construction callback
	C4AulParSet pars(pCreator);
	pObj->Call(PSF_Construction, &pars);
	// AssignRemoval called? (Con 0)
	if (!pObj->Status) { return nullptr; }
	// Do initial con (grow)
	pObj->DoCon(iCon, grow_from_center);
	// AssignRemoval called? (Con 0)
	if (!pObj->Status) { return nullptr; }
	// Success
	return pObj;
}

void C4Game::DeleteObjects(bool fDeleteInactive)
{
	// del any objects
	::Objects.DeleteObjects(fDeleteInactive);
	// reset resort flag
	fResortAnyObject = false;
}

C4Object* C4Game::CreateObject(C4ID id, C4Object *pCreator, int32_t iOwner,
                               int32_t x, int32_t y, int32_t r, bool grow_from_center,
                               C4Real xdir, C4Real ydir, C4Real rdir, int32_t iController)
{
	C4Def *pDef;
	// Get pDef
	if (!(pDef=C4Id2Def(id))) return nullptr;
	// Create object
	return NewObject(pDef,pCreator,
	                 iOwner,nullptr,
	                 x,y,r,
	                 xdir,ydir,rdir,
	                 FullCon, iController, grow_from_center);
}

C4Object* C4Game::CreateObject(C4PropList * PropList, C4Object *pCreator, int32_t iOwner,
                               int32_t x, int32_t y, int32_t r, bool grow_from_center, 
							   C4Real xdir, C4Real ydir, C4Real rdir, int32_t iController)
{
	// check Definition
	if (!PropList || !PropList->GetDef()) return nullptr;
	// Create object
	return NewObject(PropList,pCreator,
	                 iOwner,nullptr,
	                 x,y,r,
	                 xdir,ydir,rdir,
					 FullCon, iController, grow_from_center);
}

C4Object* C4Game::CreateInfoObject(C4ObjectInfo *cinf, int32_t iOwner,
                                   int32_t tx, int32_t ty)
{
	C4Def *def;
	// Valid check
	if (!cinf) return nullptr;
	// Get def
	if (!(def=C4Id2Def(cinf->id))) return nullptr;
	// Create object
	return NewObject( def,nullptr,
	                  iOwner,cinf,
	                  tx,ty,0,
	                  Fix0,Fix0,Fix0,
	                  FullCon, NO_OWNER, false);
}

C4Object* C4Game::CreateObjectConstruction(C4PropList * PropList,
    C4Object *pCreator,
    int32_t iOwner,
    int32_t iX, int32_t iBY,
    int32_t iCon,
    bool fTerrain)
{
	C4Def *pDef;
	C4Object *pObj;

	// Get def
	if (!PropList) return nullptr;
	if (!(pDef=PropList->GetDef())) return nullptr;

	int32_t dx,dy,dwdt,dhgt;
	dwdt=pDef->Shape.Wdt; dhgt=pDef->Shape.Hgt;
	dx=iX-dwdt/2; dy=iBY-dhgt;

	// Terrain
	if (fTerrain)
	{
		// Clear site background (ignored for ultra-large structures)
		if (dwdt*dhgt<12000)
			Landscape.DigFreeRect(dx,dy,dwdt,dhgt);
		// Raise Terrain
		Landscape.RaiseTerrain(dx,dy+dhgt,dwdt);
	}

	// Create object
	if (!(pObj=NewObject(PropList,
	                     pCreator,
	                     iOwner,nullptr,
	                     iX,iBY,0,
	                     Fix0,Fix0,Fix0,
	                     iCon, pCreator ? pCreator->Controller : NO_OWNER, false))) return nullptr;

	return pObj;
}

// Finds an object (OCF_Exclusive) that blocks a potential construction site in the given rectangle 
C4Object* C4Game::FindConstuctionSiteBlock(int32_t tx, int32_t ty, int32_t wdt, int32_t hgt)
{
	C4Rect rect1,rect2;
	rect1.x=tx; rect1.y=ty; rect1.Wdt=wdt; rect1.Hgt=hgt;
	C4LArea Area(&::Objects.Sectors, tx, ty, wdt, hgt); C4LSector *pSector;
	for (C4ObjectList *pObjs = Area.FirstObjectShapes(&pSector); pSector; pObjs = Area.NextObjectShapes(pObjs, &pSector))
		for (C4Object *cObj : *pObjs)
			if (cObj->Status && !cObj->Contained)
				if (cObj->OCF & OCF_Exclusive)
				{
					rect2=cObj->Shape; rect2.x+=cObj->GetX(); rect2.y+=cObj->GetY();
					if (rect1.Overlap(rect2)) return cObj;
				}
	return nullptr;
}

C4Object* C4Game::FindObject(C4Def * pDef,
                             int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt,
                             DWORD ocf,
                             C4Object *pFindNext)
{

	C4Object *pClosest=nullptr;
	int32_t iClosest = 0,iDistance,iFartherThan=-1;
	C4Object *pFindNextCpy=pFindNext;

	// check the easy case first: no instances at all?
	if (pDef && !pDef->Count) return nullptr;

	// Finding next closest: find closest but further away than last closest
	if (pFindNext && (iWdt==-1) && (iHgt==-1))
	{
		iFartherThan = (pFindNext->GetX()-iX)*(pFindNext->GetX()-iX)+(pFindNext->GetY()-iY)*(pFindNext->GetY()-iY);
		pFindNext = nullptr;
	}

	// Scan all objects
	for (C4Object *cObj : Objects)
	{
		// Not skipping to find next
		if (!pFindNext)
			// Status
			if (cObj->Status)
				// ID
				if (!pDef || (cObj->Def == pDef))
					// OCF (match any specified)
					if (cObj->OCF & ocf)
						// Area
						{
							// Point
							if ((iWdt==0) && (iHgt==0))
							{
								if (Inside<int32_t>(iX-(cObj->GetX()+cObj->Shape.x),0,cObj->Shape.Wdt-1))
									if (Inside<int32_t>(iY-(cObj->GetY()+cObj->Shape.y),0,cObj->Shape.Hgt-1))
										return cObj;
								continue;
							}
							// Closest
							if ((iWdt==-1) && (iHgt==-1))
							{
								iDistance = (cObj->GetX()-iX)*(cObj->GetX()-iX)+(cObj->GetY()-iY)*(cObj->GetY()-iY);
								// same distance?
								if ((iDistance == iFartherThan) && !pFindNextCpy)
									return cObj;
								// nearer than/first closest?
								if (!pClosest || (iDistance < iClosest))
									if (iDistance > iFartherThan)
										{ pClosest=cObj; iClosest=iDistance; }
							}
							// Range
							else if (Inside<int32_t>(cObj->GetX()-iX,0,iWdt-1) && Inside<int32_t>(cObj->GetY()-iY,0,iHgt-1))
								return cObj;
						}

		// Find next mark reached
		if (cObj == pFindNextCpy) pFindNext = pFindNextCpy = nullptr;

	}

	return pClosest;

}

C4Object *C4Game::FindVisObject(float tx, float ty, int32_t iPlr, const C4Facet &fctViewportGame, const C4Facet &fctViewportGUI,
                                float game_x, float game_y, DWORD category, float gui_x, float gui_y)
{
	// FIXME: Use C4FindObject here for optimization
	// -- can't really do that, since sectors ignore parallaxity, etc.
	// determine layer to search in
	C4Object *layer_object = nullptr;
	C4Player *plr = ::Players.Get(iPlr);
	if (plr && plr->Cursor) layer_object = plr->Cursor->Layer;
	// scan all object lists separately
	C4ObjectList *pLst = &::Objects.ForeObjects;
	while (pLst)
	{
		// Scan all objects in list
		for (C4Object *cObj : *pLst)
		{
			// Status
			if (cObj->Status == C4OS_NORMAL)
				// exclude fore-objects from main list
				if ((pLst != &Objects) || (~cObj->Category & C4D_Foreground))
					// exclude MouseIgnore-objects
					if (~cObj->Category & C4D_MouseIgnore)
						// Category (match any specified)
						if (cObj->Category & category)
							// Container
							if (!cObj->Contained)
								// Visibility
								if (cObj->IsVisible(iPlr, false))
									// Layer check: Layered objects are invisible to players whose cursor is in another layer
									// except for GUI: GUI always visible
								{
									if (cObj->Layer != layer_object)
										if (pLst != &::Objects.ForeObjects)
											continue;
									// Area
									// get object position
									float iObjX, iObjY, check_x, check_y;
									if (pLst == &::Objects.ForeObjects)
									{
										// object position for HUD object
										check_x = gui_x; check_y = gui_y;
										cObj->GetViewPos(iObjX, iObjY, -fctViewportGUI.X, -fctViewportGUI.Y, fctViewportGUI);
									}
									else
									{
										// object position for game object
										check_x = game_x; check_y = game_y;
										cObj->GetViewPos(iObjX, iObjY, tx, ty, fctViewportGame);
									}
									// Point search
									if (Inside<float>(check_x-(iObjX+cObj->Shape.x),0,float(cObj->Shape.Wdt)-1))
										if (Inside<float>(check_y-(iObjY+cObj->Shape.y-cObj->addtop()),0,float(cObj->Shape.Hgt+cObj->addtop()-1)))
											return cObj;
								}
		}
		// next list
		if (pLst == &::Objects.ForeObjects) pLst = &Objects;
		else pLst = nullptr;
	}

	// none found
	return nullptr;
}

int32_t C4Game::ObjectCount(C4ID id)
{
	C4Def *pDef;
	// check the easy cases first
	if (id != C4ID::None)
	{
		if (!(pDef=C4Id2Def(id))) return 0; // no valid def
		return pDef->Count;
	}
	int32_t iResult = 0;
	for (C4Object *cObj : Objects)
		// Status
		if (cObj->Status)
			++iResult;
	return iResult;
}

// Deletes removal-assigned data from list.
// Pointer clearance is done by AssignRemoval.

void C4Game::ObjectRemovalCheck() // Every ::Game.iTick255 by ExecObjects
{
	for (C4Object *cObj : Objects)
	{
		if (!cObj->Status && (cObj->RemovalDelay==0))
		{
			Objects.Remove(cObj);
			delete cObj;
		}
	}
}

void C4Game::ExecObjects() // Every Tick1 by Execute
{
	if (Config.General.DebugRec)
		AddDbgRec(RCT_Block, "ObjEx", 6);

	// Execute objects - reverse order to ensure
	for (C4Object *cObj : Objects.reverse())
	{
		if (cObj)
		{
			if (cObj->Status)
				// Execute object
				cObj->Execute();
			else
				// Status reset: process removal delay
				if (cObj->RemovalDelay>0) cObj->RemovalDelay--;
		}
	}

	if (Config.General.DebugRec)
		AddDbgRec(RCT_Block, "ObjCC", 6);

	// Cross check objects
	Objects.CrossCheck();

	if (Config.General.DebugRec)
		AddDbgRec(RCT_Block, "ObjRs", 6);

	// Resort
	if (fResortAnyObject)
	{
		fResortAnyObject = false;
		Objects.ResortUnsorted();
	}

	if (Config.General.DebugRec)
		AddDbgRec(RCT_Block, "ObjRm", 6);

	// Removal
	if (!::Game.iTick255) ObjectRemovalCheck();
}

C4ID DefFileGetID(const char *szFilename)
{
	C4Group hDef;
	C4Def DefCore;
	if (!hDef.Open(szFilename)) return C4ID::None;
	if (!DefCore.LoadDefCore(hDef)) { hDef.Close(); return C4ID::None; }
	hDef.Close();
	return DefCore.id;
}

bool C4Game::DropFile(const char *szFilename, float iX, float iY)
{
	C4ID c_id; C4Def *cdef;
	// Drop def to create object
	if (SEqualNoCase(GetExtension(szFilename),"ocd"))
	{
		// Get id from file
		if ((c_id=DefFileGetID(szFilename)))
			// Get loaded def or try to load def from file
			if ( (cdef=C4Id2Def(c_id))
			     || (::Definitions.Load(szFilename,C4D_Load_RX,Config.General.LanguageEx,&Application.SoundSystem) && (cdef=C4Id2Def(c_id))) )
			{
				return DropDef(c_id, iX, iY);
			}
		// Failure
		Console.Out(FormatString(LoadResStr("IDS_CNS_DROPNODEF"),GetFilename(szFilename)).getData());
		return false;
	}
	return false;
}

bool C4Game::DropDef(C4ID id, float X, float Y)
{
	// Get def
	C4Def *pDef;
	if ((pDef=C4Id2Def(id)))
	{
		::Control.DoInput(CID_EMMoveObj, C4ControlEMMoveObject::CreateObject(id, ftofix(X), ftofix(Y), nullptr), CDT_Decide);
		return true;
	}
	else
	{
		// Failure
		Console.Out(FormatString(LoadResStr("IDS_CNS_DROPNODEF"),id.ToString()).getData());
	}
	return false;
}

void C4Game::CastObjects(C4ID id, C4Object *pCreator, int32_t num, int32_t level, int32_t tx, int32_t ty, int32_t iOwner, int32_t iController, C4ValueArray *out_objects)
{
	int32_t cnt, out_obj_size=0;
	if (out_objects)
	{
		out_obj_size = out_objects->GetSize();
		out_objects->SetSize(out_obj_size + num);
	}
	for (cnt=0; cnt<num; cnt++)
	{
		// Must do these calculation steps separately, because the order of
		// invokations of Random() is not defined if they're used as parameters
		int32_t angle = Random(360);
		C4Real xdir = C4REAL10(Random(2*level+1)-level);
		C4Real ydir = C4REAL10(Random(2*level+1)-level);
		C4Real rdir = itofix(Random(3)+1);
		C4Object *obj = CreateObject(id,pCreator,iOwner,
		             tx,ty,angle,
					 false,
		             xdir,
		             ydir,
		             rdir, iController);
		if (obj && obj->Status && out_objects) (*out_objects)[out_obj_size+cnt] = C4VObj(obj);
	}
}

void C4GameSec1Timer::OnSec1Timer()
{
	// updates the game clock
	if (Game.TimeGo) { Game.Time++; Game.TimeGo = false; }
	Game.FPS=Game.cFPS; Game.cFPS=0;
}

void C4Game::Default()
{
	PointersDenumerated = false;
	IsRunning = false;
	FrameCounter=0;
	GameOver=GameOverDlgShown=InitialPlayersJoined=false;
	ScenarioFilename[0]=0;
	PlayerFilenames[0]=0;
	DefinitionFilenames[0]=0;
	DirectJoinAddress[0]=0;
	pJoinReference=nullptr;
	StartupPlayerCount=0;
	StartupTeamCount = 0;
	ScenarioTitle.Ref("");
	HaltCount=0;
	fReferenceDefinitionOverride=false;
	Evaluated=false;
	TimeGo=false;
	Time=0;
	StartTime=0;
	InitProgress=0; LastInitProgress=0;
	FPS=cFPS=0;
	fScriptCreatedObjects=false;
	fLobby=fObserve=false;
	iLobbyTimeout=0;
	iTick2=iTick3=iTick5=iTick10=iTick35=iTick255=iTick1000=0;
	FullSpeed=false;
	FrameSkip=1; DoSkipFrame=false;
	::Definitions.Default();
	::MaterialMap.Default();
	Objects.Default();
	Players.Default();
	Weather.Default();
	Landscape.Default();
	::DefaultRanks.Default();
	MassMover.Default();
	PXS.Default();
	GraphicsSystem.Default();
	C4S.Default();
	::Messages.Default();
	MessageInput.Default();
	MouseControl.Default();
	PathFinder.Default();
	TransferZones.Default();
	GroupSet.Default();
	pParentGroup=nullptr;
	pScenarioSections=pCurrentScenarioSection=nullptr;
	*CurrentScenarioSection=0;
	fResortAnyObject=false;
	pNetworkStatistics.reset();
	::Application.MusicSystem.ClearGame();
	DebugPort = 0;
	DebugPassword.Clear();
	DebugHost.Clear();
	DebugWait = false;
	assert(!ScriptGuiRoot);
	ScriptGuiRoot.reset();
}

void C4Game::Evaluate()
{

	// League game?
	bool fLeague = Network.isEnabled() && Network.isHost() && Parameters.isLeague();

	// Stop record
	StdStrBuf RecordName; BYTE RecordSHA[SHA_DIGEST_LENGTH];
	if (Control.isRecord())
		Control.StopRecord(&RecordName, fLeague ? RecordSHA : nullptr);

	// Send league result
	if (fLeague)
		Network.LeagueGameEvaluate(RecordName.getData(), RecordSHA);

	// Players
	// saving local players only, because remote players will probably not rejoin after evaluation anyway)
	Players.Evaluate();
	Players.Save(true);

	// Round results
	RoundResults.EvaluateGame();

	// Set game flag
	Log(LoadResStr("IDS_PRC_EVALUATED"));
	Evaluated=true;

}

void C4Game::DrawCrewOverheadText(C4TargetFacet &cgo, int32_t iPlayer)
{
	
	// All drawing in this function must not be affected by zoom; but remember zoom and reset later.
	ZoomData r;
	pDraw->GetZoom(&r);
	const float zoom = r.Zoom;
	r.Zoom = 1.0;
	pDraw->SetZoom(r);
	// Offset for all text/objects
	const float fixedOffsetX = -cgo.X * cgo.Zoom + cgo.X;
	const float fixedOffsetY = (-cgo.Y - 10.0f) * cgo.Zoom + cgo.Y;
	// Draw cursor mark arrow & cursor object name
	C4Facet &fctCursor = GraphicsResource.fctMouseCursor;
	for (C4Player *pPlr = Players.First; pPlr; pPlr = pPlr->Next)
	{
		// Draw a small selector & name above the cursor? F.e. after switching crew.
		const bool drawCursorInfo = (pPlr->Number == iPlayer || iPlayer == NO_OWNER) // only for the viewport's player..
			&& (pPlr->CursorFlash && pPlr->Cursor); // ..and if the player wants to show their cursor.
		// Otherwise, for allied players we might want to draw player & crew names.
		// Note that these two conditions are generally mutually-exclusive.
		const bool drawPlayerAndCursorNames = (pPlr->Number != iPlayer) // Never for own player..
			&& (Config.Graphics.ShowCrewNames || Config.Graphics.ShowCrewCNames) // ..and if the settings allow it..
			&& !Hostile(iPlayer, pPlr->Number) && !pPlr->IsInvisible(); // ..and of course only if applicable.
		
		if (!drawPlayerAndCursorNames && !drawCursorInfo) continue;

		// Lambda to calculate correct drawing position of object, (re-)adjusted by zoom.
		float drawX, drawY, drawZoom;
		auto calculateObjectTextPosition = [&](C4Object *obj)
		{
			obj->GetDrawPosition(cgo, fixtof(obj->fix_x), fixtof(obj->fix_y), 1.0, drawX, drawY, drawZoom);
			drawX = drawX * cgo.Zoom + fixedOffsetX;
			drawY = drawY * cgo.Zoom - static_cast<float>(obj->Def->Shape.Hgt) / 2.0 + fixedOffsetY;
		};

		// Actual text output!
		if (drawPlayerAndCursorNames)
		{
			// We need to show crew names for that player, we do so for every crew-member.
			for (C4Object * const & crew : pPlr->Crew)
			{
				if (!crew->Status || !crew->Def) continue;
				if (crew->Contained) continue;
				if ((crew->OCF & OCF_CrewMember) == 0) continue;
				if (!crew->IsVisible(iPlayer, false)) continue;

				calculateObjectTextPosition(crew);
				drawY -= 5.0f; // aesthetical offset

				// compose string
				char szText[C4GM_MaxText + 1];
				if (Config.Graphics.ShowCrewNames)
				if (Config.Graphics.ShowCrewCNames)
					sprintf(szText, "%s (%s)", crew->GetName(), pPlr->GetName());
				else
					SCopy(pPlr->GetName(), szText);
				else
					SCopy(crew->GetName(), szText);
				// Word wrap to cgo width
				int32_t iCharWdt, dummy;
				::GraphicsResource.FontRegular.GetTextExtent("m", iCharWdt, dummy, false);
				int32_t iMaxLine = std::max<int32_t>(cgo.Wdt / std::max<int32_t>(1, iCharWdt), 20);
				SWordWrap(szText, ' ', '|', iMaxLine);
				// Center text vertically, too
				int textWidth, textHeight;
				::GraphicsResource.FontRegular.GetTextExtent(szText, textWidth, textHeight, true);
				// Draw
				pDraw->TextOut(szText, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, drawX, drawY - static_cast<float>(textHeight) / 2.0,
					pPlr->ColorDw | 0x7f000000, ACenter);
			}
		}
		else if (drawCursorInfo)
		{
			C4Object * const cursor = pPlr->Cursor;
			calculateObjectTextPosition(cursor);
			// Draw a down-arrow above the Clonk's head
			drawY += -fctCursor.Hgt;
			fctCursor.Draw(cgo.Surface, drawX - static_cast<float>(fctCursor.Wdt) / 2.0, drawY, 4);
			// And possibly draw some info text, too
			if (cursor->Info)
			{
				int32_t texthgt = ::GraphicsResource.FontRegular.GetLineHeight();
				StdStrBuf str;
				if (cursor->Info->Rank > 0)
				{
					str.Format("%s|%s", cursor->Info->sRankName.getData(), cursor->GetName());
					texthgt += texthgt;
				}
				else str = cursor->GetName();

				pDraw->TextOut(str.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,
					drawX,
					drawY - static_cast<float>(texthgt) / 2.0,
					0xffff0000, ACenter);

			}
		}
	}
	// Reset zoom
	r.Zoom = zoom;
	pDraw->SetZoom(r);
}

void C4Game::Ticks()
{
	// Frames
	FrameCounter++; GameGo = FullSpeed;
	// Ticks
	if (++iTick2==2)       iTick2=0;
	if (++iTick3==3)       iTick3=0;
	if (++iTick5==5)       iTick5=0;
	if (++iTick10==10)     iTick10=0;
	if (++iTick35==35)     iTick35=0;
	if (++iTick255==255)   iTick255=0;
	if (++iTick1000==1000) iTick1000=0;
	// FPS / time
	cFPS++; TimeGo = true;
	// Frame skip
	if (FrameCounter % FrameSkip) DoSkipFrame = true;
	// Control
	Control.Ticks();
	// Full speed
	if (GameGo) Application.NextTick(); // short-circuit the timer
	// statistics
	if (pNetworkStatistics) pNetworkStatistics->ExecuteFrame();
}

void C4Game::CompileFunc(StdCompiler *pComp, CompileSettings comp, C4ValueNumbers * numbers)
{
	if (comp.init_mode == IM_Normal && comp.fExact)
	{
		pComp->Name("Game");
		pComp->Value(mkNamingAdapt(Time,                  "Time",                  0));
		pComp->Value(mkNamingAdapt(FrameCounter,          "Frame",                 0));
		if (comp.fSync)
		{
			pComp->Value(mkNamingAdapt(Control.ControlTick,   "ControlTick",           0));
			pComp->Value(mkNamingAdapt(Control.SyncRate,      "SyncRate",              C4SyncCheckRate));
		}
		pComp->Value(mkNamingAdapt(iTick2,                "Tick2",                 0));
		pComp->Value(mkNamingAdapt(iTick3,                "Tick3",                 0));
		pComp->Value(mkNamingAdapt(iTick5,                "Tick5",                 0));
		pComp->Value(mkNamingAdapt(iTick10,               "Tick10",                0));
		pComp->Value(mkNamingAdapt(iTick35,               "Tick35",                0));
		pComp->Value(mkNamingAdapt(iTick255,              "Tick255",               0));
		pComp->Value(mkNamingAdapt(iTick1000,             "Tick1000",              0));
		pComp->Value(mkNamingAdapt(InitialPlayersJoined,  "InitialPlayersJoined", false));
		pComp->Value(mkNamingAdapt(StartupPlayerCount,    "StartupPlayerCount",    0));
		pComp->Value(mkNamingAdapt(StartupTeamCount,      "StartupTeamCount",      0));
		pComp->Value(mkNamingAdapt(C4PropListNumbered::EnumerationIndex,"ObjectEnumerationIndex",0));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(CurrentScenarioSection),        "CurrentScenarioSection", ""));
		pComp->Value(mkNamingAdapt(fResortAnyObject,      "ResortAnyObj",          false));
		pComp->Value(mkNamingAdapt(mkParAdapt(GlobalSoundModifier, numbers),   "GlobalSoundModifier", C4Value()));
		pComp->Value(mkNamingAdapt(NextMission,           "NextMission",           StdCopyStrBuf()));
		pComp->Value(mkNamingAdapt(NextMissionText,       "NextMissionText",       StdCopyStrBuf()));
		pComp->Value(mkNamingAdapt(NextMissionDesc,       "NextMissionDesc",       StdCopyStrBuf()));
		pComp->NameEnd();

		// Music settings
		pComp->Value(mkNamingAdapt(::Application.MusicSystem, "Music"));

		// scoreboard compiles into main level [Scoreboard]
		pComp->Value(mkNamingAdapt(Scoreboard, "Scoreboard"));
		// Keyboard status of global keys synchronized for exact (runtime join) only; not for savegames,
		// as keys might be released between a savegame save and its resume
	}

	if (comp.fExact)
	{
		pComp->Value(mkNamingAdapt(Weather, "Weather"));
		pComp->Value(mkNamingAdapt(Landscape, "Landscape"));
		pComp->Value(mkNamingAdapt(Landscape.GetSky(), "Sky"));

		// save custom GUIs only if a real savegame and not for editor-scenario-saves or section changes
		if (comp.init_mode == IM_Normal)
		{
			pComp->Name("GUI");
			if (pComp->isDeserializer())
			{
				C4Value val;
				pComp->Value(mkNamingAdapt(mkParAdapt(val, numbers), "ScriptGUIs", C4VNull));
				// if loading, assume
				assert(ScriptGuiRoot->GetID() == 0); // ID of 0 means "had no subwindows ever" aka "is fresh" for root
				// we will need to denumerate and create the actual GUI post-loading
				// for now, just remember our enumerated ID
				if (val.GetType() == C4V_Enum)
				{
					int enumID = val._getInt();
					ScriptGuiRoot->SetEnumeratedID(enumID);
				}
			}
			else
			{
				C4Value *val = new C4Value(ScriptGuiRoot->ToC4Value());
				pComp->Value(mkNamingAdapt(mkParAdapt(*val, numbers), "ScriptGUIs", C4VNull));
			}
			pComp->NameEnd();
		}
	}

	if (comp.fPlayers)
	{
		assert(pComp->isSerializer());
		// player parsing: Parse all players
		// This doesn't create any players, but just parses existing by their ID
		// Primary player ininitialization (also setting ID) is done by player info list
		// Won't work this way for binary mode!
		for (C4Player *pPlr=Players.First; pPlr; pPlr=pPlr->Next)
			pComp->Value(mkNamingAdapt(mkParAdapt(*pPlr, numbers), FormatString("Player%d", pPlr->ID).getData()));
	}

	// Section load: Clear existing prop list numbering to make room for the new objects
	// Numbers will be re-acquired in C4GameObjects::PostLoad
	if (comp.init_mode == IM_Section) C4PropListNumbered::ShelveNumberedPropLists();

	pComp->Value(mkParAdapt(Objects, !comp.fExact, numbers));

	pComp->Value(mkNamingAdapt(mkParAdapt(ScriptEngine, comp.init_mode == IM_Section, numbers), "Script"));
}

bool C4Game::CompileRuntimeData(C4Group &hGroup, InitMode init_mode, bool exact, bool sync, C4ValueNumbers * numbers)
{
	::Objects.Clear(init_mode != IM_Section);
	GameText.Load(hGroup,C4CFN_Game);
	CompileSettings Settings(init_mode, false, exact, sync);
	// C4Game is not defaulted on compilation.
	// Loading of runtime data overrides only certain values.
	// Doesn't compile players; those will be done later
	if (GameText.GetData())
	{
		if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(
		    mkParAdapt(*this, Settings, numbers),
		    GameText.GetDataBuf(), C4CFN_Game))
			return false;
		// Objects
		int32_t iObjects = Objects.ObjectCount();
		if (iObjects) { LogF(LoadResStr("IDS_PRC_OBJECTSLOADED"),iObjects); }
	}
	// Success
	return true;
}

bool C4Game::SaveData(C4Group &hGroup, bool fSaveSection, bool fSaveExact, bool fSaveSync, C4ValueNumbers * numbers)
{
	if (fSaveExact)
	{
		StdStrBuf Buf;
		// Decompile (without players for scenario sections)
		DecompileToBuf_Log<StdCompilerINIWrite>(mkParAdapt(*this, CompileSettings(fSaveSection ? IM_Section : IM_Normal, !fSaveSection && fSaveExact, fSaveExact, fSaveSync), numbers), &Buf, "Game");

		// Empty? All default save a Game.txt anyway because it is used to signal the engine to not load Objects.c
		if (!Buf.getLength()) Buf.Copy(" ");

		// Save
		return hGroup.Add(C4CFN_Game,Buf,false,true);
	}
	else
	{
		// Clear any exact game data in case scenario is saved from savegame resume
		hGroup.Delete(C4CFN_Game);

		// Save objects to file using system scripts
		int32_t objects_file_handle = ::ScriptEngine.CreateUserFile();
		C4AulParSet pars(objects_file_handle);
		C4Value result_c4v(::ScriptEngine.GetPropList()->Call(PSF_SaveScenarioObjects, &pars));
		bool result = !!result_c4v;
		if (result_c4v.GetType() == C4V_Nil)
		{
			// Function returned nil: This usually means there was a script error during object writing.
			// It could also mean the scripter overloaded global func SaveScenarioObjects and returned nil.
			// In either case, objects will not match landscape any more, so better fail and don't save at all.
			LogF("ERROR: No valid result from global func " PSF_SaveScenarioObjects ". Saving objects failed.");
		}
		else
		{
			// Function completed successfully (returning true or false)
			C4AulUserFile *file = ::ScriptEngine.GetUserFile(objects_file_handle);
			if (!result || !file || !file->GetFileLength())
			{
				// Nothing written? Then we don't have objects.
				hGroup.Delete(C4CFN_ScenarioObjectsScript);
				// That's OK; not an error.
				result = true;
			}
			else
			{
				// Write objects script to file!
				StdStrBuf data = file->GrabFileContents();
				result = hGroup.Add(C4CFN_ScenarioObjectsScript, data, false, true);
			}
		}
		::ScriptEngine.CloseUserFile(objects_file_handle);
		return result;
	}
}

bool C4Game::SaveGameTitle(C4Group &hGroup)
{

	// Game not running
	if (!FrameCounter)
	{
		char* bpBytes;
		size_t iSize;
		StdStrBuf realFilename;

		if(ScenarioFile.FindEntry(FormatString("%s.*",C4CFN_ScenarioTitle).getData(),&realFilename,&iSize))
			if (ScenarioFile.LoadEntry(realFilename.getData(),&bpBytes,&iSize))
				hGroup.Add(realFilename.getData(),bpBytes,iSize,false,true);
	}

	// Fullscreen screenshot
	else if (!Application.isEditor && Application.Active)
	{
		C4Surface * sfcPic; int32_t iSfcWdt=200,iSfcHgt=150;
		if (!(sfcPic = new C4Surface(iSfcWdt,iSfcHgt,0))) return false;

		// Fullscreen
		pDraw->Blit(FullScreen.pSurface,
		                        0.0f,0.0f,float(C4GUI::GetScreenWdt()),float(C4GUI::GetScreenHgt()-::GraphicsResource.FontRegular.GetLineHeight()),
		                        sfcPic,0,0,iSfcWdt,iSfcHgt);

		bool fOkay=true;
		fOkay = sfcPic->SavePNG(Config.AtTempPath(C4CFN_TempTitle), false, false, false);
		StdStrBuf destFilename = FormatString("%s.png",C4CFN_ScenarioTitle);
		delete sfcPic; if (!fOkay) return false;
		if (!hGroup.Move(Config.AtTempPath(C4CFN_TempTitle),destFilename.getData())) return false;
	}

	return true;
}

bool C4Game::DoKeyboardInput(C4KeyCode vk_code, C4KeyEventType eEventType, bool fAlt, bool fCtrl, bool fShift, bool fRepeated, class C4GUI::Dialog *pForDialog, bool fPlrCtrlOnly, int32_t iStrength)
{
	// compose key
	C4KeyCodeEx Key(vk_code, C4KeyShiftState(fAlt*KEYS_Alt + fCtrl*KEYS_Control + fShift*KEYS_Shift), fRepeated);
	return DoKeyboardInput(Key, eEventType, pForDialog, fPlrCtrlOnly, iStrength);
}


bool C4Game::DoKeyboardInput(C4KeyCodeEx Key, C4KeyEventType eEventType, class C4GUI::Dialog *pForDialog, bool fPlrCtrlOnly, int32_t iStrength)
{
	Key.FixShiftKeys();
	// compose keyboard scope
	DWORD InScope = 0;
	if (fPlrCtrlOnly)
		InScope = KEYSCOPE_Control;
	else
	{
		if (IsRunning) InScope = KEYSCOPE_Generic;
		// if GUI has keyfocus, this overrides regular controls
		if (pGUI->HasKeyboardFocus() || pForDialog)
		{
			InScope |= KEYSCOPE_Gui;
			// control to console mode dialog: Make current keyboard target the active dlg,
			//  so it can process input
			if (pForDialog) pGUI->ActivateDialog(pForDialog);
			// any keystroke in GUI resets tooltip times
			pGUI->KeyAny();
		}
		else
		{
			if (!Application.isEditor)
			{
				if (FullScreen.pMenu && FullScreen.pMenu->IsActive()) // fullscreen menu
					InScope |= KEYSCOPE_FullSMenu;
				else if (Game.C4S.Head.Replay && C4S.Head.Film) // film view only
					InScope |= KEYSCOPE_FilmView;
				else if (::Viewports.GetViewport(NO_OWNER)) // NO_OWNER-viewport-controls
					InScope |= KEYSCOPE_FreeView;
				else
				{
					// regular player viewport controls
					InScope |= KEYSCOPE_FullSView;
					// player controls disabled during round over dlg
					if (!C4GameOverDlg::IsShown()) InScope |= KEYSCOPE_Control;
				}
			}
			else
				// regular player viewport controls
				InScope |= KEYSCOPE_Control;
		}
		// fullscreen/console (in running game)
		if (IsRunning)
		{
			if (FullScreen.Active) InScope |= KEYSCOPE_Fullscreen;
			if (Console.Active) InScope |= KEYSCOPE_Console;
		}
	}
	// okay; do input
	if (KeyboardInput.DoInput(Key, eEventType, InScope, iStrength))
		return true;

	// unprocessed key
	return false;
}

bool C4Game::CanQuickSave()
{
	// Network hosts only
	if (Network.isEnabled() && !Network.isHost())
		{ Log(LoadResStr("IDS_GAME_NOCLIENTSAVE")); return false; }

	return true;
}

bool C4Game::QuickSave(const char *strFilename, const char *strTitle, bool fForceSave)
{
	// Check
	if (!fForceSave) if (!CanQuickSave()) return false;

	// Create savegame folder
	if (!Config.General.CreateSaveFolder(Config.AtUserDataPath(C4CFN_Savegames), LoadResStr("IDS_GAME_SAVEGAMESTITLE")))
		{ Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); return false; }

	// Create savegame subfolder(s)
	char strSaveFolder[_MAX_PATH + 1];
	for (uint32_t i = 0; i < SCharCount(DirectorySeparator, strFilename); i++)
	{
		SCopy(Config.AtUserDataPath(C4CFN_Savegames), strSaveFolder); AppendBackslash(strSaveFolder);
		SCopyUntil(strFilename, strSaveFolder + SLen(strSaveFolder), DirectorySeparator, _MAX_PATH, i);
		if (!Config.General.CreateSaveFolder(strSaveFolder, strTitle))
			{ Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); return false; }
	}

	// Compose savegame filename
	StdStrBuf strSavePath;
	strSavePath.Format("%s%c%s", Config.AtUserDataPath(C4CFN_Savegames), DirectorySeparator, strFilename);

	// Must not be the scenario file that is currently open
	if (ItemIdentical(ScenarioFilename, strSavePath.getData()))
	{
		StartSoundEffect("UI::Error");
		::GraphicsSystem.FlashMessage(LoadResStr("IDS_GAME_NOSAVEONCURR"));
		Log(LoadResStr("IDS_GAME_FAILSAVEGAME"));
		return false;
	}

	// Wait message
	Log(LoadResStr("IDS_HOLD_SAVINGGAME"));
	GraphicsSystem.MessageBoard->EnsureLastMessage();

	// Save to target scenario file
	C4GameSave *pGameSave;
	pGameSave = new C4GameSaveSavegame();
	if (!pGameSave->Save(strSavePath.getData()))
		{ Log(LoadResStr("IDS_GAME_FAILSAVEGAME")); delete pGameSave; return false; }
	delete pGameSave;

	// Success
	Log(LoadResStr("IDS_CNS_GAMESAVED"));
	return true;
}

bool LandscapeFree(int32_t x, int32_t y)
{
	if (!Inside<int32_t>(x,0,::Landscape.GetWidth()-1) || !Inside<int32_t>(y,0,::Landscape.GetHeight()-1)) return false;
	return !DensitySolid(GBackDensity(x,y));
}

static void FileMonitorCallback(const char * file, const char * extrafile)
{
	Game.ReloadFile(file);
}

bool C4Game::ReloadFile(const char *szFile)
{
	// not in network
	if (::Network.isEnabled()) return false;
	const char *szRelativePath = Config.AtRelativePath(szFile);
	// a definition? or part of a definition?
	C4Def *pDef;
	if ((pDef = ::Definitions.GetByPath(szRelativePath)))
		return ReloadDef(pDef->id);
	// script?
	if (ScriptEngine.ReloadScript(szRelativePath, Config.General.LanguageEx))
	{
		ReLinkScriptEngine();
	}
	return true;
}

bool C4Game::ReloadDef(C4ID id)
{
	bool fSucc;
	// not in network
	if (::Network.isEnabled()) return false;
	// syncronize (close menus with dead surfaces, etc.)
	// no need to sync back player files, though
	Synchronize(false);
	// SolidMasks might be updated
	C4SolidMask::RemoveSolidMasks();
	// reload def
	C4Def *pDef = ::Definitions.ID2Def(id);
	if (!pDef) return false;
	// Open Graphics.ocg -- we might need to fetch some shader (slices)
	// from there when reloading mesh materials.
	if (!::GraphicsResource.RegisterGlobalGraphics()) return false;
	if (!::GraphicsResource.RegisterMainGroups()) return false;
	// Message
	LogF("Reloading %s from %s",pDef->id.ToString(),GetFilename(pDef->Filename));
	// Reload def
	if (::Definitions.Reload(pDef,C4D_Load_RX,Config.General.LanguageEx,&Application.SoundSystem))
	{
		// update script engine - this will also do include callbacks and Freeze() pDef
		ReLinkScriptEngine();
		// Success, update all concerned object faces
		// may have been done by graphics-update already - but not for objects using graphics of another def
		// better update everything :)
		for (C4Object *obj : Objects)
		{
			if (obj->id == id)
				obj->UpdateFace(true);
		}
		fSucc = true;
	}
	else
	{
		// Failure, remove all objects of this type
		for (C4Object *obj : Objects)
			if (obj->id == id)
				obj->AssignRemoval();
		// safety: If a removed def is being profiled, profiling must stop
		C4AulProfiler::Abort();
		// Kill def
		::Definitions.Remove(pDef);
		// Log
		Log("Reloading failure. All objects of this type removed.");
		fSucc = false;
	}
	// close Graphics.ocg again
	::GraphicsResource.CloseFiles();
	// update game messages
	::Messages.UpdateDef(id);
	// re-put removed SolidMasks
	C4SolidMask::PutSolidMasks();
	// done
	return fSucc;
}

bool C4Game::ReloadParticle(const char *szName)
{
	// not in network
	if (::Network.isEnabled()) return false;
	// safety
	if (!szName) return false;
	// get particle def
	C4ParticleDef *pDef = Particles.definitions.GetDef(szName);
	if (!pDef) return false;
	// verbose
	LogF("Reloading particle %s from %s",pDef->Name.getData(),GetFilename(pDef->Filename.getData()));
	// reload it
	if (!pDef->Reload())
	{
		// safer: remove all particles
		::Particles.ClearAllParticles();
		// clear def
		delete pDef;
		// log
		LogF("Reloading failure. All particles removed.");
		// failure
		return false;
	}
	// success
	return true;
}

bool C4Game::InitGame(C4Group &hGroup, InitMode init_mode, bool fLoadSky, C4ValueNumbers * numbers)
{
	// Activate debugger if requested
	// needs to happen before any scripts are compiled to bytecode so AB_DEBUG chunks will be inserted
	if (DebugPort)
	{
		if (Parameters.isLeague())
			Log("Debugger disabled. Not allowed in league.");
		else
			if (!::C4AulDebug::InitDebug(DebugPassword.getData(), DebugHost.getData()))
				return false;
	}

	if (init_mode == IM_Normal)
	{

		// file monitor
		if (Config.Developer.AutoFileReload && Application.isEditor && !pFileMonitor)
			pFileMonitor = std::make_unique<C4FileMonitor>(FileMonitorCallback);

		// system scripts
		if (!InitScriptEngine())
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }
		SetInitProgress(8);

		// Scenario components
		if (!LoadScenarioComponents())
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }
		SetInitProgress(9);

		// join local players for regular games
		// should be done before record/replay is initialized, so the players are stored in PlayerInfos.txt
		// for local savegame resumes, players are joined into PlayerInfos and later associated in InitPlayers
		if (!::Network.isEnabled())
			if (!PlayerInfos.InitLocal())
				{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }

		// for replays, make sure teams are assigned correctly
		if (C4S.Head.Replay)
		{
			PlayerInfos.RecheckAutoGeneratedTeams(); // checks that all teams used in playerinfos exist
			Teams.RecheckPlayers();                  // syncs player list of teams with teams set in PlayerInfos
		}

		// set up control (inits Record/Replay)
		if (!InitControl()) return false;

		// Graphics and fonts (may reinit main font, too)
		// redundant call in NETWORK2; but it may do scenario local overloads
		Log(LoadResStr("IDS_PRC_GFXRES"));
		if (!GraphicsResource.Init())
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }
		SetInitProgress(25);

		// Definitions
		if (!InitDefs()) return false;
		SetInitProgress(55);

		// Scenario scripts (and local system.ocg)
		::GameScript.Load(ScenarioFile, C4CFN_Script, Config.General.LanguageEx, &ScenarioLangStringTable);
		// Map scripts
		::MapScript.Load(ScenarioFile, C4CFN_MapScript, Config.General.LanguageEx, &ScenarioLangStringTable);
		// Scenario objects
		pScenarioObjectsScript->Load(ScenarioFile, C4CFN_ScenarioObjectsScript, Config.General.LanguageEx, &ScenarioLangStringTable);
		// After defs to get overloading priority
		if (!LoadAdditionalSystemGroup(ScenarioFile))
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }
		SetInitProgress(57);

		// Final init for loaded player commands. Before linking scripts, so CON_* constants are registered
		PlayerControlDefs.FinalInit();

		// Register constants for scenario options
		ScenarioParameterDefs.RegisterScriptConstants(Parameters.ScenarioParameters);

		// Now that all controls and assignments are known, resolve user overloads on control assignments
		if (!InitPlayerControlUserSettings()) return false;
		// Sort assignments by priority. Should be done last, because the user should not see this order in the control config dialog
		PlayerControlUserAssignmentSets.SortAssignments();
		// (Theoretically, PlayerControlDefaultAssignmentSets could be cleared now. However, the amount of memory used is negligible)

		// Link scripts
		if (!LinkScriptEngine()) return false;
		SetInitProgress(58);

		// Materials
		if (!InitMaterialTexture())
			{ LogFatal(LoadResStr("IDS_PRC_MATERROR")); return false; }
		SetInitProgress(60);

		// prepare script menus
		assert(!ScriptGuiRoot);
		ScriptGuiRoot = std::make_unique<C4ScriptGuiWindow>();
	}
	else if (fLoadSky)
	{
		// Sky needs graphics loaded, for shaders
		if (!GraphicsResource.Init())
			{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }
	}

	// Load section sounds
	Application.SoundSystem.LoadEffects(hGroup, nullptr, true);

	// determine startup player and team count, which may be used for initial map generation
	if (!FrameCounter)
	{
		StartupPlayerCount = PlayerInfos.GetStartupCount();
		StartupTeamCount = Teams.GetStartupTeamCount(StartupPlayerCount);
	}

	// The Landscape is the last long chunk of loading time, so it's a good place to start the music fadeout
	if (init_mode == IM_Normal) Application.MusicSystem.FadeOut(2000);
	// Landscape
	Log(LoadResStr("IDS_PRC_LANDSCAPE"));
	bool fLandscapeLoaded = false;
	if (!Landscape.Init(hGroup, init_mode != IM_Normal, fLoadSky, fLandscapeLoaded, !!C4S.Head.SaveGame))
		{ LogFatal(LoadResStr("IDS_ERR_GBACK")); return false; }
	SetInitProgress(88);
	// the savegame flag is set if runtime data is present, in which case this is to be used
	// except for scenario sections
	if (fLandscapeLoaded && (!C4S.Head.SaveGame || init_mode == IM_Section))
		Landscape.ScenarioInit();
	// clear old landscape data
	if (init_mode != IM_Normal && fLandscapeLoaded) { PXS.Clear(); MassMover.Clear(); }
	SetInitProgress(89);
	// Init main object list
	Objects.Init(Landscape.GetWidth(), Landscape.GetHeight());

	// Pathfinder
	if (init_mode == IM_Normal) PathFinder.Init( &LandscapeFree, &TransferZones );
	SetInitProgress(90);

	// PXS
	if (hGroup.FindEntry(C4CFN_PXS))
		if (!PXS.Load(hGroup))
			{ LogFatal(LoadResStr("IDS_ERR_PXS")); return false; }
	SetInitProgress(91);

	// MassMover
	if (hGroup.FindEntry(C4CFN_MassMover))
		if (!MassMover.Load(hGroup))
			{ LogFatal(LoadResStr("IDS_ERR_MOVER")); return false; }
	SetInitProgress(92);

	// definition value overloads
	// TODO: Remove this function? We could move value to script and allow it through regular overloads
	if (init_mode == IM_Normal) InitValueOverloads();

	// runtime data
	if (!CompileRuntimeData(hGroup, init_mode, C4S.Head.SaveGame, C4S.Head.NetworkGame, numbers))
		{ LogFatal(LoadResStr("IDS_PRC_FAIL")); return false; }

	SetInitProgress(93);

	// Load round results
	if (init_mode == IM_Normal)
	{
		if (hGroup.FindEntry(C4CFN_RoundResults))
		{
			if (!RoundResults.Load(hGroup, C4CFN_RoundResults))
				{ LogFatal(LoadResStr("IDS_ERR_ERRORLOADINGROUNDRESULTS")); return false; }
		}
		else
		{
			RoundResults.Init();
		}
	}

	// Denumerate game data pointers
	if (init_mode == IM_Normal) ScriptEngine.Denumerate(numbers);
	if (init_mode == IM_Normal) GlobalSoundModifier.Denumerate(numbers);
	numbers->Denumerate();
	if (init_mode == IM_Normal) ScriptGuiRoot->Denumerate(numbers);
	// Object.PostLoad must happen after number->Denumerate(), becuase UpdateFace() will access Action proplist,
	// which might have a non-denumerated prototype otherwise
	Objects.PostLoad(init_mode == IM_Section, numbers);

	// Check object enumeration
	if (!CheckObjectEnumeration()) return false;

	// Okay; everything in denumerated state from now on
	PointersDenumerated = true;

	// scenario objects script
	if (!GameText.GetData() && pScenarioObjectsScript && pScenarioObjectsScript->GetPropList())
		pScenarioObjectsScript->GetPropList()->Call(PSF_InitializeObjects);

	// Environment
	if (!C4S.Head.NoInitialize && fLandscapeLoaded)
	{
		Log(LoadResStr("IDS_PRC_ENVIRONMENT"));
		InitVegetation();
		InitInEarth();
		InitAnimals();
		InitEnvironment();
		InitRules();
		InitGoals();
		Landscape.PostInitMap();
	}
	SetInitProgress(94);

	// Weather
	if (fLandscapeLoaded) Weather.Init(!C4S.Head.SaveGame);
	SetInitProgress(96);

	// close any gfx groups, because they are no longer needed (after sky is initialized)
	GraphicsResource.CloseFiles();

	if (init_mode == IM_Normal) // reload doesn't affect the music (takes too long)
	{
		// Music
		::Application.MusicSystem.InitForScenario(ScenarioFile);
		::Application.MusicSystem.UpdateVolume();
		if (::Config.Sound.RXMusic)
		{
			// Play something that is not Frontend.mid
			::Application.MusicSystem.Play();
		}
		else
			::Application.MusicSystem.Stop();
		SetInitProgress(97);
	}

	return true;
}

bool C4Game::InitGameFinal(InitMode init_mode)
{
	// Validate object owners & assign loaded info objects
	Objects.ValidateOwners();
	Objects.AssignInfo();
	Objects.AssignLightRange(); // update FoW-repellers

	// Ambience init (before scenario construction, so the scenario can easily modify ambience in Initialize)
	if (!C4S.Head.SaveGame) ::GameScript.Call(PSF_InitializeAmbience);

	// Script constructor call
	int32_t iObjCount = Objects.ObjectCount();
	if (!C4S.Head.SaveGame) ::GameScript.Call(PSF_Initialize);
	if (Objects.ObjectCount()!=iObjCount) fScriptCreatedObjects=true;

	// Player final init
	C4Player *pPlr;
	for (pPlr = Players.First; pPlr; pPlr = pPlr->Next)
	{
		if (init_mode == IM_ReInit) pPlr->ScenarioInit();
		pPlr->FinalInit(!C4S.Head.SaveGame);
	}

	// Create viewports
	if (init_mode == IM_Normal)
	{
		for (pPlr = Players.First; pPlr; pPlr = pPlr->Next)
			if (pPlr->LocalControl)
				::Viewports.CreateViewport(pPlr->Number);
		// Check fullscreen viewports
		FullScreen.ViewportCheck();
		// update halt state
		Console.UpdateHaltCtrls(!!HaltCount);

		// Host: players without connected clients: remove via control queue
		if (Network.isEnabled() && Network.isHost())
			for (int32_t cnt = 0; cnt < Players.GetCount(); cnt++)
				if (Players.GetByIndex(cnt)->AtClient < 0)
					Players.Remove(Players.GetByIndex(cnt), true, false);

		// It should be safe now to reload stuff
		if (pFileMonitor) pFileMonitor->StartMonitoring();
	}
	return true;
}

bool C4Game::InitScriptEngine()
{
	// engine functions
	InitCoreFunctionMap(&ScriptEngine);
	InitObjectFunctionMap(&ScriptEngine);
	InitGameFunctionMap(&ScriptEngine);
	::MapScript.InitFunctionMap(&ScriptEngine);

	// system functions: check if system group is open
	if (!Application.OpenSystemGroup())
		{ LogFatal(LoadResStr("IDS_ERR_INVALIDSYSGRP")); return false; }
	C4Group &File = Application.SystemGroup;

	// get scripts
	char fn[_MAX_FNAME+1] = { 0 };
	File.ResetSearch();
	while (File.FindNextEntry(C4CFN_ScriptFiles, fn, nullptr, !!fn[0]))
	{
		// host will be destroyed by script engine, so drop the references
		C4ScriptHost *scr = new C4ExtraScriptHost();
		scr->Reg2List(&ScriptEngine);
		scr->Load(File, fn, Config.General.LanguageEx, &MainSysLangStringTable);
	}

	// if it's a physical group: watch out for changes
	if (!File.IsPacked() && Game.pFileMonitor)
		Game.pFileMonitor->AddDirectory(File.GetFullName().getData());

	// Prepare host for Objects.c script
	pScenarioObjectsScript = new C4ScenarioObjectsScriptHost();
	pScenarioObjectsScript->Reg2List(&::ScriptEngine);
	C4Value scen_obj_script_val;
	scen_obj_script_val.SetPropList(pScenarioObjectsScript->GetPropList());
	::ScriptEngine.RegisterGlobalConstant("ScenarioObjects", scen_obj_script_val);

	// load standard clonk names
	Names.Load(File, C4CFN_Names);

	return true;
}

bool C4Game::LinkScriptEngine()
{
	// Link script engine (resolve includes/appends, generate code)
	ScriptEngine.Link(&::Definitions);

	// display errors
	LogF("C4AulScriptEngine linked - %d line%s, %d warning%s, %d error%s",
		ScriptEngine.lineCnt, (ScriptEngine.lineCnt != 1 ? "s" : ""),
		ScriptEngine.warnCnt, (ScriptEngine.warnCnt != 1 ? "s" : ""),
		ScriptEngine.errCnt, (ScriptEngine.errCnt != 1 ? "s" : ""));

	// update material pointers
	::MaterialMap.UpdateScriptPointers();
	
	if (C4AulDebug *pDebug = C4AulDebug::GetDebugger())
		if (!pDebug->Listen(DebugPort, !!DebugWait))
			return false;

	return true;
}

bool C4Game::ReLinkScriptEngine()
{
	::ScriptEngine.ReLink(&::Definitions);

	// update effect pointers
	::Objects.UpdateScriptPointers();

	// update material pointers
	::MaterialMap.UpdateScriptPointers();

	return true;
}

bool C4Game::InitPlayers(C4ValueNumbers * numbers)
{
	int32_t iPlrCnt = 0;

	if (C4S.Head.NetworkRuntimeJoin)
	{
		// Load players to restore from scenario
		C4PlayerInfoList LocalRestorePlayerInfos;
		LocalRestorePlayerInfos.Load(ScenarioFile, C4CFN_SavePlayerInfos, &ScenarioLangStringTable);
		// -- runtime join player restore
		// all restore functions will be executed on RestorePlayerInfos, because the main playerinfos may be more up-to-date
		// extract all players to temp store and update filenames to point there
		if (!LocalRestorePlayerInfos.RecreatePlayerFiles())
			{ LogFatal(LoadResStr("IDS_ERR_NOPLRFILERECR")); return false; }
		// recreate the files
		if (!LocalRestorePlayerInfos.RecreatePlayers(numbers))
			{ LogFatal(LoadResStr("IDS_ERR_NOPLRNETRECR")); return false; }
	}
	else if (RestorePlayerInfos.GetActivePlayerCount(true))
	{
		// -- savegame player restore
		// for savegames or regular scenarios with restore infos, the player info list should have been loaded from the savegame
		// or got restored from game text in OpenScenario()
		// merge restore player info into main player info list now
		// -for single-host games, this will move all infos
		// -for network games, it will merge according to savegame association done in the lobby
		// for all savegames, script players get restored by adding one new script player for earch savegame script player to the host
		if (!PlayerInfos.RestoreSavegameInfos(RestorePlayerInfos))
			{ LogFatal(LoadResStr("IDS_ERR_NOPLRSAVEINFORECR")); return false; }
		RestorePlayerInfos.Clear();
		// try to associate local filenames (non-net+replay) or resources (net) with all player infos
		if (!PlayerInfos.RecreatePlayerFiles())
			{ LogFatal(LoadResStr("IDS_ERR_NOPLRFILERECR")); return false; }
		// recreate players by joining all players whose joined-flag is already set
		if (!PlayerInfos.RecreatePlayers(numbers))
			{ LogFatal(LoadResStr("IDS_ERR_NOPLRSAVERECR")); return false; }
	}

	// any regular non-net non-replay game: Do the normal control queue join
	// this includes additional player joins in savegames
	if (!Network.isEnabled() && !Control.NoInput())
		if (!PlayerInfos.LocalJoinUnjoinedPlayersInQueue())
		{
			// error joining local players - either join was done earlier somehow,
			// or the player count check will soon end this round
		}

	// non-replay player joins will be done by player info list when go tick is reached
	// this is handled by C4Network2Players and needs no further treatment here
	// set iPlrCnt for player count check in host/single games
	iPlrCnt = PlayerInfos.GetJoinIssuedPlayerCount();

	// Check valid participating player numbers (host/single only)
	if (!Network.isEnabled() || (Network.isHost() && !fLobby))
	{
#ifndef USE_CONSOLE
		// No players in fullscreen
		if (iPlrCnt==0)
			if (!Application.isEditor && !Control.NoInput())
			{
				LogFatal(LoadResStr("IDS_CNS_NOFULLSCREENPLRS")); return false;
			}
#endif
		// Too many players
		if (iPlrCnt>Game.Parameters.MaxPlayers)
		{
			if (!Application.isEditor)
			{
				LogFatal(FormatString(LoadResStr("IDS_PRC_TOOMANYPLRS"),Game.Parameters.MaxPlayers).getData());
				return false;
			}
			else
			{
				Console.Message(FormatString(LoadResStr("IDS_PRC_TOOMANYPLRS"),Game.Parameters.MaxPlayers).getData());
			}
		}
	}
	// Console and no real players: halt
	if (Console.Active)
		if (!fLobby)
			if (!(PlayerInfos.GetActivePlayerCount(false) - PlayerInfos.GetActiveScriptPlayerCount(true, false)))
				++HaltCount;
	return true;
}

bool C4Game::InitControl()
{
	// update random seed
	if (C4S.Head.NetworkGame || C4S.Head.Replay)
	{
		RandomSeed = C4S.Head.RandomSeed;
	}
	// Randomize
	FixRandom(RandomSeed);

	// Replay?
	if (C4S.Head.Replay)
	{
		// no joins
		PlayerFilenames[0]=0;
		// start playback
		if (!Control.InitReplay(ScenarioFile))
			return false;
		// no record!
		Record = false;
	}
	else if (Network.isEnabled())
	{
		// initialize
		if (!Control.InitNetwork(Clients.getLocal()))
			return false;
		// league? always record
		if (Parameters.isLeague())
			Record = true;
	}
	// Otherwise: local game
	else
	{
		// init
		if (!Control.InitLocal(Clients.getLocal()))
			return false;
	}

	// record?
	if (Record)
		if (!Control.StartRecord(true, Parameters.doStreaming()))
		{
			// Special: If this happens for a league host, the game must not start.
			if (Network.isEnabled() && Network.isHost() && Parameters.isLeague())
			{
				LogFatal(LoadResStr("IDS_ERR_NORECORD"));
				return false;
			}
			else
			{
				Log(LoadResStr("IDS_ERR_NORECORD"));
			}
		}

	return true;
}

int32_t ListExpandValids(C4IDList &rlist,
                         C4ID *idlist, int32_t maxidlist)
{
	int32_t cnt,cnt2,ccount,cpos;
	for (cpos=0,cnt=0; rlist.GetID(cnt); cnt++)
		if (C4Id2Def(rlist.GetID(cnt,&ccount)))
			for (cnt2=0; cnt2<ccount; cnt2++)
				if (cpos<maxidlist)
					{ idlist[cpos]=rlist.GetID(cnt); cpos++; }
	return cpos;
}

bool C4Game::PlaceInEarth(C4ID id)
{
	int32_t cnt,tx,ty;
	for (cnt=0; cnt<35; cnt++) // cheap trys
	{
		tx=Random(::Landscape.GetWidth()); ty=Random(::Landscape.GetHeight());
		if (GBackMat(tx,ty)==MEarth)
			if (CreateObject(id,nullptr,NO_OWNER,tx,ty,Random(360)))
				return true;
	}
	return false;
}

static bool PlaceVegetation_GetRandomPoint(int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4PropList * shape_proplist, C4PropList * out_pos_proplist, int32_t *piTx, int32_t *piTy)
{
	// Helper for C4Game::PlaceVegetation: return random position in rectangle. Use shape_proplist if provided.
	if (shape_proplist && out_pos_proplist)
	{
		C4AulParSet pars(C4VPropList(out_pos_proplist));
		if (!shape_proplist->Call(P_GetRandomPoint, &pars)) return false;
		*piTx = out_pos_proplist->GetPropertyInt(P_x);
		*piTy = out_pos_proplist->GetPropertyInt(P_y);
	}
	else
	{
		*piTx = iX + Random(iWdt);
		*piTy = iY + Random(iHgt);
	}
	return true;
}

static bool PlaceVegetation_IsPosInBounds(int32_t iTx, int32_t iTy, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, C4PropList * shape_proplist)
{
	if (shape_proplist)
	{
		// check using shape proplist
		C4AulParSet pars(C4VInt(iTx), C4VInt(iTy));
		if (!shape_proplist->Call(P_IsPointContained, &pars)) return false;
	}
	else
	{
		// check using bounds rect
		if (iTy < iY) return false;
	}
	return true;
}

C4Object* C4Game::PlaceVegetation(C4PropList * PropList, int32_t iX, int32_t iY, int32_t iWdt, int32_t iHgt, int32_t iGrowth, C4PropList * shape_proplist, C4PropList * out_pos_proplist)
{
	int32_t cnt,iTx,iTy,iMaterial;

	// Get definition
	C4Def* pDef;
	if (!PropList || !(pDef = PropList->GetDef())) return nullptr;

	// No growth specified: full growth
	if (iGrowth<=0)
	{
		iGrowth=FullCon;
	}

	// Place by placement type
	switch (PropList->GetPropertyInt(P_Placement))
	{

		// Surface soil
	case C4D_Place_Surface:
		for (cnt=0; cnt<20; cnt++)
		{
			// Random hit within target area
			if (!PlaceVegetation_GetRandomPoint(iX, iY, iWdt, iHgt, shape_proplist, out_pos_proplist, &iTx, &iTy)) break;
			// Above tunnel
			while ((iTy>0) && Landscape.GetBackPix(iTx,iTy) == 0) iTy--;
			// Above semi solid
			if (!AboveSemiSolid(iTx,iTy) || !Inside<int32_t>(iTy,50,::Landscape.GetHeight()-50))
				continue;
			// Still inside bounds?
			if (!PlaceVegetation_IsPosInBounds(iTx, iTy, iX, iY, iWdt, iHgt, shape_proplist)) continue;
			// Free above
			if (GBackSemiSolid(iTx,iTy-pDef->Shape.Hgt) || GBackSemiSolid(iTx,iTy-pDef->Shape.Hgt/2))
				continue;
			// Free upleft and upright
			if (GBackSemiSolid(iTx-pDef->Shape.Wdt/2,iTy-pDef->Shape.Hgt*2/3) || GBackSemiSolid(iTx+pDef->Shape.Wdt/2,iTy-pDef->Shape.Hgt*2/3))
				continue;
			// Soil check
			iTy+=3; // two pix into ground
			iMaterial = GBackMat(iTx,iTy);
			if (iMaterial!=MNone) if (::MaterialMap.Map[iMaterial].Soil)
				{
					iTy+=5;
					return CreateObjectConstruction(PropList,nullptr,NO_OWNER,iTx,iTy,iGrowth);
				}
		}
		break;

		// Underwater
	case C4D_Place_Liquid:
		// Random range
		if (!PlaceVegetation_GetRandomPoint(iX, iY, iWdt, iHgt, shape_proplist, out_pos_proplist, &iTx, &iTy)) return nullptr;
		// Find liquid
		if (!FindSurfaceLiquid(iTx,iTy,pDef->Shape.Wdt,pDef->Shape.Hgt))
			if (!FindLiquid(iTx,iTy,pDef->Shape.Wdt,pDef->Shape.Hgt))
				return nullptr;
		// Liquid bottom
		if (!SemiAboveSolid(iTx,iTy)) return nullptr;
		iTy+=3;
		// Still inside bounds?
		if (!PlaceVegetation_IsPosInBounds(iTx, iTy, iX, iY, iWdt, iHgt, shape_proplist)) return nullptr;
		// Create object
		return CreateObjectConstruction(PropList,nullptr,NO_OWNER,iTx,iTy,iGrowth);
		break;

		// Underground/Tunnel
	case C4D_Place_Subsurface:
		for (cnt=0; cnt<5; cnt++)
		{
			// Random range
			if (!PlaceVegetation_GetRandomPoint(iX, iY, iWdt, iHgt, shape_proplist, out_pos_proplist, &iTx, &iTy)) break;
			// Find tunnel
			if (!FindTunnel(iTx,iTy,pDef->Shape.Wdt,pDef->Shape.Hgt))
				continue;
			// Tunnel bottom
			if (!AboveSemiSolid(iTx,iTy)) continue;
			// Still inside bounds?
			if (!PlaceVegetation_IsPosInBounds(iTx, iTy, iX, iY, iWdt, iHgt, shape_proplist)) continue;
			// Soil check
			iTy+=3; // two pix into ground
			iMaterial = GBackMat(iTx,iTy);
			if (iMaterial!=MNone) if (::MaterialMap.Map[iMaterial].Soil)
				{
					// Create object
					iTy+=5;
					return CreateObjectConstruction(PropList,nullptr,NO_OWNER,iTx,iTy,iGrowth);
				}
		}

		// Under- or aboveground
	case C4D_Place_BothSurface:
		for (cnt=0; cnt<20; cnt++)
		{
			// Random hit within target area
			if (!PlaceVegetation_GetRandomPoint(iX, iY, iWdt, iHgt, shape_proplist, out_pos_proplist, &iTx, &iTy)) break;
			// Above semi solid
			if (!AboveSemiSolid(iTx,iTy) || !Inside<int32_t>(iTy,50,::Landscape.GetHeight()-50))
				continue;
			// Free above
			if (GBackSemiSolid(iTx,iTy-pDef->Shape.Hgt) || GBackSemiSolid(iTx,iTy-pDef->Shape.Hgt/2))
				continue;
			// Still inside bounds?
			if (!PlaceVegetation_IsPosInBounds(iTx, iTy, iX, iY, iWdt, iHgt, shape_proplist))
				continue;
			// Free upleft and upright
			if (GBackSemiSolid(iTx-pDef->Shape.Wdt/2,iTy-pDef->Shape.Hgt*2/3) || GBackSemiSolid(iTx+pDef->Shape.Wdt/2,iTy-pDef->Shape.Hgt*2/3))
				continue;
			// Soil check
			iTy+=3; // two pix into ground
			iMaterial = GBackMat(iTx,iTy);
			if (iMaterial!=MNone) if (::MaterialMap.Map[iMaterial].Soil)
				{
					iTy+=5;
					return CreateObjectConstruction(PropList,nullptr,NO_OWNER,iTx,iTy,iGrowth);
				}
		}

	}

	// Undefined placement type
	return nullptr;
}

C4Object* C4Game::PlaceAnimal(C4PropList* PropList)
{
	C4Def * pDef;
	if (!PropList || !(pDef = PropList->GetDef())) return nullptr;
	int32_t iX,iY;
	// Placement
	switch (PropList->GetPropertyInt(P_Placement))
	{
		// Running free
	case C4D_Place_Surface:
		iX=Random(::Landscape.GetWidth()); iY=Random(::Landscape.GetHeight());
		if (!FindSolidGround(iX,iY,pDef->Shape.Wdt)) return nullptr;
		break;
		// In liquid
	case C4D_Place_Liquid:
		iX=Random(::Landscape.GetWidth()); iY=Random(::Landscape.GetHeight());
		if (!FindSurfaceLiquid(iX,iY,pDef->Shape.Wdt,pDef->Shape.Hgt))
			if (!FindLiquid(iX,iY,pDef->Shape.Wdt,pDef->Shape.Hgt))
				return nullptr;
		iY+=pDef->Shape.Hgt/2;
		break;
		// Floating in air
	case C4D_Place_Air:
		iX=Random(::Landscape.GetWidth());
		for (iY=0; (iY<::Landscape.GetHeight()) && !GBackSemiSolid(iX,iY); iY++) {}
		if (iY<=0) return nullptr;
		iY=Random(iY);
		break;
	default:
		return nullptr;
	}
	// Create object
	return CreateObject(PropList,nullptr,NO_OWNER,iX,iY);
}

void C4Game::InitInEarth()
{
	const int32_t maxvid=100;
	int32_t cnt,vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt=(::Landscape.GetWidth()*::Landscape.GetHeight()/5000)*C4S.Landscape.InEarthLevel.Evaluate()/100;
	// List all valid IDs from C4S
	vidnum=ListExpandValids(C4S.Landscape.InEarth,vidlist,maxvid);
	// Place
	if (vidnum>0)
		for (cnt=0; cnt<amt; cnt++)
			PlaceInEarth(vidlist[Random(vidnum)]);

}

void C4Game::InitVegetation()
{
	const int32_t maxvid=100;
	int32_t cnt,vidnum;
	C4ID vidlist[maxvid];
	// Amount
	int32_t amt=(::Landscape.GetWidth()/50)*C4S.Landscape.VegLevel.Evaluate()/100;
	// Get percentage vidlist from C4S
	vidnum=ListExpandValids(C4S.Landscape.Vegetation,vidlist,maxvid);
	// Place vegetation
	if (vidnum>0)
		for (cnt=0; cnt<amt; cnt++)
			PlaceVegetation(C4Id2Def(vidlist[Random(vidnum)]),0,0,::Landscape.GetWidth(),::Landscape.GetHeight(),-1,nullptr,nullptr);
}

void C4Game::InitAnimals()
{
	int32_t cnt,cnt2;
	C4ID idAnimal; int32_t iCount;
	// Place animals
	for (cnt=0; (idAnimal=C4S.Animals.FreeLife.GetID(cnt,&iCount)); cnt++)
		for (cnt2=0; cnt2<iCount; cnt2++)
			PlaceAnimal(C4Id2Def(idAnimal));
	// Place nests
	for (cnt=0; (idAnimal=C4S.Animals.EarthNest.GetID(cnt,&iCount)); cnt++)
		for (cnt2=0; cnt2<iCount; cnt2++)
			PlaceInEarth(idAnimal);
}


bool C4Game::LoadScenarioComponents()
{
	// Info
	Info.Load(ScenarioFile,C4CFN_Info);
	// Overload clonk names from scenario file
	if (ScenarioFile.EntryCount(C4CFN_Names))
		Names.Load(ScenarioFile, C4CFN_Names);
	// scenario sections
	char fn[_MAX_FNAME+1] = { 0 };
	ScenarioFile.ResetSearch(); *fn=0;
	while (ScenarioFile.FindNextEntry(C4CFN_ScenarioSections, fn, nullptr, !!*fn))
	{
		// get section name
		char SctName[_MAX_FNAME+1];
		int32_t iWildcardPos = SCharPos('*', C4CFN_ScenarioSections);
		SCopy(fn + iWildcardPos, SctName, _MAX_FNAME);
		RemoveExtension(SctName);
		if (std::strlen(SctName)>C4MaxName || !*SctName)
		{
			DebugLog("invalid section name");
			LogFatal(FormatString(LoadResStr("IDS_ERR_SCENSECTION"), fn).getData()); return false;
		}
		// load this section into temp store
		C4ScenarioSection *pSection = new C4ScenarioSection(SctName);
		if (!pSection->ScenarioLoad(fn, false))
			{ LogFatal(FormatString(LoadResStr("IDS_ERR_SCENSECTION"), fn).getData()); return false; }
	}
	// Success
	return true;
}

bool C4Game::LoadAdditionalSystemGroup(C4Group &parent_group)
{
	// called for scenario local and definition local System.ocg groups
	C4Group SysGroup;
	char fn[_MAX_FNAME+1] = { 0 };
	if (SysGroup.OpenAsChild(&parent_group, C4CFN_System))
	{
		C4LangStringTable *pSysGroupString = new C4LangStringTable();
		C4Language::LoadComponentHost(pSysGroupString, SysGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
		// load custom scenario control definitions
		if (SysGroup.FindEntry(C4CFN_PlayerControls))
		{
			Log(LoadResStr("IDS_PRC_LOADSCEPLRCTRL"));
			C4PlayerControlFile PlayerControlFile;
			if (!PlayerControlFile.Load(SysGroup, C4CFN_PlayerControls, pSysGroupString))
			{
				// non-fatal error here
				Log(LoadResStr("IDS_PRC_LOADSCEPLRCTRLFAIL"));
			}
			else
			{
				// local definitions loaded successfully - merge into global definitions
				PlayerControlDefs.MergeFrom(PlayerControlFile.GetControlDefs());
				PlayerControlDefaultAssignmentSets.MergeFrom(PlayerControlFile.GetAssignmentSets(), C4PlayerControlAssignmentSet::MM_LowPrio);
				PlayerControlDefaultAssignmentSets.ResolveRefs(&PlayerControlDefs);
			}
		}
		// load all scripts in there
		SysGroup.ResetSearch();
		while (SysGroup.FindNextEntry(C4CFN_ScriptFiles, fn, nullptr, !!fn[0]))
		{
			// host will be destroyed by script engine, so drop the references
			C4ScriptHost *scr = new C4ExtraScriptHost();
			scr->Reg2List(&ScriptEngine);
			scr->Load(SysGroup, fn, Config.General.LanguageEx, pSysGroupString);
		}
		// if it's a physical group: watch out for changes
		if (!SysGroup.IsPacked() && Game.pFileMonitor)
			Game.pFileMonitor->AddDirectory(SysGroup.GetFullName().getData());
		SysGroup.Close();
		// release string table if no longer used
		pSysGroupString->DelRef();
	}
	return true;
}

bool C4Game::InitKeyboard()
{
	C4CustomKey::CodeList Keys;

	// clear previous
	KeyboardInput.Clear();

	// globals
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F3                ), "MusicToggle",            C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4MusicSystem>   (Application.MusicSystem, &C4MusicSystem::ToggleOnOff)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F9                ), "Screenshot",             C4KeyScope(KEYSCOPE_Fullscreen | KEYSCOPE_Gui), new C4KeyCBEx<C4GraphicsSystem, bool>(GraphicsSystem, false, &C4GraphicsSystem::SaveScreenshotKey)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F9,   KEYS_Control), "ScreenshotEx",           KEYSCOPE_Fullscreen, new C4KeyCBEx<C4GraphicsSystem, bool>(GraphicsSystem, true, &C4GraphicsSystem::SaveScreenshotKey)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_C,      KEYS_Alt), "ToggleChat",             C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4Game>   (*this, &C4Game::ToggleChat)));

	// main ingame
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F1                ), "ToggleShowHelp",         KEYSCOPE_Generic,    new C4KeyCB  <C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowHelp)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F4                ), "NetClientListDlgToggle", KEYSCOPE_Generic,    new C4KeyCB  <C4Network2>      (Network, &C4Network2::ToggleClientListDlg)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F5                ), "ZoomIn",                 KEYSCOPE_Generic,    new C4KeyCB  <C4ViewportList>  (::Viewports, &C4ViewportList::ViewportZoomIn)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F6                ), "ZoomOut",                KEYSCOPE_Generic,    new C4KeyCB  <C4ViewportList>  (::Viewports, &C4ViewportList::ViewportZoomOut)));

	// debug mode & debug displays
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F5,   KEYS_Control), "DbgModeToggle",          KEYSCOPE_Generic,    new C4KeyCB  <C4Game>          (*this, &C4Game::ToggleDebugMode)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F6,   KEYS_Control), "DbgShowVtxToggle",       KEYSCOPE_Generic,    new C4KeyCB  <C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowVertices)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F7,   KEYS_Control), "DbgShowActionToggle",    KEYSCOPE_Generic,    new C4KeyCB  <C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowAction)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_F8,   KEYS_Control), "DbgShow8BitSurface", KEYSCOPE_Generic,    new C4KeyCB  <C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShow8BitSurface)));

	// playback speed - improve...
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ADD,      KEYS_Shift), "GameSpeedUp",          KEYSCOPE_Generic,    new C4KeyCB  <C4Game>          (*this, &C4Game::SpeedUp)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_SUBTRACT, KEYS_Shift), "GameSlowDown",         KEYSCOPE_Generic,    new C4KeyCB  <C4Game>          (*this, &C4Game::SlowDown)));

	// fullscreen menu
	Keys.clear(); Keys.emplace_back(K_LEFT);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Left(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuLeft",     KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuLeft, &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.emplace_back(K_RIGHT);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Right(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuRight",    KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuRight, &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.emplace_back(K_UP);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Up(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuUp",       KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuUp, &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.emplace_back(K_DOWN);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Down(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuDown",     KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuDown, &C4FullScreen::MenuKeyControl)));
	Keys.clear(); Keys.emplace_back(K_SPACE); Keys.emplace_back(K_RETURN);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Ok(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuOK",       KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuEnter, &C4FullScreen::MenuKeyControl))); // name used by PlrControlKeyName
	Keys.clear(); Keys.emplace_back(K_ESCAPE);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Cancel(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuCancel",   KEYSCOPE_FullSMenu,  new C4KeyCBEx<C4FullScreen, BYTE>   (FullScreen, COM_MenuClose, &C4FullScreen::MenuKeyControl))); // name used by PlrControlKeyName
	Keys.clear(); Keys.emplace_back(K_SPACE);
	if (Config.Controls.GamepadGuiControl) ControllerKeys::Any(Keys);
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "FullscreenMenuOpen",     KEYSCOPE_FreeView,   new C4KeyCB  <C4FullScreen>   (FullScreen, &C4FullScreen::ActivateMenuMain))); // name used by C4MainMenu!
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RIGHT             ), "FilmNextPlayer",         KEYSCOPE_FilmView,   new C4KeyCB  <C4ViewportList>(::Viewports, &C4ViewportList::ViewportNextPlayer)));

	// chat
	Keys.clear();
	Keys.emplace_back(K_RETURN);
	Keys.emplace_back(K_F2); // alternate chat key, if RETURN is blocked by player control
	KeyboardInput.RegisterKey(new C4CustomKey(Keys,                              "ChatOpen",               KEYSCOPE_Generic,    new C4KeyCBEx<C4MessageInput, bool>(MessageInput, false, &C4MessageInput::KeyStartTypeIn)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RETURN, KEYS_Shift), "ChatOpen2Allies",        KEYSCOPE_Generic,    new C4KeyCBEx<C4MessageInput, bool>(MessageInput, true, &C4MessageInput::KeyStartTypeIn)));

	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_LEFT              ), "FreeViewScrollLeft",     KEYSCOPE_FreeView,   new C4KeyCBEx<C4ViewportList, C4Vec2D>(::Viewports, C4Vec2D(-5, 0), &C4ViewportList::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_RIGHT             ), "FreeViewScrollRight",    KEYSCOPE_FreeView,   new C4KeyCBEx<C4ViewportList, C4Vec2D>(::Viewports, C4Vec2D(+5, 0), &C4ViewportList::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_UP                ), "FreeViewScrollUp",       KEYSCOPE_FreeView,   new C4KeyCBEx<C4ViewportList, C4Vec2D>(::Viewports, C4Vec2D(0, -5), &C4ViewportList::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_DOWN              ), "FreeViewScrollDown",     KEYSCOPE_FreeView,   new C4KeyCBEx<C4ViewportList, C4Vec2D>(::Viewports, C4Vec2D(0, +5), &C4ViewportList::FreeScroll)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_TAB               ), "ScoreboardToggle",       KEYSCOPE_Generic,    new C4KeyCB  <C4Scoreboard>(Scoreboard, &C4Scoreboard::KeyUserShow)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ESCAPE            ), "GameAbort",              KEYSCOPE_Fullscreen, new C4KeyCB  <C4FullScreen>(FullScreen, &C4FullScreen::ShowAbortDlg)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_PAUSE             ), "FullscreenPauseToggle",  KEYSCOPE_Fullscreen, new C4KeyCB  <C4Game>(Game, &C4Game::TogglePause)));

	// console keys
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_PAUSE             ), "ConsolePauseToggle",     KEYSCOPE_Console,    new C4KeyCB  <C4Console>(Console, &C4Console::TogglePause)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_ADD               ), "ToolsDlgGradeUp",        KEYSCOPE_Console,    new C4KeyCBEx<C4ToolsDlg, int32_t>(Console.ToolsDlg, +5, &C4ToolsDlg::ChangeGrade)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_SUBTRACT          ), "ToolsDlgGradeDown",      KEYSCOPE_Console,    new C4KeyCBEx<C4ToolsDlg, int32_t>(Console.ToolsDlg, -5, &C4ToolsDlg::ChangeGrade)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_M,  KEYS_Control), "ToolsDlgPopMaterial",    KEYSCOPE_Console,    new C4KeyCB  <C4ToolsDlg>(Console.ToolsDlg, &C4ToolsDlg::PopMaterial)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_T,  KEYS_Control), "ToolsDlgPopTextures",    KEYSCOPE_Console,    new C4KeyCB  <C4ToolsDlg>(Console.ToolsDlg, &C4ToolsDlg::PopTextures)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_I,  KEYS_Control), "ToolsDlgIFTToggle",      KEYSCOPE_Console,    new C4KeyCB  <C4ToolsDlg>(Console.ToolsDlg, &C4ToolsDlg::ToggleIFT)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_W,  KEYS_Control), "ToolsDlgToolToggle",     KEYSCOPE_Console,    new C4KeyCB  <C4ToolsDlg>(Console.ToolsDlg, &C4ToolsDlg::ToggleTool)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(K_DELETE            ), "EditCursorDelete",       KEYSCOPE_Console,    new C4KeyCB  <C4EditCursor>(Console.EditCursor, &C4EditCursor::Delete)));

	// no default keys assigned
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "EditCursorModeToggle",   KEYSCOPE_Console,    new C4KeyCB  <C4EditCursor>(Console.EditCursor, &C4EditCursor::ToggleMode)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "ChartToggle",            C4KeyScope(KEYSCOPE_Generic | KEYSCOPE_Gui),    new C4KeyCB  <C4Game>          (*this, &C4Game::ToggleChart)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "NetObsNextPlayer",       KEYSCOPE_FreeView,   new C4KeyCB  <C4ViewportList>(::Viewports, &C4ViewportList::ViewportNextPlayer)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "CtrlRateDown",           KEYSCOPE_Generic,    new C4KeyCBEx<C4GameControl, int32_t>(Control, -1, &C4GameControl::KeyAdjustControlRate)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "CtrlRateUp",             KEYSCOPE_Generic,    new C4KeyCBEx<C4GameControl, int32_t>(Control, +1, &C4GameControl::KeyAdjustControlRate)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "NetAllowJoinToggle",     KEYSCOPE_Generic,    new C4KeyCB  <C4Network2>      (Network, &C4Network2::ToggleAllowJoin)));
	KeyboardInput.RegisterKey(new C4CustomKey(C4KeyCodeEx(KEY_Default         ), "NetStatsToggle",         KEYSCOPE_Generic,    new C4KeyCB  <C4GraphicsSystem>(GraphicsSystem, &C4GraphicsSystem::ToggleShowNetStatus)));

	// load any custom keysboard overloads
	KeyboardInput.LoadCustomConfig();

	// done, success
	return true;
}

void C4Game::UpdateLanguage()
{
	// Reload System.ocg string table
	C4Language::LoadComponentHost(&MainSysLangStringTable, Application.SystemGroup, C4CFN_ScriptStringTbl, Config.General.LanguageEx);
}

bool C4Game::InitPlayerControlSettings()
{
	// Load controls and default control sets
	C4PlayerControlFile PlayerControlFile;
	if (!PlayerControlFile.Load(Application.SystemGroup, C4CFN_PlayerControls, &MainSysLangStringTable)) { LogFatal("[!]Error loading player controls"); return false; }
	PlayerControlDefs = PlayerControlFile.GetControlDefs();
	PlayerControlDefaultAssignmentSets.Clear();
	PlayerControlDefaultAssignmentSets.MergeFrom(PlayerControlFile.GetAssignmentSets(), C4PlayerControlAssignmentSet::MM_Normal);
	PlayerControlDefaultAssignmentSets.ResolveRefs(&PlayerControlDefs);
	// Merge w/ config settings into user control sets
	// User settings will be cleared and re-merged again later after scenario/definition control overloads, but initialization
	//  is needed already for config dialogs
	if (!InitPlayerControlUserSettings()) return false;
	return true;
}

bool C4Game::InitPlayerControlUserSettings()
{
	// Merge config control settings with user settings
	PlayerControlUserAssignmentSets.Clear();
	PlayerControlUserAssignmentSets.MergeFrom(PlayerControlDefaultAssignmentSets, C4PlayerControlAssignmentSet::MM_Inherit);
	PlayerControlUserAssignmentSets.MergeFrom(Config.Controls.UserSets, C4PlayerControlAssignmentSet::MM_ConfigOverload);
	PlayerControlUserAssignmentSets.ResolveRefs(&PlayerControlDefs);
	return true;
}

C4Player *C4Game::JoinPlayer(const char *szFilename, int32_t iAtClient, const char *szAtClientName, C4PlayerInfo *pInfo)
{
	assert(pInfo);
	C4Player *pPlr;
	// Join
	if (!( pPlr = Players.Join(szFilename,true,iAtClient,szAtClientName, pInfo, nullptr) )) return nullptr;
	// Player final init
	pPlr->FinalInit(true);
	// Create player viewport
	if (pPlr->LocalControl) ::Viewports.CreateViewport(pPlr->Number);
	// Check fullscreen viewports
	FullScreen.ViewportCheck();
	// Update menus
	Console.UpdateMenus();
	// Append player name to list of session player names (no duplicates)
	if (!SIsModule(PlayerNames.getData(), pPlr->GetName()))
	{
		if (PlayerNames) PlayerNames += ";";
		PlayerNames += pPlr->GetName();
	}
	// Success
	return pPlr;
}

void C4Game::OnPlayerJoinFinished()
{
	// Do the InitializePlayers callback once all player joins have finished with at least one human player
	if (!InitialPlayersJoined && !PlayerInfos.GetJoinPendingPlayerCount() && !::Players.HasPlayerInTeamSelection() && (::Players.GetCount(C4PT_User) > 0))
	{
		InitialPlayersJoined = true;
		GRBroadcast(PSF_InitializePlayers);
	}
}

void C4Game::FixRandom(uint64_t iSeed)
{
	FixedRandom(iSeed);
}

bool C4Game::DoGameOver()
{
	// Duplication safety
	if (GameOver) return false;
	// Flag, log, call
	GameOver=true;
	Log(LoadResStr("IDS_PRC_GAMEOVER"));
	GRBroadcast(PSF_OnGameOver);
	// Flag all surviving players as winners
	for (C4Player *pPlayer = Players.First; pPlayer; pPlayer = pPlayer->Next)
		if (!pPlayer->Eliminated)
			pPlayer->EvaluateLeague(false, true);
	// Immediately save config so mission access gained by this round is stored if the game crashes during shutdown
	// or in a following round
	Config.Save();
	return true;
}

void C4Game::ShowGameOverDlg()
{
	// safety
	if (GameOverDlgShown) return;
	// flag, show
	GameOverDlgShown = true;
#ifndef USE_CONSOLE
	if (!Application.isEditor)
	{
		C4GameOverDlg *pDlg = new C4GameOverDlg();
		pDlg->SetDelOnClose();
		if (!pDlg->Show(pGUI, true)) { delete pDlg; Application.QuitGame(); }
	}
#endif
}

void C4Game::SyncClearance()
{
	Objects.SyncClearance();
}

void C4Game::Synchronize(bool fSavePlayerFiles)
{
	// Log
	LogSilentF("Network: Synchronization (Frame %i) [PlrSave: %d]",FrameCounter, fSavePlayerFiles);
	// callback to control (to start record)
	Control.OnGameSynchronizing();
	// Fix random
	FixRandom(RandomSeed);
	// Synchronize members
	::Definitions.Synchronize();
	Landscape.Synchronize();
	MassMover.Synchronize();
	Objects.Synchronize();
	// synchronize local player files if desired
	// this will reset any InActionTimes!
	// (not in replay mode)
	if (fSavePlayerFiles && !Control.isReplay()) Players.SynchronizeLocalFiles();
	// callback to network
	if (Network.isEnabled()) Network.OnGameSynchronized();
	// TransferZone synchronization: Must do this after dynamic creation to avoid synchronization loss
	// if OnSynchronized-callbacks do sync-relevant changes
	TransferZones.Synchronize();
}

bool C4Game::InitNetworkFromAddress(const char *szAddress)
{
	StdCopyStrBuf strRefQueryFailed(LoadResStr("IDS_NET_REFQUERY_FAILED"));
	// Query reference
	C4Network2RefClient RefClient;
	if (!RefClient.Init() ||
	    !RefClient.SetServer(szAddress) ||
	    !RefClient.QueryReferences())
		{ LogFatal(FormatString(strRefQueryFailed.getData(), RefClient.GetError()).getData()); return false; }
	// We have to wait for the answer
	StdStrBuf Message = FormatString(LoadResStr("IDS_NET_REFQUERY_QUERYMSG"), szAddress);
	Log(Message.getData());
	// Set up wait dialog
	C4GUI::MessageDialog *pDlg = nullptr;
	if (!Application.isEditor)
	{
		// create & show
		pDlg = new C4GUI::MessageDialog(Message.getData(), LoadResStr("IDS_NET_REFQUERY_QUERYTITLE"),
		                                C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsMedium);
		if (!pDlg || !pDlg->Show(::pGUI, true)) return false;
	}
	// Wait for response
	while (RefClient.isBusy())
	{
		// Execute GUI
		if (!Application.ScheduleProcs(100) ||
		    (pDlg && pDlg->IsAborted()))
		{
			delete pDlg;
			return false;
		}
		// Check if reference is received
		if (!RefClient.Execute(0))
			break;
	}
	// Close dialog
	delete pDlg;
	// Error?
	if (!RefClient.isSuccess())
		{ LogFatal(FormatString(strRefQueryFailed.getData(), RefClient.GetError()).getData()); return false; }
	// Get references
	C4Network2Reference **ppRefs = nullptr; int32_t iRefCount;
	if (!RefClient.GetReferences(ppRefs, iRefCount) || iRefCount <= 0)
		{ LogFatal(FormatString(strRefQueryFailed.getData(), LoadResStr("IDS_NET_REFQUERY_NOREF")).getData()); return false; }
	// Connect to first reference
	bool fSuccess = InitNetworkFromReference(*ppRefs[0]);
	// Remove references
	for (int i = 0; i < iRefCount; i++)
		delete ppRefs[i];
	delete[] ppRefs;
	return fSuccess;
}

bool C4Game::InitNetworkFromReferenceFile(const char *temp_filename)
{
	// We need winsock for address parsing
	WinSockHolder ws;
	// Load reference from temp file + delete the temp file
	bool success = false;
	C4Network2Reference ref;
	StdBuf join_data;
	if (join_data.LoadFromFile(temp_filename))
	{
		CompileFromBuf<StdCompilerBinRead>(ref, join_data);
		success = true;
	}
	EraseFile(temp_filename);
	if (!success) return false;
	return InitNetworkFromReference(ref);
}

bool C4Game::InitNetworkFromReference(const C4Network2Reference &Reference)
{
	// Find host data
	C4Client *pHostData = Reference.Parameters.Clients.getClientByID(C4ClientIDHost);
	if (!pHostData) { LogFatal(LoadResStr("IDS_NET_INVALIDREF")); return false; }
	// Save scenario title
	ScenarioTitle = Reference.getTitle();
	// Log
	LogF(LoadResStr("IDS_NET_JOINGAMEBY"), pHostData->getName());
	// Init clients
	if (!Clients.Init())
		return false;
	// Connect
	if (Network.InitClient(Reference, false) != C4Network2::IR_Success)
	{
		LogFatal(FormatString(LoadResStr("IDS_NET_NOHOSTCON"), pHostData->getName()).getData());
		return false;
	}
	// init control
	if (!Control.InitNetwork(Clients.getLocal())) return false;
	// init local player info list
	Network.Players.Init();
	return true;
}

bool C4Game::InitNetworkHost()
{
	// Network not active?
	if (!NetworkActive)
	{
		// Clear client list
		if (!C4S.Head.Replay)
			Clients.Init();
		return true;
	}
	// network not active?
	if (C4S.Head.NetworkGame)
		{ LogFatal(LoadResStr("IDS_NET_NODIRECTSTART")); return Clients.Init(); }
	// replay?
	if (C4S.Head.Replay)
		{ LogFatal(LoadResStr("IDS_PRC_NONETREPLAY")); return true; }
	// clear client list
	if (!Clients.Init())
		return false;
	// init network as host
	if (!Network.InitHost(fLobby)) return false;
	// init control
	if (!Control.InitNetwork(Clients.getLocal())) return false;
	// init local player info list
	Network.Players.Init();
	// allow connect
	Network.AllowJoin(true);
	// do lobby (if desired)
	if (fLobby)
	{
		if (!Network.DoLobby()) return false;
	}
	else
	{
		// otherwise: start manually
		if (!Network.Start()) return false;
	}
	// ok
	return true;
}

bool C4Game::CheckObjectEnumeration()
{

	struct Check
	{
		int32_t maxNumber{0};
		Check() = default;
		// Check valid & maximum number & duplicate numbers
		bool that(C4Object* cObj)
		{
			// Invalid number
			if (cObj->Number<1)
			{
				LogFatal(FormatString("Invalid object enumeration number (%d) of object %s (x=%d)", cObj->Number, cObj->id.ToString(), cObj->GetX()).getData()); return false;
			}
			// Max
			if (cObj->Number>maxNumber) maxNumber=cObj->Number;
			// Duplicate
			for (C4Object *cObj2 : Objects)
				if (cObj2!=cObj)
					if (cObj->Number==cObj2->Number)
						{ LogFatal(FormatString("Duplicate object enumeration number %d (%s and %s)",cObj2->Number,cObj->GetName(),cObj2->GetName()).getData()); return false; }
			for (C4Object *cObj2 : Objects.InactiveObjects)
				if (cObj2!=cObj)
					if (cObj->Number==cObj2->Number)
						{ LogFatal(FormatString("Duplicate object enumeration number %d (%s and %s(i))",cObj2->Number,cObj->GetName(),cObj2->GetName()).getData()); return false; }
			return true;
		}
	};

	Check check;
	for (C4Object *cObj : Objects)
	{
		if (!check.that(cObj))
			return false;
	}

	for (C4Object *cObj : Objects.InactiveObjects)
	{
		if (!check.that(cObj))
			return false;
	}

	// Adjust enumeration index
	C4PropListNumbered::SetEnumerationIndex(check.maxNumber);
	// Done
	return true;
}

void C4Game::InitValueOverloads()
{
	C4ID idOvrl; C4Def *pDef;
	// set new values
	for (int32_t cnt=0; (idOvrl=C4S.Game.Realism.ValueOverloads.GetID(cnt)); cnt++)
		if ((pDef=::Definitions.ID2Def(idOvrl)))
			pDef->Value=C4S.Game.Realism.ValueOverloads.GetIDCount(idOvrl);
}

void C4Game::InitEnvironment()
{
	// Place environment objects
	int32_t cnt,cnt2;
	C4ID idType; int32_t iCount;
	for (cnt=0; (idType=C4S.Environment.Objects.GetID(cnt,&iCount)); cnt++)
		for (cnt2=0; cnt2<iCount; cnt2++)
			CreateObject(idType,nullptr);
}

void C4Game::InitRules()
{
	// Place rule objects
	int32_t cnt,cnt2;
	C4ID idType; int32_t iCount;
	for (cnt=0; (idType=Parameters.Rules.GetID(cnt,&iCount)); cnt++)
		for (cnt2=0; cnt2<std::max<int32_t>(iCount,1); cnt2++)
			CreateObject(idType,nullptr);
}

void C4Game::InitGoals()
{
	// Place goal objects
	int32_t cnt,cnt2;
	C4ID idType; int32_t iCount;
	for (cnt=0; (idType=Parameters.Goals.GetID(cnt,&iCount)); cnt++)
		for (cnt2=0; cnt2<iCount; cnt2++)
			CreateObject(idType,nullptr);
}

void C4Game::SetInitProgress(float fToProgress)
{
	// set new progress
	InitProgress=int32_t(fToProgress);
	// if progress is more than one percent, display it
	if (InitProgress > LastInitProgress)
	{
		LastInitProgress=InitProgress;
		GraphicsSystem.MessageBoard->LogNotify();
	}
	// Cheap hack to get the Console window updated while loading
	// (unless game is running, i.e. section change - section change would be quick and timer execution can mess with things unpredictably)
	if (!IsRunning)
	{
		Application.FlushMessages();
#ifdef WITH_QT_EDITOR
		Application.ProcessQtEvents();
#endif
	}
}

void C4Game::OnResolutionChanged(unsigned int iXRes, unsigned int iYRes)
{
	// update anything that's dependant on screen resolution
	pGUI->SetBounds(C4Rect(0,0,iXRes,iYRes));
	if (FullScreen.Active)
		InitFullscreenComponents(!!IsRunning);
	// note that this may fail if the gfx groups are closed already (runtime resolution change)
	// doesn't matter; old gfx are kept in this case
	GraphicsResource.ReloadResolutionDependantFiles();
	::Viewports.RecalculateViewports();
}

void C4Game::OnKeyboardLayoutChanged()
{
	// Layout changed: Re-resolve keys
	PlayerControlDefaultAssignmentSets.ResolveRefs(&PlayerControlDefs);
	PlayerControlUserAssignmentSets.ResolveRefs(&PlayerControlDefs);
}

bool C4Game::CreateSectionFromTempFile(const char *section_name, const char *temp_filename)
{
	// Remove existing (temp) section of same name
	C4ScenarioSection *existing_section = pScenarioSections, *prev = nullptr;
	while (existing_section) if (SEqualNoCase(existing_section->name.getData(), section_name)) break; else existing_section = (prev = existing_section)->pNext;
	bool deleted_current_section = false;
	if (existing_section)
	{
		deleted_current_section = (existing_section == pCurrentScenarioSection);
		if (deleted_current_section)
		{
			pCurrentScenarioSection = nullptr;
			pScenarioObjectsScript = nullptr;
		}
		if (existing_section->pObjectScripts)
		{
			delete existing_section->pObjectScripts;
		}
		if (prev) prev->pNext = existing_section->pNext; else pScenarioSections = existing_section->pNext;
		existing_section->pNext = nullptr;
		delete existing_section;
	}
	// Create new (temp) section
	C4ScenarioSection *new_section = new C4ScenarioSection(section_name);
	if (!new_section->ScenarioLoad(temp_filename, true))
	{
		pScenarioSections = new_section->pNext;
		new_section->pNext = nullptr;
		delete new_section;
		return false;
	}
	// Re-Link current section into newly created section
	if (deleted_current_section)
	{
		pCurrentScenarioSection = new_section;
		pScenarioObjectsScript = new_section->pObjectScripts;
	}
	// Link new Objects.c (or re-link because old Objects.c was removed)
	ReLinkScriptEngine();
	return !!new_section;
}

bool C4Game::LoadScenarioSection(const char *szSection, DWORD dwFlags)
{
	// note on scenario section saving:
	// if a scenario section overwrites a value that had used the default values in the main scenario section,
	// returning to the main section with an unsaved landscape (and thus an unsaved scenario core),
	// would leave those values in the altered state of the previous section
	// scenario designers should regard this and always define any values, that are defined in subsections as well
	C4Group hGroup, *pGrp;
	// if current section was the loaded section (maybe main, but need not for resumed savegames)
	if (!pCurrentScenarioSection)
	{
		if (!*CurrentScenarioSection) SCopy(C4ScenSect_Main, CurrentScenarioSection, C4MaxName);
		pCurrentScenarioSection = new C4ScenarioSection(CurrentScenarioSection);
		pCurrentScenarioSection->pObjectScripts = Game.pScenarioObjectsScript;
	}
	// find section to load
	C4ScenarioSection *pLoadSect = pScenarioSections;
	while (pLoadSect) if (SEqualNoCase(pLoadSect->name.getData(), szSection)) break; else pLoadSect = pLoadSect->pNext;
	if (!pLoadSect)
	{
		DebugLogF("LoadScenarioSection: scenario section %s not found!", szSection);
		return false;
	}
	// save current section state
	if (pLoadSect != pCurrentScenarioSection && dwFlags & (C4S_SAVE_LANDSCAPE | C4S_SAVE_OBJECTS))
	{
		// ensure that the section file does point to temp store
		if (!pCurrentScenarioSection->EnsureTempStore(!(dwFlags & C4S_SAVE_LANDSCAPE), !(dwFlags & C4S_SAVE_OBJECTS)))
		{
			DebugLogF("LoadScenarioSection(%s): could not extract section files of current section %s", szSection, pCurrentScenarioSection->name.getData());
			return false;
		}
		// open current group
		if (!(pGrp = pCurrentScenarioSection->GetGroupfile(hGroup)))
		{
			DebugLog("LoadScenarioSection: error opening current group file");
			return false;
		}
		// store landscape, if desired (w/o material enumeration - that's assumed to stay consistent during the game)
		if (dwFlags & C4S_SAVE_LANDSCAPE)
		{
			// storing the landscape implies storing the scenario core
			// otherwise, the ExactLandscape-flag would be lost
			// maybe imply exact landscapes by the existance of Landscape.png-files?
			C4Scenario rC4S = C4S;
			rC4S.SetExactLandscape();
			if (!rC4S.Save(*pGrp, true))
			{
				DebugLog("LoadScenarioSection: Error saving C4S");
				return false;
			}
			// landscape
			{
				C4DebugRecOff DBGRECOFF;
				if (!Landscape.Save(*pGrp))
				{
					DebugLog("LoadScenarioSection: Error saving Landscape");
					return false;
				}
			}
			// PXS
			if (!PXS.Save(*pGrp))
			{
				DebugLog("LoadScenarioSection: Error saving PXS");
				return false;
			}
			// MassMover (create copy, may not modify running data)
			C4MassMoverSet MassMoverSet;
			MassMoverSet.Copy(MassMover);
			if (!MassMoverSet.Save(*pGrp))
			{
				DebugLog("LoadScenarioSection: Error saving MassMover");
				return false;
			}
		}
		// store objects
		if (dwFlags & C4S_SAVE_OBJECTS)
		{
			C4ValueNumbers numbers;
			// objects: do not save info objects or inactive objects
			if (!SaveData(*pGrp,true,false, false, &numbers))
			{
				DebugLog("LoadScenarioSection: Error saving objects");
				return false;
			}
		}
		// close current group
		if (hGroup.IsOpen()) hGroup.Close();
		// mark modified
		pCurrentScenarioSection->fModified = true;
	}
	// open section group
	if (!(pGrp=pLoadSect->GetGroupfile(hGroup)))
	{
		DebugLog("LoadScenarioSection: error opening group file");
		return false;
	}
	// remove all objects
	// do correct removal calls, because this will stop fire sounds, etc.
	for (C4Object *obj : Objects)
		obj->AssignRemoval();
	for (C4Object *obj : Objects)
		if (obj->Status)
		{
			DebugLogF("LoadScenarioSection: WARNING: Object %d created in destruction process!", (int) obj->Number);
			ClearPointers(obj);
		}
	// Final removal in case objects got recreated
	// Also kill inactive objects if scenario is reinitialized
	DeleteObjects(!!(dwFlags & C4S_REINIT_SCENARIO));
	// remove global effects
	if (::ScriptEngine.pGlobalEffects && !(dwFlags & C4S_KEEP_EFFECTS))
	{
		::ScriptEngine.pGlobalEffects->ClearAll(C4FxCall_RemoveClear);
		// scenario section call might have been done from a global effect
		// rely on dead effect removal for actually removing the effects; do not clear the array here!
	}
	if (::GameScript.pScenarioEffects && !(dwFlags & C4S_KEEP_EFFECTS))
		::GameScript.pScenarioEffects->ClearAll(C4FxCall_RemoveClear);
	// del particles as well
	Particles.ClearAllParticles();
	// clear transfer zones
	TransferZones.Clear();
	// backup old sky
	std::string old_sky;
	old_sky = C4S.Landscape.SkyDef;
	// do not warn on ignored values in main section
	// they are caused because not all parts of scenario core are compiled on section change
	bool is_main_section = SEqualNoCase(pLoadSect->name.getData(), C4ScenSect_Main);
	// overload scenario values (fails if no scenario core is present; that's OK)
	C4S.Load(*pGrp, true, is_main_section);
	// determine whether a new sky has to be loaded
	bool fLoadNewSky = !SEqualNoCase(old_sky.c_str(), C4S.Landscape.SkyDef.c_str()) || pGrp->FindEntry(C4CFN_Sky ".*");
	// set new Objects.c source
	Game.pScenarioObjectsScript = pLoadSect->pObjectScripts;
	// remove reference to FoW from viewports, so that we can safely
	// reload the landscape and its FoW.
	Viewports.DisableFoW();
	// landscape initialization resets the RNG
	// set a new seed here to get new dynamic landscapes
	// TODO: add an option to disable this?
	RandomSeed = Random(2147483647);
	FixRandom(RandomSeed);
	// re-init game in new section
	C4ValueNumbers numbers;
	if (!InitGame(*pGrp, (dwFlags & C4S_REINIT_SCENARIO) ? IM_ReInit : IM_Section, fLoadNewSky, &numbers))
	{
		DebugLog("LoadScenarioSection: Error reiniting game");
		::Viewports.EnableFoW();
		return false;
	}
	// restore shelved proplists in case loading failed
	C4PropListNumbered::UnshelveNumberedPropLists();
	// set new current section
	pCurrentScenarioSection = pLoadSect;
	SCopy(pCurrentScenarioSection->name.getData(), CurrentScenarioSection);
	// Final init on game re-init (doing mostly player initialization)
	if (dwFlags & C4S_REINIT_SCENARIO)
	{
		InitGameFinal(IM_ReInit);
		// Extra InitializePlayers callback on the already-joined players to start intros, etc.
		// (unless the call is still pending - can happen if section is loaded during player join)
		if (::Game.InitialPlayersJoined && ::Players.GetCount())
			::Game.GRBroadcast(PSF_InitializePlayers);
	}
	// resize viewports, and enable lighting again
	::Viewports.RecalculateViewports();
	::Viewports.EnableFoW();
	// done, success
	return true;
}

bool C4Game::ToggleDebugMode()
{
	// debug mode not allowed
	if (!Parameters.AllowDebug && !DebugMode) { GraphicsSystem.FlashMessage(LoadResStr("IDS_MSG_DEBUGMODENOTALLOWED")); return false; }
	DebugMode = !DebugMode;
	if (!DebugMode) GraphicsSystem.DeactivateDebugOutput();
	GraphicsSystem.FlashMessageOnOff(LoadResStr("IDS_CTL_DEBUGMODE"), DebugMode);
	return true;
}

bool C4Game::ActivateMenu(const char *szCommand)
{
	// no new menu during round evaluation
	if (C4GameOverDlg::IsShown()) return false;
	// forward to primary player
	C4Player *pPlr=::Players.GetLocalByIndex(0);
	if (!pPlr) return false;
	pPlr->Menu.ActivateCommand(pPlr->Number, szCommand);
	return true;
}

bool C4Game::ToggleChart()
{
	C4ChartDialog::Toggle();
	return true;
}

void C4Game::Abort(bool fApproved)
{
	// league needs approval
	if (Network.isEnabled() && Parameters.isLeague() && !fApproved)
	{
		if (Control.isCtrlHost() && !Game.GameOver)
		{
			Network.Vote(VT_Cancel);
			return;
		}
		if (!Control.isCtrlHost() && !Game.GameOver && ::Players.GetLocalByIndex(0))
		{
			Network.Vote(VT_Kick, true, Control.ClientID());
			return;
		}
	}
	// hard-abort: eval league and quit
	// manually evaluate league
	Players.RemoveLocal(true, true);
	Players.RemoveAtRemoteClient(true, true);
	// normal quit
	Application.QuitGame();
}

static const std::unordered_map<std::string, C4GUI::Icons> str_to_icon =
{
	{ "Locked",      C4GUI::Ico_Ex_LockedFrontal },
	{ "League",      C4GUI::Ico_Ex_League        },
	{ "GameRunning", C4GUI::Ico_GameRunning      },
	{ "Lobby",       C4GUI::Ico_Lobby            },
	{ "RuntimeJoin", C4GUI::Ico_RuntimeJoin      },

	{ "A",             C4GUI::Ico_Controller_A             },
	{ "B",             C4GUI::Ico_Controller_B             },
	{ "X",             C4GUI::Ico_Controller_X             },
	{ "Y",             C4GUI::Ico_Controller_Y             },
	{ "Back",          C4GUI::Ico_Controller_Back          },
	{ "Start",         C4GUI::Ico_Controller_Start         },
	{ "Dpad",          C4GUI::Ico_Controller_Dpad          },
	{ "DpadLeft",      C4GUI::Ico_Controller_DpadLeft      },
	{ "DpadRight",     C4GUI::Ico_Controller_DpadRight     },
	{ "DpadDown",      C4GUI::Ico_Controller_DpadDown      },
	{ "DpadUp",        C4GUI::Ico_Controller_DpadUp        },
	{ "LeftShoulder",  C4GUI::Ico_Controller_LeftShoulder  },
	{ "RightShoulder", C4GUI::Ico_Controller_RightShoulder },
	{ "LeftTrigger",   C4GUI::Ico_Controller_LeftTrigger   },
	{ "RightTrigger",  C4GUI::Ico_Controller_RightTrigger  },
	{ "LeftStick",     C4GUI::Ico_Controller_LeftStick     },
	{ "RightStick",    C4GUI::Ico_Controller_RightStick    },
};

bool GetTextSpecFacet(const char* szSpec, C4Facet& fct)
{
	// safety
	assert(szSpec && *szSpec);
	if (!szSpec) return false;
	// Special icon?
	if (SEqual2(szSpec, "@Ico:"))
	{
		szSpec += 5;
		auto it = str_to_icon.find(szSpec);
		if (it != str_to_icon.end())
		{
			fct = C4GUI::Icon::GetIconFacet(it->second);
			return true;
		}
	}

	return false;
}

bool C4Game::DrawTextSpecImage(C4Facet &fctTarget, const char *szSpec, C4DrawTransform* pTransform, uint32_t dwClr)
{
	// safety
	assert(szSpec && *szSpec);
	if (!szSpec) return false;

	C4Facet fctSource;
	if(GetTextSpecFacet(szSpec, fctSource))
	{
		fctSource.DrawXT(fctTarget.Surface, fctTarget.X, fctTarget.Y, fctTarget.Wdt, fctTarget.Hgt, 0, 0, pTransform);
		return true;
	}
	else
	{
		C4Def *pDef = C4Id2Def(C4ID(szSpec));
		if (!pDef) return false;

		pDef->Draw(fctTarget, false, dwClr, nullptr, 0, 0, pTransform);
		return true;
	}
}

float C4Game::GetTextSpecImageAspect(const char* szSpec)
{
	// safety
	assert(szSpec && *szSpec);
	if (!szSpec) return -1.0f;

	C4Facet fctSource;
	if(GetTextSpecFacet(szSpec, fctSource))
	{
		return static_cast<float>(fctSource.Wdt) / static_cast<float>(fctSource.Hgt);
	}
	else
	{
		C4Def *pDef = C4Id2Def(C4ID(szSpec));
		if (!pDef) return -1.0f;

		C4DefGraphics* pGfx = &pDef->Graphics;
		if(pGfx->Type == C4DefGraphics::TYPE_Bitmap)
		{
			return static_cast<float>(pDef->PictureRect.Wdt) / static_cast<float>(pDef->PictureRect.Hgt);
		}
		else if (pGfx->Type == C4DefGraphics::TYPE_Mesh)
		{
			const StdMesh& mesh = *pGfx->Mesh;
			const StdMeshBox& box = mesh.GetBoundingBox();
			return (box.x2 - box.x1) / (box.y2 - box.y1);
		}

		return -1.0f;
	}
}

bool C4Game::DrawPropListSpecImage(C4Facet &fctTarget, C4PropList *pSpec)
{
	// safety
	assert(pSpec);
	if (!pSpec) return false;

	// get source definition
	C4PropList *source_def_proplist = pSpec->GetPropertyPropList(P_Source);
	if (!source_def_proplist) return false;
	C4Def *source_def = source_def_proplist->GetDef();
	if (!source_def) return false;

	// get custom color
	uint32_t color = (uint32_t)pSpec->GetPropertyInt(P_Color);
	
	C4String *source_name = pSpec->GetPropertyStr(P_Name);
	if (!source_name)
	{
		// Base graphics
		source_def->Draw(fctTarget, false, color);
	}
	else
	{
		// Alternative named graphics
		C4DefGraphics *source_graphics = source_def->Graphics.Get(source_name->GetCStr());
		if (!source_graphics) return false;
		source_graphics->Draw(fctTarget, color, nullptr, 0,0, nullptr);
	}
	return true;
}

bool C4Game::SpeedUp()
{
	// As these functions work stepwise, there's the old maximum speed of 50.
	// Use /fast to set to even higher speeds.
	FrameSkip = Clamp<int32_t>(FrameSkip + 1, 1, 50);
	FullSpeed = true;
	GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_SPEED"), FrameSkip).getData());
	return true;
}

bool C4Game::SlowDown()
{
	FrameSkip = Clamp<int32_t>(FrameSkip - 1, 1, 50);
	if (FrameSkip == 1)
		FullSpeed = false;
	GraphicsSystem.FlashMessage(FormatString(LoadResStr("IDS_MSG_SPEED"), FrameSkip).getData());
	return true;
}

bool C4Game::ToggleChat()
{
	return C4ChatDlg::ToggleChat();
}

C4Value C4Game::GRBroadcast(const char *szFunction, C4AulParSet *pPars, bool fPassError, bool fRejectTest)
{
	std::string func{ szFunction };
	if (func[0] != '~')
		func.insert(0, 1, '~');

	// call objects first - scenario script might overwrite hostility, etc...
	C4Value vResult = ::Objects.GRBroadcast(func.c_str(), pPars, fPassError, fRejectTest);
	// rejection tests abort on first nonzero result
	if (fRejectTest) if (!!vResult) return vResult;
	// scenario script call
	return ::GameScript.Call(func.c_str(), pPars, fPassError);
}

void C4Game::SetDefaultGamma()
{
	// Skip this if graphics haven't been initialized yet (happens when
	// we bail during initialization)
	if (!pDraw) return;
	// Default gamma
	pDraw->ResetGamma();
	pDraw->SetGamma(float(Config.Graphics.Gamma) / 100.0,
					float(Config.Graphics.Gamma) / 100.0,
					float(Config.Graphics.Gamma) / 100.0,
					C4MaxGammaUserRamps);
}

void C4Game::SetGlobalSoundModifier(C4PropList *new_modifier)
{
	// set in prop list (for savegames) and in sound system::
	C4SoundModifier *mod;
	if (new_modifier)
	{
		GlobalSoundModifier.SetPropList(new_modifier);
		mod = ::Application.SoundSystem.Modifiers.Get(new_modifier, true);
	}
	else
	{
		GlobalSoundModifier.Set0();
		mod = nullptr;
	}
	::Application.SoundSystem.Modifiers.SetGlobalModifier(mod, NO_OWNER);
}

C4String *C4Game::GetTranslatedString(const C4Value &input_string, C4Value *selected_language, bool fail_silently) const
{
	// Resolve a localized string
	// If a string is passed, just return it
	// If a proplist like { DE="Hallo, Welt!", US="Hello, world!" } is passed, return the string matching the selected language
	// Nothing?
	if (input_string.GetType() == C4V_Nil)
	{
		return nullptr;
	}
	// Non-localized string?
	if (input_string.GetType() == C4V_String)
	{
		return input_string._getStr();
	}
	// Invalid type for this function?
	C4PropList *p = input_string._getPropList();
	if (!p || p->GetPropertyStr(P_Function) != &::Strings.P[P_Translate])
	{
		if (fail_silently)
		{
			return nullptr;
		}
		else
		{
			throw C4AulExecError(FormatString("Invalid value for translation: %s", input_string.GetDataString().getData()).getData());
		}
	}
	// This is a proplist. Resolve the language as the key.
	char lang_code[3] = "";
	for (int32_t lang_index = 0; SCopySegment(Config.General.LanguageEx, lang_index, lang_code, ',', 2); ++lang_index)
	{
		C4String *lang_string = ::Strings.FindString(lang_code);
		if (lang_string) // If the string is not found, it cannot be the key in a prop list
		{
			C4Value localized_string_val;
			if (p->GetPropertyByS(lang_string, &localized_string_val))
			{
				C4String *localized_string = localized_string_val.getStr();
				if (localized_string)
				{
					// Found it!
					if (selected_language)
					{
						selected_language->SetString(lang_string);
					}
					return localized_string;
				}
			}
		}
	}
	// No language matched. Just use any property and assume it's a language key.
	for (C4String *lang_string : p->GetSortedLocalProperties(false))
	{
		C4Value localized_string_val;
		if (p->GetPropertyByS(lang_string, &localized_string_val))
		{
			C4String *localized_string = localized_string_val.getStr();
			if (localized_string)
			{
				// Found it!
				if (selected_language)
				{
					selected_language->SetString(lang_string);
				}
				return localized_string;
			}
		}
	}
	// No string properties. There's no localized information to be found.
	return nullptr;
}

C4PropList *C4Game::AllocateTranslatedString()
{
	C4PropListScript *value_proplist = new C4PropListScript();
	value_proplist->SetProperty(P_Function, C4VString(&::Strings.P[P_Translate]));
	return value_proplist;
}
