/*
	Wipf
	Author: Maikel

	A furry creature which is the clonk's biggest friend.
*/

local turn_angle;

public func Initialize()
{
	ActMap = { Prototype = this.Prototype.ActMap };
	ActMap["Walk"] = { Prototype = ActMap["Walk"]};
	SetAction("Walk");
	SetDir(DIR_Right);
	turn_angle = 0;
	AddEffect("Activity", this, 1, 20, this);
	return;
}



func IsWalking()
{
	return (GetAction() == "Walk") || (GetAction() == "Stand");
}

func DiggingIntoBack()
{
	var tex = GetTexture(0, 0);
	var clr = GetAverageTextureColor(tex);
	var particles =
	{
		Prototype = Particles_Dust(),
		R = (clr >> 16) & 0xff,
		G = (clr >> 8) & 0xff,
		B = clr & 0xff,
		Size = PV_KeyFrames(0, 0, 0, 300, 40, 1000, 15),
	};
	CreateParticle("Dust", RandomX(-10, 10), RandomX(-10, 10), PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(18, 1 * 36), particles, 3);
}

func AbortDigIntoBack()
{
	SetActivityInterval(20);
}

func DiggingFromBack()
{
	var tex = GetTexture(0, 0);
	var clr = GetAverageTextureColor(tex);
	var particles =
	{
		Prototype = Particles_Dust(),
		R = (clr >> 16) & 0xff,
		G = (clr >> 8) & 0xff,
		B = clr & 0xff,
		Size = PV_KeyFrames(0, 0, 0, 300, 40, 1000, 15),
	};
	CreateParticle("Dust", RandomX(-10, 10), RandomX(-10, 10), PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(18, 1 * 36), particles, 3);
}

func EndDigFromBack()
{
	SetAction("Jump");
	SetSpeed(RandomX(-10, 10), RandomX(-10, 10));
}

func QueryCatchBlow()
{
	if(GetAction() == "DigIntoBack")
		return true;
	return _inherited(...);
}

func DigIntoBackground()
{
	Turn(DIR_Left);
	SetAction("DigIntoBack");
	SetActivityInterval(0);
	
	var e = GetEffect("LeftTunnel", this);
	if(e)
	{
		AddEffect("TunnelTainted", e.tunnel, 1, 36*7, e.tunnel);
	}
}

func SetActivityInterval(int to)
{
	var e = GetEffect("Activity", this);
	if(e)
		e.Interval = to;
}

func Entrance(object to)
{
	SetActivityInterval(40);
	SetAction("Stand");
	var c;
	for(var i = ContentsCount(); c = Contents(--i);)
		c->Enter(to);
}

func Departure(object from)
{
	SetActivityInterval(20);
	//if(from->GetID() == CaveSlug_TunnelExit)
	{
		var e = AddEffect("LeftTunnel", this, 1, 36 * 6, this);
		e.tunnel = from;
	}
}

func HasBeenHurtRecently()
{
	var e = GetEffect("Activity", this);
	if(!e) return false;
	return e.damage_tolerance > 0;
}

func FxActivityStart(_, effect, temp)
{
	if(temp) return;
	effect.damage_tolerance = 0;
}

