// God mode functions.

global func InitGodModeMessageBoard()
{
	AddMsgBoardCmd("godmode", "SetGodMode(%player%, \"%s\")");
	return;
}

global func SetGodMode(int plr, string mode)
{
	if (!IsFirstPlayer(plr))
		return CustomMessage("$MessageBoardOnlyHost$", nil, plr);
	if (mode == "off")
	{
		Settings_GodMode = CSETTING_GodMode_Off;
		SetGodModeOff();
		CustomMessage("$MessageBoardModeOff$", nil, plr);
		return;
	}
	if (mode == "host")
	{
		Settings_GodMode = CSETTING_GodMode_Host;
		SetGodModeHost();
		CustomMessage("$MessageBoardModeHost$", nil, plr);
		return;
	}
	if (mode == "all")
	{
		Settings_GodMode = CSETTING_GodMode_All;
		SetGodModeAll();
		CustomMessage("$MessageBoardModeAll$", nil, plr);
		return;
	}
	CustomMessage("$MessageBoardInvalidPar$", nil, plr);
	return;
}

global func SetGodModeOff()
{
	for (var plr in GetPlayers(C4PT_User))
		TakeGodMode(plr);
	return;
}

global func SetGodModeHost()
{
	for (var plr in GetPlayers(C4PT_User))
	{
		if (IsFirstPlayer(plr))
			GiveGodMode(plr);
		else
			TakeGodMode(plr);
	}
	return;
}

global func SetGodModeAll()
{
	for (var plr in GetPlayers(C4PT_User))
		GiveGodMode(plr);
	return;
}

global func GiveGodMode(int plr)
{
	var crew = GetCrew(plr);
	if (crew.has_god_mode_enabled)
		return;
	crew.has_god_mode_enabled = true;
	// Give the player the god mode UI.
	crew->ShowSandboxUI();	
	// Give the player the god mode tools.
	crew.MaxContentsCount = 9;
	crew->CreateContents(GodsHand);
	crew->CreateContents(DevilsHand);
	crew->CreateContents(SprayCan);
	crew->CreateContents(Teleporter);
	return;	
}

global func TakeGodMode(int plr)
{
	var crew = GetCrew(plr);
	if (!crew.has_god_mode_enabled)
		return;
	crew.has_god_mode_enabled = false;
	// Remove the god mode UI.
	crew->HideSandboxUI();
	// Remove god mode items from crew.
	RemoveAll(Find_Container(crew), Find_Or(Find_ID(GodsHand), Find_ID(DevilsHand), Find_ID(SprayCan), Find_ID(Teleporter)));
	crew.MaxContentsCount = 5;
	return;
}

