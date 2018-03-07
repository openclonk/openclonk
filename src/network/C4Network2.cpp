/*
 * OpenClonk, http://www.openclonk.org
 *
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

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "network/C4Network2.h"

#include "C4Version.h"
#include "control/C4GameControl.h"
#include "control/C4GameSave.h"
#include "control/C4RoundResults.h"
#include "editor/C4Console.h"
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"

// lobby
#include "gui/C4GameLobby.h"

#include "network/C4Network2Dialogs.h"
#include "network/C4League.h"

#ifdef _WIN32
#include <direct.h>
#endif
#ifndef HAVE_WINSOCK
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// compile options
#ifdef _MSC_VER
#pragma warning (disable: 4355)
#endif

// *** C4Network2Status

C4Network2Status::C4Network2Status() = default;

const char *C4Network2Status::getStateName() const
{
	switch (eState)
	{
	case GS_None: return "none";
	case GS_Init: return "init";
	case GS_Lobby: return "lobby";
	case GS_Pause: return "pause";
	case GS_Go: return "go";
	}
	return "???";
}

const char *C4Network2Status::getDescription() const
{
	switch (eState)
	{
	case GS_None: return LoadResStr("IDS_DESC_NOTINITED");
	case GS_Init: return LoadResStr("IDS_DESC_WAITFORHOST");
	case GS_Lobby: return LoadResStr("IDS_DESC_EXPECTING");
	case GS_Pause: return LoadResStr("IDS_DESC_GAMEPAUSED");
	case GS_Go: return LoadResStr("IDS_DESC_GAMERUNNING");
	}
	return LoadResStr("IDS_DESC_UNKNOWNGAMESTATE");
}

void C4Network2Status::Set(C4NetGameState enState, int32_t inTargetTick)
{
	eState = enState; iTargetCtrlTick = inTargetTick;
}

void C4Network2Status::SetCtrlMode(int32_t inCtrlMode)
{
	iCtrlMode = inCtrlMode;
}

void C4Network2Status::SetTargetTick(int32_t inTargetCtrlTick)
{
	iTargetCtrlTick = inTargetCtrlTick;
}

void C4Network2Status::Clear()
{
	eState = GS_None; iTargetCtrlTick = -1;
}

void C4Network2Status::CompileFunc(StdCompiler *pComp)
{
	CompileFunc(pComp, false);
}

void C4Network2Status::CompileFunc(StdCompiler *pComp, bool fReference)
{
	StdEnumEntry<C4NetGameState> GameStates[] =
	{
		{ "None",         GS_None     },
		{ "Init",         GS_Init     },
		{ "Lobby",        GS_Lobby    },
		{ "Paused",       GS_Pause    },
		{ "Running",      GS_Go       },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eState, GameStates), "State", GS_None));
	pComp->Value(mkNamingAdapt(mkIntPackAdapt(iCtrlMode), "CtrlMode", -1));

	if (!fReference)
		pComp->Value(mkNamingAdapt(mkIntPackAdapt(iTargetCtrlTick), "TargetTick", -1));
}

// *** C4Network2

C4Network2::C4Network2()
		: Clients(&NetIO),
		tLastActivateRequest(C4TimeMilliseconds::NegativeInfinity),
		NetpuncherGameID(C4NetpuncherID())
{

}

C4Network2::~C4Network2()
{
	Clear();
}

bool C4Network2::InitHost(bool fLobby)
{
	if (isEnabled()) Clear();
	// initialize everything
	Status.Set(fLobby ? GS_Lobby : GS_Go, ::Control.ControlTick);
	Status.SetCtrlMode(Config.Network.ControlMode);
	fHost = true;
	fStatusAck = fStatusReached = true;
	fChasing = false;
	fAllowJoin = false;
	iNextClientID = C4ClientIDStart;
	NetpuncherGameID = C4NetpuncherID();
	NetpuncherAddr = ::Config.Network.PuncherAddress;
	// initialize client list
	Clients.Init(&Game.Clients, true);
	// initialize resource list
	if (!ResList.Init(Game.Clients.getLocalID(), &NetIO))
		{ LogFatal("Network: failed to initialize resource list!"); Clear(); return false; }
	if (!Game.Parameters.InitNetwork(&ResList))
		return false;
	// create initial dynamic
	if (!CreateDynamic(true))
		return false;
	// initialize net i/o
	if (!InitNetIO(false, true))
		{ Clear(); return false; }
	// init network control
	pControl = &::Control.Network;
	pControl->Init(C4ClientIDHost, true, ::Control.getNextControlTick(), true, this);
	// init league
	bool fCancel = true;
	if (!InitLeague(&fCancel) || !LeagueStart(&fCancel))
	{
		// deinit league
		DeinitLeague();
		// user cancelled?
		if (fCancel)
			return false;
		// in console mode, bail out
#ifdef USE_CONSOLE
		return false;
#endif
	}
	// allow connect
	NetIO.SetAcceptMode(true);
	// timer
	Application.Add(this);
	// ok
	return true;
}

// Orders connection addresses to optimize joining.
static void SortAddresses(std::vector<C4Network2Address>& addrs)
{
	// TODO: Maybe use addresses from local client to avoid the extra system calls.
	auto localAddrs = C4NetIO::GetLocalAddresses();
	bool haveIPv6 = false;
	for (auto& addr : localAddrs)
	{
		if (addr.GetFamily() == C4NetIO::HostAddress::IPv6 && !addr.IsLocal() && !addr.IsPrivate())
		{
			haveIPv6 = true;
			break;
		}
	}

	auto rank = [&](const C4Network2Address& Addr)
	{
		// Rank addresses. For IPv6-enabled clients, try public IPv6 addresses first, then IPv4,
		// then link-local IPv6. For IPv4-only clients, skip IPv6.
		int rank = 0;
		auto addr = Addr.getAddr();
		switch (addr.GetFamily())
		{
		case C4NetIO::HostAddress::IPv6:
			if (addr.IsLocal())
				rank = 100;
			else if (addr.IsPrivate())
				rank = 150;
			else if (haveIPv6)
				// TODO: Rank public IPv6 addresses by longest matching prefix with local addresses.
				rank = 300;
			break;
		case C4NetIO::HostAddress::IPv4:
			if (addr.IsPrivate())
				rank = 150;
			else
				rank = 200;
			break;
		default:
			assert(!"Unexpected address family");
		}
		return rank;
	};

	// Sort by decreasing rank. Use stable sort to allow the host to prioritize addresses within a family.
	std::stable_sort(addrs.begin(), addrs.end(), [&](auto a, auto b) { return rank(a) > rank(b); });
}

C4Network2::InitResult C4Network2::InitClient(const C4Network2Reference &Ref, bool fObserver)
{
	if (isEnabled()) Clear();
	// Get host core
	const C4ClientCore &HostCore = Ref.Parameters.Clients.getHost()->getCore();
	// host core revision check
	if (!SEqualNoCase(HostCore.getRevision(), Application.GetRevision()))
	{
		StdStrBuf msg;
		msg.Format(LoadResStr("IDS_NET_ERR_VERSIONMISMATCH"), HostCore.getRevision(), Application.GetRevision());
		if (!Application.isEditor)
		{
			if (!pGUI->ShowMessageModal(msg.getData(), "[!]Network warning", C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Notify, nullptr /* do not allow to skip this message! */))
				return IR_Fatal;
		}
		else
		{
			Log(msg.getData());
		}
	}
	// repeat if wrong password
	fWrongPassword = Ref.isPasswordNeeded();
	NetpuncherGameID = Ref.getNetpuncherGameID();
	NetpuncherAddr = Ref.getNetpuncherAddr();
	StdStrBuf Password;

	// copy addresses
	std::vector<C4Network2Address> Addrs;
	for (int i = 0; i < Ref.getAddrCnt(); i++)
	{
		C4Network2Address a = Ref.getAddr(i);
		a.getAddr().SetScopeId(Ref.GetSourceAddress().GetScopeId());
		Addrs.push_back(std::move(a));
	}
	SortAddresses(Addrs);
	for (;;)
	{
		// ask for password (again)?
		if (fWrongPassword)
		{
			Password.Take(QueryClientPassword());
			if (!Password.getLength())
				return IR_Error;
			fWrongPassword = false;
		}
		// Try to connect to host
		if (InitClient(Addrs, HostCore, Password.getData()) == IR_Fatal)
			return IR_Fatal;
		// success?
		if (isEnabled())
			break;
		// Retry only for wrong password
		if (!fWrongPassword)
		{
			LogSilent("Network: Could not connect!");
			return IR_Error;
		}
	}
	// initialize resources
	if (!Game.Parameters.InitNetwork(&ResList))
		return IR_Fatal;
	// init league
	if (!InitLeague(nullptr))
	{
		// deinit league
		DeinitLeague();
		return IR_Fatal;
	}
	// allow connect
	NetIO.SetAcceptMode(true);
	// timer
	Application.Add(this);
	// ok, success
	return IR_Success;
}

C4Network2::InitialConnect::InitialConnect(const std::vector<C4Network2Address>& Addrs, const C4ClientCore& HostCore, const char *Password)
	: CStdTimerProc(DELAY), Addrs(Addrs), CurrentAddr(this->Addrs.cbegin()),
	  HostCore(HostCore), Password(Password)
{
	Application.Add(this);
}

C4Network2::InitialConnect::~InitialConnect()
{
	Done();
}

bool C4Network2::InitialConnect::Execute(int, pollfd *)
{
	if (CheckAndReset())
		TryNext();
	return true;
}

void C4Network2::InitialConnect::TryNext()
{
	StdStrBuf strAddresses; int Successes = 0;
	for (; Successes < ADDR_PER_TRY && CurrentAddr != Addrs.cend(); ++CurrentAddr)
	{
		if (!CurrentAddr->isIPNull())
		{
			auto addr = CurrentAddr->getAddr();
			std::vector<C4NetIO::addr_t> addrs;
			if (addr.IsLocal())
			{
				// Local IPv6 addresses need a scope id.
				for (auto& id : Network.Clients.GetLocal()->getInterfaceIDs())
				{
					addr.SetScopeId(id);
					addrs.push_back(addr);
				}
			}
			else
				addrs.push_back(addr);
			// connection
			int cnt = 0;
			for (auto& a : addrs)
				if (Network.NetIO.Connect(a, CurrentAddr->getProtocol(), HostCore, Password))
					cnt++;
			if (cnt == 0) continue;
			// format for message
			if (strAddresses.getLength())
				strAddresses.Append(", ");
			strAddresses.Append(CurrentAddr->toString());
			Successes++;
		}
	}
	if (Successes > 0)
	{
		LogF(LoadResStr("IDS_NET_CONNECTHOST"), strAddresses.getData());
	}
	else
	{
		Done();
	}
}

void C4Network2::InitialConnect::Done()
{
	Application.Remove(this);
}

C4Network2::InitResult C4Network2::InitClient(const std::vector<class C4Network2Address>& Addrs, const C4ClientCore &HostCore, const char *szPassword)
{
	// initialization
	Status.Set(GS_Init, -1);
	fHost = false;
	fStatusAck = fStatusReached = true;
	fChasing = true;
	fAllowJoin = false;
	// initialize client list
	Game.Clients.Init(C4ClientIDUnknown);
	Clients.Init(&Game.Clients, false);
	// initialize resource list
	if (!ResList.Init(Game.Clients.getLocalID(), &NetIO))
		{ LogFatal(LoadResStr("IDS_NET_ERR_INITRESLIST")); Clear(); return IR_Fatal; }
	// initialize net i/o
	if (!InitNetIO(true, false))
		{ Clear(); return IR_Fatal; }
	// set network control
	pControl = &::Control.Network;
	// set exclusive connection mode
	NetIO.SetExclusiveConnMode(true);
	// warm up netpuncher
	InitPuncher();
	// try to connect host
	InitialConnect iconn(Addrs, HostCore, szPassword);
	// show box
	std::unique_ptr<C4GUI::MessageDialog> pDlg = nullptr;
	if (!Application.isEditor)
	{
		StdStrBuf strMessage = FormatString(LoadResStr("IDS_NET_JOINGAMEBY"), HostCore.getName());
		// create & show
		pDlg = std::make_unique<C4GUI::MessageDialog>(
				strMessage.getData(), LoadResStr("IDS_NET_JOINGAME"),
				C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsRegular);
		if (!pDlg->Show(::pGUI, true)) { Clear(); return IR_Fatal; }
	}
	// wait for connect / timeout / abort by user (host will change status on succesful connect)
	while (Status.getState() == GS_Init)
	{
		if (!Application.ScheduleProcs(100))
			{ return IR_Fatal;}
		if (pDlg && pDlg->IsAborted())
			{ return IR_Fatal; }
	}
	// error?
	if (!isEnabled())
		return IR_Error;
	// deactivate exclusive connection mode
	NetIO.SetExclusiveConnMode(false);
	return IR_Success;
}

