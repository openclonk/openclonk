/**
	Sandbox
	In this round the player can test all items, but also play a settlement round in
	a large landscape with many elements.

	@author K-Pone, Maikel
*/

/*-- Scenario --*/

public func Initialize()
{
	InitRound();
	InitGodModeMessageBoard();
	return;
}

public func InitRound()
{
	InitGameSettings();
	return;
}

/*-- Player --*/

public func InitializePlayer(int plr)
{
	InitPlayerSettings(plr);
	GiveAllKnowledge(plr);
	GiveSettlementTools(plr);
	GiveBaseMaterials(plr);
	MovePlayerCrew(plr);
	return;
}

public func GiveAllKnowledge(int plr)
{
	var index, def;
	while (def = GetDefinition(index++))
		SetPlrKnowledge(plr, def);
	return;
}

public func GiveSettlementTools(int plr)
{
	var crew = GetCrew(plr);
	// Give all tools needed to build up a settlement.
	crew->CreateContents(Shovel);
	crew->CreateContents(Hammer);
	crew->CreateContents(Axe);
	crew->CreateContents(Pickaxe);
	crew->CreateContents(DynamiteBox);
	return;
}

public func GiveBaseMaterials(int plr)
{
	SetWealth(plr, 250);
	SetBaseMaterial(plr, Clonk, 10);
	SetBaseProduction(plr, Clonk, 2);
	SetBaseMaterial(plr, Bread, 10);
	SetBaseProduction(plr, Bread, 2);
	return;
}

public func MovePlayerCrew(int plr)
{
	// Move the crew of the player to a nice position on the map.
	var pos = FindLocation(Loc_Sky(), Loc_Space(20, CNAT_Top), Loc_Wall(CNAT_Bottom));
	if (pos)
	{
		var crew = GetCrew(plr);
		crew->SetPosition(pos.x, pos.y - 11);
	}
	return;
}