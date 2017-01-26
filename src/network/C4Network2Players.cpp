/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// NET2 network player management
// see header for some additional information
// 2do: Handle client joins after game go but before runtime join (Frame 0)?
//      Those will not receive a player info list right in time

#include "C4Include.h"
#include "network/C4Network2Players.h"
#include "control/C4PlayerInfo.h"
#include "gui/C4GameLobby.h"
#include "game/C4Game.h"
#include "network/C4Network2.h"
#include "control/C4GameControl.h"
#include "control/C4RoundResults.h"

#include "control/C4Control.h"

// *** C4Network2Players

C4Network2Players::C4Network2Players() : rInfoList(Game.Parameters.PlayerInfos)
{
	// ctor - init rInfoList-ref to only
}

void C4Network2Players::Init()
{
	// caution: In this call, only local players are joined
	// remote players may have been added already for runtime joins
	// not in replay
	if (Game.C4S.Head.Replay) return;
	// network only
	assert(::Network.isEnabled());
	// must init before game is running
	assert(!Game.IsRunning);
	if (::Network.isHost())
	{
		// host: Rejoin script players from savegame before joining local players so team distribution is done correctly
		// But prepare empty host list before recreation
		JoinLocalPlayer("", true);
		Game.PlayerInfos.CreateRestoreInfosForJoinedScriptPlayers(Game.RestorePlayerInfos);
		JoinLocalPlayer(Game.PlayerFilenames, false);
	}
	else
	{
		// Client: join the local player(s)
		JoinLocalPlayer(Game.PlayerFilenames, true);
	}
}

void C4Network2Players::Clear()
{
	// nothing...
}

bool C4Network2Players::JoinLocalPlayer(const char *szLocalPlayerFilename, bool initial)
{
	// ignore in replay
	// shouldn't even come here though
	assert(!Game.C4S.Head.Replay);
	if (Game.C4S.Head.Replay) return false;
	// if observing: don't try
	if (Game.Clients.getLocal()->isObserver()) return false;
	// network only
	assert(::Network.isEnabled());
	// create join info packet
	C4ClientPlayerInfos JoinInfo(szLocalPlayerFilename, !initial);
	// league game: get authentication for players
	if (Game.Parameters.isLeague())
		for (int i = 0; i < JoinInfo.GetPlayerCount(); i++)
		{
			C4PlayerInfo *pInfo = JoinInfo.GetPlayerInfo(i);
			if (!::Network.LeaguePlrAuth(pInfo))
			{
				JoinInfo.RemoveIndexedInfo(i);
				i--;
			}
		}
	// host or client?
	if (::Network.isHost())
	{
		// error joining players? Zero players is OK for initial packet; marks host as observer
		if (!initial && !JoinInfo.GetPlayerCount()) return false;
		// handle it as a direct request
		HandlePlayerInfoUpdRequest(&JoinInfo, true);
	}
	else
	{
		// clients request initial joins at host only
		// create player info for local player joins
		C4PacketPlayerInfoUpdRequest JoinRequest(JoinInfo);
		// any players to join? Zero players is OK for initial packet; marks client as observer
		// it's also necessary to send the empty player info packet, so the host will answer
		// with infos of all other clients
		if (!initial && !JoinRequest.Info.GetPlayerCount()) return false;
		::Network.Clients.SendMsgToHost(MkC4NetIOPacket(PID_PlayerInfoUpdReq, JoinRequest));
		// request activation
		if (JoinRequest.Info.GetPlayerCount() && !Game.Clients.getLocal()->isActivated())
			::Network.RequestActivate();
	}
	// done, success
	return true;
}

void C4Network2Players::RequestPlayerInfoUpdate(const class C4ClientPlayerInfos &rRequest)
{
	// network only
	assert(::Network.isEnabled());
	// host or client?
	if (::Network.isHost())
	{
		// host processes directly
		HandlePlayerInfoUpdRequest(&rRequest, true);
	}
	else
	{
		// client sends request to host
		C4PacketPlayerInfoUpdRequest UpdateRequest(rRequest);
		::Network.Clients.SendMsgToHost(MkC4NetIOPacket(PID_PlayerInfoUpdReq, UpdateRequest));
	}
}

