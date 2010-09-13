/*--
	Cannon
	Author: Ringwaul
	
	Fires objects great distances.
--*/


#include Library_HasExtraSlot

local aim_anim;
local olddir;

static const Fire_Velocity = 175;

public func IsToolProduct() { return 1; }

protected func Initialize()
{
	SetAction("Roll");
	olddir = GetDir();
	aim_anim =  PlayAnimation("Aim", 1,  Anim_Const(0),Anim_Const(1000));
}

//some left-overs from Lorry script. Not sure of it's purpose...
protected func ContactLeft()
{
	if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
	if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}

//Only one object fits in barrel
private func MaxContentsCount() { return 1; }

/*-- Control --*/

public func ControlUseStart(object clonk, int ix, int iy)
{
	var result = CheckForKeg(clonk);
	if (!result)
	{
		clonk->CancelUse();
		return true;
	}
		
	if (!clonk->GetItem(0))
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		clonk->CancelUse();
		return true;
	}

	if(clonk->GetOwner() != GetOwner()) SetOwner(clonk->GetOwner());

	//Animation
	var r = ConvertAngle(Angle(0,0,ix,iy));
	SetAnimationPosition(aim_anim, Anim_Const(AnimAngle(r)*3954444/100000)); //conversion. Apparently 90 blender frames is 3559 ogre frames.
	return true;
}

public func ControlUseAltStart(object clonk, int ix, int iy)
{
	var result = CheckForKeg(clonk);
	if (!result)
	{
		clonk->CancelUse();
		return true;
	}
		
	if (!clonk->GetItem(1))
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		clonk->CancelUse();
		return true;
	}
			
	if(clonk->GetOwner() != GetOwner()) SetOwner(clonk->GetOwner());

	//Animation
	var r = ConvertAngle(Angle(0,0,ix,iy));
	SetAnimationPosition(aim_anim, Anim_Const(AnimAngle(r)*3954444/100000)); //conversion. Apparently 90 blender frames is 3559 ogre frames.
	return true;
}

private func CheckForKeg(object clonk)
{
	// Check for powderkeg, is the only content.
	if (!Contents(0))
	{
		// Look for a keg inside the shooter.
		var keg = FindObject(Find_Container(clonk), Find_ID(PowderKeg));
		if (keg) // If there is a keg, put into cannon.
		{
			keg->Exit();
			keg->Enter(this);
			Sound("WoodHit*");
		}
		else // No keg, stop using cannon.
		{
			PlayerMessage(clonk->GetOwner(),"$TxtNeedsGunp$");
			return false;
		}
	}
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseAltHolding(object clonk, int ix, int iy)
{
	return ControlUseHolding(clonk, ix, iy);
}

public func ControlUseHolding(object clonk, int ix, int iy)
{
	if (!clonk)
	{
		clonk->CancelUse();
		return true;
	}
	var r = ConvertAngle(Angle(0,0,ix,iy));

	var iColor = RGB(255,255,255);
	if (!Contents(0) || GetEffect("Cooldown",this))
		iColor = RGB(255,0,0);
	AddTrajectory(this, GetX() + 5, GetY() + 2, Cos(r-90, Fire_Velocity), Sin(r-90, Fire_Velocity), iColor, 20);

	SetAnimationPosition(aim_anim, Anim_Const(AnimAngle(r)*3954444/100000));
	return true;
}

private func AnimAngle(int angle)
{
	//Conversion for animation
	var r = Normalize(angle,-180);
	r = r - GetR();
	r = Abs(r);
	r = 180-r-90;
	r = BoundBy(r,0,90);
	return r;
}

private func ConvertAngle(int angle)
{
	//More confusing conversion ;)
	var r = angle;
	if(r > 90 + GetR() && GetDir() == 1) r = 90 + GetR();
	if(r < 270 + GetR() && r != 0 && GetDir() == 0) r = 270 + GetR();
	if(r == 360 + GetR() && GetDir() == 0) r = 0 + GetR();
	//second half. Makes it relative to current direction.
	if(r - GetR() < 0 && GetDir() == 1) SetDir();
	if(r - GetR() > 359 && GetDir() == 0) SetDir(1);
	Message(Format("%d",Normalize(r - GetR(),-90)),this);
	return r;
}

public func ControlUseStop(object clonk, int ix, int iy)
{
	RemoveTrajectory(this);
	
	var result = CheckForKeg(clonk);
	if (!result)
		return true;

	var projectile = clonk->GetItem(0);
	if (!projectile) // Needs a projectile
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		return true;
	}

	//Can't fire if cannon is cooling down
	if(GetEffect("Cooldown",this))	return true;
	
	if (projectile)
	{
		DoFire(projectile, clonk, Angle(0,0,ix,iy));
		var powder = Contents(0)->Contents(0);
		if(powder)
		{
			//If there is a powder keg, take powder from it
			powder->RemoveObject();
			DoFire(projectile, clonk, Angle(0,0,ix,iy));
			AddEffect("Cooldown",this,1,1,this);
			if(Contents(0)->ContentsCount() == 0)
			{
				Contents(0)->RemoveObject();
				CreateObject(Barrel);
			}
		}
	}
	return true;
}