bool C4Network2::DoLobby()
{
	// shouldn't do lobby?
	if (!isEnabled() || (!isHost() && !isLobbyActive()))
		return true;

	// lobby runs
	fLobbyRunning = true;
	fAllowJoin = true;
	Log(LoadResStr("IDS_NET_LOBBYWAITING"));

	// client: lobby status reached, message to host
	if (!isHost())
		CheckStatusReached();
	// host: set lobby mode
	else
		ChangeGameStatus(GS_Lobby, 0);

	// determine lobby type
	if (Console.Active)
	{
		// console lobby - update console
		Console.UpdateMenus();
		// init lobby countdown if specified
		if (Game.iLobbyTimeout) StartLobbyCountdown(Game.iLobbyTimeout);
		// do console lobby
		while (isLobbyActive())
			if (!Application.ScheduleProcs())
				{ Clear(); return false; }
	}
	else
	{
		// fullscreen lobby

		// init lobby dialog
		pLobby = new C4GameLobby::MainDlg(isHost());
		if (!pLobby->FadeIn(::pGUI)) { delete pLobby; pLobby = nullptr; Clear(); return false; }

		// init lobby countdown if specified
		if (Game.iLobbyTimeout) StartLobbyCountdown(Game.iLobbyTimeout);

		// while state lobby: keep looping
		while (isLobbyActive() && pLobby && pLobby->IsShown())
			if (!Application.ScheduleProcs())
				{ Clear(); return false; }

		// check whether lobby was aborted
		if (pLobby && pLobby->IsAborted()) { delete pLobby; pLobby = nullptr; Clear(); return false; }

		// deinit lobby
		if (pLobby && pLobby->IsShown()) pLobby->Close(true);
		delete pLobby; pLobby = nullptr;

		// close any other dialogs
		::pGUI->CloseAllDialogs(false);
	}

	// lobby end
	delete pLobbyCountdown; pLobbyCountdown = nullptr;
	fLobbyRunning = false;
	fAllowJoin = !Config.Network.NoRuntimeJoin;

	// notify user that the lobby has ended (for people who tasked out)
	Application.NotifyUserIfInactive();

	// notify lobby end
	bool fGameGo = isEnabled();
	if (fGameGo) Log(LoadResStr("IDS_PRC_GAMEGO"));;

	// disabled?
	return fGameGo;
}

bool C4Network2::Start()
{
	if (!isEnabled() || !isHost()) return false;
	// change mode: go
	ChangeGameStatus(GS_Go, ::Control.ControlTick);
	return true;
}

bool C4Network2::Pause()
{
	if (!isEnabled() || !isHost()) return false;
	// change mode: pause
	return ChangeGameStatus(GS_Pause, ::Control.getNextControlTick());
}

bool C4Network2::Sync()
{
	// host only
	if (!isEnabled() || !isHost()) return false;
	// already syncing the network?
	if (!fStatusAck)
	{
		// maybe we are already sync?
		if (fStatusReached) CheckStatusAck();
		return true;
	}
	// already sync?
	if (isFrozen()) return true;
	// ok, so let's do a sync: change in the same state we are already in
	return ChangeGameStatus(Status.getState(), ::Control.getNextControlTick());
}

bool C4Network2::FinalInit()
{
	// check reach
	CheckStatusReached(true);
	// reached, waiting for ack?
	if (fStatusReached && !fStatusAck)
	{
		// wait for go acknowledgement
		Log(LoadResStr("IDS_NET_JOINREADY"));

		// any pending keyboard commands should not be routed to cancel the wait dialog - flush the message queue!
		if (!Application.FlushMessages()) return false;

		// show box
		C4GUI::Dialog *pDlg = nullptr;
		if (!Application.isEditor)
		{
			// separate dlgs for host/client
			if (isHost())
				pDlg = new C4Network2StartWaitDlg();
			else
				pDlg = new C4GUI::MessageDialog(LoadResStr("IDS_NET_WAITFORSTART"), LoadResStr("IDS_NET_CAPTION"),
				                                C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsSmall);
			// show it
			if (!pDlg->Show(::pGUI, true)) return false;
		}

		// wait for acknowledgement
		while (fStatusReached && !fStatusAck)
		{
			if (pDlg)
			{
				// execute
				if (!pDlg->Execute()) { delete pDlg; Clear(); return false; }
				// aborted?
				if (pDlg->IsAborted()) { delete pDlg; Clear(); return false; }
			}
			else if (!Application.ScheduleProcs())
				{ Clear(); return false; }
		}
		delete pDlg;
		// log
		Log(LoadResStr("IDS_NET_START"));
	}
	// synchronize
	Game.SyncClearance();
	Game.Synchronize(false);
	// finished
	return isEnabled();
}


bool C4Network2::RetrieveScenario(char *szScenario)
{
	// client only
	if (isHost()) return false;

	// wait for scenario
	C4Network2Res::Ref pScenario = RetrieveRes(*Game.Parameters.Scenario.getResCore(),
	                               C4NetResRetrieveTimeout, LoadResStr("IDS_NET_RES_SCENARIO"));
	if (!pScenario)
		return false;

	// wait for dynamic data
	C4Network2Res::Ref pDynamic = RetrieveRes(ResDynamic, C4NetResRetrieveTimeout, LoadResStr("IDS_NET_RES_DYNAMIC"));
	if (!pDynamic)
		return false;

	// create unpacked copy of scenario
	if (!ResList.FindTempResFileName(FormatString("Combined%d.ocs", Game.Clients.getLocalID()).getData(), szScenario) ||
	    !C4Group_CopyItem(pScenario->getFile(), szScenario) ||
	    !C4Group_UnpackDirectory(szScenario))
		return false;

	// create unpacked copy of dynamic data
	char szTempDynamic[_MAX_PATH + 1];
	if (!ResList.FindTempResFileName(pDynamic->getFile(), szTempDynamic) ||
	    !C4Group_CopyItem(pDynamic->getFile(), szTempDynamic) ||
	    !C4Group_UnpackDirectory(szTempDynamic))
		return false;

	// unpack Material.ocg if materials need to be merged
	StdStrBuf MaterialScenario, MaterialDynamic;
	MaterialScenario.Format("%s" DirSep  C4CFN_Material, szScenario);
	MaterialDynamic.Format("%s" DirSep  C4CFN_Material, szTempDynamic);
	if (FileExists(MaterialScenario.getData()) && FileExists(MaterialDynamic.getData()))
		if (!C4Group_UnpackDirectory(MaterialScenario.getData()) ||
		    !C4Group_UnpackDirectory(MaterialDynamic.getData()))
			return false;

	// move all dynamic files to scenario
	C4Group ScenGrp;
	if (!ScenGrp.Open(szScenario) ||
	    !ScenGrp.Merge(szTempDynamic))
		return false;
	ScenGrp.Close();

	// remove dynamic temp file
	EraseDirectory(szTempDynamic);

	// remove dynamic - isn't needed any more and will soon be out-of-date
	pDynamic->Remove();

	return true;
}

void C4Network2::OnSec1Timer()
{
	Execute();
}

void C4Network2::Execute()
{

	// client connections
	Clients.DoConnectAttempts();

	// status reached?
	CheckStatusReached();

	if (isHost())
	{
		// remove dynamic
		if (!ResDynamic.isNull() && ::Control.ControlTick > iDynamicTick)
			RemoveDynamic();
		// Set chase target
		UpdateChaseTarget();
		// check for inactive clients and deactivate them
		DeactivateInactiveClients();
		// reference
		if (!iLastReferenceUpdate || time(nullptr) > (time_t) (iLastReferenceUpdate + C4NetReferenceUpdateInterval))
			if (NetIO.IsReferenceNeeded())
			{
				// create
				C4Network2Reference *pRef = new C4Network2Reference();
				pRef->InitLocal();
				// set
				NetIO.SetReference(pRef);
				iLastReferenceUpdate = time(nullptr);
			}
		// league server reference
		if (!iLastLeagueUpdate || time(nullptr) > (time_t) (iLastLeagueUpdate + iLeagueUpdateDelay))
		{
			LeagueUpdate();
		}
		// league update reply receive
		if (pLeagueClient && fHost && !pLeagueClient->isBusy() && pLeagueClient->getCurrentAction() == C4LA_Update)
		{
			LeagueUpdateProcessReply();
		}
		// voting timeout
		if (Votes.firstPkt() && time(nullptr) > (time_t) (iVoteStartTime + C4NetVotingTimeout))
		{
			C4ControlVote *pVote = static_cast<C4ControlVote *>(Votes.firstPkt()->getPkt());
			::Control.DoInput(
			  CID_VoteEnd,
			  new C4ControlVoteEnd(pVote->getType(), false, pVote->getData()),
			  CDT_Sync);
			iVoteStartTime = time(nullptr);
		}
		// record streaming
		if (fStreaming)
		{
			StreamIn(false);
			StreamOut();
		}
	}
	else
	{
		// request activate, if neccessary
		if (!tLastActivateRequest.IsInfinite()) RequestActivate();
	}
}

void C4Network2::Clear()
{
	// stop timer
	Application.Remove(this);
	// stop streaming
	StopStreaming();
	// clear league
	if (pLeagueClient)
	{
		LeagueEnd();
		DeinitLeague();
	}
	// stop lobby countdown
	delete pLobbyCountdown; pLobbyCountdown = nullptr;
	// cancel lobby
	delete pLobby; pLobby = nullptr;
	fLobbyRunning = false;
	// deactivate
	Status.Clear();
	fStatusAck = fStatusReached = true;
	// if control mode is network: change to local
	if (::Control.isNetwork())
		::Control.ChangeToLocal();
	// clear all player infos
	Players.Clear();
	// remove all clients
	Clients.Clear();
	// close net classes
	NetIO.Clear();
	// clear resources
	ResList.Clear();
	// clear password
	sPassword.Clear();
	// stuff
	fAllowJoin = false;
	iDynamicTick = -1; fDynamicNeeded = false;
	tLastActivateRequest = C4TimeMilliseconds::NegativeInfinity;
	iLastChaseTargetUpdate = iLastReferenceUpdate = iLastLeagueUpdate = 0;
	fDelayedActivateReq = false;
	delete pVoteDialog; pVoteDialog = nullptr;
	fPausedForVote = false;
	iLastOwnVoting = 0;
	NetpuncherGameID = C4NetpuncherID();
	Votes.Clear();
	// don't clear fPasswordNeeded here, it's needed by InitClient
}

bool C4Network2::ToggleAllowJoin()
{
	// just toggle
	AllowJoin(!fAllowJoin);
	return true; // toggled
}

bool C4Network2::ToggleClientListDlg()
{
	C4Network2ClientListDlg::Toggle();
	return true;
}

void C4Network2::SetPassword(const char *szToPassword)
{
	bool fHadPassword = isPassworded();
	// clear password?
	if (!szToPassword || !*szToPassword)
		sPassword.Clear();
	else
		// no? then set it
		sPassword.Copy(szToPassword);
	// if the has-password-state has changed, the reference is invalidated
	if (fHadPassword != isPassworded()) InvalidateReference();
}

StdStrBuf C4Network2::QueryClientPassword()
{
	// ask client for a password; return nothing if user canceled
	StdStrBuf sCaption; sCaption.Copy(LoadResStr("IDS_MSG_ENTERPASSWORD"));
	C4GUI::InputDialog *pInputDlg = new C4GUI::InputDialog(LoadResStr("IDS_MSG_ENTERPASSWORD"), sCaption.getData(), C4GUI::Ico_Ex_Locked, nullptr, false);
	pInputDlg->SetDelOnClose(false);
	if (!::pGUI->ShowModalDlg(pInputDlg, false))
	{
		delete pInputDlg;
		return StdStrBuf();
	}
	// copy to buffer
	StdStrBuf Buf; Buf.Copy(pInputDlg->GetInputText());
	delete pInputDlg;
	return Buf;
}

void C4Network2::AllowJoin(bool fAllow)
{
	if (!isHost()) return;
	fAllowJoin = fAllow;
	if (Game.IsRunning)
	{
		::GraphicsSystem.FlashMessage(LoadResStr(fAllowJoin ? "IDS_NET_RUNTIMEJOINFREE" : "IDS_NET_RUNTIMEJOINBARRED"));
		Config.Network.NoRuntimeJoin = !fAllowJoin;
	}
}

void C4Network2::SetAllowObserve(bool fAllow)
{
	if (!isHost()) return;
	fAllowObserve = fAllow;
}

void C4Network2::SetCtrlMode(int32_t iCtrlMode)
{
	if (!isHost()) return;
	// no change?
	if (iCtrlMode == Status.getCtrlMode()) return;
	// change game status
	ChangeGameStatus(Status.getState(), ::Control.ControlTick, iCtrlMode);
}

void C4Network2::OnConn(C4Network2IOConnection *pConn)
{
	// Nothing to do atm... New pending connections are managed mainly by C4Network2IO
	// until they are accepted, see PID_Conn/PID_ConnRe handlers in HandlePacket.

	// Note this won't get called anymore because of this (see C4Network2IO::OnConn)
}