void C4Network2Players::HandlePlayerInfoUpdRequest(const class C4ClientPlayerInfos *pInfoPacket, bool fByHost)
{
	// network host only
	assert(::Network.isEnabled());
	assert(::Network.isHost());
	// copy client infos (need to be adjusted)
	C4ClientPlayerInfos OwnInfoPacket(*pInfoPacket);
	// safety: check any duplicate, unjoined players first
	// check those with unassigned IDs only, so update packets won't be rejected by this
	if (!OwnInfoPacket.IsInitialPacket())
	{
		C4ClientPlayerInfos *pExistingClientInfo = rInfoList.GetInfoByClientID(OwnInfoPacket.GetClientID());
		if (pExistingClientInfo)
		{
			int iCnt=OwnInfoPacket.GetPlayerCount(); C4PlayerInfo *pPlrInfo;
			C4Network2Res *pRes;
			while (iCnt--) if ((pPlrInfo=OwnInfoPacket.GetPlayerInfo(iCnt)))
					if (!pPlrInfo->GetID()) if ((pRes = pPlrInfo->GetRes()))
							if (pExistingClientInfo->GetPlayerInfoByRes(pRes->getResID()))
							{
								// double join: simply deny without message
#ifdef _DEBUG
								Log("Network: Duplicate player join rejected!");
#endif
								OwnInfoPacket.RemoveIndexedInfo(iCnt);
							}
		}
		if (!OwnInfoPacket.GetPlayerCount())
		{
			// player join request without players: probably all removed because doubled
#ifdef _DEBUG
			Log("Network: Empty player join request ignored!");
#endif
			return;
		}
	}
	// assign player IDs
	if (!rInfoList.AssignPlayerIDs(&OwnInfoPacket) && OwnInfoPacket.IsAddPacket())
	{
		// no players could be joined in an add request: probably because the maximum player limit has been reached
		return;
	}
	// check doubled savegame player usage
	UpdateSavegameAssignments(&OwnInfoPacket);
	// update teams
	rInfoList.AssignTeams(&OwnInfoPacket, fByHost);
	// update any other player colors and names
	// this may only change colors and names of all unjoined players (which is all players in lobby mode)
	// any affected players will get an updated-flag
	rInfoList.UpdatePlayerAttributes(&OwnInfoPacket, true);
	// league score gains may now be different
	rInfoList.ResetLeagueProjectedGain(true);
	int32_t iPlrInfo = 0;
	C4PlayerInfo *pPlrInfo;
	while ((pPlrInfo = OwnInfoPacket.GetPlayerInfo(iPlrInfo++))) pPlrInfo->ResetLeagueProjectedGain();
	if (Game.Parameters.isLeague())
	{
		// check league authentication for new players
		for (int i = 0; i < OwnInfoPacket.GetPlayerCount(); i++)
		{
			if (!rInfoList.GetPlayerInfoByID(OwnInfoPacket.GetPlayerInfo(i)->GetID()))
			{
				C4PlayerInfo *pInfo = OwnInfoPacket.GetPlayerInfo(i);
				// remove normal (non-script) player infos without authentication or when not in the lobby
				if (pInfo->GetType() != C4PT_Script && (!::Network.isLobbyActive() || !::Network.LeaguePlrAuthCheck(pInfo)))
				{
					OwnInfoPacket.RemoveIndexedInfo(i);
					i--;
				}
				else
					// always reset authentication ID after check - it's not needed anymore
					pInfo->SetAuthID("");
			}
		}
	}
	// send updates to all other clients and reset update flags
	SendUpdatedPlayers();
	// finally, add new player join as direct input
	// this will add the player infos directly on host side (DirectExec as a subcall),
	// so future player join request will take the other joined  clients into consideration
	// when assigning player colors, etc.; it will also start resource loading
	// in running mode, this call will also put the actual player joins into the queue
	::Control.DoInput(CID_PlrInfo, new C4ControlPlayerInfo(OwnInfoPacket), CDT_Direct);
	// notify lobby of updates
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby) pLobby->OnPlayersChange();
}

