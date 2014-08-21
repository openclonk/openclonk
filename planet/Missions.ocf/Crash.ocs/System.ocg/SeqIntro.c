/* Intro sequence */

#appendto Sequence

func Intro_Init()
{
	this.plane = CreateObject(Plane, 0, 400);
	this.pilot = CreateObject(Clonk, 100, 100, NO_OWNER);
	this.pilot->MakeInvincible();
	this.pilot->MakeNonFlammable();
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");

	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000);
	this.pilot->SetDir(DIR_Left);
	this.pilot->SetObjectLayer(this.pilot);
	this.dialogue = this.pilot->SetDialogue("Pilot");
	this.dialogue->SetInteraction(false);

	this.plane->FaceRight();
}

func Intro_Start(object hero)
{
	if (!this.plane) Intro_Init();
	this.hero = hero;
	this.plane->StartInstantFlight(90, 15);

	SetViewTarget(this.pilot);
	SetPlayerZoomByViewRange(NO_OWNER, 200,100, PLRZOOM_Set); // zoom out from plane
	
	return ScheduleNext(80);
}

func Intro_JoinPlayer(int plr)
{
	if (!this.plane) Intro_Init();
	if (this.intro_closed) return false; // too late for join - just join in village
	var crew;
	for(var index = 0; crew = GetCrew(plr, index); ++index)
	{
		crew->Enter(this.dialogue);
	}
	return true;
}

func Intro_CreateBoompack(int x, int y, int fuel)
{
	var boompack = CreateObject(Boompack, x, y, NO_OWNER);
	boompack->SetFuel(fuel);
	boompack->SetDirectionDeviation(8); // make sure direction of boompack is roughly kept
	boompack->SetControllable(false);
	return boompack;
}

func Intro_1()
{
	SetPlayerZoomByViewRange(NO_OWNER, 800,600, PLRZOOM_Set);
	MessageBoxAll("$MsgIntro1$", this.pilot, true);
	this.plane->ContainedLeft();
	return ScheduleNext(10);
}

func Intro_2()
{
	this.plane->ContainedRight();
	return ScheduleNext(46);
}

func Intro_3()
{
	this.intro_closed = true;
	this.plane->ContainedStop();
	return ScheduleNext(44);
}

func Intro_4()
{
	this.plane->ContainedLeft();
	return ScheduleNext(20);
}

func Intro_5()
{
	MessageBoxAll("$MsgIntro2$", this.hero, true);
	return ScheduleNext(36);
}

func Intro_6()
{
	this.plane->ContainedStop();
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
	this.plane->ContainedStop();
	return ScheduleNext(30);
}

func Intro_10()
{
	this.plane->ContainedRight();
	return ScheduleNext(6);
}

func Intro_11()
{
	this.plane->ContainedStop();
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
	MessageBoxAll("$MsgIntro4$", this.hero, true);
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
	Intro_CreateBoompack(RandomX(x-5,x+5), RandomX(y-5,y+5), 160)->Launch(290 + Random(26), this.pilot);
	while(this.dialogue->Contents())
		Intro_CreateBoompack(RandomX(x-5,x+5), RandomX(y-5,y+5), 160)->Launch(290 + Random(26), this.dialogue->Contents());
		
	SetViewTarget(nil);
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		SetPlrView(plr, GetCrew(plr));
	}

	return ScheduleNext(40);
}


func Intro_17()
{
	ScheduleCall(nil, this.Intro_PilotDlg, 330, 1, this.pilot);
	return Stop();
}

func Intro_Stop()
{
	SetPlayerZoomByViewRange(NO_OWNER, 400,300, PLRZOOM_Set);
	return true;
}

func Intro_PilotDlg(object pilot)
{
	pilot->SetCommand("MoveTo", nil, 120, 860);
	var dialogue = Dialogue->FindByTarget(pilot);
	dialogue->SetInteraction(true);
	dialogue->AddAttention();
	return true;
}
