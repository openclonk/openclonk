/*--
	TeleGlove
	Moves objects remotely like magic!
	
	@author: Ringwaul
--*/

local radius; //actual effect radius to grab objects
local radiusparticle; //radius of particles around object

local is_aiming;
local anim_spin;

local iAngle;
local aim_anim;
local carry_bone;

local target_object;

/*-- Engine Callbacks --*/

func Initialize()
{
	radius = 60;
	radiusparticle = radius / 4;
}

func Hit()
{
	Sound("Hits::GeneralHit?");
}

/*-- Global functions --*/

global func FxTeleGloveReleasedStart(object target, effect)
{
	effect.t0 = FrameCounter();
	return;
}

global func FxTeleGloveWeightStart(object target, proplist effect)
{
	target->SetMass(target->GetMass()/2);
}

global func FxTeleGloveWeightStop(object target, proplist effect, int reason, bool temp)
{
	target->SetMass(target->GetDefCoreVal("Mass", "DefCore"));
}

// Damaging Clonks with moving objects makes this tool stupidly strong. So it's blocked
// while moving the object and a few frames after release
global func FxTeleGloveWeightQueryHitClonk(object target, fx, object clonk) { return true; }
global func FxTeleGloveReleasedQueryHitClonk(object target, fx, object clonk) { return FrameCounter()-fx.t0 <= 5; }

/*-- Usage --*/

// The reach of the tele glove, can be modified by overloading.
public func GetTeleGloveReach() { return 150; }

public func HoldingEnabled() { return true; }

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction() || !(clonk->IsWalking() || clonk->IsJumping());
}

public func ControlUseStart(object clonk, ix, iy)
{
	StartUsage(clonk);
	UpdateGloveAngle(clonk, ix, iy);
	return true;
}

public func ControlUseHolding(object clonk, ix, iy)
{
	if (!clonk->HasHandAction() || !is_aiming || (!clonk->IsWalking() && !clonk->IsJumping()))
	{
		CancelUse(clonk);
		return true;
	}

	UpdateGloveAngle(clonk, ix, iy);

	//effects
	var xs = Sin(Random(359),radiusparticle/2);
	var ys = -Cos(Random(359),radiusparticle/2);
	var anglep = Angle(0,0,ix,iy);
	var distp = Distance(0,0,ix,iy);
	
	var particles;
	if (Random(2)) particles = Particles_ElectroSpark1();
	else particles = Particles_ElectroSpark2();
	
	if (distp < GetTeleGloveReach())
	{
		//Particles moving towards object
		CreateParticle("ElectroSpark", ix + xs, iy + ys, PV_Random(-xs/2, -xs/4), PV_Random(-ys/2, -ys/4), PV_Random(5, 10), particles, 5);
		
		//Particles emitting from clonk
		var wp = 1;
		if(Random(2)) wp = -1;
		var xp = anglep + RandomX(-3, 3);
		var yp = anglep + RandomX(-3, 3);
		var xdir = Sin(xp, 10);
		var ydir = -Cos(yp, 10);
		var distance = Random(distp);
		CreateParticle("ElectroSpark", Sin(xp, distance), -Cos(yp, distance), PV_Random(xdir - 5, xdir + 5), PV_Random(ydir - 2, ydir + 2), PV_Random(5, 10), particles, 5);
	}

	var target;
	if(target_object)
	{
		if(Distance(target_object->GetX(), target_object->GetY(), clonk->GetX() + ix, clonk->GetY() + iy) > radius ||
		Distance(target_object->GetX(), target_object->GetY(), clonk->GetX(), clonk->GetY()) > GetTeleGloveReach() ||
		target_object->~RejectTeleGloveControl(this))
		{
			LostTargetObject(target);
			target_object = nil;
		}
	}

	if(!target_object)
	{
		target = FindObject(Find_Exclude(this),
					Find_NoContainer(),
					Find_Category(C4D_Object),
					Find_And(Find_Distance(radius, ix, iy),
					Find_Distance(GetTeleGloveReach() - 15)),
					Find_Not(Find_Func("RejectTeleGloveControl", this)),
					Sort_Distance(ix,iy));

		if(target)
		{
			GainedTargetObject(target);
			target_object = target;
		}
	}

	//Has the object recently been thrown by another player?
	var e = GetEffect("TeleGloveReleased", target_object);
	var old_controller; if(e) old_controller = e.controller;
	if(target_object 
		&& !target_object->Stuck()
		&& (!e || old_controller == Contained()->GetOwner()))
	{
		var angle = Angle(target_object->GetX(), target_object->GetY(), clonk->GetX() + ix, clonk->GetY() + iy);
		var dist = Distance(target_object->GetX(), target_object->GetY(), clonk->GetX() + ix, clonk->GetY() + iy);
		var speed = dist;

		//Set speed
		target_object->SetSpeed(Sin(angle, speed), -Cos(angle, speed));

		//Particles emitting from object
		target_object->CreateParticle("ElectroSpark", 0, 0, PV_Random(xs/8, xs/10), PV_Random(ys/8, ys/10), PV_Random(5, 10), particles, 5);
	}
	else
		LostTargetObject(target_object);
	return 1;
}