void C4Network2Players::HandlePlayerInfo(const class C4ClientPlayerInfos &rInfoPacket)
{
	// network only
	assert(::Network.isEnabled());
	// copy client player infos out of packet to be used in local list
	C4ClientPlayerInfos *pClientInfo = new C4ClientPlayerInfos(rInfoPacket);
	// add client info  to local player info list - eventually deleting pClientInfo and
	// returning a pointer to the new info structure when multiple player infos are merged
	// may also replace existing info, if this is an update-call
	pClientInfo = rInfoList.AddInfo(pClientInfo);
	// make sure team list reflects teams set in player infos
	Game.Teams.RecheckPlayers();
	Game.Teams.RecheckTeams(); // recheck random teams - if a player left, teams may need to be rebalanced
	// make sure resources are loaded for those players
	rInfoList.LoadResources();
	// get associated client - note that pClientInfo might be nullptr for empty packets that got discarded
	if (pClientInfo)
	{
		const C4Client *pClient = Game.Clients.getClientByID(pClientInfo->GetClientID());
		// host, game running and client active already?
		if (::Network.isHost() && ::Network.isRunning() && pClient && pClient->isActivated())
		{
			// then join the players immediately
			JoinUnjoinedPlayersInControlQueue(pClientInfo);
		}
	}
	// adding the player may have invalidated other players (through team settings). Send them.
	SendUpdatedPlayers();
	// lobby: update players
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	if (pLobby) pLobby->OnPlayersChange();
	// invalidate reference
	::Network.InvalidateReference();
}

void C4Network2Players::SendUpdatedPlayers()
{
	// check all clients for update
	C4ClientPlayerInfos *pUpdInfo; int i=0;
	while ((pUpdInfo = rInfoList.GetIndexedInfo(i++)))
		if (pUpdInfo->IsUpdated())
		{
			pUpdInfo->ResetUpdated();
			C4ControlPlayerInfo *pkSend = new C4ControlPlayerInfo(*pUpdInfo);
			// send info to all
			::Control.DoInput(CID_PlrInfo, pkSend, CDT_Direct);
		}
}

void C4Network2Players::UpdateSavegameAssignments(C4ClientPlayerInfos *pNewInfo)
{
	// safety
	if (!pNewInfo) return;
	// check all joins of new info; backwards so they can be deleted
	C4PlayerInfo *pInfo, *pInfo2, *pSaveInfo; int i=pNewInfo->GetPlayerCount(), j, id;
	while (i--) if ((pInfo = pNewInfo->GetPlayerInfo(i)))
			if ((id=pInfo->GetAssociatedSavegamePlayerID()))
			{
				// check for non-existant savegame players
				if (!(pSaveInfo=Game.RestorePlayerInfos.GetPlayerInfoByID(id)))
				{
					pInfo->SetAssociatedSavegamePlayer(id=0);
					pNewInfo->SetUpdated();
				}
				// check for duplicates (can't really occur...)
				if (id)
				{
					j=i;
					while ((pInfo2 = pNewInfo->GetPlayerInfo(++j)))
						if (pInfo2->GetAssociatedSavegamePlayerID() == id)
						{
							// fix it by resetting the savegame info
							pInfo->SetAssociatedSavegamePlayer(id=0);
							pNewInfo->SetUpdated(); break;
						}
				}
				// check against all infos of other clients
				C4ClientPlayerInfos *pkClientInfo; int k=0;
				while ((pkClientInfo = rInfoList.GetIndexedInfo(k++)) && id)
				{
					// if it's not an add packet, don't check own client twice
					if (pkClientInfo->GetClientID() == pNewInfo->GetClientID() && !(pNewInfo->IsAddPacket()))
						continue;
					// check against all players
					j=0;
					while ((pInfo2 = pkClientInfo->GetPlayerInfo(j++)))
						if (pInfo2->GetAssociatedSavegamePlayerID() == id)
						{
							// fix it by resetting the savegame info
							pInfo->SetAssociatedSavegamePlayer(id=0);
							pNewInfo->SetUpdated(); break;
						}
				}
				// if the player joined just for the savegame assignment, and that failed, delete it
				if (!id && pInfo->IsJoinForSavegameOnly())
					pNewInfo->RemoveIndexedInfo(i);
				// prev info
			}
}

