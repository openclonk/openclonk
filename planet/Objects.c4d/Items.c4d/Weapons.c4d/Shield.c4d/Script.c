/*-- Shield --*/

#include Library_MeleeWeapon

private func Hit()
{
	Sound("WoodHit"); //TODO Some metal sond
}

local iAngle;
local aim_anim;
// TODO:
// The clonk must be able to duck behind the shield
// when he ducks, he is at least invulnerable against spears and arrows,
// other bonuses perhaps too

local solid_mask_helper;

public func ControlUseStart(object clonk, int x, int y)
{
	if(!CanStrikeWithWeapon(clonk)) return true;
	
	// figure out the kind of attack to use
	var length=0;
	if(clonk->IsWalking())
	{
		length=10;
	} else
	if(clonk->IsJumping())
	{
		if(clonk->GetYDir() < 0) length=10;
		else length=GetJumpLength(clonk);
	}
	else return true;
	
	var hand;
	if(clonk->GetItemPos(this) == 0) hand = "ShieldArmsR";
	if(clonk->GetItemPos(this) == 1) hand = "ShieldArmsL";
	
	aim_anim = clonk->PlayAnimation(hand, 10, Anim_Const(clonk->GetAnimationLength(hand)/2), Anim_Const(1000));
	clonk->UpdateAttach();

	StartWeaponHitCheckEffect(clonk, -1, 1);
	
	if(!GetEffect("ShieldStopControl", clonk))
		AddEffect("ShieldStopControl", clonk, 2, 50, this);
	
	iAngle=Angle(0,0, x,y);
	AdjustSolidMaskHelper();
	return true;
}

func AdjustSolidMaskHelper()
{
	if(aim_anim && Contained() && (Inside(iAngle, 0, 20) || Inside(iAngle, 340, 360)))
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
	
	
	var angle=BoundBy(iAngle, 0, 115);
	if(iAngle > 180) angle=BoundBy(iAngle, 180+65, 360);
	
	solid_mask_helper->SetR(angle - 90);
	var distance=8;
	var y_adjust=-5;
	var x_adjust=1;
	if(Contained()->GetDir() == DIR_Left) x_adjust=-1;
	var x=Sin(angle, distance) + x_adjust;
	var y=-Cos(angle, distance) + y_adjust;
	solid_mask_helper->SetVertexXY(0, -x, -y);
}

func ControlUseHolding(clonk, x, y)
{
	var angle=Angle(0,0, x,y);
	clonk->SetAnimationPosition(aim_anim,  Anim_Const(Abs(Normalize(angle,-180)) * 11111/1000));
	if(clonk->GetDir() == DIR_Left)
	{
		if(!Inside(angle, 180, 360)) return;
	}
	else if(!Inside(angle, 0, 180)) return;
	iAngle=angle;
	AdjustSolidMaskHelper();
}

func ControlUseStop(object clonk, int x, int y)
{
	clonk->StopAnimation(aim_anim);
	aim_anim = nil;
	clonk->UpdateAttach();
	StopWeaponHitCheckEffect(clonk);
	AdjustSolidMaskHelper();
	
	if(GetEffect("ShieldStopControl", clonk))
		RemoveEffect("ShieldStopControl", clonk);
}

func ControlUseCancel(object clonk, int ix, int iy)
{
	if(aim_anim)
	{
		clonk->StopAnimation(aim_anim);
		clonk->UpdateAttach();
		aim_anim = nil;
	}
	AdjustSolidMaskHelper();
}

func Departure()
{
	AdjustSolidMaskHelper();
}

func OnWeaponHitCheckStop()
{
	return;
}

func CheckStrike(iTime)
{
	return;
	var found=false;
	var found_alive=false;
	var push_livings=false;
	if(Contained()->GetContact(-1) & CNAT_Bottom)
		if(Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5) push_livings=true;
	for(var obj in FindObjects(Find_Distance(20), Find_Or(Find_Category(C4D_Object), Find_OCF(OCF_Alive)), Find_NoContainer(), Find_Exclude(Contained()), Find_Layer(GetObjectLayer())))
	{

		if(obj->GetX() > GetX())
		{
			if(obj->GetXDir() > 0) continue;
		}
		else if(obj->GetXDir() < 0) continue;

		var a=Angle(GetX(), GetY(), obj->GetX(), obj->GetY());
		if(!Inside(a, iAngle-45, iAngle+45))
			if(!Inside(a+360, iAngle-45, iAngle+45)) continue;
		
		if(obj->GetOCF() & OCF_Alive)
		{
			if(push_livings)
			{
				found_alive=true;
				obj->SetXDir((obj->GetXDir() + Contained()->GetXDir() * 3) / 4);
				obj->SetYDir((obj->GetYDir() + Contained()->GetYDir() * 3) / 4);
			}
			continue;
		}
		
		var speed=Sqrt((obj->GetXDir(100)**2) + (obj->GetYDir(100)**2));
		if(Abs(iAngle-a) > Abs(iAngle-(a+360)))
			a=((a+360) + iAngle)/2;
		if(!speed) continue;
		//if(speed > 5000) continue;
		speed=Max(150, speed/2);
		obj->SetXDir(+Sin(a, speed), 100);
		obj->SetYDir(-Cos(a, speed), 100);
		found=true;
	}
	
	if(found_alive)
		DoWeaponSlow(Contained(), 5000);
	
	if(found)
		this->Sound("ShieldMetalHit*", false);
	/*for(var cnt=0;cnt<10;++cnt)
	{
		CreateParticle("Spark", Sin(iAngle, 4*cnt), -Cos(iAngle, 4*cnt), 0, 0, 50, RGB(255,255,255));
	}*/
}

