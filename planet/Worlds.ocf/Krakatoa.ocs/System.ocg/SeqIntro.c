// Intro sequence for Krakatoa: Players crash with an airplane on the slope of the volcano.

#appendto Sequence

public func Intro_Init(int difficulty)
{
	// Set wind to the left, so that less lava is on the starting place.
	SetWind(-50 - Random(50));
	
	// Determine crater lava height.
	var lava_y = 0;
	while (!GBackLiquid(LandscapeWidth() / 2, lava_y) && lava_y < LandscapeHeight())
		lava_y++;
	
	// Create an airplane with pilot and fly it towards the peak.
	this.airplane = CreateObjectAbove(Airplane, LandscapeWidth() / 2 - 564, lava_y - 176);
	this.pilot = CreateObjectAbove(Clonk, LandscapeWidth() / 2 - 564, lava_y - 176);
	this.pilot->SetName("$PilotName$");
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.airplane);
	this.pilot->SetAction("Walk");
	this.pilot->SetDir(DIR_Right);
	this.pilot->SetColor(0xff0000aa);
	this.airplane->PlaneMount(this.pilot);
	this.airplane->FaceRight();
	this.airplane->StartInstantFlight(90, 15);
	this.airplane->SetXDir(12);
	this.airplane->SetYDir(-1);
	this.airplane->MakeInvincible();
	this.airplane.intro_seq = this;
	// Fill the airplane with some materials.
	this.difficulty = difficulty;
	if (difficulty <= 2)
	{
		this.airplane->CreateContents(Loam, 5);
		this.airplane->CreateContents(Bread, 5);
		this.airplane->CreateContents(Wood, 8);
		this.airplane->CreateContents(Rock, 4);
		this.airplane->CreateContents(Metal, 4);
		if (difficulty <= 1)
		{
			this.airplane->CreateContents(Pickaxe, 2);
			for (var i = 0; i < 5; i++)
				this.airplane->CreateContents(Barrel)->PutLiquid("Water", 300);
		}	
	}
	return;
}

public func Intro_Start()
{
	return ScheduleNext(4);
}

public func Intro_JoinPlayer(int plr)
{
	// Move player's crew into the plane.
	var j = 0, crew;
	while (crew = GetCrew(plr, j++))
	{
		crew->Enter(this.airplane);
		crew->SetAction("Walk");	
	}
	// Increase zoom.
	SetPlayerZoomByViewRange(plr, 700, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(this.pilot);
	return;
}

public func Intro_1()
{
	// Prepare players for drop.
	MessageBoxAll("$MsgNearVolcano$", this.pilot, true); 
	return ScheduleNext(108);
}

public func Intro_2()
{
	// Determine crater lava height.	
	var lava_y = 0;
	while (!GBackLiquid(LandscapeWidth() / 2, lava_y) && lava_y < LandscapeHeight())
		lava_y++;
	// Launch a big eruption from this location.
	AddEffect("BigEruption", nil, 100, 1, nil, nil, LandscapeWidth() / 2, lava_y - 2);
	return ScheduleNext(12);
}

public func Intro_3()
{
	// Message about volcano eruption.
	MessageBoxAll("$MsgEruption$", this.pilot, true);
	return ScheduleNext(23);
}

public func Intro_4()
{
	// Determine crater lava height.	
	var lava_y = 0;
	while (!GBackLiquid(LandscapeWidth() / 2, lava_y) && lava_y < LandscapeHeight())
		lava_y++;
	// Launch the killing chunk.
	this.chunk = CreateObjectAbove(LavaChunk, LandscapeWidth() / 2, lava_y);
	this.chunk->SetSpeed(36, -100);
	return ScheduleNext(28);
}

public func Intro_5()
{
	// Explide lava chunk at plane location.
	this.chunk->Explode(36);
	// Destroy and fling the plane.
	this.airplane->SetMeshMaterial("CrashedAirplane");
	this.airplane->MakeBroken();
	this.airplane->CancelFlight();
	this.airplane->RemovePlaneControl();
	this.airplane->SetRDir(10);
	this.airplane->SetSpeed(46, -56);
	// Forward plane hit call to sequence.
	this.plane_hitcall = this.airplane.Hit;
	this.airplane.Hit = this.Intro_PlaneHit;
	// Eject message.
	MessageBoxAll("$MsgEject$", this.pilot, true);
	SetViewTarget(this.airplane);
	return ScheduleNext(24);
}

public func Intro_6()
{
	// Let pilot get away in boompack.
	this.pilot->Exit();
	var boompack = this.pilot->CreateObjectAbove(Boompack);
	boompack->SetFuel(1000);
	boompack->SetDirectionDeviation(8);
	boompack->SetControllable(false);
	boompack->Launch(40, this.pilot);
	ScheduleCall(boompack, "RemoveObject", 100);
	ScheduleCall(this.pilot, "RemoveObject", 100);
	// Rest of intro will be triggered on plane hit.
	return true;
}

public func Intro_PlaneHit()
{
	// Airplane hit ground! Continue sequence.
	Sound("Objects::Plane::PlaneCrash", true);
	var particles = Particles_Smoke(true);
	particles.Size = PV_Linear(PV_Random(20, 60), PV_Random(50, 100));
	CreateParticle("Smoke", PV_Random(-30,30), PV_Random(-30,30), PV_Random(-60, 60), PV_Random(-20,0), PV_Random(200, 500), particles, 20);
	particles.Size = PV_Linear(PV_Random(50, 80), PV_Random(100, 200));
	CreateParticle("Smoke", PV_Random(-30,30), PV_Random(-30,30), PV_Random(-20, 20), PV_Random(-20,0), PV_Random(100, 200), particles, 20);
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var icrew = 0, crew;
		while (crew = GetCrew(plr, icrew++))
		{
			crew->Exit(0,-5, 0, 1 + Random(2), Random(3) - 5);
			crew->SetAction("Tumble");
			crew->ClearInvincible();
			crew->DoEnergy(-this.intro_seq.difficulty * RandomX(5, 8));
			crew->MakeInvincible();
		}
	}
	// Stop plane movement and rotate for crash effect.
	SetXDir(0);
	this.Hit = this.intro_seq.plane_hit;
	this.MeshTransformation = Trans_Mul(Trans_Rotate(10,0,2,1), Airplane.MeshTransformation);
	this.intro_seq->ScheduleNext(50);
	return true;
}

public func Intro_7()
{
	// Message from first clonk to other crew members.
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox("$MsgConstructPlane$", GetCrew(plr, 0), GetCrew(plr, 0), plr, true);
	}
	return ScheduleNext(12);
}

public func Intro_8()
{
	return Stop();
}

public func Intro_Stop()
{ 
	// Reset player zoom.
	SetPlayerZoomByViewRange(NO_OWNER, 500, nil, PLRZOOM_Set | PLRZOOM_LimitMax);
	return true;
}
