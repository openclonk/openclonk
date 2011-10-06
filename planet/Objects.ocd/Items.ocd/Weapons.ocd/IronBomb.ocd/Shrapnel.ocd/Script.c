/* shrapnel */

public func ProjectileDamage() { return 1; }
public func FlightTime() { return 4; }

protected func Initialize()
{
	SetAction("Flight");
	AddEffect("Fade", this, 1, 1, this);
}

protected func FxFadeTimer(object target, int num, int timer)
{
/*	SetObjAlpha(255 - ((timer * 1275)/ 100));
	if(timer >= 20)
	{
		RemoveObject();
	}*/

	if(timer > FlightTime()) RemoveObject();
}

protected func Hit()
{
	ShakeFree(6);
	RemoveObject();
}

public func HitObject(object obj)
{
	ProjectileHit(obj,ProjectileDamage(),ProjectileHit_tumble);
	Sound("ProjectileHitLiving*.ogg");
	RemoveObject();
}

public func TrailColor(int time)
{
	return RGBa(100,100,100,240*Max(0,FlightTime()-time)/FlightTime());
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLIGHT,
		NextAction = "Fly",
		Delay = 1,
		Length = 1,
	},
};