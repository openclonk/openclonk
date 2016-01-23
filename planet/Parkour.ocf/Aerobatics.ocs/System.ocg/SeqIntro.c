// Intro sequence for Aerobatics: Host tells you about the game.

#appendto Sequence

public func Intro_Start()
{
	this.airship = CreateObjectAbove(Airship, LandscapeWidth() / 4 + 40, LandscapeHeight() / 2 - 40);
	this.host = CreateObjectAbove(Clonk, LandscapeWidth() / 4 + 40, LandscapeHeight() / 2 - 50);
	this.host->SetAlternativeSkin("Mime");
	this.host->SetName("Mr. Aerobat");
	this.host->SetDir(DIR_Left);
	this.airship->SetObjectLayer(this.host);
	this.host->SetObjectLayer(this.host);
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Set);
	var crew = GetCrew(plr);
	crew->SetPosition(crew->GetX() + RandomX(-10, 10), crew->GetY());
	return;
}

public func Intro_1()
{
	// Display intro message for all players.
	MessageBoxAll("$MsgLetAerobaticsBegin$", this.host, true);
	return ScheduleNext(12);
}

public func Intro_2()
{
	AddEffect("IntroControlAirship", nil, 100, 5, nil, this->GetID(), this.airship, this.host);
	return ScheduleNext(12);
}

public func Intro_3()
{
	// Add a countdown for this attempt.
	GUI_Clock->CreateCountdown(10);
	return ScheduleNext(10 * 36);
}

public func Intro_4()
{
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 600, nil, PLRZOOM_Set);
	return true;
}

public func FxIntroControlAirshipStart(object target, proplist effect, int temp, object airship, object host)
{
	if (temp)
		return FX_OK;
	effect.airship = airship;
	effect.host = host;
	effect.host->SetCommand("Grab", effect.airship);
	effect.airship->ControlRight(effect.host);
	effect.state = 1;
	return FX_OK;
}

public func FxIntroControlAirshipTimer(object target, proplist effect, int time)
{
	if (effect.state == 1 && effect.host->GetX() > 4 * LandscapeWidth() / 5 - 30)
	{
		effect.airship->ControlStop(effect.host);
		effect.airship->ControlDown(effect.host);
		effect.state = 2;
	}
	if (effect.state == 2 && effect.airship->GetContact(-1, CNAT_Bottom))
	{
		effect.airship->ControlStop(effect.host);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func FxIntroControlAirshipStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	effect.host->SetCommand("UnGrab");
	return FX_OK;
}