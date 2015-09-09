/**
	HUD Clock
	Can display the time or a countdown in the HUD.
	
	@authors Sven2, Maikel
*/


protected func Initialize()
{
	// Set parallaxity.
	this.Parallaxity = [0, 0];
	// Set visibility.
	this.Visibility = VIS_All;
	return;
}


/*-- Time Setting --*/

// Sets the visual appearance of the clock, to_time in seconds.
public func SetTime(int to_time)
{
	if (!to_time || to_time <= 0)
		return CustomMessage("", this, GetOwner(), 0, 90);
	var minutes = to_time / 60;
	var seconds = to_time % 60;		
	var color = 0x00ff00;
	if (to_time < 10)
	{
		color = 0xff0000;
		var play_for = GetOwner();
		if (play_for == NO_OWNER)
			play_for = nil; 	
		Sound("Click", true, 80, play_for);
	}
	CustomMessage(Format("@<c %x>%02d:%02d</c>", color, minutes, seconds), this, GetOwner(), 0, 90);
	return;
}


/*-- Countdown --*/

// Creates a countdown for a duration in second for a given player.
// If for_plr == nil the clock is visible for all players.
// The game call OnCountdownFinished(int for_plr) is made when the clock runs out.
// The same call will be made to the callback object if given.
public func CreateCountdown(int total_time, int for_plr, object callback_obj)
{
	if (this != GUI_Clock)
		return;
		
	// Remove any existing countdown for the give player.
	RemoveCountdown(for_plr);
	
	// Create the clock object.
	var plr = for_plr ?? NO_OWNER;
	var clock = CreateObject(GUI_Clock, 0, 0, plr);
	clock.for_plr = for_plr;
	// Place the clock next to the goal.
	clock->SetPosition((-64 - 16) * 2 - 32 - GUI_Clock->GetDefHeight() / 2, 8 + GUI_Clock->GetDefHeight() / 2);
	if (for_plr != nil)
		clock.Visibility = VIS_Owner;
	// Add a countdown effect.
	AddEffect("IntCountdown", clock, 100, 36, clock, nil, total_time, for_plr, callback_obj);
	return clock;
}

// Removes the current countdown for the given player.
public func RemoveCountdown(int for_plr)
{
	if (this != GUI_Clock)
		return;
	for (var clock in FindObjects(Find_ID(GUI_Clock)))
		if (clock.for_plr == for_plr)
			clock->RemoveObject();
	return;
}

protected func FxIntCountdownStart(object target, proplist effect, int temporary, int total_time, int for_plr, object callback_obj)
{
	if (temporary)
		return FX_OK;
	effect.time = total_time;
	effect.for_plr = for_plr;
	effect.callback_obj = callback_obj;
	SetTime(effect.time);
	return FX_OK;
}

protected func FxIntCountdownTimer(object target, proplist effect, int time)
{
	effect.time--;
	SetTime(effect.time);
	if (effect.time <= 0)
	{
		return FX_Execute_Kill;
	}
	return FX_OK;
}

protected func FxIntCountdownStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return FX_OK;
	if (reason == FX_Call_Normal)
	{
		GameCallEx("OnCountdownFinished", effect.for_plr);
		if (effect.callback_obj)
			effect.callback_obj->~OnCountdownFinished(effect.for_plr);
		if (target)
			target->RemoveObject();
	}
	return FX_OK;
}