public func ControlUseAltStop(object clonk, int ix, int iy)
{
	RemoveTrajectory(this);
	
	var result = CheckForKeg(clonk);
	if (!result)
		return true;

	var projectile = clonk->GetItem(1);
	if (!projectile) // Needs a projectile
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		return true;
	}

	//Can't fire if cannon is cooling down
	if(GetEffect("Cooldown",this))	return true;
	
	if (projectile)
	{
		DoFire(projectile, clonk, Angle(0,0,ix,iy));
		var powder = Contents(0)->Contents(0);
		if(powder)
		{
			//If there is a powder keg, take powder from it
			powder->RemoveObject();
			DoFire(projectile, clonk, Angle(0,0,ix,iy));
			AddEffect("Cooldown",this,1,1,this);
			if(Contents(0)->ContentsCount() == 0)
			{
				Contents(0)->RemoveObject();
				CreateObject(Barrel);
			}
		}
	}
	return true;
}

public func ControlUseCancel()
{
	RemoveTrajectory(this);
	return true;
}

public func ControlUseAltCancel()
{
	return ControlUseCancel();
}

public func FxCooldownTimer(object target, int num, int timer)
{
	if(timer > 72) return -1;
}

//Activate turn animations
func ControlLeft(object clonk)
{
	SetDir();
}

func ControlRight(object clonk)
{
	SetDir(1);
}

protected func DoFire(object iammo, object clonk, int angle)
{
	if(iammo == nil) iammo = FindContents(Rock); //debug only!
	iammo->Exit();

	if(iammo->GetID() == Dynamite)
	{
		iammo->Fuse();
	}

	//Don't excede possible trajectory
	var r = Normalize(angle,-180);
	if(r > 90 + GetR()) r = 90 + GetR();
	if(r < -90 + GetR()) r = -90 + GetR();

	//Send ammo flying
	iammo->SetR(r);
	iammo->SetRDir(-4 + Random(9));
	iammo->LaunchProjectile(r, 17, Fire_Velocity);

	//Particles
	var dist = 25;
	var px = Cos(r - 90,dist);
	var py = Sin(r - 90,dist) - 4;
	CreateParticle("Flash",px,py,0,0,420,RGB(255,255,255));
	for(var i=0; i<15; ++i) //liberated from musket script... I'm horrible at particles :p
	{
		var speed = RandomX(0,10);
		CreateParticle("ExploSmoke",px,py,+Sin(r,speed)+RandomX(-2,2),-Cos(r,speed)+RandomX(-2,2),RandomX(100,400),RGBa(255,255,255,75));
	}
	CreateParticle("MuzzleFlash",px,py,px,py+4,700,RGB(255,255,255)); //muzzle flash uses speed as Rotation... so I negate the -4

	//sound
	Sound("Blast3");
}

func Timer()
{
	//Turning
	if(olddir != GetDir())
	{
		if(olddir == 0)
		{
			PlayAnimation("TurnRight", 5, Anim_Linear(0, 0, GetAnimationLength("TurnRight"), 36, ANIM_Remove), Anim_Const(1000));
		}
		
		if(olddir == 1)
		{
			PlayAnimation("TurnLeft", 5, Anim_Linear(0, 0, GetAnimationLength("TurnLeft"), 36, ANIM_Remove), Anim_Const(1000));
		}
	}
	olddir = GetDir();
}

local ActMap = {
Roll = {
	Prototype = Action,
	Name = "Roll",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 1,
	Length = 50,
	Delay = 2,
	X = 0,
	Y = 0,
	Wdt = 22,
	Hgt = 16,
	NextAction = "Roll",
},
};
func Definitio(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Translate(6000,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(35,0,1,0),Trans_Scale(1350)),def);
}

local Name = "$Name$";
local Description = "$Description$";