void C4Network2::OnDisconn(C4Network2IOConnection *pConn)
{
	// could not establish host connection?
	if (Status.getState() == GS_Init && !isHost())
	{
		if (!NetIO.getConnectionCount())
			Clear();
		return;
	}

	// connection failed?
	if (pConn->isFailed())
	{
		// call handler
		OnConnectFail(pConn);
		return;
	}

	// search client
	C4Network2Client *pClient = Clients.GetClient(pConn);
	// not found? Search by ID (not associated yet, half-accepted connection)
	if (!pClient) pClient = Clients.GetClientByID(pConn->getClientID());
	// not found? ignore
	if (!pClient) return;
	// remove connection
	pClient->RemoveConn(pConn);

	// create post-mortem if needed
	C4PacketPostMortem PostMortem;
	if (pConn->CreatePostMortem(&PostMortem))
	{
		LogSilentF("Network: Sending %d packets for recovery (%d-%d)", PostMortem.getPacketCount(), pConn->getOutPacketCounter() - PostMortem.getPacketCount(), pConn->getOutPacketCounter() - 1);
		// This might fail because of this disconnect
		// (If it's the only host connection. We're toast then anyway.)
		if (!Clients.SendMsgToClient(pConn->getClientID(), MkC4NetIOPacket(PID_PostMortem, PostMortem)))
			assert(isHost() || !Clients.GetHost()->isConnected());
	}

	// call handler
	OnDisconnect(pClient, pConn);

}

void C4Network2::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn)
{
	// find associated client
	C4Network2Client *pClient = Clients.GetClient(pConn);
	if (!pClient) pClient = Clients.GetClientByID(pConn->getClientID());

	// local? ignore
	if (pClient && pClient->isLocal()) { pConn->Close(); return; }

#define GETPKT(type, name) \
    assert(pPacket); const type &name = \
     static_cast<const type &>(*pPacket);

	switch (cStatus)
	{
	case PID_Conn: // connection request
	{
		if (!pConn->isOpen()) break;
		GETPKT(C4PacketConn, rPkt);
		HandleConn(rPkt, pConn, pClient);
	}
	break;

	case PID_ConnRe: // connection request reply
	{
		GETPKT(C4PacketConnRe, rPkt);
		HandleConnRe(rPkt, pConn, pClient);
	}
	break;

	case PID_JoinData:
	{
		// host->client only
		if (isHost() || !pClient || !pClient->isHost()) break;
		if (!pConn->isOpen()) break;
		// handle
		GETPKT(C4PacketJoinData, rPkt)
		HandleJoinData(rPkt);
	}
	break;

	case PID_Status: // status change
	{
		// by host only
		if (isHost() || !pClient || !pClient->isHost()) break;
		if (!pConn->isOpen()) break;
		// must be initialized
		if (Status.getState() == GS_Init) break;
		// handle
		GETPKT(C4Network2Status, rPkt);
		HandleStatus(rPkt);
	}
	break;

	case PID_StatusAck: // status change acknowledgement
	{
		// host->client / client->host only
		if (!pClient) break;
		if (!isHost() && !pClient->isHost()) break;
		// must be initialized
		if (Status.getState() == GS_Init) break;
		// handle
		GETPKT(C4Network2Status, rPkt);
		HandleStatusAck(rPkt, pClient);
	}
	break;

	case PID_ClientActReq: // client activation request
	{
		// client->host only
		if (!isHost() || !pClient || pClient->isHost()) break;
		// must be initialized
		if (Status.getState() == GS_Init) break;
		// handle
		GETPKT(C4PacketActivateReq, rPkt)
		HandleActivateReq(rPkt.getTick(), pClient);
	}
	break;

	}

#undef GETPKT
}

void C4Network2::HandleLobbyPacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2IOConnection *pConn)
{
	// find associated client
	C4Network2Client *pClient = Clients.GetClient(pConn);
	if (!pClient) pClient = Clients.GetClientByID(pConn->getClientID());
	// forward directly to lobby
	if (pLobby) pLobby->HandlePacket(cStatus, pBasePkt, pClient);
}

bool C4Network2::HandlePuncherPacket(C4NetpuncherPacket::uptr pkt, C4NetIO::HostAddress::AddressFamily family)
{
	// TODO: is this all thread-safe?
	assert(pkt);
#define GETPKT(c) dynamic_cast<C4NetpuncherPacket##c*>(pkt.get())
	switch (pkt->GetType())
	{
		case PID_Puncher_CReq:
			if (isHost())
			{
				NetIO.Punch(GETPKT(CReq)->GetAddr());
				return true;
			}
			else
			{
				// The IP/Port should be already in the masterserver list, so just keep trying.
				return Status.getState() == GS_Init;
			}
		case PID_Puncher_AssID:
			if (isHost())
			{
				getNetpuncherGameID(family) = GETPKT(AssID)->GetID();
				InvalidateReference();
			}
			else
			{
				// The netpuncher hands out IDs for everyone, but clients have no use for them.
			}
			return true;
		default: return false;
	}
}

C4NetpuncherID::value& C4Network2::getNetpuncherGameID(C4NetIO::HostAddress::AddressFamily family)
{
    switch (family)
    {
    case C4NetIO::HostAddress::IPv4: return NetpuncherGameID.v4;
    case C4NetIO::HostAddress::IPv6: return NetpuncherGameID.v6;
    case C4NetIO::HostAddress::UnknownFamily: assert(!"Unexpected address family");
    }
    // We need to return a valid reference to satisfy the compiler, even though the code here is unreachable.
    return NetpuncherGameID.v4;
}

void C4Network2::OnPuncherConnect(C4NetIO::addr_t addr)
{
	// NAT punching is only relevant for IPv4, so convert here to show a proper address.
	auto maybe_v4 = addr.AsIPv4();
	Application.InteractiveThread.ThreadLogS("Adding address from puncher: %s", maybe_v4.ToString().getData());
	// Add for local client
	C4Network2Client *pLocal = Clients.GetLocal();
	if (pLocal)
	{
		pLocal->AddAddrFromPuncher(maybe_v4);
		// Do not ::Network.InvalidateReference(); yet, we're expecting an ID from the netpuncher
	}
	auto family = maybe_v4.GetFamily();
	if (isHost())
	{
		// Host connection: request ID from netpuncher
		NetIO.SendPuncherPacket(C4NetpuncherPacketIDReq(), family);
	}
	else
	{
		// Client connection: request packet from host.
		if (Status.getState() == GS_Init && getNetpuncherGameID(family))
			NetIO.SendPuncherPacket(C4NetpuncherPacketSReq(getNetpuncherGameID(family)), family);
	}
}


void C4Network2::InitPuncher()
{
	// We have an internet connection, so let's punch the puncher server here in order to open an udp port
	C4NetIO::addr_t PuncherAddr;
	PuncherAddr.SetAddress(getNetpuncherAddr(), C4NetIO::HostAddress::IPv4);
	if (!PuncherAddr.IsNull())
	{
	    PuncherAddr.SetDefaultPort(C4NetStdPortPuncher);
	    NetIO.InitPuncher(PuncherAddr);
	}
	PuncherAddr.SetAddress(getNetpuncherAddr(), C4NetIO::HostAddress::IPv6);
	if (!PuncherAddr.IsNull())
	{
	    PuncherAddr.SetDefaultPort(C4NetStdPortPuncher);
	    NetIO.InitPuncher(PuncherAddr);
	}
}

void C4Network2::OnGameSynchronized()
{
	// savegame needed?
	if (fDynamicNeeded)
	{
		// create dynamic
		bool fSuccess = CreateDynamic(false);
		// check for clients that still need join-data
		C4Network2Client *pClient = nullptr;
		while ((pClient = Clients.GetNextClient(pClient)))
			if (!pClient->hasJoinData())
			{
				if (fSuccess)
					// now we can provide join data: send it
					SendJoinData(pClient);
				else
					// join data could not be created: emergency kick
					Game.Clients.CtrlRemove(pClient->getClient(), LoadResStr("IDS_ERR_ERRORWHILECREATINGJOINDAT"));
			}
	}
}

void C4Network2::DrawStatus(C4TargetFacet &cgo)
{
	if (!isEnabled()) return;

	C4Network2Client *pLocal = Clients.GetLocal();

	StdStrBuf Stat;

	// local client status
	Stat.AppendFormat("Local: %s %s %s (ID %d)",
	                  pLocal->isObserver() ? "Observing" : pLocal->isActivated() ? "Active" : "Inactive", pLocal->isHost() ? "host" : "client",
	                  pLocal->getName(), pLocal->getID());

	// game status
	Stat.AppendFormat( "|Game Status: %s (tick %d)%s%s",
	                   Status.getStateName(), Status.getTargetCtrlTick(),
	                   fStatusReached ? " reached" : "", fStatusAck ? " ack" : "");

	// available protocols
	C4NetIO *pMsgIO = NetIO.MsgIO(), *pDataIO = NetIO.DataIO();
	if (pMsgIO && pDataIO)
	{
		C4Network2IOProtocol eMsgProt = NetIO.getNetIOProt(pMsgIO),
		                                eDataProt = NetIO.getNetIOProt(pDataIO);
		int32_t iMsgPort = 0, iDataPort = 0;
		switch (eMsgProt)
		{
		case P_TCP: iMsgPort = Config.Network.PortTCP; break;
		case P_UDP: iMsgPort = Config.Network.PortUDP; break;
		case P_NONE: assert(eMsgProt != P_NONE); break;
		}
		switch (eDataProt)
		{
		case P_TCP: iDataPort = Config.Network.PortTCP; break;
		case P_UDP: iDataPort = Config.Network.PortUDP; break;
		case P_NONE: assert(eMsgProt != P_NONE); break;
		}
		Stat.AppendFormat( "|Protocols: %s: %s (%d i%d o%d bc%d)",
		                   pMsgIO != pDataIO ? "Msg" : "Msg/Data",
		                   NetIO.getNetIOName(pMsgIO), iMsgPort,
		                   NetIO.getProtIRate(eMsgProt), NetIO.getProtORate(eMsgProt), NetIO.getProtBCRate(eMsgProt));
		if (pMsgIO != pDataIO)
			Stat.AppendFormat( ", Data: %s (%d i%d o%d bc%d)",
			                   NetIO.getNetIOName(pDataIO), iDataPort,
			                   NetIO.getProtIRate(eDataProt), NetIO.getProtORate(eDataProt), NetIO.getProtBCRate(eDataProt));
	}
	else
		Stat.Append("|Protocols: none");

	// some control statistics
	Stat.AppendFormat( "|Control: %s, Tick %d, Behind %d, Rate %d, PreSend %d, ACT: %d",
	                   Status.getCtrlMode() == CNM_Decentral ? "Decentral" : Status.getCtrlMode() == CNM_Central ? "Central" : "Async",
	                   ::Control.ControlTick, pControl->GetBehind(::Control.ControlTick),
	                   ::Control.ControlRate, pControl->getControlPreSend(), pControl->getAvgControlSendTime());

	// Streaming statistics
	if (fStreaming)
		Stat.AppendFormat( "|Streaming: %lu waiting, %u in, %lu out, %lu sent",
		                   static_cast<unsigned long>(pStreamedRecord ? pStreamedRecord->GetStreamingBuf().getSize() : 0),
		                   pStreamedRecord ? pStreamedRecord->GetStreamingPos() : 0,
		                   static_cast<unsigned long>(getPendingStreamData()),
		                   static_cast<unsigned long>(iCurrentStreamPosition));

	// clients
	Stat.Append("|Clients:");
	for (C4Network2Client *pClient = Clients.GetNextClient(nullptr); pClient; pClient = Clients.GetNextClient(pClient))
	{
		// ignore local
		if (pClient->isLocal()) continue;
		// client status
		const C4ClientCore &Core = pClient->getCore();
		const char *szClientStatus;
		switch (pClient->getStatus())
		{
		case NCS_Joining: szClientStatus = " (joining)"; break;
		case NCS_Chasing: szClientStatus = " (chasing)"; break;
		case NCS_NotReady: szClientStatus = " (!rdy)"; break;
		case NCS_Remove: szClientStatus = " (removed)"; break;
		default: szClientStatus = ""; break;
		}
		Stat.AppendFormat( "|- %s %s %s (ID %d) (wait %d ms, behind %d)%s%s",
		                   Core.isObserver() ? "Observing" : Core.isActivated() ? "Active" : "Inactive", Core.isHost() ? "host" : "client",
		                   Core.getName(), Core.getID(),
		                   pControl->ClientPerfStat(pClient->getID()),
		                   ::Control.ControlTick - pControl->ClientNextControl(pClient->getID()),
		                   szClientStatus,
		                   pClient->isActivated() && !pControl->ClientReady(pClient->getID(), ::Control.ControlTick) ? " (!ctrl)" : "");
		// connections
		if (pClient->isConnected())
		{
			Stat.AppendFormat( "|   Connections: %s: %s (%s p%d l%d)",
			                   pClient->getMsgConn() == pClient->getDataConn() ? "Msg/Data" : "Msg",
			                   NetIO.getNetIOName(pClient->getMsgConn()->getNetClass()),
							   pClient->getMsgConn()->getPeerAddr().ToString().getData(),
			                   pClient->getMsgConn()->getPingTime(),
			                   pClient->getMsgConn()->getPacketLoss());
			if (pClient->getMsgConn() != pClient->getDataConn())
				Stat.AppendFormat( ", Data: %s (%s:%d p%d l%d)",
				                   NetIO.getNetIOName(pClient->getDataConn()->getNetClass()),
								   pClient->getDataConn()->getPeerAddr().ToString().getData(),
				                   pClient->getDataConn()->getPingTime(),
				                   pClient->getDataConn()->getPacketLoss());
		}
		else
			Stat.Append("|   Not connected");
	}
	if (!Clients.GetNextClient(nullptr))
		Stat.Append("| - none -");

	// draw
	pDraw->TextOut(Stat.getData(), ::GraphicsResource.FontRegular, 1.0, cgo.Surface,cgo.X + 20,cgo.Y + 50);
}

