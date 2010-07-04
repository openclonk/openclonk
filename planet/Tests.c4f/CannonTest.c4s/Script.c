/*-- Cannon Test --*/

protected func Initialize()
{
	CreateObject(Cannon, 225, 630);
	CreateObject(Chest, 175, 645)->CreateContents(Rock,30);
	return;
}

func InitializePlayer(int plr)
{
	SetFoW(false,plr);
	FindObject(Find_ID(Clonk),Find_Owner(plr))->SetPosition(180,630);
}