func FxActivityTimer(_, effect, time)
{
	if(Contained())
	{
		if(effect.damage_tolerance > 0)
			--effect.damage_tolerance;
		return 1;
	}
	
	if(!IsWalking()) return 1;
	
	if(effect.damage_tolerance > 0)
	{
		if(!Random(3))
			effect.damage_tolerance -= 1;
		if(effect.damage_tolerance > 10)
		{
			DigIntoBackground();
			return 1;
		}
	}
	
	if(GBackLiquid())
	{
		DigIntoBackground();
		return 1;
	}

	if(GetCommand())
		if(!Random(5)) FinishCommand();
	
	for(var o in FindObjects(Find_Distance(100), Find_OCF(OCF_Alive), Find_Hostile(GetOwner()), Find_NoContainer(), Sort_Distance()))
	{
		if(PathFree(GetX(), GetY(), o->GetX(), o->GetY()))
		{
			var d = o->GetX() - GetX();
			if(d < 0) Turn(DIR_Left, true);
			else Turn(DIR_Right, true);
		}
		else
		{
			if(ObjectDistance(this, o) > 40) break;
			if(GetEffect("NoNewCommandFollow", o)) continue;
			
			AddEffect("NoNewCommandFollow", o, 1, 36*10, this);
			if(!GetCommand())
				SetCommand("MoveTo", o);
			return 1;
		}

		if(ObjectDistance(this, o) <= 15)
		{
			SetAction("Dig");//StartDigging(); // it's a synonym
		}
		return 1;
	}
	
	if(!ContentsCount())
	for(var o in FindObjects(Find_Distance(30), Find_NoContainer(), Sort_Reverse(Sort_Func("CaveSlugItemValue", this))))
	{
		if(!(o->GetCategory() & C4D_Object) && !o->~GetNutritionalValue()) continue;
		
		if(PathFree(GetX(), GetY(), o->GetX(), o->GetY()))
		{
			var d = o->GetX() - GetX();
			if(d < 0) Turn(DIR_Left, true);
			else Turn(DIR_Right, true);
		}
		else
		{
			if((Abs(GetY() - o->GetY()) > 15) || o->GetY() > GetY() + 10) continue;
			if(GetEffect("NoNewSearch", o)) continue;
			
			if(!GetCommand())
			{
				SetCommand("MoveTo", o);
				AddEffect("NoNewSearch", o, 1, 36*10, this);
			}
			return 1;
		}

		if(ObjectDistance(this, o) <= 10)
		{
			SetAction("Dig");//StartDigging(); // it's a synonym
			AddEffect("Collecting", this, 1, 25, this);
		}
		return 1;
	}
	
	if(!ContentsCount())
	{
		if(!Random(10)) {Turn(DIR_Left, true); return 1;}
		else if(!Random(10)) {Turn(DIR_Right, true); return 1;}
	}
	if(!Random(10))
	{
		DigIntoBackground();
		return 1;
	}
}

func FxActivityDamage(_, effect, damage, cause)
{
	if(damage > 0) return damage;
	effect.damage_tolerance += -damage / 1000;
	return damage;
}

global func CaveSlugItemValue(slug)
{
	if(this->~NutritionValue())
		if(slug->GetEnergy() < slug.MaxEnergy/1000) return 1000;
	return GetValue();
}

func StartDead()
{
	PlayAnimation("Die", 5, Anim_Linear(0, 0, GetAnimationLength("Die"), 40, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Hold));
}

func StartDigging()
{
	SetXDir(0);
	SetComDir(COMD_Stop);
	if(!GetEffect("IntDig", this))
		AddEffect("IntDig", this, 1, 1, this);
}

func StopDigging()
{
	if(GetAction() != "Dig")
	{
		RemoveEffect("IntDig", this);
	}
}

func GetRandomDigAnimation()
{
	var a = ["Dig", "Dig2"];
	return a[Random(GetLength(a))];
}

func FxIntDigStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	var anim = GetRandomDigAnimation();
	effect.var1 = PlayAnimation(anim, 5, Anim_Linear(0, 0, GetAnimationLength(anim), 28, ANIM_Remove), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	SetActivityInterval(0);
}

func FxIntDigTimer(pTarget, effect, iTime)
{
	if(iTime < 20) return 1;
	
	Sound("Dig*");
	
	var fac = 1;
	if(GetDir() == DIR_Left) fac*=-1;
	DigFree(GetX() + 10 * fac, GetY() - 7, 5);
	var x = 0;
	if(fac == -1) x = -15;
	DigFreeRect(GetX() + x, GetY() - 7, 15, 14);
	
	// also punches stuff
	var o = FindObject(Find_Distance(10, 10 * fac, 0), Find_OCF(OCF_Alive), Find_Hostile(GetOwner()));
	if(o)
		this->Punch(o, 20);
	
	// collect stuff too!
	if(!ContentsCount())
	{
		for(var o in FindObjects(Find_Distance(20), Find_NoContainer(), Sort_Reverse(Sort_Func("CaveSlugItemValue", this))))
		{
			var isobj = o->GetCategory() & C4D_Object;
			var isactable = o->~IsInteractable(this);
			var isfood = o->~NutritionalValue();
			
			if(!isobj && !(isfood && isactable)) continue;
			
			if(isactable)
			{
				o->~Interact(this);
				SetAction("Dig");
				return -1;
			}
			
			o->Enter(this);
			break;
		}
	}
	return -1;
}

func Collection2(what)
{
	if(what->~NutritionalValue())
		if(GetEnergy() < MaxEnergy/1000)
		{
			DoEnergy(what->NutritionalValue());
			what->RemoveObject(1);
			Sound("Munch*");
			return;
		}
}

