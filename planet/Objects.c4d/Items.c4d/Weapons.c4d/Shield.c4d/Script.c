/*-- Shield --*/

#include Library_MeleeWeapon

local iAngle;		// -180 .. 180
local aim_anim;
local carry_bone;
local mTrans;

local solid_mask_helper;

/* Usage callbacks */

public func ControlUseStart(object clonk, int x, int y)
{
	// may only be used while walking
	if(!clonk->HasHandAction() || !clonk->IsWalking() || !CanStrikeWithWeapon(clonk))
	{
		AddEffect("IntShieldSuspend", clonk, 1, 5, this);
	}
	else
	{
		StartUsage(clonk);
		UpdateShieldAngle(clonk, x,y);
	}
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	UpdateShieldAngle(clonk, x, y);
}

public func HoldingEnabled() { return true; }

public func ControlUseStop(object clonk)
{
	ControlUseCancel(clonk);
}

public func ControlUseCancel(object clonk)
{
	EndUsage(clonk);
	if(GetEffect("IntShieldSuspend", clonk))
		RemoveEffect("IntShieldSuspend", clonk);
}

private func StartUsage(object clonk)
{
	var hand;
	// which animation to use? (which hand)
	if(clonk->GetItemPos(this) == 0)
	{
		carry_bone = "pos_hand2";
		hand = "ShieldArms.R";
	}
	if(clonk->GetItemPos(this) == 1)
	{
		carry_bone = "pos_hand1";
		hand = "ShieldArms.L";
	}

	aim_anim = clonk->PlayAnimation(hand, 10, Anim_Const(clonk->GetAnimationLength(hand)/2), Anim_Const(1000));
	clonk->UpdateAttach();
	clonk->ReplaceAction("Stand", ["ShieldStandUp.L", "ShieldStandDown.L", 500]);
	clonk->ReplaceAction("Walk", ["ShieldWalkUp.L", "ShieldWalkDown.L", 500]);
	clonk->ReplaceAction("Run", ["ShieldWalkUp.L", "ShieldWalkDown.L", 500]);

	StartWeaponHitCheckEffect(clonk, -1, 1);
	
	if(!GetEffect("ShieldStopControl", clonk))
		AddEffect("ShieldStopControl", clonk, 2, 5, this);
}

private func EndUsage(object clonk)
{
	carry_bone = nil;
	aim_anim = nil;
	iAngle = 0;
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->UpdateAttach();

	clonk->ReplaceAction("Stand", nil);
	clonk->ReplaceAction("Walk",  nil);
	clonk->ReplaceAction("Run",   nil);
	
	AdjustSolidMaskHelper();
	if(GetEffect("ShieldStopControl", clonk))
		RemoveEffect("ShieldStopControl", clonk);

	StopWeaponHitCheckEffect(clonk);
}

// Update the shield angle
private func UpdateShieldAngle(object clonk, int x, int y)
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

	var weight = 0;
	if( Abs(iAngle) > 90) weight = 1000*( Abs(iAngle)-60 )/90;

	clonk->ReplaceAction("Stand", ["ShieldStandUp.L", "ShieldStandDown.L", weight]);
	clonk->ReplaceAction("Walk", ["ShieldWalkUp.L", "ShieldWalkDown.L", weight]);
	clonk->ReplaceAction("Run", ["ShieldWalkUp.L", "ShieldWalkDown.L", weight]);

	if(!GetEffect("IntShieldSuspend", clonk))
	{
		clonk->SetAnimationPosition(aim_anim,  Anim_Const(Abs(iAngle) * 11111/1000));
		AdjustSolidMaskHelper();
	}
}

// Adjust solid mask of shield
// if the shield is held up, it has a solid mask on which other clonks can climb onto
private func AdjustSolidMaskHelper()
{
	if(aim_anim && Contained() && Abs(iAngle) <= 20)
	{
		if(!solid_mask_helper)
		{
			solid_mask_helper=CreateObject(Shield_SolidMask, 0, 0, NO_OWNER);
			solid_mask_helper->SetAction("Attach", Contained());
		}
	}
	else
	{
		if(solid_mask_helper) return solid_mask_helper->RemoveObject();
		else return;
	}
	
	
	var angle=BoundBy(iAngle, -115, 115);
	
	solid_mask_helper->SetR(angle - 90);
	var distance=8;
	var y_adjust=-5;
	var x_adjust=1;
	if(Contained()->GetDir() == DIR_Left) x_adjust=-1;
	var x=Sin(angle, distance) + x_adjust;
	var y=-Cos(angle, distance) + y_adjust;
	solid_mask_helper->SetVertexXY(0, -x, -y);
}