bool C4Network2::InitNetIO(bool fNoClientID, bool fHost)
{
	// clear
	NetIO.Clear();
	Config.Network.CheckPortsForCollisions();
	// discovery: disable for client
	int16_t iPortDiscovery = fHost ? Config.Network.PortDiscovery : -1;
	int16_t iPortRefServer = fHost ? Config.Network.PortRefServer : -1;
	// init subclass
	if (!NetIO.Init(Config.Network.PortTCP, Config.Network.PortUDP, iPortDiscovery, iPortRefServer, fHost, !!Config.Network.EnableUPnP))
		return false;
	// set core (unset ID if sepecified, has to be set later)
	C4ClientCore Core = Game.Clients.getLocalCore();
	if (fNoClientID) Core.SetID(C4ClientIDUnknown);
	NetIO.SetLocalCCore(Core);
	// safe addresses of local client
	Clients.GetLocal()->AddLocalAddrs(
	  NetIO.hasTCP() ? Config.Network.PortTCP : -1,
	  NetIO.hasUDP() ? Config.Network.PortUDP : -1);
	// ok
	return true;
}

void C4Network2::HandleConn(const C4PacketConn &Pkt, C4Network2IOConnection *pConn, C4Network2Client *pClient)
{
	// security
	if (!pConn) return;

	// Handles a connect request (packet PID_Conn).
	// Check if this peer should be allowed to connect, make space for the new connection.

	// connection is closed?
	if (pConn->isClosed())
		return;

	// set up core
	const C4ClientCore &CCore = Pkt.getCCore();
	C4ClientCore NewCCore = CCore;

	// accept connection?
	StdStrBuf reply;
	bool fOK = false;

	// search client
	if (!pClient && Pkt.getCCore().getID() != C4ClientIDUnknown)
		pClient = Clients.GetClient(Pkt.getCCore());

	// check engine version
	bool fWrongPassword = false;
	if (Pkt.getVer() != C4XVER1*100 + C4XVER2)
	{
		reply.Format("wrong engine (%d.%d, I have %d.%d)", Pkt.getVer()/100, Pkt.getVer()%100, C4XVER1, C4XVER2);
		fOK = false;
	}
	else
	{
		if (pClient)
			if (CheckConn(NewCCore, pConn, pClient, &reply))
			{
				// accept
				if (!reply) reply = "connection accepted";
				fOK = true;
			}
		// client: host connection?
		if (!fOK && !isHost() && Status.getState() == GS_Init && !Clients.GetHost())
			if (HostConnect(NewCCore, pConn, &reply))
			{
				// accept
				if (!reply) reply = "host connection accepted";
				fOK = true;
			}
		// host: client join? (NewCCore will be changed by Join()!)
		if (!fOK && isHost() && !pClient)
		{
			// check password
			if (!sPassword.isNull() && !SEqual(Pkt.getPassword(), sPassword.getData()))
			{
				reply = "wrong password";
				fWrongPassword = true;
			}
			// accept join
			else if (Join(NewCCore, pConn, &reply))
			{
				// save core
				pConn->SetCCore(NewCCore);
				// accept
				if (!reply) reply = "join accepted";
				fOK = true;
			}
		}
	}

	// denied? set default reason
	if (!fOK && !reply) reply = "connection denied";

	// OK and already half accepted? Skip (double-checked: ok).
	if (fOK && pConn->isHalfAccepted())
		return;

	// send answer
	C4PacketConnRe pcr(fOK, fWrongPassword, reply.getData());
	if (!pConn->Send(MkC4NetIOPacket(PID_ConnRe, pcr)))
		return;

	// accepted?
	if (fOK)
	{
		// set status
		if (!pConn->isClosed())
			pConn->SetHalfAccepted();
	}
	// denied? close
	else
	{
		// log & close
		LogSilentF("Network: connection by %s (%s) blocked: %s", CCore.getName(), pConn->getPeerAddr().ToString().getData(), reply.getData());
		pConn->Close();
	}
}

bool C4Network2::CheckConn(const C4ClientCore &CCore, C4Network2IOConnection *pConn, C4Network2Client *pClient, StdStrBuf * szReply)
{
	if (!pConn || !pClient) return false;
	// already connected? (shouldn't happen really)
	if (pClient->hasConn(pConn))
		{ *szReply = "already connected"; return true; }
	// check core
	if (CCore.getDiffLevel(pClient->getCore()) > C4ClientCoreDL_IDMatch)
		{ *szReply = "wrong client core"; return false; }
	// accept
	return true;
}

bool C4Network2::HostConnect(const C4ClientCore &CCore, C4Network2IOConnection *pConn, StdStrBuf *szReply)
{
	if (!pConn) return false;
	if (!CCore.isHost()) { *szReply = "not host"; return false; }
	// create client class for host
	// (core is unofficial, see InitClient() -  will be overwritten later in HandleJoinData)
	C4Client *pClient = Game.Clients.Add(CCore);
	if (!pClient) return false;
	// accept
	return true;
}

bool C4Network2::Join(C4ClientCore &CCore, C4Network2IOConnection *pConn, StdStrBuf *szReply)
{
	if (!pConn) return false;
	// security
	if (!isHost()) { *szReply = "not host"; return false; }
	if (!fAllowJoin && !fAllowObserve) { *szReply = "join denied"; return false; }
	if (CCore.getID() != C4ClientIDUnknown) { *szReply = "join with set id not allowed"; return false; }
	// find free client id
	CCore.SetID(iNextClientID++);
	// observer?
	if (!fAllowJoin) CCore.SetObserver(true);
	// deactivate - client will have to ask for activation.
	CCore.SetActivated(false);
	// Name already in use? Find unused one
	if (Clients.GetClient(CCore.getName()))
	{
		char szNameTmpl[256+1], szNewName[256+1];
		SCopy(CCore.getName(), szNameTmpl, 254); SAppend("%d", szNameTmpl, 256);
		int32_t i = 1;
		do
			sprintf(szNewName, szNameTmpl, ++i);
		while (Clients.GetClient(szNewName));
		CCore.SetName(szNewName);
	}
	// join client
	::Control.DoInput(CID_ClientJoin, new C4ControlClientJoin(CCore), CDT_Direct);
	// get client, set status
	C4Network2Client *pClient = Clients.GetClient(CCore);
	if (pClient) pClient->SetStatus(NCS_Joining);
	// warn if client revision doesn't match our host revision
	if (!SEqualNoCase(CCore.getRevision(), Application.GetRevision()))
	{
		LogF("[!]WARNING! Client %s engine revision (%s) differs from local revision (%s). Client might run out of sync.", CCore.getName(), CCore.getRevision(), Application.GetRevision());
	}
	// ok, client joined.
	return true;
	// Note that the connection isn't fully accepted at this point and won't be
	// associated with the client. The new-created client is waiting for connect.
	// Somewhat ironically, the connection may still timeout (resulting in an instant
	// removal and maybe some funny message sequences).
	// The final client initialization will be done at OnClientConnect.
}

void C4Network2::HandleConnRe(const C4PacketConnRe &Pkt, C4Network2IOConnection *pConn, C4Network2Client *pClient)
{
	// Handle the connection request reply. After this handling, the connection should
	// be either fully associated with a client (fully accepted) or closed.
	// Note that auto-accepted connection have to processed here once, too, as the
	// client must get associated with the connection. After doing so, the connection
	// auto-accept flag will be reset to mark the connection fully accepted.

	// security
	if (!pConn) return;
	if (!pClient) { pConn->Close(); return; }

	// negative reply?
	if (!Pkt.isOK())
	{
		// wrong password?
		fWrongPassword = Pkt.isPasswordWrong();
		// show message
		LogSilentF("Network: connection to %s (%s) refused: %s", pClient->getName(), pConn->getPeerAddr().ToString().getData(), Pkt.getMsg());
		// close connection
		pConn->Close();
		return;
	}

	// connection is closed?
	if (!pConn->isOpen())
		return;

	// already accepted? ignore
	if (pConn->isAccepted() && !pConn->isAutoAccepted()) return;

	// first connection?
	bool fFirstConnection = !pClient->isConnected();

	// accept connection
	pConn->SetAccepted(); pConn->ResetAutoAccepted();

	// add connection
	pConn->SetCCore(pClient->getCore());
	if (pConn->getNetClass() == NetIO.MsgIO()) pClient->SetMsgConn(pConn);
	if (pConn->getNetClass() == NetIO.DataIO()) pClient->SetDataConn(pConn);

	// add peer connect address to client address list
	if (!pConn->getConnectAddr().IsNull())
	{
		C4Network2Address Addr(pConn->getConnectAddr(), pConn->getProtocol());
		pClient->AddAddr(Addr, Status.getState() != GS_Init);
	}

	// handle
	OnConnect(pClient, pConn, Pkt.getMsg(), fFirstConnection);
}

void C4Network2::HandleStatus(const C4Network2Status &nStatus)
{
	// set
	Status = nStatus;
	// log
	LogSilentF("Network: going into status %s (tick %d)", Status.getStateName(), nStatus.getTargetCtrlTick());
	// reset flags
	fStatusReached = fStatusAck = false;
	// check: reached?
	CheckStatusReached();
}

void C4Network2::HandleStatusAck(const C4Network2Status &nStatus, C4Network2Client *pClient)
{
	// security
	if (!pClient->hasJoinData() || pClient->isRemoved()) return;
	// status doesn't match?
	if (nStatus.getState() != Status.getState() || nStatus.getTargetCtrlTick() < Status.getTargetCtrlTick())
		return;
	// host: wait until all clients are ready
	if (isHost())
	{
		// check: target tick change?
		if (!fStatusAck && nStatus.getTargetCtrlTick() > Status.getTargetCtrlTick())
			// take the new status
			ChangeGameStatus(nStatus.getState(), nStatus.getTargetCtrlTick());
		// already acknowledged? Send another ack
		if (fStatusAck)
			pClient->SendMsg(MkC4NetIOPacket(PID_StatusAck, nStatus));
		// mark as ready (will clear chase-flag)
		pClient->SetStatus(NCS_Ready);
		// check: everyone ready?
		if (!fStatusAck && fStatusReached)
			CheckStatusAck();
	}
	else
	{
		// target tick doesn't match? ignore
		if (nStatus.getTargetCtrlTick() != Status.getTargetCtrlTick())
			return;
		// reached?
		// can be ignored safely otherwise - when the status is reached, we will send
		// status ack on which the host should generate another status ack (see above)
		if (fStatusReached)
		{
			// client: set flags, call handler
			fStatusAck = true; fChasing = false;
			OnStatusAck();
		}

	}
}

void C4Network2::HandleActivateReq(int32_t iTick, C4Network2Client *pByClient)
{
	if (!isHost()) return;
	// not allowed or already activated? ignore
	if (pByClient->isObserver() || pByClient->isActivated()) return;
	// not joined completely yet? ignore
	if (!pByClient->isWaitedFor()) return;
	// check behind limit
	if (isRunning())
	{
		// make a guess how much the client lags.
		int32_t iLagFrames = Clamp(pByClient->getMsgConn()->getPingTime() * Game.FPS / 500, 0, 100);
		if (iTick < Game.FrameCounter - iLagFrames - C4NetMaxBehind4Activation)
			return;
	}
	// activate him
	::Control.DoInput(CID_ClientUpdate,
	                  new C4ControlClientUpdate(pByClient->getID(), CUT_Activate, true),
	                  CDT_Sync);
}

void C4Network2::HandleJoinData(const C4PacketJoinData &rPkt)
{
	// init only
	if (Status.getState() != GS_Init)
		{ LogSilentF("Network: unexpected join data received!"); return; }
	// get client ID
	if (rPkt.getClientID() == C4ClientIDUnknown)
		{ LogSilentF("Network: host didn't set client ID!"); Clear(); return; }
	// set local ID
	ResList.SetLocalID(rPkt.getClientID());
	Game.Parameters.Clients.SetLocalID(rPkt.getClientID());
	// read and validate status
	HandleStatus(rPkt.getStatus());
	if (Status.getState() != GS_Lobby && Status.getState() != GS_Pause && Status.getState() != GS_Go)
		{ LogSilentF("Network: join data has bad game status: %s", Status.getStateName()); Clear(); return; }
	// copy scenario parameter defs for lobby display
	::Game.ScenarioParameterDefs = rPkt.ScenarioParameterDefs;
	// copy parameters
	::Game.Parameters = rPkt.Parameters;
	// set local client
	C4Client *pLocalClient = Game.Clients.getClientByID(rPkt.getClientID());
	if (!pLocalClient)
		{ LogSilentF("Network: Could not find local client in join data!"); Clear(); return; }
	// save back dynamic data
	ResDynamic = rPkt.getDynamicCore();
	iDynamicTick = rPkt.getStartCtrlTick();
	// initialize control
	::Control.ControlRate = rPkt.Parameters.ControlRate;
	pControl->Init(rPkt.getClientID(), false, rPkt.getStartCtrlTick(), pLocalClient->isActivated(), this);
	pControl->CopyClientList(Game.Parameters.Clients);
	// set local core
	NetIO.SetLocalCCore(pLocalClient->getCore());
	// add the resources to the network resource list
	Game.Parameters.GameRes.InitNetwork(&ResList);
	// load dynamic
	if (!ResList.AddByCore(ResDynamic))
		{ LogFatal("Network: can not not retrieve dynamic!"); Clear(); return; }
	// load player resources
	Game.Parameters.PlayerInfos.LoadResources();
	// send additional addresses
	Clients.SendAddresses(nullptr);
}

