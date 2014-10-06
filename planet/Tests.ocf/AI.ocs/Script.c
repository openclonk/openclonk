func Initialize()
{
	// Script player as opponent.
	CreateScriptPlayer("S2", 0xffffc000, nil, CSPF_FixedAttributes);
}

func InitializePlayer(int plr)
{
	if (GetPlayerType(plr) == C4PT_Script) return InitializeScriptPlayer(plr);
	GetCrew(plr)->SetPosition(LandscapeWidth()-10, 190);
	GetCrew(plr)->CreateContents(Bow);
	GetCrew(plr)->CreateContents(Arrow);
	GetCrew(plr)->CreateContents(Rock);
	GetCrew(plr)->CreateContents(Rock);
	GetCrew(plr)->CreateContents(Rock);
	GetCrew(plr)->CreateContents(Rock);
	GetCrew(plr)->CreateContents(Rock);
	GetCrew(plr)->CreateContents(Rock);
	return true;
}

func InitializeScriptPlayer(int plr)
{
	// Remove old crew.
	var index = 0;
	while (GetCrew(plr, index))
	{
		GetCrew(plr, index)->RemoveObject();
		index++;
	}
	// Create enemies
	CreateEnemy(Clonk, 40,200, plr, [Rock, Rock, Sword, Shield], 40);
	return;
}

func CreateEnemy(id clonktype, int x,int y, int plr, array contents, int life)
{
	var enemy = CreateObject(clonktype, x, y, plr);
	if (!enemy) return nil;
	enemy->SetDir(DIR_Right);
	enemy->MakeCrewMember(plr);
	enemy->SetMaxEnergy(life);
	if (contents) for (var c in contents) enemy->CreateContents(c);
	S2AI->AddAI(enemy);
	//CreateObject(EnergyBar)->SetTarget(enemy);
	return enemy;
}
