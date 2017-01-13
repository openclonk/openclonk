// Intro sequence for Iron Peak: Players are dropped with balloons from air plane. 

#appendto Sequence

public func Intro_Init()
{
	// Create an airplane with pilot and fly it towards the peak.
	this.airplane = CreateObjectAbove(Airplane, 24, LandscapeHeight() - 480);
	this.pilot = CreateObjectAbove(Clonk, 24, LandscapeHeight() - 480);
	this.pilot->SetName("$PilotName$");
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.airplane);
	this.pilot->SetAction("Walk");
	this.pilot->SetDir(DIR_Right);
	this.pilot->SetColor(0xffcc11aa);
	this.airplane->PlaneMount(this.pilot);
	this.airplane->FaceRight();
	this.airplane->StartInstantFlight(60, 15);
	this.airplane->SetXDir(12);
	this.airplane->SetYDir(-10);
	this.airplane->MakeInvincible();
	return;
}

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	var j = 0, crew;
	while (crew = GetCrew(plr, j++))
	{
		crew->Enter(this.airplane);
		crew->SetAction("Walk");	
	}
	
	// Reduce zoom 
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(this.pilot);
	return;
}

public func Intro_1()
{
	// Readjust airplane flight.
	this.airplane->ContainedLeft(this.pilot);
	ScheduleCall(this.airplane, "ContainedStop", 16, 0, this.pilot);
	
	// Prepare players for drop.
	MessageBoxAll("$MsgIronPeakAhead$", this.pilot, true); 
	return ScheduleNext(32);
}

public func Intro_2()
{
	// Readjust airplane flight.
	this.airplane->ContainedLeft(this.pilot);
	ScheduleCall(this.airplane, "ContainedStop", 12, 0, this.pilot);
	return ScheduleNext(12);
}

public func Intro_3()
{
	// View target is the crew itself again.
	SetViewTarget(nil);
	
	// Drop players.
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var j = 0, crew;
		while (crew = GetCrew(plr, j++))
		{
			crew->Exit();
			var balloon = crew->CreateContents(Balloon);
			balloon->ControlUseStart(crew, 10, 3);			
		}
		SetPlrView(plr, GetCrew(plr));
	}
	return ScheduleNext(20);
}

public func Intro_4()
{
	// Tell players to land on peak.
	MessageBoxAll("$MsgLandOnPeak$", this.pilot, true);
	return ScheduleNext(4);
}

public func Intro_5()
{
	// Try to land the crew on the peak.
	var all_balloons = FindObjects(Find_ID(BalloonDeployed));
	if (GetLength(all_balloons) == 0)
		return ScheduleNext(4);
	
	if (HasLandingSpace(all_balloons[0]->GetX(), all_balloons[0]->GetY()))
		for (var balloon in all_balloons)
			balloon->Deflate();
	else
		for (var balloon in all_balloons)
			balloon->ControlRight();

	return ScheduleSame(4);
}

public func HasLandingSpace(int x, int y)
{
	// Check for solid below.
	var dy = 0;
	while (!GBackSemiSolid(x, y + dy) && dy < 50)
		dy++;
	if (dy < 50)
		return true;
	// Check for solid on the right.
	var dx = 0;
	while (!GBackSemiSolid(x + dx, y - 8) && !GBackSemiSolid(x + dx, y) && !GBackSemiSolid(x + dx, y + 8) && dx < 30)
		dx++;
	if (dx < 20)
		return true;
	return false;
}

public func Intro_6()
{
	// Remove balloon from crew's contents.
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var j = 0, crew;
		while (crew = GetCrew(plr, j++))
			RemoveAll(Find_ID(Balloon), Find_Container(crew));		
	}
	
	// Tell players about tools and the mission goal.
	MessageBoxAll("$MsgCrewLanded$", this.pilot, true);
	SetPlayerZoomByViewRange(NO_OWNER, 500, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return ScheduleNext(80);
}

public func Intro_7()
{
	return Stop();
}

public func Intro_Stop()
{ 
	// Remove airplane and pilot.
	this.pilot->RemoveObject();
	this.airplane->RemoveObject();
	
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 500, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}
