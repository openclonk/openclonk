func InitializePlayer(int plr)
{
	Log("init plr");
	var clonk = GetCrew(plr);
	if (clonk)
	{
		clonk->CreateContents(GrappleBow);
		clonk->CreateContents(WindBag);
		clonk->CreateContents(Shovel);
		clonk->CreateContents(GrappleBow);
		clonk->CreateContents(Dynamite);
		clonk->CreateContents(Dynamite);
		clonk->SetPosition(300,100);
	}
}
