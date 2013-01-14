// Clonks may only have one grapplebow and jar of winds.

#appendto Clonk

protected func RejectCollect(id def)
{
	if (def == GrappleBow)
		if (ObjectCount(Find_Container(this), Find_ID(GrappleBow)) >= 1)
			return true;
	if (def == JarOfWinds)
		if (ObjectCount(Find_Container(this), Find_ID(JarOfWinds)) >= 1)
			return true;
	return _inherited(def, ...);
}