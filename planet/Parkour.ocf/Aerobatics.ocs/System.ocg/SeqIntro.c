// Intro sequence for Aerobatics: Host tells you about the game.

#appendto Sequence

public func Intro_Start(array start, array finish)
{
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");
	var start_x = start[0] * map_zoom + 40;
	var start_y = start[1] * map_zoom - 20;
	this.finish_x = finish[0] * map_zoom;
	this.airship = CreateObjectAbove(Airship, start_x, start_y);
	this.host = CreateObjectAbove(Clonk, start_x, start_y - 12);
	this.host->SetAlternativeSkin("Mime");
	this.host->SetName("Mr. Aerobat");
	this.host->SetDir(DIR_Left);
	this.airship->SetObjectLayer(this.host);
	this.host->SetObjectLayer(this.host);
	// Set contact density to 85 for ship and pilot to avoid collisions with the landscape, except brick and solidmasks.
	this.airship->SetContactDensity(85);
	this.host->SetContactDensity(85);
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
	return ScheduleNext(24);
}

public func Intro_2()
{
	// Add a countdown for this attempt.
	GUI_Clock->CreateCountdown(10, nil, nil, true);
	return ScheduleNext(5 * 36);
}

public func Intro_3()
{
	// Start moving airship to the right later, such that a well times dynamite jump gets you on the airship.
	AddEffect("IntroControlAirship", nil, 100, 5, nil, this->GetID(), this.airship, this.host, this.finish_x);
	return ScheduleNext(5 * 36);
}

public func Intro_4()
{
	return Stop();
}

public func Intro_Stop()
{
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 720, nil, PLRZOOM_Set);
	return true;
}

public func FxIntroControlAirshipStart(object target, proplist effect, int temp, object airship, object host, int finish_x)
{
	if (temp)
		return FX_OK;
	effect.airship = airship;
	effect.host = host;
	effect.host->SetCommand("Grab", effect.airship);
	effect.airship->ControlRight(effect.host);
	effect.finish_x = finish_x;
	effect.state = 1;
	return FX_OK;
}

public func FxIntroControlAirshipTimer(object target, proplist effect, int time)
{
	if (!effect.host || !effect.airship)
		return FX_Execute_Kill;	
	if (effect.state == 1 && effect.host->GetX() > effect.finish_x - 30)
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
	if (effect.airship)
		effect.airship->SetContactDensity(C4M_Solid);
	if (effect.host)
	{
		effect.host->SetCommand("UnGrab");
		effect.host->SetContactDensity(C4M_Solid);
	}
	return FX_OK;
}