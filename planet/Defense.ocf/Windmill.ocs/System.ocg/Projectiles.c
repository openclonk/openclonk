#appendto Arrow
#appendto Javelin

local fade_out_forced = false;

// Arrows and javelins should fade out very fast after hitting, they are useless by then anyway

private func Stick()
{
	if (!fade_out_forced)
	{
		fade_out_forced = true;
		if (g_object_fade) AddEffect("IntFadeOut", this, 100, 1, g_object_fade, Rule_ObjectFade);
	}
	_inherited(...);
}

public func FadeOutForced() { return fade_out_forced; }