/* other callbacks */

func Hit()
{
	Sound("WoodHit");
}

func OnWeaponHitCheckStop()
{
	return;
}

func HitByWeapon(by, iDamage)
{
	var object_angle = Angle(GetX(),GetY(),by->GetX(), by->GetY());
	var shield_angle = iAngle;
	// angle difference: 0..180
	var angle_diff = Abs(Normalize(shield_angle-object_angle,-180));
	if (angle_diff > 45) return 0;

	Sound(Format("ShieldMetalHit%d.ogg", Random(4)+1));
	
	// bash him hard!
	ApplyWeaponBash(by, 100, iAngle);
	
	// uber advantage in melee combat
	AddEffect("ShieldBlockWeaponCooldown", by, 1, 30, this);
	
	// shield factor
	return 100;
}

/* main shield effect */

func FxShieldStopControlStart(object target, int num, temp)
{
	target->SetPhysical("Walk", 30000, PHYS_StackTemporary);
	if(temp) return;
}

func FxShieldStopControlStop(object target, int num, iCause, temp)
{
	target->ResetPhysical("Walk");
	if(temp) return;
}

func FxShieldStopControlTimer(object target, int num)
{
	// suspend usage if not walking
	if(!target->IsWalking())
	{
		AddEffect("IntShieldSuspend", target, 1, 5, this);
		EndUsage(target);
	}
}

func FxShieldStopControlQueryCatchBlow(object target, int num, object obj)
{
	if (obj->GetOCF() & OCF_Alive) return false;
	
	// angle of shield
	var shield_angle = iAngle;
	
	// angle of object
	var object_angle;
	if(obj->GetXDir() || obj->GetYDir())
		object_angle = Angle(obj->GetXDir(), obj->GetYDir(),0,0);
	else
		object_angle = Angle(GetX(),GetY(),obj->GetX(), obj->GetY());
	
	// angle difference: 0..180
	var angle_diff = Abs(Normalize(shield_angle-object_angle,-180));

	// objects hits if the angle difference is greater than 45°
	if (angle_diff > 45) return false;
	
	// projectile bounces off
	var sxd=Sin(shield_angle, 15);
	var syd=-Cos(shield_angle, 15);
	var xd=obj->GetXDir();
	var yd=obj->GetYDir();
	obj->SetXDir(-xd/3 + sxd);
	obj->SetYDir(-yd/3 + syd);
	
	// dont collect blocked objects
	AddEffect("NoCollection", obj, 1, 30);
	
	Sound(Format("ShieldMetalHit%d.ogg", Random(4)+1));
	
	return true;
}

/* Suspend effect */

func FxIntShieldSuspendTimer(object target, int num)
{
	if(target->IsWalking() && CanStrikeWithWeapon(target))
	{
		StartUsage(target);
		return -1;
	}
}

/* Shield animation */

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarrySpecial(clonk) { return carry_bone; }
public func GetCarryTransform(clonk, sec, back)
{
	if(aim_anim && !sec) return Trans_Mul(Trans_Rotate(180,0,0,1),Trans_Rotate(90,0,0,1));
	if(aim_anim && sec) return Trans_Rotate(180,1,0,0);

	if(mTrans != nil) return mTrans;
	if(!sec)
	{
		if(back) return Trans_Mul(Trans_Rotate(-90, 0, 0, 1),Trans_Translate(0,0,-400));
		return nil;
	}
	if(back) return Trans_Mul(Trans_Mul(Trans_Rotate(90, 0, 0, 1),Trans_Rotate(180, 0, 1)),Trans_Translate(0,0,-400));
	return Trans_Rotate(180,1,0,0);
}

/* Definition */

func Definition(def) {
	def.Name = "$Name$";
	def.Description = "$Description$";
	def.Collectible = 1;
	
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(1000,-500),Trans_Rotate(20,1,1,-1),Trans_Scale(1200)),def);
}
