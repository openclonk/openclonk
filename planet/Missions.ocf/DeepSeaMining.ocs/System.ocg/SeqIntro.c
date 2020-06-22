#appendto Sequence

static g_intro_sky_moving;

func Intro_Start()
{
	// Intro starts high up in the clouds
	Music("TheSkylands");
	LoadScenarioSection("Intro");
	SetWind(-100);
	this.intro_skyscroll_xdir = -10;
	SetSkyParallax(0, 20, 20, this.intro_skyscroll_xdir, 0);
	
	this.plane = CreateObjectAbove(Airplane, 500, 200);
	this.plane->SetColor(0xa04000);
	this.pilot = CreateObjectAbove(Clonk, 100, 100, NO_OWNER);
	this.pilot->MakeInvincible();
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");

	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000);
	this.pilot->SetAlternativeSkin("MaleBrownHair");
	this.pilot->SetDir(DIR_Left);
	this.pilot->SetObjectLayer(this.pilot);
	this.pilot->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0)); // Hat is seen in the cockpit!

	this.plane->PlaneMount(this.pilot);
	this.plane->CreateEffect(FxFloatPlane, 1, 1);
	this.plane->FaceRight();
	this.plane->StartInstantFlight(90, 15);
	g_intro_sky_moving = true;
	

	SetViewTarget(this.plane);

	return ScheduleNext(100, 1);
}

local FxFloatPlane = new Effect
{
	Timer = func(int time)
	{
		if (g_intro_sky_moving)
		{
			var rdir = BoundBy((80 + Random(21) - Target->GetR()) / 5, -1, 1);
			Target->SetR(Target->GetR() + rdir);
			Target->SetXDir(0);
			Target->SetYDir(Target->GetR() * 2 - Target->GetY() + Random(5), 10);
		}
	}
};

func Intro_JoinPlayer(int plr)
{
	if (g_intro_done) return false; // too late for join - just join on island
	for (var index = 0, crew; crew = GetCrew(plr, index); ++index) crew->Enter(this);
	return true;
}

func Intro_1()
{
	MessageBoxAll("$Intro1$", GetHero(), true); // y clouds?
	return ScheduleNext(200);
}

func Intro_2()
{
	MessageBoxAll("$Intro2$", this.pilot, true); // cuz u told me
	return ScheduleNext(300);
}

func Intro_3()
{
	MessageBoxAll("$Intro3$", GetHero(), true); // 2 turbulent
	return ScheduleNext(200);
}

func Intro_4()
{
	MessageBoxAll("$Intro4$", this.pilot, true); // cuz condensation
	return ScheduleNext(380);
}

func Intro_5()
{
	MessageBoxAll("$Intro5$", GetHero(), true); // go lower now
	return ScheduleNext(300);
}

func Intro_6()
{
	MessageBoxAll("$Intro6$", this.pilot, true); // fuk u
	return ScheduleNext(450);
}

func Intro_7()
{
	MessageBoxAll("$Intro7$", this.pilot, true); // u fly
	return ScheduleNext(200);
}

func Intro_8()
{
	MessageBoxAll("$Intro8$", GetHero(), true); // ...
	return ScheduleNext(100);
}

func Intro_9()
{
	MessageBoxAll("$Intro9$", GetHero(), true); // ok
	return ScheduleNext(150);
}

func Intro_10()
{
	g_intro_sky_moving = false;
	Schedule(this, "SetSkyParallax(0, 20, 20, ++this.intro_skyscroll_xdir, 0)", 10, -this.intro_skyscroll_xdir);
	GetHero()->~PlaySoundScream();
	this.plane.rdir = 0;
	this.plane->StartInstantFlight(this.plane->GetR(), 15);
	MessageBoxAll("$Intro10$", GetHero(), true); // aaaah
	for (var i = 0, plr; i<GetPlayerCount(C4PT_User); ++i)
	{
		plr = GetPlayerByIndex(i, C4PT_User);
		for (var index = 0, crew; crew = GetCrew(plr, index); ++index)
		{
			crew->Exit();
			crew->SetPosition(this.plane->GetX()+10, this.plane->GetY());
			crew->SetAction("Tumble");
			if (!index) SetPlrView(plr, crew);
		}
	}
	SetViewTarget();
	return ScheduleNext(200, 11);
}

