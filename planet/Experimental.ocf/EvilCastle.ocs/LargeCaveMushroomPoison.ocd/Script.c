/*-- LargeCaveMushroomPoison --*/

func LaunchPoison(int x, int y, int vx, int vy, int owner)
{
	var poison = CreateObject(LargeCaveMushroomPoison, x,y, owner);
	if (poison) poison->Launch(vx,vy);
	return poison;
}

func Launch(int vx, int vy)
{
	SetXDir(vx); SetYDir(vy);
	SetRDir(Random(21)-10);
	var fx = AddEffect("Move", this, 1, 2, this);
	fx.lifetime = 100+Random(50);
	return true;
}

func ContactBottom()
{
	// bouncy bouncy!
	SetXDir(Random(11)-5);
	SetYDir(-5-Random(10));
	SetRDir(Random(21)-10);
	return true;
}

public func FxMoveTimer(object target, fx, int time)
{
	// lifetime
	if (time > fx.lifetime || Stuck() || GBackLiquid()) { RemoveObject(); return FX_Execute_Kill; }
	// Search victim to poison
	var victim = FindObject(Find_AtRect(-5,-5,11,11), Find_OCF(OCF_Alive), Find_Layer(GetObjectLayer()));
	if (victim)
	{
		var dx=victim->GetX()-GetX(), dy=victim->GetY()-GetY();
		var d=Max(Distance(dx,dy),1);
		dx=dx*20/d; dy=dy*20/d;
		SetXDir(dx); SetYDir(dy-5);
		victim->DoEnergy(-1, false, FX_Call_EngCorrosion, GetOwner());
	}
	// gfx effect
	Smoke(Random(3)-1,-Random(3),5+Random(10),0xffa0ff80);
}

local Plane = 550;
