/**
	NamedParams.c
	Adapts some functions with lots of optional parameters to take a proplist instead.

	@author Marky
*/

// documented in /docs/sdk/script/fn
global func Sound(string name, opts, ...)
{
	if (GetType(opts) == C4V_PropList)
		return inherited(name, opts.global, opts.volume, opts.player, opts.loop_count, opts.custom_falloff_distance, opts.pitch, opts.modifier);
	return inherited(name, opts, ...);
}

// documented in /docs/sdk/script/fn
global func SoundAt(string name, int x, int y, opts, ...)
{
	if (GetType(opts) == C4V_PropList)
		return inherited(name, x, y, opts.volume, opts.player, opts.custom_falloff_distance, opts.pitch, opts.modifier);
	return inherited(name, x, y, opts, ...);
}
