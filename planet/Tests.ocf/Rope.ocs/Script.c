

func Initialize()
{
	var  a = CreateObject(Rock, 100, 100, NO_OWNER);
	var  b = CreateObject(Rock, 200, 100, NO_OWNER);
	a->SetCategory(C4D_StaticBack);
	b->SetCategory(C4D_StaticBack);
	a->CreateRope2(a, b, 20);
	//a->CreateRope2(a, b, 1);
}

func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr, 0);
	var  a = CreateObject(Rock, clonk->GetX(), clonk->GetY()-100, NO_OWNER);
	a->SetCategory(C4D_StaticBack);
	a->CreateRope2(a, clonk, 20);
}
