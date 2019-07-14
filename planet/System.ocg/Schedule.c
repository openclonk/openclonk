/**
	Schedule.c
	Schedule can be used to execute scripts or functions repetitively with delay.

	@author
*/

// Executes a script repetitively with delay.
// documented in /docs/sdk/script/fn
global func Schedule(object obj, string script, int interval, int repeats)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	// Create effect.
	var fx = AddEffect("IntSchedule", obj, 1, interval, obj);
	if (!fx)
		return false;
	// Set variables.
	fx.Script = script;
	fx.Repeats = repeats;
	return true;
}

global func FxIntScheduleTimer(object obj, effect fx)
{
	// Just a specific number of repeats.
	var done = --fx.Repeats <= 0;
	// Execute.
	eval(fx.Script);
	// Remove schedule if done.
	if (done)
		return FX_Execute_Kill;
	return FX_OK;
}

// Adds a timed call to an object, replacement of DefCore TimerCall.
global func AddTimer(call_function, int interval)
{
	if (!this)
		return false;
	// Default to one second.
	if (interval == nil)
		interval = 36;
	// Create effect and add function and repeat infinitely.
	var fx = AddEffect("IntScheduleCall", this, 1, interval, this);
	fx.Function = call_function;
	fx.NoStop = true;
	fx.Pars = [];
	return true;
}

// removes a timer from an object that was added earlier with AddTimer. This removes exactly one timer that fits to the name and returns true on success
global func RemoveTimer(call_function /* name or pointer to the timer to remove */)
{
	if (!this)
		return false;
		
	var fx, index = 0;
	while (fx = GetEffect("IntScheduleCall", this, index++))
	{
		if (fx.Function != call_function) continue;
		if (fx.NoStop != true) continue;
		RemoveEffect(nil, this, fx);
		return true;
	}
	
	// not found
	return false;
}

// Executes a function repetitively with delay.
// documented in /docs/sdk/script/fn
global func ScheduleCall(object obj, call_function, int interval, int repeats, par0, par1, par2, par3, par4)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	// Create effect.
	var fx = AddEffect("IntScheduleCall", obj, 1, interval, obj);
	if (!fx)
		return false;
	// Set variables.
	fx.Function = call_function;
	fx.Repeats = repeats;
	fx.Pars = [par0, par1, par2, par3, par4];
	return true;
}

global func FxIntScheduleCallTimer(object obj, effect fx)
{
	// Just a specific number of repeats.
	var done = --fx.Repeats <= 0;
	// Execute.
	Call(fx.Function, fx.Pars[0], fx.Pars[1], fx.Pars[2], fx.Pars[3], fx.Pars[4]);
	// Remove schedule call if done, take into account infinite schedules.
	if (done && !fx.NoStop)
		return FX_Execute_Kill;
	return FX_OK;
}

// documented in /docs/sdk/script/fn
global func ClearScheduleCall(object obj, call_function)
{
	var i;
	// Count downwards from effectnumber, to remove effects.
	i = GetEffectCount("IntScheduleCall", obj);
	while (i--) {
		// Check All ScheduleCall-Effects.
		var fx = GetEffect("IntScheduleCall", obj, i);
		if (fx && fx.Function == call_function)
			// Found right function. Remove effect.
			RemoveEffect(nil, obj, fx);
	}
	return;
}
