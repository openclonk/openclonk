
global func IntroStart()
{
	if(/*!g_intro_initialized && */!GetEffect("IntIntro"))
	{
		AddEffect("IntIntro", nil, 1, 10, nil, nil);
		//g_intro_initialized = true;
	}
}

global func IntroAddPlayer(int plr)
{
	var effect = GetEffect("IntIntro");
	if(!effect) return false;
	if(effect.Time > 30) return false;

	var crew;
	for(var index = 0; crew = GetCrew(plr, index); ++index)
		crew->Enter(effect.Pilot); //Airplane);

	return true;
}

global func IntroCreateBoompack(int x, int y, int fuel)
{
	var boompack = CreateObject(Boompack, x, y, NO_OWNER);
	boompack->SetFuel(fuel);
	boompack->SetDirectionDeviation(8); // make sure direction of boompack is roughly kept
	return boompack;
}

global func FxIntIntroStart(object target, proplist effect)
{
	effect.Plane = CreateObject(Plane, 0, 400);
	effect.Pilot = CreateObject(Clonk, 100, 100, NO_OWNER);
	effect.Pilot->SetSkin(2);
	effect.Pilot->Enter(effect.Plane);
	effect.Pilot->SetAction("Walk");

	effect.Pilot->SetName("$NamePilot$");
	effect.Pilot->SetColor(RGB(55, 65, 75));
	effect.Pilot->SetDir(DIR_Left);
	effect.Pilot->SetObjectLayer(effect.Pilot);
	effect.Dialog = effect.Pilot->SetDialogue("Pilot");

	effect.Plane->FaceRight();
	effect.Plane->StartInstantFlight(90, 15);
}

global func FxIntIntroTimer(object target, proplist effect, int time)
{
	if(effect.Time == 80)
	{
		effect.Dialog->MessageBoxAll("$MsgIntro1$", effect.Pilot);
		effect.Plane->ContainedLeft();
	}

	if(effect.Time == 90)
		effect.Plane->ContainedRight();
	if(effect.Time == 100)
		effect.Plane->ContainedStop();

	if(effect.Time == 140)
		effect.Dialog->MessageBoxAll("$MsgIntro2$");
	if(effect.Time == 150)
		effect.Plane->ContainedRight();
	if(effect.Time == 160)
		effect.Plane->ContainedLeft();
	if(effect.Time == 170)
		effect.Plane->ContainedRight();
	if(effect.Time == 180)
		effect.Plane->ContainedStop();

	if(effect.Time == 220)
		effect.Dialog->MessageBoxAll("$MsgIntro3$");
	if(effect.Time == 230)
		effect.Plane->ContainedLeft();
	if(effect.Time == 250)
		effect.Plane->ContainedRight();
	if(effect.Time == 260)
		effect.Plane->ContainedStop();

	if(effect.Time == 340)
		effect.Plane->CancelFlight();

	if(effect.Time == 350)
		effect.Dialog->MessageBoxAll("$MsgIntro4$");

	if(effect.Time == 370)
		effect.Dialog->MessageBoxAll("$MsgIntro5$");

	if(effect.Time == 390)
	{
		var x = effect.Plane->GetX();
		var y = effect.Plane->GetY();

		IntroCreateBoompack(RandomX(x-5,x+5), RandomX(y-5,y+5), 150)->Launch(280 + Random(51), effect.Pilot);
		while(effect.Pilot->Contents())
			IntroCreateBoompack(RandomX(x-5,x+5), RandomX(y-5,y+5), 150)->Launch(280 + Random(51), effect.Pilot->Contents());
	}

	if(effect.Time == 430)
		for(var i = 0; i < GetPlayerCount(); ++i)
			GetCursor(GetPlayerByIndex(i))->CloseMenu();

	if(effect.Time >= 750)
	{
		effect.Pilot->SetCommand("MoveTo", effect.Pilot, 120 - effect.Pilot->GetX(), 860 - effect.Pilot->GetY());
		return -1;
	}
}
