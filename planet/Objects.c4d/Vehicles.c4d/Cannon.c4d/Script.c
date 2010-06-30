/*-- 
	Cannon
	Author: Ringwaul
	
	Fires objects great distances.
--*/

#include Library_ItemContainer

private func MenuOnControlUseAlt() { return true; }

local aim_anim;
local olddir;
local grabber;

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
private func MaxContentsCount() { return 6; }

/* Control */

public func ControlUseStart(object clonk, int ix, int iy)
{
	if(clonk->GetOwner() != GetOwner()) SetOwner(clonk->GetOwner());
	AddEffect("TrajAngle", this, 50, 1, this);
	return 1;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int ix, int iy)
{
	var r = ConvertAngle(Angle(0,0,ix,iy));

	if(r - GetR() < 0 && GetDir() == 1) SetDir();
	if(r - GetR() > 359 && GetDir() == 0) SetDir(1);
	Message(Format("%d",Normalize(r - GetR(),-90)),this);

	var iColor = RGB(255,255,255);
	if(!Contents(0) || GetEffect("Cooldown",this)) iColor = RGB(255,0,0);
	AddTrajectory(this, GetX() + 5, GetY() + 2, Cos(r-90, Fire_Velocity), Sin(r-90, Fire_Velocity), iColor, 20);

	SetAnimationPosition(aim_anim, Anim_Const(AnimAngle(r)*3954444/100000)); //conversion. Apparently 90 blender frames is 3559 ogre frames.
	return 1;
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
	return r;
}

public func ControlUseStop(object clonk, int ix, int iy)
{
	RemoveTrajectory(this);
	var gunp = FindObject(Find_Container(this), Find_ID(Blackpowder));
	if (!gunp) // Needs gunpowder
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsGunp$");
		return true;
	}
	var projectile = FindObject(Find_Container(this), Find_Not(Find_ID(Blackpowder)));
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
		gunp->RemoveObject();
		AddEffect("Cooldown",this,1,1,this);
	}
	return true;
}

public func ControlUseCancel(object clonk, int ix, int iy)
{
	RemoveTrajectory(this);
	return 1;
}

public func Grabbed(object clonk, bool grabbed)
{
	if(!grabbed) ControlUseCancel();
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
	if(r > 90) r = 90;
	if(r < -90) r = -90;

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

func Definition(def) {
	SetProperty("ActMap", {
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
},	}, def);
	SetProperty("Name", "$Name$", def);
}