void C4Network2Players::ResetUpdatedPlayers()
{
	// mark all client packets as up-to-date
	C4ClientPlayerInfos *pUpdInfo; int i=0;
	while ((pUpdInfo = rInfoList.GetIndexedInfo(i++))) pUpdInfo->ResetUpdated();
}

void C4Network2Players::JoinUnjoinedPlayersInControlQueue(C4ClientPlayerInfos *pNewPacket)
{
	// only host may join any players to the queue
	assert(::Network.isHost());
	// check all players
	int i=0; C4PlayerInfo *pInfo;
	while ((pInfo = pNewPacket->GetPlayerInfo(i++)))
		// not yet joined and no savegame assignment?
		if (!pInfo->HasJoinIssued()) if (!pInfo->GetAssociatedSavegamePlayerID())
			{
				// join will be marked when queue is executed (C4Player::Join)
				// but better mark join now already to prevent permanent sending overkill
				pInfo->SetJoinIssued();
				// do so!
				C4Network2Res *pPlrRes = pInfo->GetRes();
				C4Network2Client *pClient = ::Network.Clients.GetClientByID(pNewPacket->GetClientID());
				if (!pPlrRes || (!pClient && pNewPacket->GetClientID() != ::Control.ClientID()))
					if (pInfo->GetType() != C4PT_Script)
					{
						// failure: Non-script players must have a res to join from!
						const char *szPlrName = pInfo->GetName(); if (!szPlrName) szPlrName="???";
						LogF("Network: C4Network2Players::JoinUnjoinedPlayersInControlQueue failed to join player %s!", szPlrName);
						continue;
					}
				if (pPlrRes)
				{
					// join with resource
					Game.Input.Add(CID_JoinPlr,
					               new C4ControlJoinPlayer(pPlrRes->getFile(), pNewPacket->GetClientID(), pInfo->GetID(), pPlrRes->getCore()));
				}
				else
				{
					// join without resource (script player)
					Game.Input.Add(CID_JoinPlr,
					               new C4ControlJoinPlayer(nullptr, pNewPacket->GetClientID(), pInfo->GetID()));
				}
			}
}

void C4Network2Players::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2IOConnection *pConn)
{
	if (!pConn) return;

	// find associated client
	C4Network2Client *pClient = ::Network.Clients.GetClient(pConn);
	if (!pClient) pClient = ::Network.Clients.GetClientByID(pConn->getClientID());

#define GETPKT(type, name) \
    assert(pPacket); const type &name = \
     static_cast<const type &>(*pPacket);

	// player join request?
	if (cStatus == PID_PlayerInfoUpdReq)
	{
		GETPKT(C4PacketPlayerInfoUpdRequest, pkPlrInfo);
		// this packet is sent to the host only, and thus cannot have been sent from the host
		if (!::Network.isHost()) return;
		// handle this packet
		HandlePlayerInfoUpdRequest(&pkPlrInfo.Info, false);
	}
	else if (cStatus == PID_LeagueRoundResults)
	{
		GETPKT(C4PacketLeagueRoundResults, pkLeagueInfo);
		// accepted from the host only
		if (!pClient || !pClient->isHost()) return;
		// process
		Game.RoundResults.EvaluateLeague(pkLeagueInfo.sResultsString.getData(), pkLeagueInfo.fSuccess, pkLeagueInfo.Players);
	}

#undef GETPKT
}

