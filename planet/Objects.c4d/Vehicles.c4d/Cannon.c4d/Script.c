/*-- 
	Cannon
	Author: Ringwaul
	
	Fires objects great distances.
--*/

local aim_anim;
local olddir;
local aimangle;
local aimdir;
local grabber;

static const Fire_Velocity = 175;

public func IsToolProduct() { return 1; }

protected func Initialize()
{
	SetAction("Roll");
	olddir = GetDir();
	aimangle = 0;
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

//One space for blackpowder, one space for shot.
private func MaxContents() { return 2; }

protected func RejectCollect(id idObj,object pObj)
{
	if(ContentsCount() < MaxContents()) { Sound("Clonk"); return 0; }

	if(pObj->Contained()) return Message("$TxtCannonisfull$");
	if(Abs(pObj->GetXDir())>6) pObj->SetYDir(-5);
	Sound("WoodHit*");
	return 1;
}

/* Control */

public func ControlUseStart(object clonk, int ix, int iy)
{
	//ammo requirements
	if(!FindObject(Find_Container(this),Find_ID(Blackpowder)))//needs some powder
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedspowder$");
		return 1;
	}
	if(!FindObject(Find_Container(this),Find_Category(C4D_Object),Find_Not(Find_ID(Blackpowder))))//needs a shot
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsammo$");
		return 1;
	}

	//Can't fire if cannon is cooling down
	if(GetEffect("Cooldown",this))	return 1;
	
	if(FindObject(Find_Container(this),Find_ID(Blackpowder)) &&
		FindObject(Find_Container(this),Find_Category(C4D_Object), Find_Not(Find_ID(Blackpowder))))
	{
		DoFire(FindObject(Find_Container(this),Find_Category(C4D_Object), Find_Not(Find_ID(Blackpowder))),clonk);
		FindContents(Blackpowder)->RemoveObject();
		AddEffect("Cooldown",this,1,1,this);
	}
	return 1;
}

public func FxCooldownTimer(object target, int num, int timer)
{
	if(timer > 72) return -1;
}

//Sends variables to Timer() for press & release control
func ControlUp(object clonk)
{
	if(aimangle < 90)
	{
		aimdir = 1;
	}
}

func ControlDown(object clonk)
{
	if(aimangle > 0)
	{
		aimdir = -1;
	}
}

//Player has released button? Stop rotating barrel.
func ControlStop(object clonk, int control)
{
	aimdir = 0;
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

protected func DoFire(object iammo, object clonk)
{
	if(iammo == nil) iammo = FindContents(Rock); //debug only!
	iammo->Exit();

	if(iammo->GetID() == Dynamite)
	{
		iammo->Fuse();
	}

	var r = ConvertAngle();

	//Send ammo flying
	var spread = -1 + Random(3);
	iammo->SetR(r);
	iammo->SetRDir(-4 + Random(9));
	iammo->LaunchProjectile(r + spread, 17, Fire_Velocity);

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

	//Recoil
	if(aimangle < 88) aimangle = aimangle + Random(3);
	PlayAnimation("Fire", 5, Anim_Linear(0,0, GetAnimationLength("Fire"), 30, ANIM_Remove), Anim_Const(500));
}

public func Grabbed(object clonk, bool grabbed)
{
	if(grabbed)
	{
		if(!grabber) grabber = clonk;
		if(clonk->GetOwner() != GetOwner()) SetOwner(clonk->GetOwner());
		AddEffect("TrajAngle", this, 50, 1, this);
		return 1;
	}

	if(!grabbed)
	{
		grabber = nil;
		RemoveEffect("TrajAngle",this);	
		RemoveTrajectory(this);
	return 1;
	}
}

public func FxTrajAngleTimer(object target, int num, int timer)
{
	if(grabber->GetProcedure() != "PUSH")
	{
		RemoveTrajectory(this);
		grabber = nil;
		return -1;
	}
	
	var r = ConvertAngle();

	var iColor = RGB(255,255,255);
	if(!FindContents(Blackpowder) || FindObject(Find_Category(C4D_Object),Find_Not(Find_ID(Blackpowder))) == nil ) iColor = RGB(255,0,0);
	target->AddTrajectory(target, target->GetX(), target->GetY()-3, Cos(r-90, Fire_Velocity), Sin(r-90, Fire_Velocity), iColor, 20);
}


func Timer()
{	
	// Does a whole bunch of good stuff. Updates animation and handles press & release control method.
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

	//Aiming animation
	if(aimangle >= 0 && aimangle <= 90)
	{
		//stop player from going over or under 0-90 degrees.
		if(aimangle == 0 && aimdir == -1) return 1;
		if(aimangle == 90 && aimdir == +1) return 1;
		aimangle = aimangle + aimdir;
	}
	SetAnimationPosition(aim_anim, Anim_Const(aimangle*3954444/100000)); //conversion. Apparently 90 blender frames is 3559 ogre frames.
}

private func ConvertAngle()
{
	//Finds the angle based on chassis rotation and direction.
	var iangle = (aimangle - GetR()); //modifies angle when the cannon's chassis rotates
	if(GetDir() == 0) iangle = (aimangle + GetR());
	var angle = (90 - iangle);//angle modifier which acknowledges direction
	if(GetDir() == 0) angle = (270 + iangle);
	return angle;
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
