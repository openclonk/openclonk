/*--
	Cannon
	Author: Ringwaul
	
	Fires objects great distances.
--*/


#include Library_HasExtraSlot

local animAim;
local animTurn;
local turnDir;

local Fire_Velocity = 175;

public func IsArmoryProduct() { return true; }
public func IsVehicle() { return true; }

protected func Initialize()
{
	turnDir = 1;
	SetAction("Roll");
	animAim = PlayAnimation("Aim", 1,  Anim_Const(0),Anim_Const(1000));
	animTurn = PlayAnimation("TurnRight", 5, Anim_Const(0), Anim_Const(1000));
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
	return UseAnyStart(clonk,ix,iy,0);
}

public func ControlUseAltStart(object clonk, int ix, int iy)
{
	return UseAnyStart(clonk,ix,iy,1);
}

private func UseAnyStart(object clonk, int ix, int iy, int item)
{
	var result = CheckForKeg(clonk);
	if (!result)
	{
		clonk->CancelUse();
		return true;
	}
		
	if (!clonk->GetHandItem(item))
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		clonk->CancelUse();
		return true;
	}
			
	if(clonk->GetOwner() != GetOwner()) SetOwner(clonk->GetOwner());

	//Animation
	var r = ConvertAngle(Angle(0,0,ix,iy));
	SetAnimationPosition(animAim, Anim_Const(AnimAngle(r)*3954444/100000)); //conversion. Apparently 90 blender frames is 3559 ogre frames.
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
			Sound("WoodHit?");
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

local angPrec = 1000;

public func ControlUseHolding(object clonk, int ix, int iy)
{
	if (!clonk)
	{
		clonk->CancelUse();
		return true;
	}
	var r = ConvertAngle(Angle(0,0,ix,iy,angPrec));

	var iColor = RGB(255,255,255);
	if (!Contents(0) || GetEffect("IntCooldown",this))
		iColor = RGB(255,0,0);
	AddTrajectory(this, GetX() + 5, GetY() + 2, Cos(r - 90 * angPrec, Fire_Velocity,angPrec), Sin(r - 90 * angPrec, Fire_Velocity,angPrec), iColor, 20);

	SetAnimationPosition(animAim, Anim_Const(AnimAngle(r/angPrec)*3954444/100000));
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
	var nR = BoundBy(Normalize(angle,-180 * angPrec,angPrec), (-90 * angPrec) + (GetR() * angPrec), (90 * angPrec) + (GetR() * angPrec));
	var r2 = nR - GetR() * angPrec;
	//debug messages
	//Message(Format("nR = %d|rL = %d",nR,r2));
	
	//Turn the cannon into the pointed direction
	if(nR - (GetR() * angPrec) < 0 && turnDir == 1) TurnCannon(0);
	if(nR - (GetR() * angPrec) > 0 && turnDir == 0) TurnCannon(1);
	
	//renormalize the angle to 0/360 from -180/180
	return Normalize(nR,0,angPrec);
}

public func ControlUseStop(object clonk, int ix, int iy)
{
	return UseAnyStop(clonk,ix,iy,0);
}

public func ControlUseAltStop(object clonk, int ix, int iy)
{
	return UseAnyStop(clonk,ix,iy,1);
}

private func UseAnyStop(object clonk, int ix, int iy, int item)
{

	RemoveTrajectory(this);
	
	var result = CheckForKeg(clonk);
	if (!result)
		return true;

	var projectile = clonk->GetHandItem(item);
	if (!projectile) // Needs a projectile
	{
		PlayerMessage(clonk->GetOwner(),"$TxtNeedsAmmo$");
		return true;
	}

	//Can't fire if cannon is cooling down or turning
	if(GetEffect("IntCooldown",this) || GetEffect("IntTurning",this))	return true;
	
	if (projectile)
	{
		DoFire(projectile, clonk, Angle(0,0,ix,iy,angPrec));
		var powder = Contents(0)->PowderCount();
		if(powder >= 1 || projectile->~IsSelfPropellent())
		{
			var powderkeg = Contents(0);
			//If there is a powder keg, take powder from it
			powderkeg->SetPowderCount(powderkeg->PowderCount() -1);
			
			DoFire(projectile, clonk, Angle(0,0,ix,iy, angPrec));
			AddEffect("IntCooldown",this,1,1,this);
			if(powderkeg->PowderCount() == 0)
			{
				powderkeg->RemoveObject();
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

//Stops the player from shooting for the defined amount of frames
public func FxIntCooldownTimer(object target, effect, int timer)
{
	if(timer > 72) return -1;
}


func ControlLeft(object clonk)
{
	if(turnDir == 1){
		TurnCannon(0);
	}
}

func ControlRight(object clonk)
{
	if(turnDir == 0){
		TurnCannon(1);
	}
}

func TurnCannon(int dir)
{
	turnDir = dir;
	//Remove any old effect
	if(GetEffect("IntTurnCannon", this)) RemoveEffect("IntTurnCannon", this);
	//Add a new one
	return AddEffect("IntTurnCannon", this, 1, 1, this);
}
	

func FxIntTurnCannonTimer(object cannon, proplist effect, int timer)
{
	var current = GetAnimationPosition(animTurn);
	var target = GetAnimationLength("TurnRight");
	if(turnDir == 1) target = 0;	
	var tickAmount = 50;	//by how much the animation will move forward each frame	
	
	//advance turn animation
	if((current != GetAnimationLength("TurnRight") && turnDir == 0) || (current != 0 && turnDir == 1)){
		SetAnimationPosition(animTurn, Anim_Const(MoveTowards(current, target, tickAmount)));
	}
	else return -1;
}

protected func DoFire(object iammo, object clonk, int angle)
{
	iammo->~Fuse();

	//Don't excede possible trajectory
	var r = Normalize(angle,-180 * angPrec, angPrec);
	if(r > 90 * angPrec + GetR() * angPrec) r = 90 * angPrec + GetR() * angPrec;
	if(r < -90 * angPrec + GetR() * angPrec) r = -90 * angPrec + GetR() * angPrec;

	//Send ammo flying
	iammo->SetR(r / angPrec);
	iammo->SetRDir(-4 + Random(9));
	iammo->LaunchProjectile(r, 17, Fire_Velocity, 0,0, angPrec);

	//Particles
	var dist = 25;
	var px = Cos(r/angPrec - 90,dist);
	var py = Sin(r/angPrec - 90,dist) - 4;
	CreateParticle("Flash",px,py,0,0,420,RGB(255,255,255));
	for(var i=0; i<15; ++i) //liberated from musket script... I'm horrible at particles :p
	{
		var speed = RandomX(0,10);
		CreateParticle("ExploSmoke",px,py,+Sin(r/angPrec,speed)+RandomX(-2,2),-Cos(r/angPrec,speed)+RandomX(-2,2),RandomX(100,400),RGBa(255,255,255,75));
	}
	CreateParticle("MuzzleFlash",px,py,px,py+4,700,RGB(255,255,255)); //muzzle flash uses speed as Rotation... so I negate the -4

	//sound
	Sound("Blast3");
}

local ActMap = {
Roll = {
	Prototype = Action,
	Name = "Roll",
	Procedure = DFA_NONE,
	Directions = 1,
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

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1;
local Rebuy = true;