void C4Network2::OnConnect(C4Network2Client *pClient, C4Network2IOConnection *pConn, const char *szMsg, bool fFirstConnection)
{
	// log
	LogSilentF("Network: %s %s connected (%s/%s) (%s)", pClient->isHost() ? "host" : "client",
	           pClient->getName(), pConn->getPeerAddr().ToString().getData(),
	           NetIO.getNetIOName(pConn->getNetClass()), szMsg ? szMsg : "");

	// first connection for this peer? call special handler
	if (fFirstConnection) OnClientConnect(pClient, pConn);
}

void C4Network2::OnConnectFail(C4Network2IOConnection *pConn)
{
	LogSilentF("Network: %s connection to %s failed!", NetIO.getNetIOName(pConn->getNetClass()),
	           pConn->getPeerAddr().ToString().getData());

	// maybe client connection failure
	// (happens if the connection is not fully accepted and the client disconnects.
	//  See C4Network2::Join)
	C4Network2Client *pClient = Clients.GetClientByID(pConn->getClientID());
	if (pClient && !pClient->isConnected())
		OnClientDisconnect(pClient);
}

void C4Network2::OnDisconnect(C4Network2Client *pClient, C4Network2IOConnection *pConn)
{
	LogSilentF("Network: %s connection to %s (%s) lost!", NetIO.getNetIOName(pConn->getNetClass()),
	           pClient->getName(), pConn->getPeerAddr().ToString().getData());

	// connection lost?
	if (!pClient->isConnected())
		OnClientDisconnect(pClient);
}

void C4Network2::OnClientConnect(C4Network2Client *pClient, C4Network2IOConnection *pConn)
{
	// host: new client?
	if (isHost())
	{
		// dynamic available?
		if (!pClient->hasJoinData())
			SendJoinData(pClient);

		// notice lobby (doesn't do anything atm?)
		C4GameLobby::MainDlg *pDlg = GetLobby();
		if (isLobbyActive()) pDlg->OnClientConnect(pClient->getClient(), pConn);

	}

	// discover resources
	ResList.OnClientConnect(pConn);

}

void C4Network2::OnClientDisconnect(C4Network2Client *pClient)
{
	// league: Notify regular client disconnect within the game
	if (pLeagueClient && (isHost() || pClient->isHost())) LeagueNotifyDisconnect(pClient->getID(), C4LDR_ConnectionFailed);
	// host? Remove this client from the game.
	if (isHost())
	{
		// log
		LogSilentF(LoadResStr("IDS_NET_CLIENTDISCONNECTED"), pClient->getName()); // silent, because a duplicate message with disconnect reason will follow
		// remove the client
		Game.Clients.CtrlRemove(pClient->getClient(), LoadResStr("IDS_MSG_DISCONNECTED"));
		// check status ack (disconnected client might be the last that was waited for)
		CheckStatusAck();
		// unreached pause/go? retry setting the state with current control tick
		// (client might be the only one claiming to have the given control)
		if (!fStatusReached)
			if (Status.getState() == GS_Go || Status.getState() == GS_Pause)
				ChangeGameStatus(Status.getState(), ::Control.ControlTick);
#ifdef USE_CONSOLE
		// Dedicated server: stop hosting if there is only one client left we're hosting for.
		// TODO: Find a better place to do this.
		if (Game.IsRunning && Clients.Count() <= 3) Application.Quit(); // Off-by-1 error
#endif // USE_CONSOLE
	}
	// host disconnected? Clear up
	if (!isHost() && pClient->isHost())
	{
		StdStrBuf sMsg; sMsg.Format(LoadResStr("IDS_NET_HOSTDISCONNECTED"), pClient->getName());
		Log(sMsg.getData());
		// host connection lost: clear up everything
		Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, sMsg.getData());
		Clear();
	}
}

void C4Network2::SendJoinData(C4Network2Client *pClient)
{
	if (pClient->hasJoinData()) return;
	// host only, scenario must be available
	assert(isHost());
	// dynamic available?
	if (ResDynamic.isNull() || iDynamicTick < ::Control.ControlTick)
	{
		fDynamicNeeded = true;
		// add synchronization control (will callback, see C4Game::Synchronize)
		::Control.DoInput(CID_Synchronize, new C4ControlSynchronize(false, true), CDT_Sync);
		return;
	}
	// save his client ID
	C4PacketJoinData JoinData;
	JoinData.SetClientID(pClient->getID());
	// save status into packet
	JoinData.SetGameStatus(Status);
	// scenario parameter defs for lobby display (localized in host language)
	JoinData.ScenarioParameterDefs = ::Game.ScenarioParameterDefs;
	// parameters
	JoinData.Parameters = Game.Parameters;
	// core join data
	JoinData.SetStartCtrlTick(iDynamicTick);
	JoinData.SetDynamicCore(ResDynamic);
	// send
	pClient->SendMsg(MkC4NetIOPacket(PID_JoinData, JoinData));
	// send addresses
	Clients.SendAddresses(pClient->getMsgConn());
	// flag client (he will have to accept the network status sent next)
	pClient->SetStatus(NCS_Chasing);
	if (!iLastChaseTargetUpdate) iLastChaseTargetUpdate = time(nullptr);
}

C4Network2Res::Ref C4Network2::RetrieveRes(const C4Network2ResCore &Core, int32_t iTimeoutLen, const char *szResName, bool fWaitForCore)
{
	C4GUI::ProgressDialog *pDlg = nullptr;
	bool fLog = false;
	int32_t iProcess = -1;
	C4TimeMilliseconds tTimeout = C4TimeMilliseconds::Now() + iTimeoutLen;
	// wait for resource
	while (isEnabled())
	{
		// find resource
		C4Network2Res::Ref pRes = ResList.getRefRes(Core.getID());
		// res not found?
		if (!pRes)
		{
			if (Core.isNull())
			{
				// should wait for core?
				if (!fWaitForCore) return nullptr;
			}
			else
			{
				// start loading
				pRes = ResList.AddByCore(Core);
			}
		}
		// res found and loaded completely
		else if (!pRes->isLoading())
		{
			// log
			if (fLog) LogF(LoadResStr("IDS_NET_RECEIVED"), szResName, pRes->getCore().getFileName());
			// return
			if (pDlg) delete pDlg;
			return pRes;
		}

		// check: progress?
		if (pRes && pRes->getPresentPercent() != iProcess)
		{
			iProcess = pRes->getPresentPercent();
			tTimeout = C4TimeMilliseconds::Now() + iTimeoutLen;
		}
		else
		{
			// if not: check timeout
			if (C4TimeMilliseconds::Now() > tTimeout)
			{
				LogFatal(FormatString(LoadResStr("IDS_NET_ERR_RESTIMEOUT"), szResName).getData());
				if (pDlg) delete pDlg;
				return nullptr;
			}
		}

		// log
		if (!fLog)
		{
			LogF(LoadResStr("IDS_NET_WAITFORRES"), szResName);
			fLog = true;
		}
		// show progress dialog
		if (!pDlg && !Console.Active && ::pGUI)
		{
			// create
			pDlg = new C4GUI::ProgressDialog(FormatString(LoadResStr("IDS_NET_WAITFORRES"), szResName).getData(),
			                                 LoadResStr("IDS_NET_CAPTION"), 100, 0, C4GUI::Ico_NetWait);
			// show dialog
			if (!pDlg->Show(::pGUI, true)) { delete pDlg; return nullptr; }
		}

		// wait
		if (pDlg)
		{
			// set progress bar
			pDlg->SetProgress(iProcess);
			// execute (will do message handling)
			if (!pDlg->Execute())
				{ if (pDlg) delete pDlg; return nullptr; }
			// aborted?
			if (pDlg->IsAborted()) break;
		}
		else
		{
			if (!Application.ScheduleProcs(tTimeout - C4TimeMilliseconds::Now()))
				{ return nullptr; }
		}

	}
	// aborted
	delete pDlg;
	return nullptr;
}


bool C4Network2::CreateDynamic(bool fInit)
{
	if (!isHost()) return false;
	// remove all existing dynamic data
	RemoveDynamic();
	// log
	Log(LoadResStr("IDS_NET_SAVING"));
	// compose file name
	char szDynamicBase[_MAX_PATH+1], szDynamicFilename[_MAX_PATH+1];
	sprintf(szDynamicBase, Config.AtNetworkPath("Dyn%s"), GetFilename(Game.ScenarioFilename), _MAX_PATH);
	if (!ResList.FindTempResFileName(szDynamicBase, szDynamicFilename))
		Log(LoadResStr("IDS_NET_SAVE_ERR_CREATEDYNFILE"));
	// save dynamic data
	C4GameSaveNetwork SaveGame(fInit);
	if (!SaveGame.Save(szDynamicFilename) || !SaveGame.Close())
		{ Log(LoadResStr("IDS_NET_SAVE_ERR_SAVEDYNFILE")); return false; }
	// add resource
	C4Network2Res::Ref pRes = ResList.AddByFile(szDynamicFilename, true, NRT_Dynamic);
	if (!pRes) { Log(LoadResStr("IDS_NET_SAVE_ERR_ADDDYNDATARES")); return false; }
	// save
	ResDynamic = pRes->getCore();
	iDynamicTick = ::Control.getNextControlTick();
	fDynamicNeeded = false;
	// ok
	return true;
}

void C4Network2::RemoveDynamic()
{
	C4Network2Res::Ref pRes = ResList.getRefRes(ResDynamic.getID());
	if (pRes) pRes->Remove();
	ResDynamic.Clear();
	iDynamicTick = -1;
}

bool C4Network2::isFrozen() const
{
	// "frozen" means all clients are garantueed to be in the same tick.
	// This is only the case if the game is not started yet (lobby) or the
	// tick has been ensured (pause) and acknowledged by all joined clients.
	// Note unjoined clients must be ignored here - they can't be faster than
	// the host, anyway.
	if (Status.getState() == GS_Lobby) return true;
	if (Status.getState() == GS_Pause && fStatusAck) return true;
	return false;
}

bool C4Network2::ChangeGameStatus(C4NetGameState enState, int32_t iTargetCtrlTick, int32_t iCtrlMode)
{
	// change game status, announce. Can only be done by host.
	if (!isHost()) return false;
	// set status
	Status.Set(enState, iTargetCtrlTick);
	// update reference
	InvalidateReference();
	// control mode change?
	if (iCtrlMode >= 0) Status.SetCtrlMode(iCtrlMode);
	// log
	LogSilentF("Network: going into status %s (tick %d)", Status.getStateName(), iTargetCtrlTick);
	// set flags
	Clients.ResetReady();
	fStatusReached = fStatusAck = false;
	// send new status to all clients
	Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_Status, Status));
	// check reach/ack
	CheckStatusReached();
	// ok
	return true;
}

void C4Network2::CheckStatusReached(bool fFromFinalInit)
{
	// already reached?
	if (fStatusReached) return;
	if (Status.getState() == GS_Lobby)
		fStatusReached = fLobbyRunning;
	// game go / pause: control must be initialized and target tick reached
	else if (Status.getState() == GS_Go || Status.getState() == GS_Pause)
	{
		if (Game.IsRunning || fFromFinalInit)
		{
			// Make sure we have reached the tick and the control queue is empty (except for chasing)
			if (::Control.CtrlTickReached(Status.getTargetCtrlTick()) &&
			    (fChasing || !pControl->CtrlReady(::Control.ControlTick)))
				fStatusReached = true;
			else
			{
				// run ctrl so the tick can be reached
				pControl->SetRunning(true, Status.getTargetCtrlTick());
				Game.HaltCount = 0;
				Console.UpdateHaltCtrls(!! Game.HaltCount);
			}
		}
	}
	if (!fStatusReached) return;
	// call handler
	OnStatusReached();
	// host?
	if (isHost())
		// all clients ready?
		CheckStatusAck();
	else
	{
		Status.SetTargetTick(::Control.ControlTick);
		// send response to host
		Clients.SendMsgToHost(MkC4NetIOPacket(PID_StatusAck, Status));
		// do delayed activation request
		if (fDelayedActivateReq)
		{
			fDelayedActivateReq = false;
			RequestActivate();
		}
	}
}

void C4Network2::CheckStatusAck()
{
	// host only
	if (!isHost()) return;
	// status must be reached and not yet acknowledged
	if (!fStatusReached || fStatusAck) return;
	// all clients ready?
	if ((fStatusAck = Clients.AllClientsReady()))
	{
		// pause/go: check for sync control that can be executed
		if (Status.getState() == GS_Go || Status.getState() == GS_Pause)
			pControl->ExecSyncControl();
		// broadcast ack
		Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_StatusAck, Status));
		// handle
		OnStatusAck();
	}
}

void C4Network2::OnStatusReached()
{
	// stop ctrl, wait for ack
	if (pControl->IsEnabled())
	{
		Console.UpdateHaltCtrls(!!++Game.HaltCount);
		pControl->SetRunning(false);
	}
}

