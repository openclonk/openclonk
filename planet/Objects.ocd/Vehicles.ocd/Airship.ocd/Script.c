/**
	Airship
	Lighter-than-air travel and transport vehicle. The airship uses an attached hitbox object for the balloon.

	@authors Ringwaul
*/

#include Library_AlignVehicleRotation

// Graphic module variables for animation
local propanim, turnanim;


local throttle;
local enginesound;

local health = 30;

//Rectangle defining where to look for objents contained in the gondola
local gondola = [-20,-2,40,30];


protected func Initialize()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	throttle = 0;

	// init graphics
	propanim = PlayAnimation("Flight", 5, Anim_Const(0), Anim_Const(1000));

	//Create Hitbox
	var hitbox = CreateObjectAbove(Airship_Hitbox);
	hitbox->SetAction("Attach", this);

	// The airship starts facing left; so default to that value
	SetDir(DIR_Left);

	turnanim = PlayAnimation("TurnLeft", 10, Anim_Const(GetAnimationLength("TurnLeft")), Anim_Const(1000));

	// Start the Airship behaviour
	AddEffect("IntAirshipMovement", this, 1, 1, this);
}

public func Damage()
{
	if(GetDamage() >= health)
	{
		AirshipDeath();
	}
}

public func FxIntAirshipMovementStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return 1;
	SetComDir(COMD_Stop);
	effect.AnimDir = DIR_Left;
	return 1;
}


//Moves the propeller 1 tick per call
func AnimationForward()
{
	var i = 50;
	//Loop animation
	if(GetAnimationPosition(propanim) + i > GetAnimationLength("Flight"))
	{
		SetAnimationPosition(propanim, Anim_Const(GetAnimationPosition(propanim) + i - GetAnimationLength("Flight")));
		return 1;
	}

	//advance animation
	else
	{
		SetAnimationPosition(propanim, Anim_Const(GetAnimationPosition(propanim) + i));
		return 1;
	}
	//SoundEffect?
}

public func GetTurnAngle()
{
	var dir = GetAnimDir();
	var r = GetAnimationPosition(turnanim) * 1242 / 10000;
	
	if (dir == DIR_Left)
		r = 180 - r;
	return r;
}

public func FxIntAirshipMovementTimer(object target, proplist effect, int time)
{
	// Is the engine running?
	if (GetComDir() != COMD_Stop && AirshipPilot())
	{
		//Turn the propeller
		AnimationForward();

		// Emit engine smoke
		var i = 20;
		var colour = 240;
		// Is the airship facing right?
		if (effect.AnimDir == DIR_Right) 
			i = -25; 
		if (GetAnimationPosition(turnanim) == GetAnimationLength("TurnLeft")) //Don't smoke if turning... airship blocks view
		{
			var particles = 
			{
				Prototype = Particles_Smoke(),
				R = colour, G = colour, B = colour
			};
			CreateParticle("Smoke", i, 18, 0, 0, PV_Random(36, 2 * 36), particles, 2);
		}
		// Fan-blade sound
		if (!enginesound)
		{
			enginesound = true;
			Sound("FanLoop",nil,nil,nil,1);
		}
	}
	else if(enginesound == true)
	{
		Sound("FanLoop", nil, nil, nil, -1);
		enginesound = false;
	}

	// Wind movement if in the air
	if (!GetContact(-1))
		/* TODO: Implement */;

	// Fall down if there no pilot and ground.
	if (!AirshipPilot())
	{
		if (GetContact(-1) & CNAT_Bottom)
			SetComDir(COMD_Stop);		
		else
			SetComDir(COMD_Down);
	}
	
	//Rise in water
	if (GBackLiquid(0,25))
		//effect.SpeedY = -10;
	if (GBackLiquid(0,25) && !GBackLiquid(0,24) && effect.SpeedY > 1)
		//effect.SpeedY = 0;

	var dir = GetComDir();
	//Log("%v", dir );
	// Turn the airship around if needed
	if (effect.AnimDir == DIR_Left && GetXDir(100) > 30)
		if (GetComDir() == COMD_Right || GetComDir() == COMD_UpRight || GetComDir() == COMD_DownRight)
			target->TurnAirship(DIR_Right);
	if (effect.AnimDir == DIR_Right && GetXDir(100) < -30)
		if (GetComDir() == COMD_Left || GetComDir() == COMD_UpLeft || GetComDir() == COMD_DownLeft)
			target->TurnAirship(DIR_Left);
	
	//Log("animdir: %v, xdir: %v, comdir: %v", effect.AnimDir, GetXDir(100), GetComDir());
	
	return 1;
}

