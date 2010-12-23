/*--
		Schedule.c
		Authors:
		
		Schedule can be used to execute scripts or functions repetitively with delay.
--*/

// Executes a script repetitively with delay.
global func Schedule(string script, int interval, int repeats, object obj)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	if (!obj)
		obj = this;
	// Create effect.
	var effect = AddEffect("IntSchedule", obj, 1, interval, obj);
	if (!effect)
		return false;
	// Set variables.
	effect.var0 = script;
	effect.var1 = repeats;
	return true;
}

global func FxIntScheduleTimer(object obj, effect)
{
	// Just a specific number of repeats.
	var done = --effect.var1 <= 0;
	// Execute.
	eval(effect.var0);
	return -done;
}

// Executes a function repetitively with delay.
global func ScheduleCall(object obj, string function, int interval, int repeats, par0, par1, par2, par3, par4)
{
	// Defaults.
	if (!repeats)
		repeats = 1;
	if (!obj)
		obj = this;
	// Create effect.
	var effect = AddEffect("IntScheduleCall", obj, 1, interval, obj);
	if (!effect)
		return false;
	// Set variables.
	effect.function = function;
	effect.var1 = repeats;
	effect.par = [par0, par1, par2, par3, par4];
	return true;
}

global func FxIntScheduleCallTimer(object obj, effect)
{
	// Just a specific number of repeats.
	var done = --effect.var1 <= 0;
	// Execute.
	Call(effect.function, effect.par[0], effect.par[1], effect.par[2], effect.par[3], effect.par[4]);
	return -done;
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
			if (effect.function == function)
				// Remove effect.
				RemoveEffect(0, obj, effect);
	return;
}