void C4Network2Players::OnClientPart(C4Client *pPartClient)
{
	// lobby could be notified about the removal - but this would be redundant, because
	// client leave notification is already done directly; this will delete any associated players
	C4ClientPlayerInfos **ppCltInfo = rInfoList.GetInfoPtrByClientID(pPartClient->getID());
	// abort here if no info is registered - client seems to have had a short life only, anyway...
	if (!ppCltInfo) return;
	// remove all unjoined player infos
	for (int32_t i = 0; i < (*ppCltInfo)->GetPlayerCount();)
	{
		C4PlayerInfo *pInfo = (*ppCltInfo)->GetPlayerInfo(i);
		// not joined yet? remove it
		if (!pInfo->HasJoined())
			(*ppCltInfo)->RemoveIndexedInfo(i);
		else
			// just ignore, the "removed" flag will be set eventually
			i++;
	}
	// empty? remove
	if (!(*ppCltInfo)->GetPlayerCount())
		rInfoList.RemoveInfo(ppCltInfo);
	// update team association to left player
	Game.Teams.RecheckPlayers();
	// host: update player data according to leaver
	if (::Network.isHost() && ::Network.isEnabled())
	{
		// host: update any player colors and names
		rInfoList.UpdatePlayerAttributes();
		// team distribution of remaining unjoined players may change
		Game.Teams.RecheckTeams();
		// league score gains may now be different
		Game.PlayerInfos.ResetLeagueProjectedGain(true);
		// send changes to all clients and reset update flags
		SendUpdatedPlayers();
	}
	// invalidate reference
	if (::Network.isHost())
		::Network.InvalidateReference();
}

void C4Network2Players::OnStatusGoReached()
{
	// host only
	if (!::Network.isHost()) return;
	// check all player lists
	int i=0; C4ClientPlayerInfos *pkInfo;
	while ((pkInfo = rInfoList.GetIndexedInfo(i++)))
		// any unsent player joins?
		if (pkInfo->HasUnjoinedPlayers())
		{
			// get client core
			const C4Client *pClient = Game.Clients.getClientByID(pkInfo->GetClientID());
			// don't send if client is unknown or not activated yet
			if (!pClient || !pClient->isActivated()) continue;
			// send them w/o info packet
			// info packets are synced during pause mode
			JoinUnjoinedPlayersInControlQueue(pkInfo);
		}
}

C4ClientPlayerInfos *C4Network2Players::GetLocalPlayerInfoPacket() const
{
	// get local client ID
	int iLocalClientID = Game.Clients.getLocalID();
	// check all packets for same client ID as local
	int i=0; C4ClientPlayerInfos *pkInfo;
	while ((pkInfo = rInfoList.GetIndexedInfo(i++)))
		if (pkInfo->GetClientID() == iLocalClientID)
			// found
			return pkInfo;
	// not found
	return nullptr;
}

C4ClientPlayerInfos *C4Network2Players::GetIndexedPlayerInfoPacket(int iIndex)
{
	// just get from info list
	return  rInfoList.GetIndexedInfo(iIndex);
}

DWORD C4Network2Players::GetClientChatColor(int idForClient, bool fLobby) const
{
	// return color of first joined player; force to white for unknown
	// deactivated always white
	const C4Client *pClient = Game.Clients.getClientByID(idForClient);
	if (pClient && pClient->isActivated())
	{
		// get players for activated
		C4ClientPlayerInfos *pInfoPacket = rInfoList.GetInfoByClientID(idForClient);
		C4PlayerInfo *pPlrInfo;
		if (pInfoPacket && (pPlrInfo = pInfoPacket->GetPlayerInfo(0, C4PT_User)))
		{
			if (fLobby)
				return pPlrInfo->GetLobbyColor();
			else
				return pPlrInfo->GetColor();
		}
	}
	// default color
	return 0xffffff;
}

