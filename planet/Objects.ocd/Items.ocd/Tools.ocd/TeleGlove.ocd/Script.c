/*--
	TeleGlove
	Author: Ringwaul

	Move objects remotely.
--*/

local reach;
local radius; //actual effect radius to grab objects
local radiusparticle; //radius of particles around object

local aiming;
local anim_spin;

local iAngle;
local aim_anim;
local carry_bone;

local target_object;

protected func Initialize()
{
	reach = 150;
	radius = 60;
	radiusparticle = radius / 4;
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarrySpecial(clonk) { return carry_bone; }
public func GetCarryBone()	{	return "main";	}
public func GetCarryTransform()
{
	//Left hand's bone is different? I don't know, but this is a work-around.
	if(carry_bone == "pos_hand1") return Trans_Rotate(180,0,0,1);
	return Trans_Rotate(-90,0,0,1);
}

protected func HoldingEnabled() { return true; }

protected func ControlUseStart(object clonk, ix, iy)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction() || (!clonk->IsWalking() && !clonk->IsJumping()))
	return true;
	else
	{
		StartUsage(clonk);
		UpdateGloveAngle(clonk, ix, iy);
	}

	return 1;
}

private func StartUsage(object clonk)
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

	aiming = 1;

	aim_anim = clonk->PlayAnimation(hand, 10, Anim_Const(clonk->GetAnimationLength(hand)/2), Anim_Const(1000));
	clonk->UpdateAttach();


	//Animations and effects for TeleGlove
	Sound("Electrical",nil,nil,nil,+1);
	PlayAnimation("Opening", -5, Anim_Linear(0,0,GetAnimationLength("Opening"), 10, ANIM_Hold), Anim_Const(1000));
	anim_spin = PlayAnimation("Spin",5, Anim_Linear(0,0,GetAnimationLength("Spin"), 40, ANIM_Loop), Anim_Const(1000));
}

private func EndUsage(object clonk)
{
	carry_bone = nil;
	aim_anim = nil;
	iAngle = 0;
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->UpdateAttach();
}

// Update the glove aim angle
private func UpdateGloveAngle(object clonk, int x, int y)
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

//	var weight = 0;
//	if( Abs(iAngle) > 90) weight = 1000*( Abs(iAngle)-60 )/90;

	clonk->SetAnimationPosition(aim_anim,  Anim_Const(Abs(iAngle) * 11111/1000));
}

public func ControlUseHolding(object clonk, ix, iy)
{
	if(!clonk->HasHandAction() || !aiming || (!clonk->IsWalking() && !clonk->IsJumping()))
	{
		CancelUse(clonk);
		return 1;
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
	
	if(distp < reach)
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
		Distance(target_object->GetX(), target_object->GetY(), clonk->GetX(), clonk->GetY()) > reach)
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
					Find_Distance(reach - 15)),
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

public func GainedTargetObject(object target)
{
	if(!GetEffect("TeleGloveWeight", target))
	{
		//Who holds the object? For killtracing
		target->SetController(Contained()->GetController());
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

global func FxTeleGloveReleasedStart(object target, effect)
{
	return;
}

global func FxTeleGloveWeightStart(object target, int num)
{
	target->SetMass(target->GetMass()/2);
}

global func FxTeleGloveWeightStop(object target, int num, int reason, bool temp)
{
	target->SetMass(target->GetDefCoreVal("Mass", "DefCore"));
}

protected func ControlUseStop(object clonk, ix, iy)
{
	CancelUse(clonk);
	return 1;
}

protected func ControlUseCancel(object clonk, int ix, int iy)
{
	CancelUse(clonk);
}

protected func CancelUse(object clonk)
{
	EndUsage(clonk);
	Sound("Electrical",nil,nil,nil,-1);
	if(aiming = 1) PlayAnimation("Closing", -5, Anim_Linear(0,0,GetAnimationLength("Closing"), 10, ANIM_Hold), Anim_Const(1000));
	StopAnimation(anim_spin);
	aiming = 0;
	if(target_object) LostTargetObject(target_object);
	target_object = nil;
	return 1;
}

func Hit()
{
	Sound("GeneralHit?");
}

func IsInventorProduct() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Rotate(-60,1,0,1),def);
}

local Name = "$Name$";
local UsageHelp = "$UsageHelp$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
