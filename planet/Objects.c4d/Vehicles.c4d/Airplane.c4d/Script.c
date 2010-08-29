/*-- 
	Airplane
	Author: Ringwaul
	
	Acrobatic air-vehicle. Capable of firing lead shot.
--*/

protected func ContactLeft()
{
	if (Stuck() && !Random(5))
		SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
	if (Stuck() && !Random(5)) 
		SetRDir(RandomX(-7, +7));
}

local throttle;
local rdir;
local thrust;
local dir;
local propanim;
local reticle;
local health;
local clonkmesh;

protected func Construction(object byobj)
{
	SetR(-90);
}

protected func Initialize()
{	
	propanim = PlayAnimation("Propellor", 15,  Anim_Const(0),Anim_Const(1000));
	AddEffect("IntPlane",this,1,1,this);
	SetAction("Land");
	throttle = 0;
	thrust = 0;
	rdir = 0;
	dir = 0;
	health = 50;
}

/*-- Control --*/

public func ContainedUseStart(object clonk, int ix, int iy)
{
	reticle = CreateObject(GUI_Reticle);
	reticle->SetOwner(clonk->GetController());
	reticle->SetAction("Show", this);
	return 1;
}

public func ContainedUseStop(object clonk, int ix, int iy)
{
	if(reticle) reticle->RemoveObject();

	var ammo = FindObject(Find_Container(clonk),Find_Func("IsMusketAmmo"));
	if(GetEffect("IntCooldown",this)) return 1;
	if(ammo)
	{
		var shot = ammo->TakeObject();
		var angle = this->GetR();
		shot->Launch(clonk, angle, 35, 200);
		Sound("GunShoot*.ogg");

		// Muzzle Flash & gun smoke
		var IX = Sin(GetR(), 30);
		var IY = -Cos(GetR(), 30);

		for(var i=0; i<10; ++i)
		{
			var speed = RandomX(0,10);
			var r = angle;
			CreateParticle("ExploSmoke",IX,IY,+Sin(r,speed)+RandomX(-2,2) + GetXDir(),-Cos(r,speed)+RandomX(-2,2) + GetYDir(),RandomX(100,400),RGBa(255,255,255,50));
		}
		CreateParticle("MuzzleFlash",IX,IY,+Sin(angle,500),-Cos(angle,500),600,RGB(255,255,255),this);
		CreateParticle("Flash",0,0,GetXDir(),GetYDir(),800,RGBa(255,255,64,150));

		AddEffect("IntCooldown", this,1,1,this);	
	}
	return 1;
}

public func ContainedUseCancel(object clonk, int ix, int iy)
{
	if(reticle) reticle->RemoveObject();
	return 1;
}

public func FxIntCooldownTimer(object target, int num, int timer)
{
	if(timer > 50) return -1;
}

public func ContainedUp(object clonk)
{
	//plane is broken?
	if(GetDamage() > health)
		return;

	//engine start
	if(GetAction() == "Land")
	{
		StartFlight();
		throttle = 15;
		return;
	}
}

public func ContainedDown(object clonk)
{
	//plane is broken?
	if(GetDamage() > health)
		return;

	//engine shutoff
	if(GetAction() == "Fly")
	{
		CancelFlight();
		return;
	}
	//allow reverse
	if(GetAction() == "Land")
	{
		StartFlight();
		throttle = -5;
		return;
	}
}

public func ContainedLeft(object clonk)
{
	rdir = -1;
}

public func ContainedRight(object clonk)
{
	rdir = 1;
}

public func ContainedStop(object clonk)
{
	rdir = 0;
}

public func StartFlight()
{
	Sound("EngineStart.ogg");
	AddEffect("IntSoundDelay",this,1,1,this);
	SetAction("Fly");
}

public func CancelFlight()
{
	RemoveEffect("IntSoundDelay",this);
	Sound("EngineLoop.ogg",0,100,nil,-1);
	Sound("EngineStop.ogg");
	SetAction("Land");
	rdir = 0;
	throttle = 0;
}

private func FxIntSoundDelayTimer(object target, int num, int timer)
{
	if(timer >= 78)
	{
		Sound("EngineLoop.ogg",0,100,nil,1);
		return -1;
	}
}

