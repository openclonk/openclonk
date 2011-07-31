/*--
	Javelin
	Author: Ringwaul
	
	A simple but dangerous throwing weapon.
--*/

#include Library_Stackable

public func MaxStackCount() { return 3; }

local animation_set;

func Initialize()
{
	animation_set = {
		AimMode         = AIM_Position, // The aiming animation is done by adjusting the animation position to fit the angle
		AnimationAim    = "SpearAimArms",
		AnimationShoot  = "SpearThrowArms",
		AnimationShoot2 = "SpearThrow2Arms",
		AnimationShoot3 = "SpearThrow3Arms",
		ShootTime       = 16,
		ShootTime2      =  8,
		WalkBack        =  0,
	};
}

public func GetAnimationSet() { return animation_set; }

local fAiming;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_HandBack; }
public func GetCarryBone() { return "Javelin"; }
public func GetCarrySpecial(clonk) { if(fAiming > 0) return "pos_hand2"; }
public func GetCarryTransform() { if(fAiming == 1) return Trans_Rotate(180, 1, 0, 0); }

public func ControlUseStart(object clonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
		return true;

	fAiming = true;

	clonk->StartAim(this);

	ControlUseHolding(clonk, x, y);
	
	Sound("DrawJavelin.ogg");
	return 1;
}

public func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, ix, iy)
{
	var angle = Angle(0,0,ix,iy);
	angle = Normalize(angle,-180);

	clonk->SetAimPosition(angle);

	return true;
}

protected func ControlUseStop(object clonk, ix, iy)
{
	if(fAiming)
		clonk->StopAim();
	return true;
}

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	clonk->StartShoot(this);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	clonk->CancelAiming(this);
	return true;
}

public func Reset(clonk)
{
	fAiming = 0;
}

// Called in the half of the shoot animation (when ShootTime2 is over)
public func DuringShoot(object clonk, int angle)
{
	DoThrow(clonk, angle);
}

public func DoThrow(object clonk, int angle)
{
	var javelin=TakeObject();
	
	var div = 60; // 40% is converted to the direction of the throwing angle.
	var xdir = clonk->GetXDir(1000);
	var ydir = clonk->GetYDir(1000);
	var speed = clonk.ThrowSpeed * 21 + (100 - div) * Sqrt(xdir**2 + ydir**2) / 100;
	var jav_x = div * xdir / 100 + Sin(angle, speed);
	var jav_y = div * ydir / 100 - Cos(angle, speed);
		
	javelin->SetXDir(jav_x, 1000);
	javelin->SetYDir(jav_y, 1000);
	javelin->SetPosition(javelin->GetX(),javelin->GetY()+6);
	
	SetController(clonk->GetController());
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->AddEffect("HitCheck",javelin,1,1,nil,nil,clonk);
	
	Sound("ThrowJavelin*.ogg");
	
	fAiming = -1;
	clonk->UpdateAttach();
}

protected func JavelinStrength() { return 14; }

//slightly modified HitObject() from arrow
public func HitObject(object obj)
{
	var relx = GetXDir() - obj->GetXDir();
	var rely = GetYDir() - obj->GetYDir();
	var speed = Sqrt(relx*relx+rely*rely);

	var dmg = JavelinStrength()*speed/100;
	ProjectileHit(obj,dmg,ProjectileHit_tumble);
	
	Stick();
}

// called by successful hit of object after from ProjectileHit(...)
public func OnStrike(object obj)
{
	if(obj->GetAlive())
		Sound("ProjectileHitLiving*.ogg");
	else
		Sound("JavelinHitGround.ogg");
}

protected func Hit()
{
	if(GetEffect("Flight",this))
	{
		Stick();
		Sound("JavelinHitGround.ogg");
	}
	else
		Sound("WoodHit");
}

protected func Stick()
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

func Entrance()
{
	// reset sticky-vertex
	SetVertex(2,VTX_Y,0,1);
}

protected func FxFlightStart(object pTarget, effect)
{
	pTarget->SetProperty("Collectible",0);
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightTimer(object pTarget, effect, int iEffectTime)
{
	//Using Newton's arrow rotation. This would be much easier if we had tan^-1 :(
	var oldx = effect.x;
	var oldy = effect.y;
	var newx = GetX();
	var newy = GetY();

	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	pTarget->SetRDir(anglediff/2);
	effect.x = newx;
	effect.y = newy;
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightStop(object pTarget, effect)
{
	pTarget->SetProperty("Collectible", 1);
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(40,0,0,1),Trans_Rotate(-10,1,0,0)),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
