/*--
	Airship
	Ringwaul
	
	Lighter-than-air travel and transport vehicle.
	The airship uses several objects to function; the base control/collision object,
	an attached graphics object (due to engine limitations with solidmasks),
	and a hitbox for the balloon.
--*/

local throttle;

//sums of x,y dir forces upon blimp
local xtarget;
local ytarget;

//Airship Control
local xthrottle;
local ythrottle;

//Attached modules
local graphic;
local hitbox;

//Graphic module variables for animation
local animdir;
local turnanim;

protected func Initialize()
{
	SetAction("Fly");
	SetComDir(COMD_None);
	throttle = 0;
	xtarget = 0;
	ytarget = 0;

	//Create 3D Graphic
	graphic = CreateObject(Airship_Graphic);
		graphic->SetAction("Attach", this);
		graphic->SetAirshipParent(this);

	//Create Hitbox
	hitbox = CreateObject(Airship_Hitbox);
		hitbox->SetAction("Attach", this);
		hitbox->SetAirshipParent(this);

	//The airship starts facing left; so default to that value
	animdir = -1;

	//Start the Airship behaviour
	AddEffect("FlyEffect",this,1,1,this);
}

local enginesound;

public func FxFlyEffectTimer(object target, int num, int timer)
{
	//Cancel effect if there is no graphic.
	if(!graphic) return -1;

	//Is the engine running?
	if(Abs(xthrottle) == 1 || Abs(ythrottle) == 1)
	{
		//Turn the propeller
		graphic->AnimationForward();

		//Emit engine smoke
		var i = 20;
		var colour = 240;
			if(animdir == 1) i = -25; //Is the airship facing right?
		if(graphic->GetAnimationPosition(turnanim) == graphic->GetAnimationLength("TurnLeft")) //Don't smoke if turning... airship blocks view
			CreateParticle("EngineSmoke", i, 18,0,0,RandomX(20,40),RGBa(colour,colour,colour,colour));

		//Fan-blade sound
		if(!enginesound)
		{
			enginesound = true;
			Sound("FanLoop.ogg",nil,nil,nil,1);
		}
	}
	else
	{
		if(enginesound == true)
		{
			Sound("FanLoop.ogg",nil,nil,nil,-1);
			enginesound = nil;
		}
	}

	//Control proxy
	if(xthrottle) xtarget = (xthrottle * 12);
	if(ythrottle) ytarget = (ythrottle * 6);
	if(!xthrottle) xtarget = 0;
	if(!ythrottle) ytarget = 0;

	//Wind movement if in the air
	if(!GetContact(-1))
		xtarget = xtarget + GetWind()/10;

	//Fall down if there is no water below nor pilot
	if(!AirshipPilot() && !GBackLiquid(0,26) && !GetContact(-1))
	{
		ytarget = 10;
	}

	//Rise in water
	if(GBackLiquid(0,25)) ytarget = -10;
	if(GBackLiquid(0,25) && !GBackLiquid(0,24) && ytarget > 1) ytarget = 0;

	//Fall to the ground if there is no pilot on board
	if(!AirshipPilot())
	{
		xthrottle = 0;
		ythrottle = 0;
	}

	//x target speed
	if(GetXDir(100) != xtarget * 10)
	{
		if(GetXDir(100) > xtarget * 10) SetXDir(GetXDir(100) -1,100);
		if(GetXDir(100) < xtarget * 10) SetXDir(GetXDir(100) +1,100);
	}

	//y target speed
	if(GetYDir() != ytarget)
	{
		if(GetYDir() > ytarget) SetYDir(GetYDir() -1);
		if(GetYDir() < ytarget) SetYDir(GetYDir() +1);
	}

	//Turn the airship right
	if(animdir == -1 && GetXDir() > 1 && xthrottle == 1)
	{
		StopAnimation(turnanim);
		turnanim = graphic->PlayAnimation("TurnRight", 10, Anim_Linear(0, 0, graphic->GetAnimationLength("TurnRight"), 36, ANIM_Hold), Anim_Const(1000));
		animdir = 1;
	}

	//turn the airship left
	if(animdir == 1 && GetXDir() < -1 && xthrottle == -1)
	{
		StopAnimation(turnanim);
		turnanim = graphic->PlayAnimation("TurnLeft", 10, Anim_Linear(0, 0, graphic->GetAnimationLength("TurnLeft"), 36, ANIM_Hold), Anim_Const(1000));
		animdir = -1;
	}

	//fun debug output stuff
/*	if(!AirshipPilot()) Message(Format("^ 3^|XTAR:%d|YTAR:%d|XDIR:%d|YDIR:%d",xtarget,ytarget,GetXDir(),GetYDir()));
	else
	Message(Format("o _o|XTAR:%d|YTAR:%d|XDIR:%d|YDIR:%d",xtarget,ytarget,GetXDir(),GetYDir())); */
}

/* -- Control Inputs -- */
//These are routed into the Airship's behaviour (in the function above)

func ControlLeft(object clonk)
{
	xthrottle = -1;
}

func ControlRight(object clonk)
{
	xthrottle = 1;
}

func ControlUp(object clonk)
{
	ythrottle = -1;
}

func ControlDown(object clonk)
{
	ythrottle = 1;
}

func ControlStop(object clonk, int control)
{
	if(control == 11 || control == 12) xthrottle = 0;
	if(control == 13 || control == 14) ythrottle = 0;
}

private func AirshipPilot()
{
	//Looks for a clonk within the Gondola
	var clonk = FindObject(Find_ID(Clonk), Find_OCF(OCF_Alive),Find_InRect(-19,0,35,20));
	if(clonk)
		return clonk;
	else
		return false;
}

/* -- Airship Destruction --*/
//This command is called from the hitbox object

func AirshipDeath()
{
	//First let's create the burnt airship
	var burntairship = CreateObject(Airship_Burnt,0,27); //27 pixels down to align ruin with original

	//Now let's copy it's animation, and hold it there
	var animspot;
	animspot = graphic->GetAnimationPosition(turnanim);
	if(turnanim == -1) burntairship->PlayAnimation("TurnLeft", 10, Anim_Const(animspot), Anim_Const(1000));
	else
		burntairship->PlayAnimation("TurnRight", 10, Anim_Const(animspot), Anim_Const(1000));

	//Set ruin on fire
	burntairship->Incinerate();

	//Remove the old graphic (which also stops airship behaviour, how nifty!)
	graphic->RemoveObject();
	//Remove the hitbox
	hitbox->RemoveObject();

	//Make sure engine sound is gone
	Sound("FanLoop.ogg",nil,nil,nil,-1);

	//This object has served its purpose.
	Explode(27);
}


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
local Plane = 275;
