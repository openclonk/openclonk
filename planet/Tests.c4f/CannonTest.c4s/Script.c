/*-- Cannon Test --*/

protected func Initialize()
{
	CreateObject(Cannon, 225, 630);
	var chest = CreateObject(Chest, 175, 645);
	chest->CreateContents(Rock,32);
	chest->CreateContents(PowderKeg,4);
	return;
}

func InitializePlayer(int plr)
{
	SetFoW(false,plr);
	FindObject(Find_ID(Clonk),Find_Owner(plr))->SetPosition(180,630);
}
