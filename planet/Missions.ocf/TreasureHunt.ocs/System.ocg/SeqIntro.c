/* Intro sequence */

#appendto Sequence

func Intro_Init(object flagpole)
{
	// Fix plane outside landscape for now
	this.plane = CreateObjectAbove(Airplane, 100,-20);
	this.plane->FaceRight();
	this.plane->SetR(80);
	this.plane->SetColor(0xa04000);
	this.plane_Hit = this.plane.Hit;
	this.plane.Hit = this.Intro_PlaneHit;
	this.plane.intro_seq = this;
	this.plane_cat = this.plane->GetCategory();
	this.plane->SetCategory(C4D_StaticBack);
	this.plane->MakeInvincible();
	this.plane.Touchable = 0;
	
	// Pyrit the pilot
	this.pilot = npc_pyrit = CreateObjectAbove(Clonk, 100, 100, NO_OWNER);
	this.pilot->MakeInvincible();
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");
	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000);
	this.pilot->SetDir(DIR_Left);
	this.pilot->SetObjectLayer(this.pilot);
	this.pilot->SetAlternativeSkin("MaleBrownHair");
	
	this.plane->PlaneMount(this.pilot);
	// Pyit has a red hat!
	this.pilot->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0));

	// Dialogue object also used as helper container for clonks
	this.dialogue = this.pilot->SetDialogue("Pyrit");
	this.dialogue->SetInteraction(false);

}

func Intro_Start(object flagpole)
{
	// Intro starts at flagpole; some time before plane drops
	this.flagpole = flagpole;
	SetViewTarget(this.flagpole);
	
	SetPlayerZoomByViewRange(NO_OWNER, 800, 600, PLRZOOM_Set); // zoom out from plane
	
	return ScheduleNext(80);
}

func Intro_JoinPlayer(int plr)
{
	// Players joining initially start out in plane
	// Late joiners are placed at flagpole
	for (var index = 0, crew; crew = GetCrew(plr, index); ++index)
	{
		if (this.plane_crashed)
			crew->SetPosition(this.flagpole->GetX(), this.flagpole->GetY());
		else
			crew->Enter(this.dialogue);
	}
	return true;
}

func Intro_1()
{
	// Start plane drop
	this.plane->SetCategory(this.plane_cat);
	this.plane->SetPosition(50, 840);
	this.plane->SetXDir(100);
	this.plane->SetYDir(200);
	this.plane->SetR(170);
	return ScheduleNext(10);
}

func Intro_2()
{
	// Plane drop sound when it enters view range
	if (this.plane->GetY() > 900)
		Sound("Goal_TreasureHunt::PlaneDrop", true);
	else
		ScheduleSame(2);
	return true;
}

func Intro_PlaneHit()
{
	// Plane hit ground! Continue sequence.
	Sound("Objects::Plane::PlaneCrash", true);
	SetR(-90);
	var particles = Particles_Smoke(true);
	particles.Size = PV_Linear(PV_Random(20, 60), PV_Random(50, 100));
	CreateParticle("Smoke", PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(-60, 60), PV_Random(-20, 0), PV_Random(200, 500), particles, 20);
	particles.Size = PV_Linear(PV_Random(50, 80), PV_Random(100, 200));
	CreateParticle("Smoke", PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(-20, 20), PV_Random(-20, 0), PV_Random(100, 200), particles, 20);
	for (var iplr = 0, plr; iplr<GetPlayerCount(C4PT_User); ++iplr)
	{
		plr = GetPlayerByIndex(iplr, C4PT_User);
		var icrew = 0, crew;
		while (crew = GetCrew(plr, icrew++))
		{
			crew->Exit(0,-5, 0, Random(1)+1, Random(5)-6);
			crew->SetAction("Tumble");
		}
	}
	SetMeshMaterial("CrashedAirplane");
	this.MeshTransformation = Trans_Mul(Trans_Rotate(10, 0, 2, 1), Airplane.MeshTransformation);
	this.Hit = this.intro_seq.plane_Hit;
	this.intro_seq.plane_crashed = true;
	this.intro_seq->ScheduleNext(50);
	return true;
}

func Intro_3()
{
	MessageBox_last_pos = true; // force first message right side of screen
	MessageBoxAll("$Intro1$", GetHero(), true); // pyrit?
	return ScheduleNext(50);
}

func Intro_4()
{
	npc_pyrit->Exit();
	return ScheduleNext(30);
}

func Intro_5()
{
	Dialogue->SetSpeakerDirs(npc_pyrit, GetHero());
	MessageBoxAll("$Intro2$", GetHero(), true); // y out of fuel?
	return ScheduleNext(320);
}

func Intro_6()
{
	MessageBoxAll("$Intro3$", npc_pyrit, true); // cuz detour
	return ScheduleNext(400);
}

func Intro_7()
{
	MessageBoxAll("$Intro4$", GetHero(), true); // ur fault
	return ScheduleNext(200);
}

func Intro_8()
{
	MessageBoxAll("$Intro5$", GetHero(), true); // what now?
	return ScheduleNext(200);
	return Stop();
}

func Intro_9()
{
	MessageBoxAll("$Intro6$", npc_pyrit, true); // u oil, me plane
	return ScheduleNext(330);
}

func Intro_10()
{
	MessageBoxAll("$Intro7$", GetHero(), true); // ok
	return ScheduleNext(80);
}

func Intro_11()
{
	return Stop();
}

func Intro_Stop()
{
	this.dialogue->SetInteraction(true);
	this.dialogue->AddAttention();
	this.dialogue->Dlg_Pyrit_StartHammering(npc_pyrit);
	SetPlayerZoomByViewRange(NO_OWNER, 400, 300, PLRZOOM_Set);
	g_goal = CreateObject(Goal_TreasureHunt, 0, 0);
	return true;
}
