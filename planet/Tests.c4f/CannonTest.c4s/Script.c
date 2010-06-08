/*-- Cannon Test --*/

protected func Initialize()
{
	var cannon = CreateObject(Cannon, 225, 630);
	cannon->CreateContents(Rock,99);
	cannon->CreateContents(Blackpowder,99);
	return;
}

func InitializePlayer(int plr)
{
	SetFoW(false,plr);
	FindObject(Find_ID(Clonk),Find_Owner(plr))->SetPosition(180,630);
}
