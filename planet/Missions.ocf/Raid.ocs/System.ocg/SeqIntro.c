/* Intro sequence */

#appendto Sequence

func Intro_Start(object hero)
{
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(hero);
	return ScheduleNext(5);
}

func Intro_JoinPlayer(int plr)
{
	// early joiners drop from the sky
	if (this.seq_progress < 4)
	{
		var crew = GetCrew(plr);
		if (!this.hero) this.hero = crew;
		var x = 300, y = 20; // 328
		if (crew != this.hero) x += plr*15 + Random(10);
		crew->SetPosition(x, y);
		crew->SetXDir(10); crew->SetYDir(3);
		var balloon = crew->CreateContents(Balloon);
		balloon->ControlUseStart(crew, x + 10, y + 3);
	}
	return true;
}

func Intro_1()
{
	MessageBox_last_pos = true; // force first message right side of screen
	MessageBoxAll("$Intro1$", GetHero(), true); // finally there
	SetPlayerZoomByViewRange(NO_OWNER, 500, 300, PLRZOOM_Set | PLRZOOM_LimitMax);
	return ScheduleNext(150);
}

func Intro_2()
{
	// Wait until balloon has dropped
	var all_balloons = FindObjects(Find_ID(BalloonDeployed)), balloon;
	if (GetHero()) this.balloon = GetHero()->GetActionTarget();
	if (this.balloon)
	{
		if (this.balloon->GetX() > 330)
			for (balloon in all_balloons) balloon->ControlLeft();
		else if (this.balloon->GetX() < 300)
			for (balloon in all_balloons) balloon->ControlRight();
		else if (GetHero()->GetY() > 310)
			for (balloon in all_balloons) balloon->~ControlUp();
		return ScheduleSame(10);
	}
	return ScheduleNext(20);
}

func Intro_3()
{
	npc_newton.has_sequence = true;
	Dialogue->SetSpeakerDirs(GetHero(), npc_newton);
	MessageBoxAll(Format("$Intro2$", GetHero()->GetName()), npc_newton, true); // u got my letter?
	SetViewTarget(npc_newton);
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	return ScheduleNext(180);
}

func Intro_4()
{
	MessageBoxAll("$Intro3$", GetHero(), true); // yes n i came 2 help
	SetViewTarget(GetHero());
	return ScheduleNext(220);
}

func Intro_5()
{
	MessageBoxAll("$Intro4$", npc_newton, true); // we're in trouble. need castle.
	SetViewTarget(npc_newton);
	return ScheduleNext(400);
}

func Intro_6()
{
	MessageBoxAll("$Intro5$", GetHero(), true); // why?
	SetViewTarget(GetHero());
	return ScheduleNext(120);
}

func Intro_7()
{
	MessageBoxAll("$Intro6$", npc_newton, true); // later. more wood now.
	SetViewTarget(npc_newton);
	return ScheduleNext(240);
}

func Intro_8()
{
	MessageBoxAll("$Intro7$", GetHero(), true); // work work
	SetViewTarget(GetHero());
	return ScheduleNext(60);
}

func Intro_9()
{
	RemoveAll(Find_ID(Balloon)); // they just confuse players
	return Stop();
}

func Intro_Stop()
{
	npc_newton.has_sequence = false; // continue hammering
	g_goal = CreateObject(Goal_Raid);
	SetPlayerZoomByViewRange(NO_OWNER, 400, 300, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}