func Intro_11()
{
	g_intro_done = true;
	LoadScenarioSection("main");
	SetWind(0);
	SetSkyParallax(0, 20, 20, 0, 0);
	GameCall("PostIntroInitialize");
	for (var i = 0, plr; i<GetPlayerCount(C4PT_User); ++i)
	{
		plr = GetPlayerByIndex(i, C4PT_User);
		GameCall("InitializePlayer", plr);
		for (var index = 0, crew; crew = GetCrew(plr, index); ++index)
		{
			crew->SetPosition(g_tuesday_pos[0],-100);
			crew->SetXDir(-10); crew->SetYDir(-30);
			crew->SetAction("Tumble");
			if (!index) SetPlrView(plr, crew);
		}
	}
	return ScheduleNext(200, 20);
}

func Intro_20()
{
	MessageBoxAll("$Intro20$", GetHero(), true); // ouch
	for (var i = 0, plr; i<GetPlayerCount(C4PT_User); ++i)
	{
		plr = GetPlayerByIndex(i, C4PT_User);
		for (var index = 0, crew; crew = GetCrew(plr, index); ++index)
		{
			crew->SetCommand("MoveTo", nil, g_tuesday_pos[0]-15 + Random(20), g_tuesday_pos[1]);
		}
	}
	return ScheduleNext(300);
}

func Intro_21()
{
	Dialogue->SetSpeakerDirs(npc_tuesday, GetHero());
	MessageBoxAll("$Intro21$", npc_tuesday, true); // hi friend
	return ScheduleNext(300);
}

func Intro_22()
{
	MessageBoxAll("$Intro22$", GetHero(), true); // where is plane?
	return ScheduleNext(300);
}

func Intro_23()
{
	MessageBoxAll("$Intro23$", npc_tuesday, true); // 2 cloudy 4 plane
	return ScheduleNext(350);
}

func Intro_24()
{
	MessageBoxAll("$Intro24$", GetHero(), true); // but i need plane
	return ScheduleNext(250);
}

func Intro_25()
{
	MessageBoxAll("$Intro25$", npc_tuesday, true); // u stay here
	return ScheduleNext(300);
}

func Intro_26()
{
	MessageBoxAll("$Intro26$", GetHero(), true); // make fire?
	return ScheduleNext(300);
}

func Intro_27()
{
	MessageBoxAll("$Intro27$", npc_tuesday, true); // fire sux
	return ScheduleNext(400);
}

func Intro_28()
{
	MessageBoxAll("$Intro28$", npc_tuesday, true); // use communicator
	return ScheduleNext(300);
}

func Intro_29()
{
	MessageBoxAll("$Intro29$", GetHero(), true); // ok. where?
	return ScheduleNext(250);
}

func Intro_30()
{
	MessageBoxAll("$Intro30$", npc_tuesday, true); // not built yet
	return ScheduleNext(450);
}

func Intro_31()
{
	MessageBoxAll("$Intro31$", npc_tuesday, true); // go east and finish it with metal + gems
	return ScheduleNext(400);
}

func Intro_32()
{
	MessageBoxAll("$Intro32$", GetHero(), true); // where gems?
	return ScheduleNext(250);
}

func Intro_33()
{
	MessageBoxAll("$Intro33$", npc_tuesday, true); // fish say gems in sea
	return ScheduleNext(400);
}

func Intro_34()
{
	MessageBoxAll("$Intro34$", GetHero(), true); // fish talk?
	return ScheduleNext(150);
}

func Intro_35()
{
	MessageBoxAll("$Intro35$", npc_tuesday, true); // coconut talk!
	return ScheduleNext(200);
}

func Intro_36()
{
	MessageBoxAll("$Intro36$", GetHero(), true); // ok...
	return ScheduleNext(150);
}

func Intro_37()
{
	npc_tuesday->SetDialogue("Tuesday", true);
	return Stop();
}
