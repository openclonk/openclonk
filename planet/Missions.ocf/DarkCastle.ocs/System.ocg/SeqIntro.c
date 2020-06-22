/* Intro sequence */

#appendto Sequence

func Intro_Init()
{
	this.plane = CreateObjectAbove(Airplane, 0, 800);
	this.plane->SetColor(0xa04000);
	this.plane.HitPoints = 9999999;
	this.plane.intro_seq = this;
	
	this.pilot = npc_pyrit = CreateObjectAbove(Clonk, 100, 100, NO_OWNER);
	this.pilot->MakeInvincible();
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");

	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000);
	this.pilot->SetAlternativeSkin("MaleBrownHair");
	this.pilot->SetDir(DIR_Left);
	this.pilot->SetObjectLayer(this.pilot);
	this.pilot->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0));

	this.dialogue = this.pilot->SetDialogue("Pyrit");
	this.dialogue->SetInteraction(false);

	this.plane->PlaneMount(this.pilot);
	this.plane->FaceRight();
	this.plane.PlaneDeath = this.Intro_PlaneDeath;
}

func Intro_Start(object hero)
{
	this.hero = hero;
	this.plane->StartInstantFlight(90, 40);

	SetViewTarget(this.pilot);
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set); // zoom out from plane
	
	return ScheduleNext(50);
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

func Intro_1()
{
	SetPlayerZoomByViewRange(NO_OWNER, 800, 600, PLRZOOM_Set); // zoom out from plane
	MessageBoxAll("$MsgIntro1$", this.pilot, true); // we've reached the castle
	return ScheduleNext(10);
}

func Intro_2()
{
	if (this.plane->GetX() < 400) return ScheduleSame(3);
	this.plane_r = 90;
	return ScheduleNext(1);
}

func Intro_3()
{
	this.plane->StartInstantFlight(--this.plane_r, 40);
	if (this.plane_r>45) return ScheduleSame(2);
	return ScheduleNext(10);
}

func Intro_4()
{
	MessageBoxAll("$MsgIntro2$", g_cannoneer, true); // alert! intruders!
	g_cannoneer->SetCommand("Grab", g_cannon);
	return ScheduleNext(5);
}

func Intro_5()
{
	// Wait until placed diagonally above cannon
	var dx = this.plane->GetX() - g_cannon->GetX();
	var dy = this.plane->GetY() - g_cannon->GetY();
	var r = Angle(0, 0, dx + 30, dy, g_cannon.angPrec); // aim a bit ahead
	if (dx < dy)
	{
		r = g_cannon->ConvertAngle(r);
		g_cannon->SetCannonAngle(r);
		return ScheduleSame(3);
	}
	// Boooom!
	this.projectile = g_cannon->CreateContents(IronBomb);
	g_cannon->DoFire(this.projectile, g_cannoneer, r);
	return ScheduleNext(5);
}

// Don't explode. Plane death handled in sequence.
func Intro_PlaneDeath() { return true; }

func Intro_6()
{
	// Wait for hit
	if (this.projectile->GetX() > this.plane->GetX()) return ScheduleSame(1);
	// Booom!
	this.intro_closed = true;
	this.projectile->DoExplode();
	this.plane->SetMeshMaterial("CrashedAirplane");
	this.plane->CancelFlight();
	this.plane->RemovePlaneControl();
	this.plane->SetRDir(-10);
	// Calc fling direction to land near flagpole
	var tx = 135 - this.plane->GetX();
	var ty = 1200 - this.plane->GetY();
	var xdir = -50;
	var t = tx * 10 / xdir;
	var g = GetGravity();
	var ydir = 10*ty/t - g*t/20;
	this.plane->SetXDir(xdir);
	this.plane->SetYDir(ydir);
	return ScheduleNext(30);
}

func Intro_7()
{
	MessageBoxAll("$MsgIntro3$", npc_pyrit, true); // aargh
	this.plane_Hit = this.plane.Hit;
	this.plane.Hit = this.Intro_PlaneHit;
	g_cannoneer->SetCommand("Ungrab");
	return true; // next schedule on plane hit
}

func Intro_8()
{
	MessageBoxAll("$MsgIntro4$", GetHero(), true); // seems like we need to find another way in
	ScheduleNext(40);
}

func Intro_9()
{
	npc_pyrit->SetDir(DIR_Right);
	return Stop();
}

func Intro_Stop()
{
	//this.dialogue->SetInteraction(true); - no dialogue yet
	//this.dialogue->AddAttention();
	AI->AddAI(g_cannoneer);
	AI->SetHome(g_cannoneer);
	AI->SetGuardRange(g_cannoneer, g_cannoneer->GetX()-100, g_cannoneer->GetY()-100, 300, 110);
	g_cannoneer->CreateContents(Sword);
	AI->BindInventory(g_cannoneer);
	g_cannoneer->DoEnergy(10000);
	g_cannoneer->AddEnergyBar();
	g_cannoneer.SpecialDeathMessage = "$DeathOfBrutus$";
	SetPlayerZoomByViewRange(NO_OWNER, 400, 300, PLRZOOM_Set);
	return true;
}

func Intro_PlaneHit()
{
	// Plane hit ground! Continue sequence.
	Sound("Objects::Plane::PlaneCrash", true);
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
			crew->Exit(0,-5, 0, Random(3)-1, Random(5)-6);
			crew->SetAction("Tumble");
		}
	}
	npc_pyrit->Exit(0,-5, 0, -1, -2);
	npc_pyrit->SetAction("Tumble");
	this.Hit = this.intro_seq.plane_Hit;
	this.MeshTransformation = Trans_Mul(Trans_Rotate(10, 0, 2, 1), Airplane.MeshTransformation);
	this.intro_seq->ScheduleNext(50);
	SetObjectLayer(this); // plane is broken
	return true;
}