public func ControlUseCancel(object clonk, int ix, int iy)
{
	return CancelUse(clonk);
}

public func ControlUseStop(object clonk, ix, iy)
{
	return CancelUse(clonk);
}

func StartUsage(object clonk)
{
	var hand;
	// which animation to use? (which hand)
	if(clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 0)
	{
		carry_bone = "pos_hand2";
		hand = "AimArmsGeneric.R";
	}
	else
	{
		carry_bone = "pos_hand1";
		hand = "AimArmsGeneric.L";
	}

	is_aiming = true;

	aim_anim = clonk->PlayAnimation(hand, CLONK_ANIM_SLOT_Arms, Anim_Const(clonk->GetAnimationLength(hand)/2), Anim_Const(1000));
	clonk->UpdateAttach();


	//Animations and effects for TeleGlove
	Sound("Objects::Electrical",nil,nil,nil,+1);
	PlayAnimation("Opening", -5, Anim_Linear(0,0,GetAnimationLength("Opening"), 10, ANIM_Hold));
	anim_spin = PlayAnimation("Spin",5, Anim_Linear(0,0,GetAnimationLength("Spin"), 40, ANIM_Loop));
	
	// Light effects
	SetLightRange(50, 10);
	SetLightColor(0xa0a0ff);
}

func EndUsage(object clonk)
{
	carry_bone = nil;
	aim_anim = nil;
	iAngle = 0;
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->UpdateAttach();
	SetLightRange();
}

// Update the glove aim angle
func UpdateGloveAngle(object clonk, int x, int y)
{
	var angle=Normalize(Angle(0,0, x,y),-180);
	angle=BoundBy(angle,-150,150);
	
	if(clonk->GetDir() == DIR_Left)
	{
		if(angle > 0) return;
	}
	else
	{
		if(angle < 0) return;
	}

	iAngle=angle;

	clonk->SetAnimationPosition(aim_anim,  Anim_Const(Abs(iAngle) * 11111/1000));
	
	// Light position at remote location
	this.LightOffset = [x, y];
	return true;
}

public func GainedTargetObject(object target)
{
	if (!GetEffect("TeleGloveWeight", target))
	{
		//Who holds the object? For killtracing
		target->SetController(Contained()->GetController());
		target->~OnTeleGloveControl(this);
		AddEffect("TeleGloveWeight", target, 1, 0, target);
		return 1;
	}
	else
		return 0;
}

public func LostTargetObject(object target)
{
	RemoveEffect("TeleGloveWeight", target);
	var effect = AddEffect("TeleGloveReleased", target, 1, 35*5);
	effect.controller = Contained()->GetController();
}


protected func CancelUse(object clonk)
{
	EndUsage(clonk);
	Sound("Objects::Electrical",nil,nil,nil,-1);
	if (is_aiming)
		PlayAnimation("Closing", -5, Anim_Linear(0,0,GetAnimationLength("Closing"), 10, ANIM_Hold));
	StopAnimation(anim_spin);
	is_aiming = false;
	if(target_object) LostTargetObject(target_object);
	target_object = nil;
	return true;
}

/*-- Production --*/

func IsInventorProduct() { return true; }
public func GetSubstituteComponent(id component) // Can be made from diamond, ruby or amethyst
{
	if (component == Diamond)
		return [Ruby, Amethyst];
}

/*-- Display --*/

public func GetCarryMode() { return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { return carry_bone; }

public func GetCarryTransform(object clonk, bool idle, bool nohand)
{
	if (nohand)
		return Trans_Mul(Trans_Rotate(45, 0, 1), Trans_Rotate(25, 0, 0, 1), Trans_Translate(4000, 0, 1000));

	//Left hand's bone is different? I don't know, but this is a work-around.
	if(carry_bone == "pos_hand1") return Trans_Rotate(180,0,1,0);
	return Trans_Rotate(-90,0,1,0);
}

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Rotate(-60,1,0,1),def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Metal = 2, Diamond = 1};