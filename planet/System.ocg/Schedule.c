/*--
		Schedule.c
		Authors:
		
		Schedule can be used to execute scripts or functions repetitively with delay.
--*/

// Executes a script repetitively with delay.
global func Schedule(object obj, string script, int interval, int repeats)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	// Create effect.
	var effect = AddEffect("IntSchedule", obj, 1, interval, obj);
	if (!effect)
		return false;
	// Set variables.
	effect.Script = script;
	effect.Repeats = repeats;
	return true;
}

global func FxIntScheduleTimer(object obj, proplist effect)
{
	// Just a specific number of repeats.
	var done = --effect.Repeats <= 0;
	// Execute.
	eval(effect.Script);
	// Remove schedule if done.
	if (done)
		return FX_Execute_Kill;
	return FX_OK;
}

// Adds a timed call to an object, replacement of DefCore TimerCall.
global func AddTimer(
	string function /* function that will be called every time step */
	, int interval /* interval in frames in which the function will be called */)
{
/*
// script for a mine that checks for enemies every second
func Initialize()
{
	AddTimer("CheckForEnemies", 36);
}

func CheckForEnemies()
{
	if(FindObject(Find_AtPoint(), Find_OCF(OCF_Alive)))
		Explode(30);
}
*/

	if (!this)
		return false;
	// Default to one second.
	if (interval == nil)
		interval = 36;
	// Create effect and add function and repeat infinitely.
	var effect = AddEffect("IntScheduleCall", this, 1, interval, this);
	effect.Function = function;
	effect.NoStop = true;
	effect.Pars = [];
	return true;
}

// removes a timer from an object that was added earlier with AddTimer. This removes exactly one timer that fits to the name and returns true on success
global func RemoveTimer(string function /* name of the timer to remove */)
{
	if(!this)
		return false;
		
	var effect, index = 0;
	while(effect = GetEffect("IntScheduleCall", this, index++))
	{
		if(effect.Function != function) continue;
		if(effect.NoStop != true) continue;
		RemoveEffect(nil, this, effect);
		return true;
	}
	
	// not found
	return false;
}

// Executes a function repetitively with delay.
global func ScheduleCall(object obj, string function, int interval, int repeats, par0, par1, par2, par3, par4)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	// Create effect.
	var effect = AddEffect("IntScheduleCall", obj, 1, interval, obj);
	if (!effect)
		return false;
	// Set variables.
	effect.Function = function;
	effect.Repeats = repeats;
	effect.Pars = [par0, par1, par2, par3, par4];
	return true;
}

global func FxIntScheduleCallTimer(object obj, proplist effect)
{
	// Just a specific number of repeats.
	var done = --effect.Repeats <= 0;
	// Execute.
	Call(effect.Function, effect.Pars[0], effect.Pars[1], effect.Pars[2], effect.Pars[3], effect.Pars[4]);
	// Remove schedule call if done, take into account infinite schedules.
	if (done && !effect.NoStop)
		return FX_Execute_Kill;
	return FX_OK;
}

global func ClearScheduleCall(object obj, string function)
{
	var i, effect;
	// Count downwards from effectnumber, to remove effects.
	i = GetEffectCount("IntScheduleCall", obj);
	while (i--)
		// Check All ScheduleCall-Effects.
		if (effect = GetEffect("IntScheduleCall", obj, i))
			// Found right function.
			if (effect.Function == function)
				// Remove effect.
				RemoveEffect(nil, obj, effect);
	return;
}
