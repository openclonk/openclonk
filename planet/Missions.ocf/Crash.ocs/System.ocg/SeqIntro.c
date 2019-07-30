/* Intro sequence */

#appendto Sequence

func Intro_Init()
{
	this.plane = CreateObjectAbove(Airplane, 0, 400);
	this.plane->SetColor(0xa04000);
	this.pilot = npc_pyrit = CreateObjectAbove(Clonk, 100, 100, NO_OWNER);
	this.pilot->MakeInvincible();
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");

	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000); // currently overridden by skin
	this.pilot->SetAlternativeSkin("MaleBrownHair");
	this.pilot->SetDir(DIR_Left);
	this.pilot->SetObjectLayer(this.pilot);
	this.pilot->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0));
	this.dialogue = this.pilot->SetDialogue("Pilot");
	this.dialogue->SetInteraction(false);

	this.plane->FaceRight();
	this.plane->PlaneMount(this.pilot);
	this.plane.Touchable = 0;
	this.plane->MakeInvincible();
}

func Intro_Start(object hero)
{
	this.hero = hero;
	this.plane->StartInstantFlight(90, 15);

	SetViewTarget(this.pilot);
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set); // zoom out from plane
	
	// Lava goes crazy during the intro
	var lava = FindObject(Find_ID(BoilingLava));
	if (lava) lava->SetIntensity(500);
	
	return ScheduleNext(80);
}

func Intro_JoinPlayer(int plr)
{
	if (this.intro_closed) return false; // too late for join - just join in village
	var crew;
	for (var index = 0; crew = GetCrew(plr, index); ++index)
	{
		crew->Enter(this.dialogue);
	}
	return true;
}

func Intro_CreateBoompack(int x, int y, int fuel)
{
	var boompack = CreateObjectAbove(Boompack, x, y, NO_OWNER);
	boompack->SetFuel(fuel);
	boompack->SetDirectionDeviation(8); // make sure direction of boompack is roughly kept
	boompack->SetControllable(false);
	return boompack;
}

func Intro_1()
{
	SetPlayerZoomByViewRange(NO_OWNER, 800, 600, PLRZOOM_Set);
	MessageBoxAll("$MsgIntro1$", this.pilot, true);
	this.plane->ContainedLeft(this.pilot);
	return ScheduleNext(10);
}

func Intro_2()
{
	this.plane->ContainedRight(this.pilot);
	return ScheduleNext(46);
}

func Intro_3()
{
	this.intro_closed = true;
	this.plane->ContainedStop(this.pilot);
	return ScheduleNext(44);
}

func Intro_4()
{
	this.plane->ContainedLeft(this.pilot);
	return ScheduleNext(20);
}

func Intro_5()
{
	MessageBoxAll("$MsgIntro2$", GetHero(), true);
	return ScheduleNext(36);
}

func Intro_6()
{
	this.plane->ContainedStop(this.pilot);
	return ScheduleNext(64);
}

func Intro_7()
{
	MessageBoxAll("$MsgIntro3$", this.pilot, true);
	return ScheduleNext(20);
}

func Intro_8()
{
	this.plane->ContainedRight();
	return ScheduleNext(6);
}

func Intro_9()
{
	this.plane->ContainedStop(this.pilot);
	return ScheduleNext(30);
}

func Intro_10()
{
	this.plane->ContainedRight(this.pilot);
	return ScheduleNext(6);
}

func Intro_11()
{
	this.plane->ContainedStop(this.pilot);
	return ScheduleNext(28);
}

func Intro_12()
{
	this.plane->Sound("EngineDie");
	return ScheduleNext(10);
}

func Intro_13()
{
	this.plane->CancelFlight();
	return ScheduleNext(4);
}

func Intro_14()
{
	MessageBoxAll("$MsgIntro4$", GetHero(), true);
	return ScheduleNext(20);
}

func Intro_15()
{
	MessageBoxAll("$MsgIntro5$", this.pilot, true);
	return ScheduleNext(36);
}

func Intro_16()
{
	var x = this.plane->GetX();
	var y = this.plane->GetY();
	this.pilot->Exit();
	Intro_CreateBoompack(RandomX(x-5, x + 5), RandomX(y-5, y + 5), 160)->Launch(290 + Random(26), this.pilot);
	var clonk;
	while (clonk = this.dialogue->Contents())
	{
		clonk->Exit();
		Intro_CreateBoompack(RandomX(x-5, x + 5), RandomX(y-5, y + 5), 160)->Launch(290 + Random(26), clonk);
	}
	return ScheduleNext(100);
}

func Intro_17()
{
	this.pilot->SetCommand("MoveTo", nil, 120, 860);
	for (var i = 0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew = GetCrew(plr);
		if (crew)
		{
			crew->SetCommand("MoveTo", nil, 135 + Random(25), 860);
		}
	}
	this.timer = 0;
	return ScheduleNext(100);
}

func Intro_18()
{
	// wait until pilot has arrived in village
	if (!Inside(this.pilot->GetX(), 100, 140))
	{
		++this.timer;
		if (this.timer < 12) return ScheduleSame(18);
		// Pilot didn't arrive on time. Just put him there.
		this.pilot->SetPosition(120, 860);
	}
	this.pilot->SetCommand("None");
	this.pilot->SetComDir(COMD_Stop);
	this.pilot->SetXDir();
	return ScheduleNext(30);
}

func Intro_19()
{
	// Begin dialogue in village
	this.pilot->SetDir(DIR_Right);
	MessageBoxAll("$MsgIntro6$", this.pilot, true); // that was close
	return ScheduleNext(150);
}

func Intro_20()
{
	GetHero()->SetDir(DIR_Left);
	MessageBoxAll("$MsgIntro7$", GetHero(), true); // what now?
	return ScheduleNext(150);
}

func Intro_21()
{
	MessageBoxAll("$MsgIntro8$", this.pilot, true); // plane crashed into mountain on other side
	return ScheduleNext(250);
}

func Intro_22()
{
	MessageBoxAll("$MsgIntro9$", this.pilot, true); // u go there n save it
	return ScheduleNext(220);
}

func Intro_23()
{
	MessageBoxAll("$MsgIntro10$", GetHero(), true); // ok
	return ScheduleNext(40);
}

func Intro_24()
{
	return Stop();
}

func Intro_Stop()
{
	// Lava gets quiet after intro
	var lava = FindObject(Find_ID(BoilingLava));
	if (lava) lava->SetIntensity(25);
	// if players got stuck somewhere, unstick them
	for (var i = 0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var crew = GetCrew(plr);
		if (crew && !Inside(crew->GetX(),125, 170))
		{
			crew->SetPosition(135 + Random(25), 860);
		}
		crew->Extinguish();
		crew->DoEnergy(100);
	}
	this.dialogue->SetInteraction(true);
	this.dialogue->AddAttention();
	SetPlayerZoomByViewRange(NO_OWNER, 400, 300, PLRZOOM_Set);
	
	// Turn and relocate the airplane to make starting it easier.
	var plane = FindObject(Find_ID(Airplane));
	if (plane)
	{
		plane->FaceLeft();
		plane->SetR(-90);
		plane->SetPosition(1387, 345);
	}
	return true;
}