func HitByWeapon(pFrom, iDamage)
{
	if(pFrom->GetX() > GetX())
	{
		if(!Inside(iAngle, 0, 180)) return 0;
	}
	else if(!Inside(iAngle, 180, 360)) return 0;
		
	// bash him hard!
	ApplyWeaponBash(pFrom, 500*50, iAngle);
	
	// uber advantage in melee combat
	AddEffect("ShieldBlockWeaponCooldown", pFrom, 1, 20, this);
	
	// shield factor
	return 100;
}

func FxShieldStopControlStart(pTarget, iEffectNumber, iTemp)
{
	pTarget->SetPhysical("Walk", 0, PHYS_StackTemporary);
	if(iTemp) return;
}

func FxShieldStopControlStop(pTarget, iEffectNumber, iCause, iTemp)
{
	pTarget->ResetPhysical("Walk");
	if(iTemp) return;
}

func FxShieldStopControlTimer(pTarget, iEffectNumber)
{
	return 1;
}

func AngleInside(angle1, angle2, allowance)
{
	angle1=Normalize(angle1, 0);
	angle2=Normalize(angle2, 0);
	if(Inside(angle1, angle2-allowance, angle2+allowance)) return true;
	if(Abs(angle1 - angle2) > Abs(angle1 - (360+angle2))) if(Inside(angle1, 360+angle2-allowance, 360+angle2+allowance)) return true;
	if(Abs(angle2 - angle1) > Abs(angle2 - (360+angle1))) if(Inside(angle2, 360+angle1-allowance, 360+angle1+allowance)) return true;
	
	return false;
}

func FxShieldStopControlControlJump(target, effect_number)
{
	return true;
}

func FxShieldStopControlQueryCatchBlow(target, effect_number, object obj)
{
	/*var x=1;
	if(Contained()->GetDir() == DIR_Left) x=-1;
	var direction=BoundBy(obj->GetXDir(), -1, 1);
	if(Abs(obj->GetXDir()*/
	var angle=BoundBy(iAngle, 0, 115);
	if(iAngle > 180) angle=BoundBy(iAngle, 180+65, 360);
	var posX=Sin(angle, 12);
	var posY=-Cos(angle, 12);
	var object_angle=Angle(0, 0, obj->GetXDir(), obj->GetYDir());
	if(Distance(GetX()+posX, GetY()+posY, obj->GetX(), obj->GetY()) > 7) return false;
	target->Message(Format("%d", Distance(GetX()+posX, GetY()+posY, obj->GetX(), obj->GetY())));
	if(AngleInside(angle, object_angle, 45)) return false;
	
	/*var xd=obj->GetXDir();
	var yd=obj->GetYDir();
	 
	var sxd=Sin(angle, 10);
	var syd=-Cos(angle, 10);
	var b=(sxd + syd);
	sxd/=b;
	syd/=b;
	 
	// considered to be normalized
	var dot=xd*sxd + yd*syd;
	var nx=sxd*dot;
	var ny=syd*dot;
	b=(nx+ny);
	nx/=b;
	ny/=b;
	 
	var s=Sqrt(obj->GetXDir()+obj->GetYDir());
	obj->SetXDir(nx*s);
	obj->SetYDir(ny*s);*/
	
	// boring stuff for now
	var sxd=Sin(angle, 15);
	var syd=-Cos(angle, 15);
	var xd=obj->GetXDir();
	var yd=obj->GetYDir();
	obj->SetXDir(-xd/3 + sxd);
	obj->SetYDir(-yd/3 + syd);
	// dont collect blocked objects
	AddEffect("NoCollection", obj, 1, 30);
	return true;
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_HandBack; }
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

local mTrans;

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(1000,-500),Trans_Rotate(20,1,1,-1),Trans_Scale(1200)),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
