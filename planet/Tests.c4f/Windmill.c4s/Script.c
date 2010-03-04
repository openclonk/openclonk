/*
	Boom command
	Authors: Randrian, Newton
	
	Funny little survival scenario to train with the bow.
*/

func Initialize()
{
	var offs = 45;
	CreateObject(WindGenerator, 1147, 938+offs)->SetR(7);
	CreateObject(WindGenerator, 985, 1130+offs)->SetR(-170);
	CreateObject(WindGenerator, 1055, 1109+offs)->SetR(140);
	CreateObject(WindGenerator, 971, 857+offs)->SetR(-20);
	CreateObject(WindGenerator, 1147, 1059+offs)->SetR(160);
	CreateObject(WindGenerator, 1036, 870+offs)->SetR(-10);
	CreateObject(WindGenerator, 1081, 873+offs)->SetR(18);
	CreateObject(WindGenerator, 858, 930+offs)->SetR(-10);
	
	CreateObject(Goal_SaveTheWindmills,10,10);
	PlaceGrass(100, 800, 1400);
}

global func FxBoomAttackTimer()
{
	var angle = Random(360);
	var radius = Min(LandscapeWidth()/2, LandscapeHeight()/2);
	var x =  Sin(angle, radius)+LandscapeWidth()/2;
	var y = -Cos(angle, radius)+LandscapeHeight()/2;
	CreateObject(Boomattack, x, y);
}

func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	SetFoW(false,iPlr);
	// yeah, for every player there are extra booms
	AddEffect("BoomAttack", 0, 100, 35*5);
	JoinPlayer(iPlr);
	return;
}

func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(1000);
	clonk->SetPosition(LandscapeWidth()/2, LandscapeHeight()/2);
	clonk->CreateContents(Bow);
	clonk->Collect(CreateObject(Arrow));
	return;
}