void C4Network2::OnStatusAck()
{
	// log it
	LogSilentF("Network: status %s (tick %d) reached", Status.getStateName(), Status.getTargetCtrlTick());
	// pause?
	if (Status.getState() == GS_Pause)
	{
		// set halt-flag (show hold message)
		Console.UpdateHaltCtrls(!!++Game.HaltCount);
	}
	// go?
	if (Status.getState() == GS_Go)
	{
		// set mode
		pControl->SetCtrlMode(static_cast<C4GameControlNetworkMode>(Status.getCtrlMode()));
		// notify player list of reached status - will add some input to the queue
		Players.OnStatusGoReached();
		// start ctrl
		pControl->SetRunning(true);
		// reset halt-flag
		Game.HaltCount = 0;
		Console.UpdateHaltCtrls(!!Game.HaltCount);
	}
}

void C4Network2::RequestActivate()
{
	// neither observer nor activated?
	if (Game.Clients.getLocal()->isObserver() || Game.Clients.getLocal()->isActivated())
	{
		tLastActivateRequest = C4TimeMilliseconds::NegativeInfinity;
		return;
	}
	// host? just do it
	if (fHost)
	{
		// activate him
		::Control.DoInput(CID_ClientUpdate,
		                  new C4ControlClientUpdate(C4ClientIDHost, CUT_Activate, true),
		                  CDT_Sync);
		return;
	}
	// ensure interval
	if(C4TimeMilliseconds::Now() < tLastActivateRequest + C4NetActivationReqInterval)
		return;
	// status not reached yet? May be chasing, let's delay this.
	if (!fStatusReached)
	{
		fDelayedActivateReq = true;
		return;
	}
	// request
	Clients.SendMsgToHost(MkC4NetIOPacket(PID_ClientActReq, C4PacketActivateReq(Game.FrameCounter)));
	// store time
	tLastActivateRequest = C4TimeMilliseconds::Now();
}

void C4Network2::DeactivateInactiveClients()
{
	// host only and not in editor
	if (!isHost() || ::Application.isEditor) return;
	// update activity
	Clients.UpdateClientActivity();
	// find clients to deactivate
	for (C4Network2Client *pClient = Clients.GetNextClient(nullptr); pClient; pClient = Clients.GetNextClient(pClient))
		if (!pClient->isLocal() && pClient->isActivated())
			if (pClient->getLastActivity() + C4NetDeactivationDelay < Game.FrameCounter)
				::Control.DoInput(CID_ClientUpdate, new C4ControlClientUpdate(pClient->getID(), CUT_Activate, false), CDT_Sync);
}

void C4Network2::UpdateChaseTarget()
{
	// no chasing clients?
	C4Network2Client *pClient;
	for (pClient = Clients.GetNextClient(nullptr); pClient; pClient = Clients.GetNextClient(pClient))
		if (pClient->isChasing())
			break;
	if (!pClient)
	{
		iLastChaseTargetUpdate = 0;
		return;
	}
	// not time for an update?
	if (!iLastChaseTargetUpdate || long(iLastChaseTargetUpdate + C4NetChaseTargetUpdateInterval) > time(nullptr))
		return;
	// copy status, set current tick
	C4Network2Status ChaseTarget = Status;
	ChaseTarget.SetTargetTick(::Control.ControlTick);
	// send to everyone involved
	for (pClient = Clients.GetNextClient(nullptr); pClient; pClient = Clients.GetNextClient(pClient))
		if (pClient->isChasing())
			pClient->SendMsg(MkC4NetIOPacket(PID_Status, ChaseTarget));
	iLastChaseTargetUpdate = time(nullptr);
}

void C4Network2::LeagueGameEvaluate(const char *szRecordName, const BYTE *pRecordSHA)
{
	// already off?
	if (!pLeagueClient) return;
	// already evaluated?
	if (fLeagueEndSent) return;
	// end
	LeagueEnd(szRecordName, pRecordSHA);
}

void C4Network2::LeagueSignupDisable()
{
	// already off?
	if (!pLeagueClient) return;
	// no post-disable if league is active
	if (pLeagueClient && Game.Parameters.isLeague()) return;
	// signup end
	LeagueEnd(); DeinitLeague();
}

bool C4Network2::LeagueSignupEnable()
{
	// already running?
	if (pLeagueClient) return true;
	// Start it!
	if (InitLeague(nullptr) && LeagueStart(nullptr)) return true;
	// Failure :'(
	DeinitLeague();
	return false;
}

void C4Network2::InvalidateReference()
{
	// Update both local and league reference as soon as possible
	iLastReferenceUpdate = 0;
	iLeagueUpdateDelay = C4NetMinLeagueUpdateInterval;
}

bool C4Network2::InitLeague(bool *pCancel)
{

	if (fHost)
	{

		// Clear parameters
		MasterServerAddress.Clear();
		Game.Parameters.League.Clear();
		Game.Parameters.LeagueAddress.Clear();
		if (pLeagueClient) delete pLeagueClient; pLeagueClient = nullptr;

		// Not needed?
		if (!Config.Network.MasterServerSignUp && !Config.Network.LeagueServerSignUp)
			return true;

		// Save address
		MasterServerAddress = Config.Network.GetLeagueServerAddress();
		if (Config.Network.LeagueServerSignUp)
		{
			Game.Parameters.LeagueAddress = MasterServerAddress;
			// enforce some league rules
			Game.Parameters.EnforceLeagueRules(&Game.C4S);
		}

	}
	else
	{

		// Get league server from parameters
		MasterServerAddress = Game.Parameters.LeagueAddress;

		// Not needed?
		if (!MasterServerAddress.getLength())
			return true;

	}

	// Init
	pLeagueClient = new C4LeagueClient();
	if (!pLeagueClient->Init() ||
	    !pLeagueClient->SetServer(MasterServerAddress.getData()))
	{
		// Log message
		StdStrBuf Message = FormatString(LoadResStr("IDS_NET_ERR_LEAGUEINIT"), pLeagueClient->GetError());
		LogFatal(Message.getData());
		// Clear league
		delete pLeagueClient; pLeagueClient = nullptr;
		if (fHost)
			Game.Parameters.LeagueAddress.Clear();
		// Show message, allow abort
		bool fResult = true;
		if (!Application.isEditor)
			fResult = ::pGUI->ShowMessageModal(Message.getData(), LoadResStr("IDS_NET_ERR_LEAGUE"),
			                                   (pCancel ? C4GUI::MessageDialog::btnOK : 0) | C4GUI::MessageDialog::btnAbort,
			                                   C4GUI::Ico_Error);
		if (pCancel) *pCancel = fResult;
		return false;
	}

	// Add to message loop
	Application.Add(pLeagueClient);

	// OK
	return true;
}

void C4Network2::DeinitLeague()
{
	// league clear
	MasterServerAddress.Clear();
	Game.Parameters.League.Clear();
	Game.Parameters.LeagueAddress.Clear();
	if (pLeagueClient)
	{
		Application.Remove(pLeagueClient);
		delete pLeagueClient; pLeagueClient = nullptr;
	}
}

bool C4Network2::LeagueStart(bool *pCancel)
{
	// Not needed?
	if (!pLeagueClient || !fHost)
		return true;

	// Default
	if (pCancel) *pCancel = true;

	// Do update
	C4Network2Reference Ref;
	Ref.InitLocal();
	if (!pLeagueClient->Start(Ref))
	{
		// Log message
		StdStrBuf Message = FormatString(LoadResStr("IDS_NET_ERR_LEAGUE_STARTGAME"), pLeagueClient->GetError());
		LogFatal(Message.getData());
		// Show message
		if (!Application.isEditor)
		{
			// Show option to cancel, if possible
			bool fResult = ::pGUI->ShowMessageModal(
			                 Message.getData(),
			                 LoadResStr("IDS_NET_ERR_LEAGUE"),
			                 pCancel ? (C4GUI::MessageDialog::btnOK | C4GUI::MessageDialog::btnAbort) : C4GUI::MessageDialog::btnOK,
			                 C4GUI::Ico_Error);
			if (pCancel)
				*pCancel = !fResult;
		}
		// Failed
		return false;
	}

	InitPuncher();

	// Let's wait for response
	StdStrBuf Message = FormatString(LoadResStr("IDS_NET_LEAGUE_REGGAME"), pLeagueClient->getServerName());
	Log(Message.getData());
	// Set up a dialog
	C4GUI::MessageDialog *pDlg = nullptr;
	if (!Application.isEditor)
	{
		// create & show
		pDlg = new C4GUI::MessageDialog(Message.getData(), LoadResStr("IDS_NET_LEAGUE_STARTGAME"),
		                                C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsRegular);
		if (!pDlg || !pDlg->Show(::pGUI, true))  return false;
	}
	// Wait for response
	while (pLeagueClient->isBusy())
	{
		// Execute GUI
		if (!Application.ScheduleProcs() ||
		    (pDlg && pDlg->IsAborted()))
		{
			// Clear up
			if (pDlg) delete pDlg;
			return false;
		}
		// Check if league server has responded
		if (!pLeagueClient->Execute(100))
			break;
	}
	// Close dialog
	if (pDlg)
	{
		pDlg->Close(true);
		delete pDlg;
	}
	// Error?
	StdStrBuf LeagueServerMessage, League, StreamingAddr;
	int32_t Seed = Game.RandomSeed, MaxPlayersLeague = 0;
	if (!pLeagueClient->isSuccess() ||
	    !pLeagueClient->GetStartReply(&LeagueServerMessage, &League, &StreamingAddr, &Seed, &MaxPlayersLeague))
	{
		const char *pError = pLeagueClient->GetError() ? pLeagueClient->GetError() :
		                     LeagueServerMessage.getLength() ? LeagueServerMessage.getData() :
		                     LoadResStr("IDS_NET_ERR_LEAGUE_EMPTYREPLY");
		StdStrBuf Message = FormatString(LoadResStr("IDS_NET_ERR_LEAGUE_REGGAME"), pError);
		// Log message
		Log(Message.getData());
		// Show message
		if (!Application.isEditor)
		{
			// Show option to cancel, if possible
			bool fResult = ::pGUI->ShowMessageModal(
			                 Message.getData(),
			                 LoadResStr("IDS_NET_ERR_LEAGUE"),
			                 pCancel ? (C4GUI::MessageDialog::btnOK | C4GUI::MessageDialog::btnAbort) : C4GUI::MessageDialog::btnOK,
			                 C4GUI::Ico_Error);
			if (pCancel)
				*pCancel = !fResult;
		}
		// Failed
		return false;
	}

	// Show message
	if (LeagueServerMessage.getLength())
	{
		StdStrBuf Message = FormatString(LoadResStr("IDS_MSG_LEAGUEGAMESIGNUP"), pLeagueClient->getServerName(), LeagueServerMessage.getData());
		// Log message
		Log(Message.getData());
		// Show message
		if (!Application.isEditor)
		{
			// Show option to cancel, if possible
			bool fResult = ::pGUI->ShowMessageModal(
			                 Message.getData(),
			                 LoadResStr("IDS_NET_ERR_LEAGUE"),
			                 pCancel ? (C4GUI::MessageDialog::btnOK | C4GUI::MessageDialog::btnAbort) : C4GUI::MessageDialog::btnOK,
			                 C4GUI::Ico_Error);
			if (pCancel)
				*pCancel = !fResult;
			if (!fResult)
			{
				LeagueEnd(); DeinitLeague();
				return false;
			}
		}
	}

	// Set game parameters for league game
	Game.Parameters.League = League;
	Game.RandomSeed = Seed;
	if (MaxPlayersLeague)
		Game.Parameters.MaxPlayers = MaxPlayersLeague;
	if (!League.getLength())
	{
		Game.Parameters.LeagueAddress.Clear();
		Game.Parameters.StreamAddress.Clear();
	}
	else
	{
		Game.Parameters.StreamAddress = StreamingAddr;
	}

	// All ok
	fLeagueEndSent = false;
	return true;
}

bool C4Network2::LeagueUpdate()
{
	// Not needed?
	if (!pLeagueClient || !fHost)
		return true;

	// League client currently busy?
	if (pLeagueClient->isBusy())
		return true;

	// Create reference
	C4Network2Reference Ref;
	Ref.InitLocal();

	// Do update
	if (!pLeagueClient->Update(Ref))
	{
		// Log
		LogF(LoadResStr("IDS_NET_ERR_LEAGUE_UPDATEGAME"), pLeagueClient->GetError());
		return false;
	}

	// Timing
	iLastLeagueUpdate = time(nullptr);
	iLeagueUpdateDelay = Config.Network.MasterReferencePeriod;

	return true;
}