private func FxIntPlaneTimer(object target, int num, int timer)
{
	//Lift
	var lift = Distance(0,0,GetXDir(),GetYDir()) / 2;
	if(lift > 20) lift = 20;
	if(throttle < 1) lift = 0;

	if(GetAction() == "Fly")
	{
		//Ailerons
		//clockwise
		if(rdir == 1)
			if(GetRDir() < 5) SetRDir(GetRDir() + 1);
		//counter-clockwise
		if(rdir == -1)
			if(GetRDir() > -5) SetRDir(GetRDir() - 1);
		if(rdir == 0) SetRDir();

		//Match dir
		//temporary
		if(throttle > 0)
		{
			if(GetXDir() > 10 && dir != 1) RollPlane(1);
			if(GetXDir() < -10 && dir != 0) RollPlane(0);
		}

		//Vfx
		var colour = 255 - (GetDamage() * 3);
		CreateParticle("PlaneSmoke",0,0,0,0,RandomX(70,90),RGB(colour,colour,colour));
	}

	//Throttle-to-thrust lag
	if(timer % 10 == 0)
	{
		if(throttle > thrust) ++thrust;
		if(throttle < thrust) --thrust;
	}
	
	//propellor
	var change = GetAnimationPosition(propanim) + thrust * 3;
	if(change > GetAnimationLength("Propellor"))
		change = (GetAnimationPosition(propanim) + thrust * 3) - GetAnimationLength("Propellor");
	if(change < 0)
		change = (GetAnimationLength("Propellor") - thrust * 3);

	SetAnimationPosition(propanim, Anim_Const(change));

	//Thrust
	SetXDir(Sin(GetR(),thrust) + GetXDir(100), 100);
	SetYDir(-Cos(GetR(),thrust) + GetYDir(100) - lift, 100);

	//Drag
	var maxspeed = 40;
	var speed = Distance(0,0,GetXDir(),GetYDir());
	if(speed > 40)
	{
		SetXDir(GetXDir(100)*maxspeed/speed,100);
		SetYDir(GetYDir(100)*maxspeed/speed,100);
	}

	//No pilot?
	if(!Contents(0) && throttle != 0) CancelFlight();

	//Pilot, but no mesh? In case they are scripted into the plane.
	if(FindContents(Clonk) && !clonkmesh)
		PlaneMount(FindContents(Clonk));

	//Gun Sights
	if(reticle)
	{
		var retcol = 255;
		if(GetEffect("IntCooldown",this)) retcol = 0;
		reticle->SetClrModulation(RGB(255,retcol,retcol));
	}
}

public func RollPlane(int rolldir)
{
	if(dir != rolldir)
	{
		PlayAnimation(Format("Roll%d",rolldir), 10, Anim_Linear(0, 0, GetAnimationLength(Format("Roll%d",rolldir)), 36, ANIM_Remove), Anim_Const(1000));
		dir = rolldir;
	}
}

public func IsProjectileTarget(target,shooter) { return true; }

public func Damage(int change, int byplayer)
{
	if(GetDamage() > health)
	{
		if(Random(2)) PlaneDeath();
		else
			CancelFlight();
	}
}

private func PlaneDeath()
{
	while(Contents(0))
		Contents(0)->Exit();
	Explode(50);
}

public func Hit()
{
	if(GetDamage() > 50) PlaneDeath();
}

public func ActivateEntrance(object pby)
{
	if(ContentsCount() > 0)
		if(pby->Contained() == this)
		{
			pby->Exit();
			return;
		}

	if(ContentsCount() > 0)
		if(pby->Contained() != this)
		{
			return;
		}
	if(ContentsCount() == 0)
	{
		pby->Enter(this);
		PlaneMount(pby);
	}
}

public func Ejection(object obj)
{
	PlaneDismount(obj);
	obj->SetSpeed(this->GetXDir(),this->GetYDir());
}

public func PlaneMount(object clonk)
{
	SetOwner(clonk->GetController());
	clonk->PlayAnimation("Stand", 15, 0, Anim_Const(1000));
	clonkmesh = AttachMesh(clonk,"pilot","skeleton_body",Trans_Mul(Trans_Rotate(180,0,1,0), Trans_Translate(0,-3000,-1000)));
	return true;
}

public func PlaneDismount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(15));
	DetachMesh(clonkmesh);
	clonkmesh = nil;
	return true;
}

func Definition(def) {
	SetProperty("ActMap", {
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 0,
	Length = 10,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 40,
	Hgt = 56,
	NextAction = "Fly",
},  
Land = {
	Prototype = Action,
	Name = "Land",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 2,
	X = 0,
	Y = 0,
	Wdt = 40,
	Hgt = 56,
	NextAction = "Land",
}, 
}, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(90,0,0,1), Trans_Translate(-10000,-3000,0), Trans_Rotate(25,0,1,0)));
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}
