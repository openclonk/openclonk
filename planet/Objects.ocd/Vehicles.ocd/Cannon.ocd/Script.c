/*--
	Cannon
	Author: Ringwaul
	
	Fires objects great distances.
--*/


#include Library_HasExtraSlot
#include Library_ElevatorControl
#include Library_Destructible

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
	animAim = PlayAnimation("Aim", 1,  Anim_Const(0));
	animTurn = PlayAnimation("TurnRight", 5, Anim_Const(0));
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

protected func RejectCollect(id def, object obj)
{
	// Only accept powder kegs.
	if (def != PowderKeg)
		return true;
	// Only one powder keg at a time.
	if (ContentsCount() >= 1)
		return true;
	return false;
}


/*-- Control --*/

public func ControlUseStart(object clonk, int ix, int iy)
{
	var result = CheckForKeg(clonk);
	if (!result)
	{
		clonk->CancelUse();
		return true;
	}
		
	if (!clonk->GetHandItem(0))
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
			Sound("Hits::Materials::Wood::WoodHit?");
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

local angPrec = 1000;

public func ControlUseHolding(object clonk, int ix, int iy)
{
	if (!clonk) return true;
	
	var r = ConvertAngle(Angle(0,0,ix,iy,angPrec));

	var iColor = RGB(255,255,255);
	if (!Contents(0) || GetEffect("IntCooldown",this))
		iColor = RGB(255,0,0);
	Trajectory->Create(this, GetX() + 5, GetY() + 2, Cos(r - 90 * angPrec, Fire_Velocity,angPrec), Sin(r - 90 * angPrec, Fire_Velocity,angPrec));

	SetCannonAngle(r);
	
	return true;
}

public func SetCannonAngle(int r)
{
	return SetAnimationPosition(animAim, Anim_Const(AnimAngle(r/angPrec)*3954444/100000));
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
	//var r2 = nR - GetR() * angPrec;
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
	Trajectory->Remove(this);

	if (!CheckForKeg(clonk))
		return true;

	var projectile = clonk->GetHandItem(0);
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
		var powder = Contents(0)->GetPowderCount();
		if(powder >= 1 || projectile->~IsSelfPropellent())
		{
			var powderkeg = Contents(0);
			//If there is a powder keg, take powder from it
			powderkeg->DoPowderCount(-1);
			
			DoFire(projectile, clonk, Angle(0,0,ix,iy, angPrec));
			AddEffect("IntCooldown",this,1,1,this);
			if(powderkeg->GetPowderCount() == 0)
			{
				powderkeg->RemoveObject();
				CreateObjectAbove(Barrel);
			}
		}
	}
	return true;
}

public func ControlUseCancel()
{
	Trajectory->Remove(this);
	return true;
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

public func TurnCannon(int dir, bool instant)
{
	turnDir = dir;
	//Remove any old effect
	if(GetEffect("IntTurnCannon", this)) RemoveEffect("IntTurnCannon", this);
	// Instant turn?
	if (instant)
	{
		// Simply set anim position to desired side
		var target = 0;
		if (dir == DIR_Left) target = GetAnimationLength("TurnRight");
		SetAnimationPosition(animTurn, Anim_Const(target));
	}
	else
	{
		// Non-instant turn: Add timer to adjust animation (I wonder why it's not just using Anim_Linear?)
		return AddEffect("IntTurnCannon", this, 1, 1, this);
	}
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
	//Don't excede possible trajectory
	var r = Normalize(angle,-180 * angPrec, angPrec);
	if(r > 90 * angPrec + GetR() * angPrec) r = 90 * angPrec + GetR() * angPrec;
	if(r < -90 * angPrec + GetR() * angPrec) r = -90 * angPrec + GetR() * angPrec;

	//Send ammo flying
	iammo->SetR(r / angPrec);
	iammo->SetRDir(-4 + Random(9));
	iammo->LaunchProjectile(r, 17, Fire_Velocity, 0,0, angPrec);
	if (clonk)
		iammo->SetController(clonk->GetController());
	iammo->~OnCannonShot(this);

	//Particles
	var dist = 25;
	var px = Cos(r/angPrec - 90,dist);
	var py = Sin(r/angPrec - 90,dist) - 4;
	CreateParticle("Flash", px, py, 0, 0, 8, Particles_Flash());

	var x = Sin(r/angPrec, 20);
	var y = -Cos(r/angPrec, 20);
	CreateParticle("Smoke", px, py, PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), PV_Random(40, 60), Particles_Smoke(), 20);
	CreateMuzzleFlash(px, py, r / angPrec, 60);
	//sound
	Sound("Fire::Blast3");
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
local BorderBound = C4D_Border_Sides;
local ContactCalls = true;
local Components = {Metal = 4, Wood = 2};
local HitPoints = 150;