func FxIntDigStop(target, effect, reason, temp)
{
	if(temp) return;
	if(GetAction() == "Dig")
		SetAction("Walk");
	SetActivityInterval(20);
}

func ContactLeft()
{
	if(GetAction() != "Walk") return;
	if(!Random(3)) return Jump();
	if(GetCommand()) return;
	if(GetEffect("NoContactDig", this)) return Turn(DIR_Right);
	AddEffect("NoContactDig", this, 1, 35*1, this);
	SetAction("Dig");
}

func ContactRight()
{
	if(GetAction() != "Walk") return;
	if(!Random(3)) return Jump();
	if(GetCommand()) return;
	if(GetEffect("NoContactDig", this)) return Turn(DIR_Left);
	AddEffect("NoContactDig", this, 1, 35*1, this);
	SetAction("Dig");
}

func StartStand()
{
	PlayAnimation("Stand", 5, GetWalkAnimationPosition("Stand"), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
}

func GetWalkAnimationPosition(string anim, int pos)
{
	if(pos == nil) pos = 0;
	return Anim_Linear(pos, 0, GetAnimationLength(anim), 35, ANIM_Loop);
}


func StartWalk()
{
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func StartDigFromBack()
{
	var anim = "DigFromBack";
	PlayAnimation(anim, 5, Anim_Linear(0, 0, GetAnimationLength(anim), 18 * 2, ANIM_Remove), Anim_Const(1000));
}

func StartDigIntoBack()
{
	var anim = "DigFromBack";
	var len = GetAnimationLength(anim);
	PlayAnimation(anim, 5, Anim_Linear(len, len, 0, 18 * 2, ANIM_Remove), Anim_Const(1000));
}

func GetCurrentWalkAnimation()
{
	if(Contained())
		return "Stand";
	var velocity = Distance(0,0,GetXDir(),GetYDir());
	if(GetComDir() == COMD_None || GetComDir() == COMD_Stop) return "Stand";
	if(velocity < 10) return "Walk";
	return "Walk";
}


func CheckTurn()
{
	if(GetXDir() < 0) if(GetDir() != DIR_Left) SetDir(DIR_Left);
	else if(GetXDir() > 0) if(GetDir() != DIR_Right) SetDir(DIR_Right);
	
	var t = false;
	if(turn_angle == 0 && GetDir() == DIR_Left) t = true;
	else if(turn_angle == 180 && GetDir() == DIR_Right) t = true;
	
	if(t)
	{
		if(!GetEffect("IntTurning", this))
			AddEffect("IntTurning", this, 1, 1, this);
	}
}

func Turn(int dir, bool move)
{
	if(move)
	{
		if(dir == DIR_Left) SetComDir(COMD_Left);
		else SetComDir(COMD_Right);
	}
	if(GetDir() == dir) return;
	SetXDir(0);
	SetDir(dir);
	CheckTurn();
}

func FxIntTurningStart(_, effect, temp)
{
	if(temp)
		return true;
}

func FxIntTurningTimer(_, effect, time)
{
	if(GetDir() == DIR_Left)
		turn_angle += 10;
	else turn_angle -= 10;

	if(turn_angle < 0 || turn_angle > 180)
	{
		turn_angle = BoundBy(turn_angle, 0, 180);
		this.MeshTransformation = Trans_Rotate(turn_angle,0,1,0);
		return -1;
	}
	this.MeshTransformation = Trans_Rotate(turn_angle,0,1,0);
	return 1;
}

func FxIntWalkStart(pTarget, effect, fTmp)
{
	if(fTmp) return;
	// Always start in Stand for now... should maybe fade properly from previous animation instead
	effect.anim_name = GetCurrentWalkAnimation();
	effect.animation = PlayAnimation(effect.anim_name, 5, GetWalkAnimationPosition(effect.anim_name), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	effect.cycle = 0;
}

func FxIntWalkTimer(target, effect, time)
{
	CheckTurn();
	
	var cur = GetCurrentWalkAnimation();
	if(effect.anim_name != cur)
	{
		effect.anim_name = cur;
		effect.animation = PlayAnimation(effect.anim_name, 5, GetWalkAnimationPosition(effect.anim_name), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		effect.cycle = 0;
		//Log("Play %s", cur);
	}
	if(GetComDir() == COMD_Stop || GetComDir() == COMD_None) return 1;
	// movement follows a sine curve
	var s = Abs(Sin(++effect.cycle * 10, 160));
	ActMap.Walk.Speed = s * 3;
	
	// particles
	var dir = Sign(GetXDir());
	var clr = GetAverageTextureColor(GetTexture(0,10));
	var particles =
	{
		Prototype = Particles_Dust(),
		R = (clr >> 16) & 0xff,
		G = (clr >> 8) & 0xff,
		B = clr & 0xff,
	};
	if(s > 50)
		CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 5, PV_Random(dir * 2, dir * 1), PV_Random(-2, -3), PV_Random(36, 2 * 36), particles, 5);
	return 1;
}

func FxIntWalkStop(target, effect, cause, temp)
{
	if(temp) return;
	if(effect.animation)
		StopAnimation(effect.animation);
}

func FxIntWalkReset(pTarget, effect)
{
	effect.animation = 0;
}

func StartJump()
{
	SetActivityInterval(0);
}

func EndJump()
{
	SetActivityInterval(20);
}

func Jump()
{
	SetAction("Jump");
	var dir = 10;
	if(GetDir() == DIR_Left) dir *= -1;
	SetSpeed(GetXDir() + dir, -20 - GetXDir()/2);
	for(var c = 0; c < 5; ++c)
	{
		var dir = Sign(GetXDir());
		var clr = GetAverageTextureColor(GetTexture(0,10));
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
		};
		CreateParticle("Dust", PV_Random(dir * -2, dir * -1), 5, PV_Random(dir * 2, dir * 1), PV_Random(-2, -3), PV_Random(36, 2 * 36), particles, 5);
	}
		
}


/*-- DefCore --*/

local Name = "$Name$";
local Description = "$Description$";
local MaxEnergy = 50000;
local NoBurnDecay = 1;


/*-- Act Map --*/

local ActMap = {
	Walk = {
		Prototype = Action,
		Name = "Walk",
		Procedure = DFA_WALK,
		Accel = 32,
		Decel = 44,
		Speed = 300,
		Directions = 2,
		FlipDir = 0,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 16,
		Hgt = 10,
		StartCall = "StartWalk",
		AbortCall = "StopWalk",
		EndCall = "StopWalk",
	},
	Stand = {
		Prototype = Action,
		Name = "Stand",
		Procedure = DFA_THROW,
		Directions = 2,
		FlipDir = 0,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 16,
		Hgt = 10,
		StartCall = "StartStand",
	},
	Kneel = {
		Prototype = Action,
		Name = "Kneel",
		Procedure = DFA_KNEEL,
		Directions = 2,
		FlipDir = 0,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 16,
		Hgt = 10,
	},
	Dig = {
		Prototype = Action,
		Name = "Dig",
		Procedure = DFA_THROW,
		Speed = 5,
		Accel = 1,
		Directions = 2,
		Length = 28,
		Delay = 1,
		X = 0,
		Y = 60,
		Wdt = 16,
		Hgt = 10,
		NextAction = "Walk",
		StartCall = "StartDigging",
		AbortCall = "StopDigging"//,
		//Attach = CNAT_Left | CNAT_Right | CNAT_Bottom,
	},
	Jump = {
		Prototype = Action,
		Name = "Jump",
		Procedure = DFA_FLIGHT,
		Speed = 240,
		Accel = 14,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 120,
		Wdt = 16,
		Hgt = 10,
		Animation = "Stand",
		PhaseCall = "CheckStuck",
		StartCall = "StartJump",
		EndCall = "EndJump",
		AbortCall = "EndJump",
	},
	DigFromBack = {
		Prototype = Action,
		Name = "DigFromBack",
		Procedure = DFA_FLOAT,
		Directions = 2,
		Length = 18,
		Delay = 2,
		Animation = "DigFromBack",
		StartCall = "StartDigFromBack",
		EndCall = "EndDigFromBack",
		PhaseCall = "DiggingFromBack",
		NextAction = "Jump",
	},
	DigIntoBack = {
		Prototype = Action,
		Name = "DigIntoBack",
		Procedure = DFA_FLOAT,
		Directions = 2,
		Length = 18,
		Delay = 2,
		Animation = "DigFromBack",
		StartCall = "StartDigIntoBack",
		EndCall = "EndDigIntoBack",
		PhaseCall = "DiggingIntoBack",
		AbortCall = "AbortDigIntoBack",
		NextAction = "Walk",
	},
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Directions = 2,
		X = 0,
		Y = 240,
		Wdt = 16,
		Hgt = 10,
		Length = 1,
		Delay = 0,
		NextAction = "Hold",
		StartCall = "StartDead",
		NoOtherAction = 1,
		ObjectDisabled = 1,
	},
};