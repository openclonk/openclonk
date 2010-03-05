/*
	Boom command
	Authors: Randrian, Newton
	
	Funny little survival scenario to train with the bow.
*/

// in seconds
static const Boomattack_wave_delay = 25;

static const Boomattack_angle_spread = 45;
// the bigger this value, the slower does the attack size grow
static const Boomattack_attack_growth = 90;

func Initialize()
{
	var offs = 45;
	CreateObject(WindGenerator, 1147, 938+offs)->SetR(7);
	CreateObject(WindGenerator, 985, 1125+offs)->SetR(-170);
	CreateObject(WindGenerator, 1055, 1109+offs)->SetR(140);
	CreateObject(WindGenerator, 971, 857+offs)->SetR(-20);
	CreateObject(WindGenerator, 1147, 1054+offs)->SetR(160);
	CreateObject(WindGenerator, 1036, 870+offs)->SetR(-10);
	CreateObject(WindGenerator, 1081, 873+offs)->SetR(18);
	CreateObject(WindGenerator, 858, 930+offs)->SetR(-10);
	
	CreateObject(Goal_SaveTheWindmills,10,10);
	PlaceGrass(100, 800, 1400);
	SetSkyParallax(0,25,25,0,0,0,50);
	AddEffect("BoomAttack", 0, 100, 35);
	Sound("WindLoop.ogg",true,40,nil,+1);
}

global func FxBoomAttackTimer(object target, int effect, int time)
{
	var wave = 1+time/35/Boomattack_wave_delay;

	if(time/35 % Boomattack_wave_delay == 1)
	{
		if (wave < 13)
		{
			Message("                   $MsgWave$                   ",nil,wave);
			var wave_strength = Sqrt(9+GetPlayerCount()*time/Boomattack_attack_growth);
			CreateAttackWave( Random(360) , wave_strength,Boomattack_angle_spread);
		}
		else if (wave == 13)
		{
			Message("                   $MsgBoss$                   ");
			CreateAttackWave( Random(360) , -1, Boomattack_angle_spread);
		}

	}
	
	// won!
	if(wave > 13)
	{
		if (!FindObject(Find_ID(BigBoomattack)))
		{
			GameOver();
		}
	}
}

global func CreateAttackWave(int angle, int rockets, int anglespread)
{
	var radius = Min(LandscapeWidth()/2, LandscapeHeight()/2);
	var rocket_id = Boomattack;
	// boss
	if(rockets == -1)
	{
		rockets = 1;
		rocket_id = BigBoomattack;
	}
	
	for(var i=0; i<rockets; ++i) 
	{
		var rocket_angle = angle + Random(anglespread) - anglespread/2;
		var rocket_radius = radius * RandomX(80,100) / 100;
		var x =  Sin(rocket_angle, rocket_radius)+LandscapeWidth()/2;
		var y = -Cos(rocket_angle, rocket_radius)+LandscapeHeight()/2;

		CreateObject(rocket_id, x, y)->Launch(rocket_angle + 180);
	}
	
	for(var i=0; i<GetPlayerCount(); ++i)
	{
		var owner = GetPlayerByIndex(i);
		var gui_arrow = FindObject(Find_ID(GUI_GoalArrow), Find_Owner(owner));
		if(!gui_arrow)
		{
			gui_arrow = CreateObject(GUI_GoalArrow,0,0,owner);
			gui_arrow->SetAction("Show", GetCursor(owner));
			gui_arrow->SetClrModulation(RGB(255,0,0));
			gui_arrow->SetObjectBlitMode(GFX_BLIT_Mod2);
		}
		gui_arrow->SetR(angle);
	}
}

func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam)
{
	SetFoW(false,iPlr);
	JoinPlayer(iPlr);
	GetHiRank(iPlr)->SetPosition(LandscapeWidth()/2, LandscapeHeight()/2);
	return;
}

func RemovePlayer(int iPlr)
{
	for(var obj in FindObjects(Find_Owner(iPlr)))
	{
		if(obj)
			obj->RemoveObject();
	}
}

func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, LandscapeWidth()/2, 600, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	SelectCrew(plr, clonk, true);
	JoinPlayer(plr);
	var gui_arrow = FindObject(Find_ID(GUI_GoalArrow), Find_Owner(plr));
	gui_arrow->SetAction("Show", GetCursor(plr));
}

func JoinPlayer(int iPlr)
{
	var clonk = GetCrew(iPlr);
	clonk->DoEnergy(1000);

	var shovel = FindObject(Find_ID(Shovel),Find_Container(clonk));
	if(shovel) shovel->RemoveObject();
	clonk->CreateContents(Bow);
	clonk->Collect(CreateObject(Arrow));
	clonk->CreateContents(Musket);
	clonk->Collect(CreateObject(LeadShot));
	return;
}
