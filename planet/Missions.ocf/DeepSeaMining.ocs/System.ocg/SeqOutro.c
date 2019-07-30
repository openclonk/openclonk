#appendto Sequence

func Outro_Start()
{
	this.communicator = FindObject(Find_Func("IsCrystalCommunicator"));
	if (!this.communicator) return false; // what?
	this.hero = this.communicator->FindObject(Find_ID(Clonk), Find_OCF(OCF_Alive), this.communicator->Sort_Distance());
	SetViewTarget(this.communicator);
	// Outro
	ScheduleCall(nil, this.Outro_Fade2Darkness, 15, 32, {});
	Dialogue->MessageBoxAll("$Outro1$", this.hero, true); // ok turn it on
	return ScheduleNext(100);
}

func Outro_1()
{
	this.communicator->StartCommunication(); // 250 frames
	return ScheduleNext(650);
}

func Outro_2()
{
	Dialogue->MessageBoxAll("$Outro2$", this.hero, true); // let's see if it works
	return ScheduleNext(50);
}

func Outro_3()
{
	this.communicator->SendCode("...---..."); // 159 frames
	return ScheduleNext(200);
}

func Outro_4()
{
	this.communicator->StopCommunication();
	MessageBoxAll("$Outro3$", this.hero, true); // i wonder if anyone has heard us
	this.plane = CreateObjectAbove(Airplane, 100, main_island_y-100);
	this.plane->SetContactDensity(85); // only collision with brick for proper landing
	this.pilot = CreateObjectAbove(Clonk, 100, 100);
	this.pilot->MakeInvincible();
	this.pilot->SetSkin(2);
	this.pilot->Enter(this.plane);
	this.pilot->SetAction("Walk");
	this.pilot->SetName("Pyrit");
	this.pilot->SetColor(0xff0000);
	this.pilot->SetAlternativeSkin("MaleBrownHair");
	this.pilot->SetDir(DIR_Right);
	this.pilot->AttachMesh(Hat, "skeleton_head", "main", Trans_Translate(5500, 0, 0));
	this.plane->PlaneMount(this.pilot);
	this.plane->FaceRight();
	this.plane->StartInstantFlight(90, 15);
	return ScheduleNext(5);
}

func Outro_5()
{
	// Wait for plane to arrive
	if (this.plane->GetX() < this.communicator->GetX() - 200) return ScheduleSame(5);
	// Plane in range! Ensure players see it.
	SetPlayerZoomByViewRange(NO_OWNER, 500, 350, PLRZOOM_Direct | PLRZOOM_LimitMax);
	MessageBoxAll("$Outro4$", this.pilot, true); // hey, our friends!
	return ScheduleNext(100);
}

func Outro_6()
{
	MessageBoxAll("$Outro5$", this.hero, true); // we're saved!
	this.plane->StartInstantFlight(245, 15);
	this.plane->SetContactDensity(C4M_Solid);
	return ScheduleNext(60);
}

func Outro_7()
{
	this.plane->StartInstantFlight(280, 5);
	return ScheduleNext(15);
}

func Outro_8()
{
	this.plane->CancelFlight();
	return ScheduleNext(40);
}

func Outro_9()
{
	this.pilot->Exit();
	MessageBoxAll("$Outro6$", this.pilot, true); // hop on everyone!
	return ScheduleNext(100);
}

func Outro_10()
{
	this.plane->FaceRight();
	return ScheduleNext(100);
}

func Outro_11()
{
	Sound("UI::Fanfare");
	return GameOver();
}

func Outro_Fade2Darkness(proplist v)
{
	v.t += 8;
	var fade_val = Max(0xff-v.t);
	SetSkyAdjust(RGB(fade_val, fade_val, fade_val));
}
