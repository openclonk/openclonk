/**
	CrewHUDUser
	Different callbacks.

*/

func Initialize()
{
	AddEffect("HUDBarUpdater", this, 1, 0, this);
	return _inherited(...);
}

func FxHUDBarUpdaterDamage(target, effect, int damage, int cause)
{
	if(effect.last == effect.Time) return damage;
	effect.last = effect.Time;
	
	ScheduleUpdateHUDHealthBar();
	return damage;
}

private func ScheduleUpdateHUDHealthBar()
{
	Schedule(Format("UpdateHUDHealthBar(%d)", GetOwner()), 1, 0);
}

private func ScheduleUpdateBackpack()
{
	Schedule(Format("UpdateBackpack(%d)", GetOwner()), 1, 0);
}

func Collection2()
{
	ScheduleUpdateBackpack();
	return _inherited(...);
}

func Ejection()
{
	ScheduleUpdateBackpack();
	return _inherited(...);
}

func ControlContents()
{
	ScheduleUpdateBackpack();
	return _inherited(...);
}

func CrewSelection()
{
	ScheduleUpdateBackpack();
	ScheduleUpdateHUDHealthBar();
	return _inherited(...);
}

func Recruitment()
{
	ScheduleUpdateBackpack();
	ScheduleUpdateHUDHealthBar();
	return _inherited(...);
}