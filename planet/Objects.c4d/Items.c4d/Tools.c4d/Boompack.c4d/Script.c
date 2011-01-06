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

local fuel;
local rider;
local ridervis;
local riderattach;

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 700; }

protected func Construction()
{
	//flight length
	fuel=100;
}

protected func Destruction()
{
	if(rider) JumpOff(rider);
}

func ControlRight()
{
	SetRDir(+3);
	return true;
}

func ControlLeft()
{
	SetRDir(-3);
	return true;
}

func ControlStop()
{
	SetRDir(0);
	return true;
}

func ControlJump(object clonk)
{
	JumpOff(clonk,60);
	return true;
}

func ControlUseStart(object clonk, int x, int y)
{
	// forward control to item
	if(clonk->GetProcedure()=="ATTACH") return false;
}

func ControlUse(object clonk, int x, int y)
{
	// forward control to item
	if(clonk->GetProcedure()=="ATTACH") return false;

	// only use during walk or jump
	if(clonk->GetProcedure()!="WALK" && clonk->GetProcedure()!="FLIGHT") return true;

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

	if(fuel<=0)
	{
		DoFireworks();
	}

	var ignition = iEffectTime % 9;
	
	if(!ignition)
	{
		var angle = GetR()+RandomX(-12,12);
		SetXDir(3*GetXDir()/4+Sin(angle,24));
		SetYDir(3*GetYDir()/4-Cos(angle,24));
		SetR(angle);
	}
	
	var sizemod = ignition*ignition/3;
	
	var x = -Sin(GetR(),22);
	var y = +Cos(GetR(),22);
	
	CreateParticle("ExploSmoke",x,y,RandomX(-1,1),RandomX(-1,2),RandomX(120,280),RGBa(130,130,130,75));
	CreateParticle("Thrust",x,y,GetXDir()/2,GetYDir()/2,RandomX(80,120)+sizemod,RGBa(255,200,200,160));
	
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
}

protected func Hit()
{
	if(rider)
	{
		JumpOff(rider);
	}
	//Message("I have hit something",this);
	if(GetEffect("Flight",this)) DoFireworks();
	Sound("WoodHit");
}

public func OnMount(clonk)
{
	var iDir = 1;
	if(clonk->GetDir() == 1) iDir = -1;
	clonk->PlayAnimation("PosRocket", 10, Anim_Const(0), Anim_Const(1000));
	riderattach = AttachMesh(clonk, "main", "pos_tool1", Trans_Mul(Trans_Translate(2000, -1000, -2000*iDir), Trans_Rotate(90*iDir,0,1,0)));
	return true;
}

public func OnUnmount(clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	DetachMesh(riderattach);
	return true;
}

func Launch(int angle, object clonk)
{
	SetProperty("Collectible",0);
	SetCategory(C4D_Vehicle);

	Exit();
	AddEffect("Flight",this,150,1,this,this);
	//AddEffect("HitCheck", this, 1,1, nil,nil, clonk, true);

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
}

func DoFireworks()
{
	RemoveEffect("Flight",this);
	Fireworks();
	Explode(30);
}

func SetFuel(int new)
{
	fuel = new;
}

func GetFuel()
{
	return fuel;
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
