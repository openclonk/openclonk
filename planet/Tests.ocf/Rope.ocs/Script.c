

func Initialize()
{
	var  a = CreateObject(Rock, 100, 100, NO_OWNER);
	var  b = CreateObject(Rock, 200, 100, NO_OWNER);
	//a->SetCategory(C4D_StaticBack);
	//b->SetCategory(C4D_StaticBack);
	Scenario.rope = CreateRope(a, b, 20, LiftTower_Rope);
	//CreateRope(a, b, 1, LiftTower_Rope);
}

func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr, 0);
	var  a = CreateObject(Rock, clonk->GetX(), clonk->GetY()-100, NO_OWNER);
	a->SetCategory(C4D_StaticBack);
	var rope = CreateRope(a, clonk, 20, LiftTower_Rope);

	rope->SetFrontAutoSegmentation(200);
}
