/* Outro sequence */

#appendto Sequence

func Outro_Start(object goal, object plane)
{
	this.goal = goal;
	this.plane = plane;
	this.hero = plane->FindObject(Find_Not(Find_Owner(NO_OWNER)), Find_ID(Clonk), plane->Sort_Distance());
	
	SetViewTarget(this.plane);
	
	MessageBoxAll("$Outro1$", this.hero, true);
	
	return ScheduleNext(150);
}

func Outro_1()
{
	MessageBoxAll("$Outro2$", npc_pyrit, true);
	return ScheduleNext(220);
}

func Outro_2()
{
	MessageBoxAll("$Outro3$", this.hero, true);
	return ScheduleNext(180);
}

func Outro_3()
{
	MessageBoxAll("$Outro4$", npc_pyrit, true);
	return ScheduleNext(150);
}

func Outro_4()
{
	return Stop();
}

func Outro_Stop()
{
	this.goal->SetFulfilled();
	return true;
}
