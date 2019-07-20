/* End sequence */

#appendto Sequence

func Outro_Start(object plane)
{
	// Player closest to plane becomes outro protagonist
	this.plane = plane;
	this.hero = plane->FindObject(Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER)), plane->Sort_Distance());
	SetPlayerZoomByViewRange(NO_OWNER, 200, 100, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetViewTarget(this.hero);
	npc_pyrit.has_sequence = true; // Pyrit stops hammering
	return ScheduleNext(5);
}

func Outro_1()
{
	Dialogue->SetSpeakerDirs(this.hero, npc_pyrit);
	MessageBoxAll("$Outro1$", this.hero, true); // how 2 fly plane?
	return ScheduleNext(200);
}

func Outro_2()
{
	Dialogue->SetSpeakerDirs(this.hero, npc_pyrit);
	MessageBoxAll("$Outro2$", npc_pyrit, true); // i c u r well prepared
	return ScheduleNext(200);
}

func Outro_3()
{
	MessageBoxAll("$Outro3$", npc_pyrit, true); // dun worry. me b pilot
	return ScheduleNext(200);
}

func Outro_4()
{
	MessageBoxAll("$Outro4$", this.hero, true); // let's go then
	return ScheduleNext(150);
}

func Outro_5()
{
	// Pyrit enters plane. Rest leaves to make sure Pyrit is pilot.
	var clonk;
	while (clonk = this.plane->FindContents(Clonk)) clonk->Exit();
	npc_pyrit->SetCommand("Enter", this.plane);
	return ScheduleNext(30);
}

func Outro_6()
{
	if (npc_pyrit->Contained() != this.plane) return ScheduleSame(5);
	// After Pyrit is inside, rest enters freely
	this.plane->PlaneMount(npc_pyrit);
	this.plane->SetEntrance(true);
	for (var clonk in this.plane->FindObjects(this.plane->Find_AtRect(-60,-30, 120, 60), Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER))))
		clonk->SetCommand("Enter", this.plane);
	return ScheduleNext(100);
}

func Outro_7()
{
	this.plane->FaceRight();
	this.plane->StartFlight(15);
	return ScheduleNext(100);
}

func Outro_8()
{
	this.plane->ContainedLeft(npc_pyrit);
	return ScheduleNext(100);
}

func Outro_9()
{
	return Stop();
}

func Outro_Stop()
{
	GameOver();
	return true;
}
