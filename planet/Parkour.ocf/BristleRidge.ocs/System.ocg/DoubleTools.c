// Clonks may only have one grapplebow and wind bag

#appendto Clonk

protected func RejectCollect(id def)
{
	if (def == GrappleBow)
		if (ObjectCount(Find_Container(this), Find_ID(GrappleBow)) >= 1)
			return true;
	if (def == WindBag)
		if (ObjectCount(Find_Container(this), Find_ID(WindBag)) >= 1)
			return true;
	return _inherited(def, ...);
}