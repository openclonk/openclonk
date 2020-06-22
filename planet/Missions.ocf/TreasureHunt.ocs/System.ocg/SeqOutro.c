/* End sequence */

#appendto Sequence

func Outro_Start(object plane)
{
	// Player closest to plane becomes outro protagonist
	this.plane = plane;
	GetHero(npc_pyrit);
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(GetHero());
	npc_pyrit.has_sequence = true; // Pyrit stops hammering
	this.plane->SetR(90); // in case Pyrit isn't done yet
	this.plane.MeshTransformation = Airplane.MeshTransformation;
	var max_unstick = 10;
	while (this.plane->Stuck() && max_unstick--) this.plane->SetPosition(this.plane->GetX(), this.plane->GetY()-1);
	return ScheduleNext(5);
}

func Outro_1()
{
	Dialogue->SetSpeakerDirs(GetHero(), npc_pyrit);
	MessageBoxAll("$Outro1$", npc_pyrit, true); // took u a while
	return ScheduleNext(200);
}

func Outro_2()
{
	Dialogue->SetSpeakerDirs(GetHero(), npc_pyrit);
	MessageBoxAll("$Outro2$", GetHero(), true); // plane looks good
	return ScheduleNext(200);
}

func Outro_3()
{
	MessageBoxAll("$Outro3$", npc_pyrit, true); // let'sgo
	return ScheduleNext(150);
}

func Outro_4()
{
	// Pyrit enters plane. Rest leaves to make sure Pyrit is pilot.
	var clonk;
	while (clonk = this.plane->FindContents(Clonk)) clonk->Exit();
	npc_pyrit->SetCommand("Enter", this.plane);
	return ScheduleNext(30);
}

func Outro_5()
{
	if (npc_pyrit->Contained() != this.plane) return ScheduleSame(5);
	// After Pyrit is inside, rest enters freely
	this.plane->PlaneMount(npc_pyrit);
	this.plane->SetEntrance(true);
	for (var clonk in this.plane->FindObjects(this.plane->Find_AtRect(-60,-30, 120, 60), Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER))))
		clonk->SetCommand("Enter", this.plane);
	return ScheduleNext(100);
}

func Outro_6()
{
	this.plane->FaceRight();
	this.plane->StartFlight(15);
	return ScheduleNext(100);
}

func Outro_7()
{
	this.plane->ContainedLeft(npc_pyrit);
	return ScheduleNext(100);
}

func Outro_8()
{
	return Stop();
}

func Outro_Stop()
{
	GameOver();
	return true;
}
