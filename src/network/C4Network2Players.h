/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2005  Sven Eberhardt
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
// network player management
// manages synchronization of the player info list in network mode (lobby and runtime)
// also handles player joins from the list
//
// Synchronization is centralized; i.e. clients send join request only
// (C4PlayerJoinRequest), and the host then broadcasts list changes to all clients
// (C4ControlPlayerInfo). PlayerInfo-controls are sent as host packets. In pause mode,
// they will be sent end executed directly, while in running mode exchange will be done
// via queue to ensure synchronization.
//
// Upon entering running mode, actual ControlQueue-PlayerJoin packets are sent by the
// host for all list entries [/added since the run mode was last left]. In running mode,
// the PlayerJoin-control will be sent directly after the PlayerInfo-control.

#ifndef INC_C4Network2Players
#define INC_C4Network2Players

#include "C4PacketBase.h"

// class predefs
class C4Network2Players;

// network player management
class C4Network2Players
	{
	private:
		class C4PlayerInfoList &rInfoList;  // list of player infos - points to Game.PlayerInfos

	public:
		C4Network2Players();     // ctor
		~C4Network2Players() { } // dtor

		void Init();            // add local players; add player file ressources - should be called with net connections initialized
		void Clear();           // clear all player infos
		bool JoinLocalPlayer(const char *szLocalPlayerFilename, bool fAdd); // join a local player (to game or lobby) - sends to host/and or schedules for queue

	public:
		void SendUpdatedPlayers(); // send all player infos with updated flags to all clients (host only!)
		void ResetUpdatedPlayers(); // resets all update-flags (host only!)

	private:
		void UpdateSavegameAssignments(class C4ClientPlayerInfos *pNewInfo); // resolve any savegame assignment conflicts of unjoined players; sets update-flags (host only!)

		// add join-packet for given client players to control queue (host only!)
		// if fSendInfo is true, the given info packet is sent via queue, too, and all players in it are marked as joined
		void JoinUnjoinedPlayersInControlQueue(class C4ClientPlayerInfos *pNewPacket);
	public:
		// callbacks from network system
		void HandlePacket(char cStatus, const C4PacketBase *pPacket, class C4Network2IOConnection *pConn);
		void OnClientPart(class C4Client *pPartClient); // called when a client disconnects - deletes all player infos and removes all players
		void OnStatusGoReached();                         // called when game starts, or continues from pause mode: send any unsent player joins

		// request (client) or directly process (host) an update to existing player infos
		// calls HandlePlayerInfoUpdRequest on host
		void RequestPlayerInfoUpdate(const class C4ClientPlayerInfos &rRequest);

		// player info packet received; handle it (CID_PlayerInfo, host and clients)
		void HandlePlayerInfo(const class C4ClientPlayerInfos &rInfoPacket);

		// player join request received; handle it (PID_PlayerInfoUpdReq, host only)
		// adjusts the player data (colors, etc.), and creates update packets/controls
		void HandlePlayerInfoUpdRequest(const class C4ClientPlayerInfos *pInfoPacket, bool fByHost);

		// some query fns
		C4ClientPlayerInfos *GetLocalPlayerInfoPacket() const; // get player info packet for local client (created in Init())
		C4ClientPlayerInfos *GetIndexedPlayerInfoPacket(int iIndex);  // get player info packet by index
		DWORD GetClientChatColor(int idForClient, bool fLobby) const;
	};

#endif // INC_C4Network2Players
