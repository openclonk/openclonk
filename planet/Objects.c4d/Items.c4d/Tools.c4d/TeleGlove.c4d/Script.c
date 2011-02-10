/*--
	TeleGlove
	Author: Ringwaul

	Move objects remotely.
--*/

local reach;
local aiming;
local anim_spin;

local iAngle;
local aim_anim;
local carry_bone;

protected func Initialize()
{
	reach = 150;
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
	if(clonk->GetItemPos(this) == 0)
	{
		carry_bone = "pos_hand2";
		hand = "AimArmsGeneric.R";
	}
	if(clonk->GetItemPos(this) == 1)
	{
		carry_bone = "pos_hand1";
		hand = "AimArmsGeneric.L";
	}

	aiming = 1;

	aim_anim = clonk->PlayAnimation(hand, 10, Anim_Const(clonk->GetAnimationLength(hand)/2), Anim_Const(1000));
	clonk->UpdateAttach();


	//Animations and effects for TeleGlove
	Sound("Electrical.ogg",nil,nil,nil,+1);
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
	var xs = Sin(Random(359),30);
	var ys = -Cos(Random(359),30);
	var anglep = Angle(0,0,ix,iy);
	var distp = Distance(0,0,ix,iy);

	if(distp < reach)
	{
		//Particles moving towards object
		for(var i; i < 2; i++)
		{
			CreateParticle("Spark1", ix + xs, iy + ys, -xs/15, -ys/15, RandomX(30,90), RGB(185,250,250));
			CreateParticle("Spark2", ix + xs, iy + ys, -xs/15, -ys/15, RandomX(30,90), RGB(185,250,250));
		}
		
		var wp = 1;
		if(Random(2)) wp = -1;
		var xp = anglep - 90 + (Angle(0,0,distp,30 * wp));
		var yp = anglep - 90 + (Angle(0,0,distp,30 * wp));

		CreateParticle("Spark1", Sin(xp, Random(distp)), -Cos(yp, Random(distp)), Sin(xp, 10), -Cos(yp, 10), RandomX(30,90), RGB(185,250,250));
	}

	var target = FindObject(Find_Exclude(this),
				Find_NoContainer(),
				Find_Category(C4D_Object),
				Find_And(Find_Distance(60, ix, iy),
				Find_Distance(reach - 15)),
				Sort_Distance(ix,iy));
	if(target && !target->Stuck())
	{
		var angle = Angle(target->GetX(), target->GetY(), clonk->GetX() + ix, clonk->GetY() + iy);
		var dist = Distance(target->GetX(), target->GetY(), clonk->GetX() + ix, clonk->GetY() + iy);
		var speed = dist;

		//Set speed
		target->SetSpeed(Sin(angle, speed), -Cos(angle, speed));

		//Particles emitting from object
		target->CreateParticle("Spark1", 0, 0, xs/10, ys/10, RandomX(20,40), RGB(185,250,250));
	}
	return 1;
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
	Log("CancelUse");
	Sound("Electrical.ogg",nil,nil,nil,-1);
	PlayAnimation("Closing", -5, Anim_Linear(0,0,GetAnimationLength("Closing"), 10, ANIM_Hold), Anim_Const(1000));
	StopAnimation(anim_spin);
	aiming = 0;
	return 1;
}

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Rotate(-60,1,0,1),def);
}
local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