func TurnAirship(int to_dir)
{
	// Default direction is left
	var animName = "TurnLeft";
	if (to_dir == DIR_Right)
		animName = "TurnRight";

	StopAnimation(turnanim);
	turnanim = PlayAnimation(animName, 10, Anim_Linear(0, 0, GetAnimationLength(animName), 36, ANIM_Hold), Anim_Const(1000));
	
	SetAnimDir(to_dir);
	
	var g = gondola;
	AlignObjectsToRotation(this, g[0],g[1],g[2],g[3]);
	
	return;
}

public func SetAnimDir(int to_dir)
{
	var effect = GetEffect("IntAirshipMovement", this);
	if (effect)
		effect.AnimDir = to_dir;
	return;
}

public func GetAnimDir()
{
	var effect = GetEffect("IntAirshipMovement", this);
	if (effect)
		return effect.AnimDir;
	return;
}

/* -- Control Inputs -- */
//These are routed into the Airship's behaviour (in the function above)

func ControlLeft(object clonk)
{
	if (GetComDir() == COMD_Up)
		SetComDir(COMD_UpLeft);
	else if (GetComDir() == COMD_Down)
		SetComDir(COMD_DownLeft);
	else
		SetComDir(COMD_Left);
	return true;
}

func ControlRight(object clonk)
{
	if (GetComDir() == COMD_Up)
		SetComDir(COMD_UpRight);
	else if (GetComDir() == COMD_Down)
		SetComDir(COMD_DownRight);
	else
		SetComDir(COMD_Right);
	return true;
}

func ControlUp(object clonk)
{
	if (GetComDir() == COMD_Left)
		SetComDir(COMD_UpLeft);
	else if (GetComDir() == COMD_Right)
		SetComDir(COMD_UpRight);
	else
		SetComDir(COMD_Up);
	return true;
}

func ControlDown(object clonk)
{
	if (GetComDir() == COMD_Left)
		SetComDir(COMD_DownLeft);
	else if (GetComDir() == COMD_Right)
		SetComDir(COMD_DownRight);
	else
		SetComDir(COMD_Down);
	return true;
}

func ControlStop(object clonk, int control)
{
	var comdir = GetComDir();
	var newdir;
	if (control == CON_Left)
	{
		if (comdir == COMD_Left)
			newdir = COMD_Stop;
		if (comdir == COMD_UpLeft)
			newdir = COMD_Up;
		if (comdir == COMD_DownLeft)
			newdir = COMD_Down;
	}
	if (control == CON_Right)
	{
		if (comdir == COMD_Right)
			newdir = COMD_Stop;
		if (comdir == COMD_UpRight)
			newdir = COMD_Up;
		if (comdir == COMD_DownRight)
			newdir = COMD_Down;
	}
	if (control == CON_Up)
	{
		if (comdir == COMD_Up)
			newdir = COMD_Stop;
		if (comdir == COMD_UpLeft)
			newdir = COMD_Left;
		if (comdir == COMD_UpRight)
			newdir = COMD_Right;
	}
	if (control == CON_Down)
	{
		if (comdir == COMD_Down)
			newdir = COMD_Stop;
		if (comdir == COMD_DownLeft)
			newdir = COMD_Left;
		if (comdir == COMD_DownRight)
			newdir = COMD_Right;
	}
	SetComDir(newdir);
	return true;
}

private func AirshipPilot()
{
	//Looks for a clonk within the Gondola
	var g = gondola;
	var clonk = FindObject(Find_ID(Clonk), Find_OCF(OCF_Alive),Find_InRect(g[0],g[1],g[2],g[3]));
	if(clonk)
		return clonk;
	else
		return false;
}

/* -- Airship Destruction --*/

func AirshipDeath()
{
	//First let's create the burnt airship
	var burntairship = CreateObjectAbove(Airship_Burnt,0,27); //27 pixels down to align ruin with original

	//Now let's copy it's animation, and hold it there
	var animspot;
	animspot = GetAnimationPosition(turnanim);
	if(turnanim == -1) burntairship->PlayAnimation("TurnLeft", 10, Anim_Const(animspot), Anim_Const(1000));
	else
		burntairship->PlayAnimation("TurnRight", 10, Anim_Const(animspot), Anim_Const(1000));

	//Set ruin on fire
	burntairship->Incinerate();

	//Make sure engine sound is gone
	Sound("FanLoop",nil,nil,nil,-1);

	//This object has served its purpose.
	Explode(27);
}

public func IsShipyardProduct() { return true; }
public func IsVehicle() { return true; }

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		Directions = 1,
		X = 0,
		Y = 0,
		Wdt = 64,
		Hgt = 54,
		Speed = 100,
		Accel = 4,
		Decel = 8,
		NextAction = "Fly",
	},
};

func Definition(def)
{
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-25,1,0,0),Trans_Rotate(40,0,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local Rebuy = true;
local Plane = 500;
local SolidMaskPlane = 275;
