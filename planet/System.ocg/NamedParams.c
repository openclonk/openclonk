/*--
		NamedParams.c

		Adapts some functions with lots of optional parameters to take a proplist instead.
--*/

global func Sound(string name, opts, ...)
{
	if (GetType(opts) == C4V_PropList)
		return inherited(name, opts.global, opts.volume, opts.player, opts.loop_count, opts.custom_falloff_distance, opts.pitch, opts.modifier);
	else
		return inherited(name, opts, ...);
}

global func SoundAt(string name, int x, int y, opts, ...)
{
	if (GetType(opts) == C4V_PropList)
		return inherited(name, x, y, opts.volume, opts.player, opts.custom_falloff_distance, opts.pitch, opts.modifier);
	else
		return inherited(name, x, y, opts, ...);
}
