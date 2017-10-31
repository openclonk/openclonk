/*--
	Javelin
	A simple but dangerous throwing weapon.
	
	@author: Ringwaul
--*/

#include Library_Stackable
#include Library_RangedWeapon
#include Library_Flammable

// Default timing values for animation set
// (Adjusted for speeed multiplier and stored in animation set by Library_RangedWeapon)
local DefaultShootTime = 16;
local DefaultShootTime2 = 8;

local animation_set;
local aiming;

/*-- Engine Callbacks --*/

func Initialize(...)
{
	animation_set = {
		AimMode         = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim    = "SpearAimArms",
		AnimationShoot  = "SpearThrowArms",
		AnimationShoot2 = "SpearThrow2Arms",
		AnimationShoot3 = "SpearThrow3Arms",
		WalkBack        =  0,
	};
	return _inherited(...);
}

func Hit()
{
	if(GetEffect("Flight",this))
	{
		Stick();
		Sound("Objects::Weapons::Javelin::HitGround");
	}
	else
		Sound("Hits::Materials::Wood::WoodHit?");
}

func Entrance()
{
	// reset sticky-vertex
	SetVertex(2,VTX_Y,0,1);
}

/*-- Callbacks --*/

public func MaxStackCount() { return 3; }

public func GetAnimationSet() { return animation_set; }

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	clonk->StartShoot(this);
	return true;
}

// Called in the half of the shoot animation (when ShootTime2 is over)
public func DuringShoot(object clonk, int angle)
{
	DoThrow(clonk, angle);
}

//slightly modified HitObject() from arrow
public func HitObject(object obj)
{
	var relx = GetXDir() - obj->GetXDir();
	var rely = GetYDir() - obj->GetYDir();
	var speed = Sqrt(relx*relx+rely*rely);
	
	var dmg = JavelinStrength() * speed * 1000 / 60;

	if (WeaponCanHit(obj))
	{
		if (obj->GetAlive())
			Sound("Hits::ProjectileHitLiving?");
		else
			Sound("Objects::Weapons::Javelin::HitGround");
		
		obj->~OnProjectileHit(this);
		WeaponDamage(obj, dmg, FX_Call_EngObjHit, true);
		WeaponTumble(obj, this->TumbleStrength());
		if (!this) return;
	}
	
	Stick();
}

/*-- Usage --*/

public func HoldingEnabled() { return true; }

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction(false, false, true);
}

public func ControlUseStart(object clonk, int x, int y)
{
	aiming = 1;

	clonk->StartAim(this);

	ControlUseHolding(clonk, x, y);
	
	Sound("Objects::Weapons::Javelin::Draw");
	return 1;
}

public func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);

	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func ControlUseStop(object clonk, ix, iy)
{
	if(aiming)
		clonk->StopAim();
	return true;
}

// Note that the javelin damage also takes the speed into account. A direct eye-to-eye hit will do roughly this damage.
public func JavelinStrength() { return 16; }
public func TumbleStrength() { return 100; }

public func Reset(clonk)
{
	aiming = 0;
}

public func DoThrow(object clonk, int angle)
{
	var javelin = TakeObject();
	
	var div = 60; // 40% is converted to the direction of the throwing angle.
	var xdir = clonk->GetXDir(1000);
	var ydir = clonk->GetYDir(1000);
	var speed = clonk.ThrowSpeed * shooting_strength + (100 - div) * Sqrt(xdir**2 + ydir**2) / 100;
	var jav_x = div * xdir / 100 + Sin(angle, speed);
	var jav_y = div * ydir / 100 - Cos(angle, speed);
	
	SetController(clonk->GetController());
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->AddEffect("HitCheck",javelin,1,1,nil,nil,clonk);	
		
	javelin->SetXDir(jav_x, 1000);
	javelin->SetYDir(jav_y, 1000);
	javelin->SetPosition(clonk->GetX(), clonk->GetY() - 6);
		
	Sound("Objects::Weapons::Javelin::Throw?");
	
	aiming = -1;
	clonk->UpdateAttach();
	
	return javelin;
}

func Stick()
{
	if(GetEffect("Flight",this))
	{
		SetXDir(0);
		SetYDir(0);
		SetRDir(0);

		RemoveEffect("Flight",this);
		RemoveEffect("HitCheck",this);
		
		var x=Sin(GetR(),+16);
		var y=Cos(GetR(),-16);
		var mat = GetMaterial(x,y);
		if(mat != -1)
		{
			//if(GetMaterialVal("DigFree","Material",mat))
			//{
			// stick in landscape
			SetVertex(2,VTX_Y,-18,1);
			//}
		}
		return;
	}
}

func FxFlightStart(object target, effect fx, int temp)
{
	if (temp)
		return FX_OK;
	target.Collectible = false;
	target->SetR(Angle(0,0,target->GetXDir(),target->GetYDir()));
	target->SetVertex(1, VTX_Y, 0, 1);
	return FX_OK;
}

func FxFlightTimer(object target, effect fx, int time)
{
	//Using Newton's arrow rotation. This would be much easier if we had arctan :(
	var oldx = fx.x;
	var oldy = fx.y;
	var newx = GetX();
	var newy = GetY();

	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	target->SetRDir(anglediff/2);
	fx.x = newx;
	fx.y = newy;
	target->SetR(Angle(0,0,target->GetXDir(),target->GetYDir()));
	
	if (time == 10)
		target->SetVertex(1, VTX_Y, 13, 1);
	return FX_OK;
}

func FxFlightStop(object target, effect fx, int reason, bool temp)
{
	if (temp)
		return FX_OK;	
	target.Collectible = true;
	target->SetVertex(1, VTX_Y, 13, 1);
	return FX_OK;
}

/*-- Production --*/

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle, bool nohand)
{
	if (idle || nohand)
		return CARRY_Back;

	if (aiming > 0)
		return CARRY_Hand;

	return CARRY_Spear;
}

public func GetCarryBone() { return "Javelin"; }

public func GetCarrySpecial(clonk)
{
	if(aiming > 0) return "pos_hand2";
}

public func GetCarryTransform()
{
	if(aiming == 1) return Trans_Rotate(180, 0, 0, 1);
}

func Definition(def, ...) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(40,0,0,1),Trans_Rotate(-10,1,0,0)),def);
	return _inherited(def, ...);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local ForceFreeHands = true;
local Components = {Wood = 2, Metal = 1};
local BlastIncinerate = 30;
local MaterialIncinerate = true;
// Multiplication factor to clonk.ThrowSpeed
local shooting_strength = 21;