/*--
	Boompack
	Authors: Ringwaul, Newton

	A risky method of flight. When the boompack is used and launched towards
	the sky, the category of the boompack is changed to be a vehicle and set
	to be non-collectible. The clonk is then attached to the boompack. While
	he is attached, he has more control over it than if it were just in his
	inventory: The ControlLeft/Right/Up/Down callbacks are issued to the boom-
	pack too. Here, they are used to slightly steer the boompack to the left
	or right plus to jump off the rocket.
--*/

#include Library_CarryHeavy

local fuel;
local rider;
local ridervis;
local riderattach;
local dirdev;
local controllable;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 700; }

public func GetCarryTransform(clonk)
{
	if(GetCarrySpecial(clonk))
		return Trans_Translate(0, 6500, 0);
	
	return Trans_Translate(0, 0, -1500);
}

protected func Construction()
{
	//flight length
	fuel=100;
	dirdev=12;
	controllable = true;
}

func Fuse()
{
	Launch(GetR());
}

func Incineration(int caused_by)
{
	SetController(caused_by);
	Fuse();
}

protected func Destruction()
{
	if(rider) JumpOff(rider);
}

func ControlRight()
{
	if(controllable)
		SetRDir(+3);
	return true;
}

func ControlLeft()
{
	if(controllable)
		SetRDir(-3);
	return true;
}

func ControlStop()
{
	if(controllable)
		SetRDir(0);
	return true;
}

func ControlJump(object clonk)
{
	if(controllable)
		JumpOff(clonk,60);
	return true;
}

public func RejectUse(object clonk)
{
	// If the Clonk is on the boompack, we won't stop the command here, but forward it later.
	if (clonk->GetProcedure() == "ATTACH") return false;
	// Only allow during walking or jumping.
	return clonk->GetProcedure() != "WALK" && clonk->GetProcedure() != "FLIGHT";
}


func ControlUse(object clonk, int x, int y)
{
	// forward control to item
	if(clonk->GetProcedure()=="ATTACH") return false;

	var angle=Angle(0,0,x,y);
	Launch(angle,clonk);

	return true;
}


protected func FxFlightTimer(object pTarget, effect, int iEffectTime)
{
	// clonk does sense the danger and with great presence of mind jumps of the rocket
	if(fuel<20 && rider)
	{
		JumpOff(rider,30);
	}

	if(!Random(105)) Sound("Fire::Cracker");

	if(fuel<=0)
	{
		DoFireworks();
		return;
	}

	var ignition = iEffectTime % 9;
	
	if(!ignition)
	{
		var angle = GetR()+RandomX(-dirdev,dirdev);
		SetXDir(3*GetXDir()/4+Sin(angle,24));
		SetYDir(3*GetYDir()/4-Cos(angle,24));
		SetR(angle);
	}
	
	var x = -Sin(GetR(), 10);
	var y = +Cos(GetR(), 10);

	var xdir = GetXDir() / 2;
	var ydir = GetYDir() / 2;
	CreateParticle("FireDense", x, y, PV_Random(xdir - 4, xdir + 4), PV_Random(ydir - 4, ydir + 4), PV_Random(16, 38), Particles_Thrust(), 5);
	
	fuel--;
}

private func JumpOff(object clonk, int speed)
{
	rider = nil;

	if(!clonk) return;
	if(!(clonk->GetProcedure() == "ATTACH")) return;
	if(!(clonk->GetActionTarget() == this)) return;
	
	var xdir = 200;
	var ydir = clonk.JumpSpeed;
	// which direction does the clonk jump?
	if(GetRDir() == 0) xdir = 0;
	if(GetRDir() < 0) xdir = -xdir;
	
	clonk->SetAction("Tumble");
	clonk->SetXDir(GetXDir(50)+speed*xdir/100,100);
	clonk->SetYDir(GetYDir(50)-speed*ydir/100,100);
	
	// Add hit check to explode on mid-air contact.
	// This increases the military efficacy of the boompack.
	AddEffect("HitCheck", this, 1, 2, nil, nil, clonk);
}

protected func Hit()
{
	if(rider)
	{
		JumpOff(rider);
	}
	Sound("Hits::GeneralHit?");
	if(GetEffect("Flight",this)) DoFireworks();
}

// Called when hitting something mid-air after the Clonk jumped off.
public func HitObject(object target)
{
	if (target && WeaponCanHit(target))
		target->~OnProjectileHit(this);
	if (this)
		DoFireworks();
}

public func OnMount(clonk)
{
	var iDir = 1;
	if(clonk->GetDir() == 1) iDir = -1;
	clonk->PlayAnimation("PosRocket", CLONK_ANIM_SLOT_Arms, Anim_Const(0), Anim_Const(1000));
	riderattach = AttachMesh(clonk, "main", "pos_tool1", Trans_Mul(Trans_Translate(-1000,2000*iDir,2000), Trans_Rotate(-90*iDir,1,0,0)));
	
	//Modify picture transform to fit icon on clonk mount
	this.PictureTransformation = Trans_Mul(Trans_Translate(5000 * clonk->GetDir(),0,0), Trans_Rotate(-20,1,0,0), Trans_Rotate(0,0,0,1), Trans_Rotate(0,0,1,0), Trans_Scale(700));
	return true;
}

public func OnUnmount(clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	DetachMesh(riderattach);
	DefaultPicTransform();
	return true;
}

func Launch(int angle, object clonk)
{
	SetProperty("Collectible",0);
	SetCategory(C4D_Vehicle);

	Exit();
	Sound("Objects::Boompack::Launch");
	AddEffect("Flight",this,150,1,this);
	Sound("Objects::Boompack::Fly", false, 60, nil, 1);
	
	// Add hit check to explode on mid-air contact.
	// But only if not ridden by a clonk, for riding clonks
	// this effect is added when the clonk jumps off.
	if (!clonk)
		AddEffect("HitCheck", this, 1, 2, nil, nil);

	//Ride the rocket!
	if(clonk)
	{
		clonk->SetAction("Ride",this);
		rider=clonk;
		SetOwner(clonk->GetController());
	}

	var level = 16;
	var i=0, count = 3+level/8, r = Random(360);
	while(count > 0 && ++i < count*6) {
		r += RandomX(40,80);
		var smokex = +Sin(r,RandomX(level/4,level/2));
		var smokey = -Cos(r,RandomX(level/4,level/2));
		if(GBackSolid(smokex,smokey))
			continue;
		CreateSmokeTrail(2*level,r,smokex,smokey,nil,true);
		count--;
	}
	
	SetR(angle);
	SetVelocity(angle,60);
	this.Collectible = false;
}

func DoFireworks()
{
	RemoveEffect("Flight",this);
	Fireworks();
	Sound("Fire::BlastFirework", false, 200);
	Explode(30);
}

func SetFuel(int new)
{
	fuel = new;
}

func SetDirectionDeviation(int new)
{
	dirdev = new;
}

func SetControllable(bool new)
{
	controllable = new;
}

func GetFuel()
{
	return fuel;
}

public func IsProjectileTarget(object projectile)
{
	return projectile->GetID() != GetID();
}

func OnProjectileHit(object projectile)
{
	Incinerate(100, projectile->GetController());
}

func IsInventorProduct() { return true; }

private func DefaultPicTransform()
{
	this.PictureTransformation = this.Prototype.PictureTransformation;
}

public func Definition(def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(-3000, -1000, 0), Trans_Rotate(45,0,0,1),Trans_Rotate(-35,1,0,0),Trans_Scale(1200));
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local BlastIncinerate = 1;
local ContactIncinerate = 1;
