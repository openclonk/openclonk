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
	if (effect <= 0) 
		return false;
	// Set variables.
	EffectVar(0, obj, effect) = script;
	EffectVar(1, obj, effect) = repeats;
	return true;
}

global func FxIntScheduleTimer(object obj, int effect)
{
	// Just a specific number of repeats.
	var done = --EffectVar(1, obj, effect) <= 0;
	// Execute.
	eval(EffectVar(0, obj, effect));
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
	if (effect <= 0) 
		return false;
	// Set variables.
	EffectVar(0, obj, effect) = function;
	EffectVar(1, obj, effect) = repeats;
	// EffectVar(2): Just there for backwards compatibility.
	EffectVar(2, obj, effect) = obj; 
	for (var i = 0; i < 5; i++)
		EffectVar(i + 3, obj, effect) = Par(i + 4);
	return true;
}

global func FxIntScheduleCallTimer(object obj, int effect)
{
	// Just a specific number of repeats.
	var done = --EffectVar(1, obj, effect) <= 0;
	// Execute.
	Call(EffectVar(0, obj, effect), EffectVar(3, obj, effect), EffectVar(4, obj, effect), EffectVar(5, obj, effect), EffectVar(6, obj, effect), EffectVar(7, obj, effect));
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
			if (EffectVar(0, obj, effect) == function)      
				// Remove effect.
				RemoveEffect(0, obj, effect);
	return;
}