bool C4Network2::LeagueUpdateProcessReply()
{
	// safety: A reply must be present
	assert(pLeagueClient);
	assert(fHost);
	assert(!pLeagueClient->isBusy());
	assert(pLeagueClient->getCurrentAction() == C4LA_Update);
	// check reply success
	C4ClientPlayerInfos PlayerLeagueInfos;
	StdStrBuf LeagueServerMessage;
	bool fSucc = pLeagueClient->isSuccess() && pLeagueClient->GetUpdateReply(&LeagueServerMessage, &PlayerLeagueInfos);
	pLeagueClient->ResetCurrentAction();
	if (!fSucc)
	{
		const char *pError = pLeagueClient->GetError() ? pLeagueClient->GetError() :
		                     LeagueServerMessage.getLength() ? LeagueServerMessage.getData() :
		                     LoadResStr("IDS_NET_ERR_LEAGUE_EMPTYREPLY");
		StdStrBuf Message = FormatString(LoadResStr("IDS_NET_ERR_LEAGUE_UPDATEGAME"), pError);
		// Show message - no dialog, because it's not really fatal and might happen in the running game
		Log(Message.getData());
		return false;
	}
	// evaluate reply: Transfer data to players
	// Take round results
	C4PlayerInfoList &TargetList = Game.PlayerInfos;
	C4ClientPlayerInfos *pInfos; C4PlayerInfo *pInfo, *pResultInfo;
	for (int iClient = 0; (pInfos = TargetList.GetIndexedInfo(iClient)); iClient++)
		for (int iInfo = 0; (pInfo = pInfos->GetPlayerInfo(iInfo)); iInfo++)
			if ((pResultInfo = PlayerLeagueInfos.GetPlayerInfoByID(pInfo->GetID())))
			{
				int32_t iLeagueProjectedGain = pResultInfo->GetLeagueProjectedGain();
				if (iLeagueProjectedGain != pInfo->GetLeagueProjectedGain())
				{
					pInfo->SetLeagueProjectedGain(iLeagueProjectedGain);
					pInfos->SetUpdated();
				}
			}
	// transfer info update to other clients
	Players.SendUpdatedPlayers();
	// if lobby is open, notify lobby of updated players
	if (pLobby) pLobby->OnPlayersChange();
	// OMFG SUCCESS!
	return true;
}

bool C4Network2::LeagueEnd(const char *szRecordName, const BYTE *pRecordSHA)
{
	C4RoundResultsPlayers RoundResults;
	StdStrBuf sResultMessage;
	bool fIsError = true;

	// Not needed?
	if (!pLeagueClient || !fHost || fLeagueEndSent)
		return true;

	// Make sure league client is available
	LeagueWaitNotBusy();

	// Try until either aborted or successful
	const int MAX_RETRIES = 10;
	for (int iRetry = 0; iRetry < MAX_RETRIES; iRetry++)
	{

		// Do update
		C4Network2Reference Ref;
		Ref.InitLocal();
		if (!pLeagueClient->End(Ref, szRecordName, pRecordSHA))
		{
			// Log message
			sResultMessage = FormatString(LoadResStr("IDS_NET_ERR_LEAGUE_FINISHGAME"), pLeagueClient->GetError());
			Log(sResultMessage.getData());
			// Show message, allow retry
			if (Application.isEditor) break;
			bool fRetry = ::pGUI->ShowMessageModal(sResultMessage.getData(), LoadResStr("IDS_NET_ERR_LEAGUE"),
			                                       C4GUI::MessageDialog::btnRetryAbort, C4GUI::Ico_Error);
			if (fRetry) continue;
			break;
		}
		// Let's wait for response
		StdStrBuf Message = FormatString(LoadResStr("IDS_NET_LEAGUE_SENDRESULT"), pLeagueClient->getServerName());
		Log(Message.getData());
		// Wait for response
		while (pLeagueClient->isBusy())
		{
			// Check if league server has responded
			if (!pLeagueClient->Execute(100))
				break;
		}
		// Error?
		StdStrBuf LeagueServerMessage;
		if (!pLeagueClient->isSuccess() || !pLeagueClient->GetEndReply(&LeagueServerMessage, &RoundResults))
		{
			const char *pError = pLeagueClient->GetError() ? pLeagueClient->GetError() :
			                     LeagueServerMessage.getLength() ? LeagueServerMessage.getData() :
			                     LoadResStr("IDS_NET_ERR_LEAGUE_EMPTYREPLY");
			sResultMessage.Take(FormatString(LoadResStr("IDS_NET_ERR_LEAGUE_SENDRESULT"), pError));
			if (Application.isEditor) continue;
			// Only retry if we didn't get an answer from the league server
			bool fRetry = !pLeagueClient->isSuccess();
			fRetry = ::pGUI->ShowMessageModal(sResultMessage.getData(), LoadResStr("IDS_NET_ERR_LEAGUE"),
			                                  fRetry ? C4GUI::MessageDialog::btnRetryAbort : C4GUI::MessageDialog::btnAbort,
			                                  C4GUI::Ico_Error);
			if (fRetry) continue;
		}
		else
		{
			// All OK!
			sResultMessage.Copy(LoadResStr(Game.Parameters.isLeague() ? "IDS_MSG_LEAGUEEVALUATIONSUCCESSFU" : "IDS_MSG_INTERNETGAMEEVALUATED"));
			fIsError = false;
		}
		// Done
		break;
	}

	// Show message
	Log(sResultMessage.getData());

	// Take round results
	Game.RoundResults.EvaluateLeague(sResultMessage.getData(), !fIsError, RoundResults);

	// Send round results to other clients
	C4PacketLeagueRoundResults LeagueUpdatePacket(sResultMessage.getData(), !fIsError, RoundResults);
	Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LeagueRoundResults, LeagueUpdatePacket));

	// All done
	fLeagueEndSent = true;
	return true;
}

bool C4Network2::LeaguePlrAuth(C4PlayerInfo *pInfo)
{

	// Not possible?
	if (!pLeagueClient)
		return false;

	// Make sure league client is avilable
	LeagueWaitNotBusy();

	// Official league?
	bool fOfficialLeague = SEqual(pLeagueClient->getServerName(), "league.openclonk.org");

	StdStrBuf Account, Password;
	bool fRememberLogin = false;

	// Default password from login token if present
	if (Config.Network.GetLeagueLoginData(pLeagueClient->getServerName(), pInfo->GetName(), &Account, &Password))
	{
		fRememberLogin = (Password.getLength()>0);
	}
	else
	{
		Account.Copy(pInfo->GetName());
	}

	for (;;)
	{
		// ask for account name and password
		if (!C4LeagueSignupDialog::ShowModal(pInfo->GetName(), Account.getData(), pLeagueClient->getServerName(), &Account, &Password, !fOfficialLeague, false, &fRememberLogin))
			return false;

		// safety (modal dlg may have deleted network)
		if (!pLeagueClient) return false;

		// Send authentication request
		if (!pLeagueClient->Auth(*pInfo, Account.getData(), Password.getData(), nullptr, nullptr, fRememberLogin))
			return false;

		// safety (modal dlg may have deleted network)
		if (!pLeagueClient) return false;

		// Wait for a response
		StdStrBuf Message = FormatString(LoadResStr("IDS_MSG_TRYLEAGUESIGNUP"), pInfo->GetName(), Account.getData(), pLeagueClient->getServerName());
		Log(Message.getData());
		// Set up a dialog
		C4GUI::MessageDialog *pDlg = nullptr;
		if (!Application.isEditor)
		{
			// create & show
			pDlg = new C4GUI::MessageDialog(Message.getData(), LoadResStr("IDS_DLG_LEAGUESIGNUP"), C4GUI::MessageDialog::btnAbort, C4GUI::Ico_NetWait, C4GUI::MessageDialog::dsRegular);
			if (!pDlg || !pDlg->Show(::pGUI, true))  return false;
		}
		// Wait for response
		while (pLeagueClient->isBusy())
		{
			// Execute GUI
			if (!Application.ScheduleProcs() ||
			    (pDlg && pDlg->IsAborted()))
			{
				// Clear up
				if (pDlg) delete pDlg;
				return false;
			}
			// Check if league server has responded
			if (!pLeagueClient->Execute(0))
				break;
		}
		// Close dialog
		if (pDlg)
		{
			pDlg->Close(true);
			delete pDlg;
		}

		// Success?
		StdStrBuf AUID, AccountMaster, LoginToken; bool fUnregistered = false;
		if (pLeagueClient->GetAuthReply(&Message, &AUID, &AccountMaster, &fUnregistered, &LoginToken))
		{

			// Set AUID
			pInfo->SetAuthID(AUID.getData());

			// Remember login data; set or clear login token
			Config.Network.SetLeagueLoginData(pLeagueClient->getServerName(), pInfo->GetName(), Account.getData(), fRememberLogin ? LoginToken.getData() : "");

			// Show welcome message, if any
			bool fSuccess;
			if (Message.getLength())
				fSuccess = ::pGUI->ShowMessageModal(
				             Message.getData(), LoadResStr("IDS_DLG_LEAGUESIGNUPCONFIRM"),
				             C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Ex_League);
			else if (AccountMaster.getLength())
				fSuccess = ::pGUI->ShowMessageModal(
				             FormatString(LoadResStr("IDS_MSG_LEAGUEPLAYERSIGNUPAS"), pInfo->GetName(), AccountMaster.getData(), pLeagueClient->getServerName()).getData(), LoadResStr("IDS_DLG_LEAGUESIGNUPCONFIRM"),
				             C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Ex_League);
			else
				fSuccess = ::pGUI->ShowMessageModal(
				             FormatString(LoadResStr("IDS_MSG_LEAGUEPLAYERSIGNUP"), pInfo->GetName(), pLeagueClient->getServerName()).getData(), LoadResStr("IDS_DLG_LEAGUESIGNUPCONFIRM"),
				             C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Ex_League);

			// Approved?
			if (fSuccess)
				// Done
				return true;
			else
				// Sign-up was cancelled by user
				::pGUI->ShowMessageModal(FormatString(LoadResStr("IDS_MSG_LEAGUESIGNUPCANCELLED"), pInfo->GetName()).getData(), LoadResStr("IDS_DLG_LEAGUESIGNUP"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Notify);

		}
		else
		{

			// Authentification error
			LogF(LoadResStr("IDS_MSG_LEAGUESIGNUPERROR"), Message.getData());
				::pGUI->ShowMessageModal(FormatString(LoadResStr("IDS_MSG_LEAGUESERVERMSG"), Message.getData()).getData(), LoadResStr("IDS_DLG_LEAGUESIGNUPFAILED"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			// after a league server error message, always fall-through to try again
		}

		// Try given account name as default next time
		if (AccountMaster.getLength())
			Account.Take(std::move(AccountMaster));

		// safety (modal dlg may have deleted network)
		if (!pLeagueClient) return false;
	}

}

bool C4Network2::LeaguePlrAuthCheck(C4PlayerInfo *pInfo)
{

	// Not possible?
	if (!pLeagueClient)
		return false;

	// Make sure league client is available
	LeagueWaitNotBusy();

	// Ask league server to check the code
	if (!pLeagueClient->AuthCheck(*pInfo))
		return false;

	// Log
	StdStrBuf Message = FormatString(LoadResStr("IDS_MSG_LEAGUEJOINING"), pInfo->GetName());
	Log(Message.getData());

	// Wait for response
	while (pLeagueClient->isBusy())
		if (!pLeagueClient->Execute(100))
			break;

	// Check response validity
	if (!pLeagueClient->isSuccess())
	{
		LeagueShowError(pLeagueClient->GetError());
		return false;
	}

	// Check if league server approves. pInfo will have league info if this call is successful.
	if (!pLeagueClient->GetAuthCheckReply(&Message, Game.Parameters.League.getData(), pInfo))
	{
		LeagueShowError(FormatString(LoadResStr("IDS_MSG_LEAGUEJOINREFUSED"), pInfo->GetName(), Message.getData()).getData());
		return false;
	}

	return true;
}

void C4Network2::LeagueNotifyDisconnect(int32_t iClientID, C4LeagueDisconnectReason eReason)
{
	// league active?
	if (!pLeagueClient || !Game.Parameters.isLeague()) return;
	// only in running game
	if (!Game.IsRunning || Game.GameOver) return;
	// clients send notifications for their own players; host sends for the affected client players
	if (!isHost()) { if (!Clients.GetLocal()) return; iClientID = Clients.GetLocal()->getID(); }
	// clients only need notifications if they have players in the game
	const C4ClientPlayerInfos *pInfos = Game.PlayerInfos.GetInfoByClientID(iClientID);
	if (!pInfos) return;
	int32_t i=0; C4PlayerInfo *pInfo;
	while ((pInfo = pInfos->GetPlayerInfo(i++))) if (pInfo->IsJoined() && !pInfo->IsRemoved()) break;
	if (!pInfo) return;
	// Make sure league client is avilable
	LeagueWaitNotBusy();
	// report the disconnect!
	LogF(LoadResStr("IDS_LEAGUE_LEAGUEREPORTINGUNEXPECTED"), (int) eReason);
	pLeagueClient->ReportDisconnect(*pInfos, eReason);
	// wait for the reply
	LeagueWaitNotBusy();
	// display it
	const char *szMsg;
	StdStrBuf sMessage;
	if (pLeagueClient->GetReportDisconnectReply(&sMessage))
		szMsg = LoadResStr("IDS_MSG_LEAGUEUNEXPECTEDDISCONNEC");
	else
		szMsg = LoadResStr("IDS_ERR_LEAGUEERRORREPORTINGUNEXP");
	LogF(szMsg, sMessage.getData());
}

void C4Network2::LeagueWaitNotBusy()
{
	// league client busy?
	if (!pLeagueClient || !pLeagueClient->isBusy()) return;
	// wait for it
	Log(LoadResStr("IDS_LEAGUE_WAITINGFORLASTLEAGUESERVE"));
	while (pLeagueClient->isBusy())
		if (!pLeagueClient->Execute(100))
			break;
	// if last request was an update request, process it
	if (pLeagueClient->getCurrentAction() == C4LA_Update)
		LeagueUpdateProcessReply();
}

void C4Network2::LeagueSurrender()
{
	// there's currently no functionality to surrender in the league
	// just stop responding so other clients will notify the disconnect
	DeinitLeague();
}

void C4Network2::LeagueShowError(const char *szMsg)
{
	if (!Application.isEditor)
	{
		::pGUI->ShowErrorMessage(szMsg);
	}
	else
	{
		LogF(LoadResStr("IDS_LGA_SERVERFAILURE"), szMsg);
	}
}

void C4Network2::Vote(C4ControlVoteType eType, bool fApprove, int32_t iData)
{
	// Original vote?
	if (!GetVote(C4ClientIDUnknown, eType, iData))
	{
		// Too fast?
		if (time(nullptr) < (time_t) (iLastOwnVoting + C4NetMinVotingInterval))
		{
			Log(LoadResStr("IDS_TEXT_YOUCANONLYSTARTONEVOTINGE"));
			if ((eType == VT_Kick && iData == Game.Clients.getLocalID()) || eType == VT_Cancel)
				OpenSurrenderDialog(eType, iData);
			return;
		}
		// Save timestamp
		iLastOwnVoting = time(nullptr);
	}
	// Already voted? Ignore
	if (GetVote(::Control.ClientID(), eType, iData))
		return;
	// Set pause mode if this is the host
	if (isHost() && isRunning())
	{
		Pause();
		fPausedForVote = true;
	}
	// send vote control
	::Control.DoInput(CID_Vote, new C4ControlVote(eType, fApprove, iData), CDT_Direct);
}

void C4Network2::AddVote(const C4ControlVote &Vote)
{
	// Save back timestamp
	if (!Votes.firstPkt())
		iVoteStartTime = time(nullptr);
	// Save vote back
	Votes.Add(CID_Vote, new C4ControlVote(Vote));
	// Set pause mode if this is the host
	if (isHost() && isRunning())
	{
		Pause();
		fPausedForVote = true;
	}
	// Check if the dialog should be opened
	OpenVoteDialog();
}

C4IDPacket *C4Network2::GetVote(int32_t iClientID, C4ControlVoteType eType, int32_t iData)
{
	C4ControlVote *pVote;
	for (C4IDPacket *pPkt = Votes.firstPkt(); pPkt; pPkt = Votes.nextPkt(pPkt))
		if (pPkt->getPktType() == CID_Vote)
			if ((pVote = static_cast<C4ControlVote *>(pPkt->getPkt())))
				if (iClientID == C4ClientIDUnknown || pVote->getByClient() == iClientID)
					if (pVote->getType() == eType && pVote->getData() == iData)
						return pPkt;
	return nullptr;
}

void C4Network2::EndVote(C4ControlVoteType eType, bool fApprove, int32_t iData)
{
	// Remove all vote packets
	C4IDPacket *pPkt; int32_t iOrigin = C4ClientIDUnknown;
	while ((pPkt = GetVote(C4ClientIDAll, eType, iData)))
	{
		if (iOrigin == C4ClientIDUnknown)
			iOrigin = static_cast<C4ControlVote *>(pPkt->getPkt())->getByClient();
		Votes.Delete(pPkt);
	}
	// Reset timestamp
	iVoteStartTime = time(nullptr);
	// Approved own voting? Reset voting block
	if (fApprove && iOrigin == Game.Clients.getLocalID())
		iLastOwnVoting = 0;
	// Dialog open?
	if (pVoteDialog)
		if (pVoteDialog->getVoteType() == eType && pVoteDialog->getVoteData() == iData)
		{
			// close
			delete pVoteDialog;
			pVoteDialog = nullptr;
		}
	// Did we try to kick ourself? Ask if we'd like to surrender
	bool fCancelVote = (eType == VT_Kick && iData == Game.Clients.getLocalID()) || eType == VT_Cancel;
	if (!fApprove && fCancelVote && iOrigin == Game.Clients.getLocalID())
		OpenSurrenderDialog(eType, iData);
	// Check if the dialog should be opened
	OpenVoteDialog();
	// Pause/unpause voting?
	if (fApprove && eType == VT_Pause)
		fPausedForVote = !iData;
	// No voting left? Reset pause.
	if (!Votes.firstPkt())
		if (fPausedForVote)
		{
			Start();
			fPausedForVote = false;
		}
}

void C4Network2::OpenVoteDialog()
{
	// Dialog already open?
	if (pVoteDialog) return;
	// No vote available?
	if (!Votes.firstPkt()) return;
	// Can't vote?
	C4ClientPlayerInfos *pPlayerInfos = Game.PlayerInfos.GetInfoByClientID(Game.Clients.getLocalID());
	if (!pPlayerInfos || !pPlayerInfos->GetPlayerCount() || !pPlayerInfos->GetJoinedPlayerCount())
		return;
	// Search a voting we have to vote on
	for (C4IDPacket *pPkt = Votes.firstPkt(); pPkt; pPkt = Votes.nextPkt(pPkt))
	{
		// Already voted on this matter?
		C4ControlVote *pVote = static_cast<C4ControlVote *>(pPkt->getPkt());
		if (!GetVote(::Control.ClientID(), pVote->getType(), pVote->getData()))
		{
			// Compose message
			C4Client *pSrcClient = Game.Clients.getClientByID(pVote->getByClient());
			StdStrBuf Msg; Msg.Format(LoadResStr("IDS_VOTE_WANTSTOALLOW"), pSrcClient ? pSrcClient->getName() : "???", pVote->getDesc().getData());
			Msg.AppendChar('|');
			Msg.Append(pVote->getDescWarning());

			// Open dialog
			pVoteDialog = new C4VoteDialog(Msg.getData(), pVote->getType(), pVote->getData(), false);
			pVoteDialog->SetDelOnClose();
			pVoteDialog->Show(::pGUI, true);

			break;
		}
	}
}

void C4Network2::OpenSurrenderDialog(C4ControlVoteType eType, int32_t iData)
{
	if (!pVoteDialog)
	{
		pVoteDialog = new C4VoteDialog(
		  LoadResStr("IDS_VOTE_SURRENDERWARNING"), eType, iData, true);
		pVoteDialog->SetDelOnClose();
		pVoteDialog->Show(::pGUI, true);
	}
}


void C4Network2::OnVoteDialogClosed()
{
	pVoteDialog = nullptr;
}


// *** C4VoteDialog

C4VoteDialog::C4VoteDialog(const char *szText, C4ControlVoteType eVoteType, int32_t iVoteData, bool fSurrender)
		: MessageDialog(szText, LoadResStr("IDS_DLG_VOTING"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_Confirm, C4GUI::MessageDialog::dsRegular, nullptr, true),
		eVoteType(eVoteType), iVoteData(iVoteData), fSurrender(fSurrender)
{

}

void C4VoteDialog::OnClosed(bool fOK)
{
	bool fAbortGame = false;
	// notify that this object will be deleted shortly
	::Network.OnVoteDialogClosed();
	// Was league surrender dialog
	if (fSurrender)
	{
		// League surrender accepted
		if (fOK)
		{
			// set game leave reason, although round results dialog isn't showing it ATM
			Game.RoundResults.EvaluateNetwork(C4RoundResults::NR_NetError, LoadResStr("IDS_ERR_YOUSURRENDEREDTHELEAGUEGA"));
			// leave game
			::Network.LeagueSurrender();
			::Network.Clear();
			// We have just league-surrendered. Abort the game - that is what we originally wanted.
			// Note: as we are losing league points and this is a relevant game, it would actually be
			// nice to show an evaluation dialog which tells us that we have lost and how many league
			// points we have lost. But until the evaluation dialog can actually do that, it is better
			// to abort completely.
			// Note2: The league dialog will never know that, because the game will usually not be over yet.
			// Scores are not calculated until after the game.
			fAbortGame = true;
		}
	}
	// Was normal vote dialog
	else
	{
		// Vote still active? Then vote.
		if (::Network.GetVote(C4ClientIDUnknown, eVoteType, iVoteData))
			::Network.Vote(eVoteType, fOK, iVoteData);
	}
	// notify base class
	MessageDialog::OnClosed(fOK);
	// Abort game
	if (fAbortGame)
		Game.Abort(true);
}


/* Lobby countdown */

void C4Network2::StartLobbyCountdown(int32_t iCountdownTime)
{
	// abort previous
	if (pLobbyCountdown) AbortLobbyCountdown();
	// start new
	pLobbyCountdown = new C4GameLobby::Countdown(iCountdownTime);
}

void C4Network2::AbortLobbyCountdown()
{
	// aboert lobby countdown
	if (pLobbyCountdown)
	{
		pLobbyCountdown->Abort();
		delete pLobbyCountdown;
		pLobbyCountdown = nullptr;
	}
}

/* Streaming */

bool C4Network2::StartStreaming(C4Record *pRecord)
{
	// Save back
	fStreaming = true;
	pStreamedRecord = pRecord;
	iLastStreamAttempt = time(nullptr);

	// Initialize compressor
	ZeroMem(&StreamCompressor, sizeof(StreamCompressor));
	if (deflateInit(&StreamCompressor, 9) != Z_OK)
		return false;

	// Create stream buffer
	StreamingBuf.New(C4NetStreamingMaxBlockSize);
	StreamCompressor.next_out = reinterpret_cast<BYTE*>(StreamingBuf.getMData());
	StreamCompressor.avail_out = C4NetStreamingMaxBlockSize;

	// Initialize HTTP client
	pStreamer = new C4Network2HTTPClient();
	if (!pStreamer->Init())
		return false;
	Application.Add(pStreamer);

	return true;
}

bool C4Network2::FinishStreaming()
{
	if (!fStreaming) return false;

	// Stream
	StreamIn(true);

	// Reset record pointer
	pStreamedRecord = nullptr;

	// Try to get rid of remaining data immediately
	iLastStreamAttempt = 0;
	StreamOut();

	return true;
}

bool C4Network2::StopStreaming()
{
	if (!fStreaming) return false;

	// Clear
	Application.Remove(pStreamer);
	fStreaming = false;
	pStreamedRecord = nullptr;
	deflateEnd(&StreamCompressor);
	StreamingBuf.Clear();
	delete pStreamer;
	pStreamer = nullptr;

	// ... finalization?
	return true;
}

bool C4Network2::StreamIn(bool fFinish)
{
	if (!pStreamedRecord) return false;

	// Get data from record
	const StdBuf &Data = pStreamedRecord->GetStreamingBuf();
	if (!fFinish)
		if (!Data.getSize() || !StreamCompressor.avail_out)
			return false;

	do
	{

		// Compress
		StreamCompressor.next_in = const_cast<BYTE *>(getBufPtr<BYTE>(Data));
		StreamCompressor.avail_in = Data.getSize();
		int ret = deflate(&StreamCompressor, fFinish ? Z_FINISH : Z_NO_FLUSH);

		// Anything consumed?
		unsigned int iInAmount = Data.getSize() - StreamCompressor.avail_in;
		if (iInAmount > 0)
			pStreamedRecord->ClearStreamingBuf(iInAmount);

		// Done?
		if (!fFinish || ret == Z_STREAM_END)
			break;

		// Error while finishing?
		if (ret != Z_OK)
			return false;

		// Enlarge buffer, if neccessary
		size_t iPending = getPendingStreamData();
		size_t iGrow = StreamingBuf.getSize();
		StreamingBuf.Grow(iGrow);
		StreamCompressor.avail_out += iGrow;
		StreamCompressor.next_out = getMBufPtr<BYTE>(StreamingBuf, iPending);

	}
	while (true);

	return true;
}

bool C4Network2::StreamOut()
{
	// Streamer busy?
	if (!pStreamer || pStreamer->isBusy())
		return false;

	// Streamer done?
	if (pStreamer->isSuccess())
	{

		// Move new data to front of buffer
		if (getPendingStreamData() != iCurrentStreamAmount)
			StreamingBuf.Move(iCurrentStreamAmount, getPendingStreamData() - iCurrentStreamAmount);

		// Free buffer space
		StreamCompressor.next_out -= iCurrentStreamAmount;
		StreamCompressor.avail_out += iCurrentStreamAmount;

		// Advance stream
		iCurrentStreamPosition += iCurrentStreamAmount;

		// Get input
		StreamIn(false);
	}

	// Clear streamer
	pStreamer->Clear();

	// Record is still running?
	if (pStreamedRecord)
	{

		// Enough available to send?
		if (getPendingStreamData() < C4NetStreamingMinBlockSize)
			return false;

		// Overflow protection
		if (iLastStreamAttempt && iLastStreamAttempt + C4NetStreamingInterval >= time(nullptr))
			return false;

	}
	// All data finished?
	else if (!getPendingStreamData())
	{
		// Then we're done.
		StopStreaming();
		return false;
	}

	// Set stream address
	StdStrBuf StreamAddr;
	StreamAddr.Copy(Game.Parameters.StreamAddress);
	StreamAddr.AppendFormat("pos=%d&end=%d", iCurrentStreamPosition, !pStreamedRecord);
	pStreamer->SetServer(StreamAddr.getData());

	// Send data
	size_t iStreamAmount = getPendingStreamData();
	iCurrentStreamAmount = iStreamAmount;
	iLastStreamAttempt = time(nullptr);
	return  pStreamer->Query(StdBuf(StreamingBuf.getData(), iStreamAmount), false);
}

bool C4Network2::isStreaming() const
{
	// Streaming must be active and there must still be anything to stream
	return fStreaming;
}
