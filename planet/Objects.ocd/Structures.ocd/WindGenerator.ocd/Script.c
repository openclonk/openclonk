/*-- Wind generator --*/

#include Library_Ownable
#include Library_PowerProducer

/* Initialisierung */

local wind_anim;
local last_wind;

func TurnAimation(){return "Turn";}
func MinRevolutionTime(){return 4500;} // in frames

protected func Construction()
{
	SetProperty("MeshTransformation",Trans_Mul(Trans_Rotate(RandomX(-15,15),0,1,0), Trans_Translate(1200,0,0)));
	return _inherited(...);
}

protected func Initialize()
{
	wind_anim = PlayAnimation(TurnAimation(), 5, Anim_Const(0), Anim_Const(1000));
	
	// create wheel
	(this.wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER))->Set(this);
	
	// Set initial position
	Wind2Turn();
	return _inherited(...);
}

// used by the windmill too
// returns the wind weighted over several points - not the absolute value!
func GetWeightedWind()
{
	// hardcoded for performance reasons
	return (
		(10 * (GetWind(-150, -30))) + 
		(25 * (GetWind(-75, -30))) + 
		(30 * (GetWind(0, -30))) + 
		(25 * (GetWind(+75, -30))) + 
		(10 * (GetWind(+150, -30)))
		) / 100;
}

// used by the windmill too
func Wind2Turn()
{
	if(GetCon()  < 100) return;
	
	var current_wind = this->GetWeightedWind();
	var power = 0;
	var stopped = false;
	if(this.wheel->Stuck() || (stopped = this.wheel->HasStopped()))
	{
		current_wind = 0;
		power = 0;
		
		// try if the wheel can turn
		this.wheel->SetRDir(10);
	}
	else
	{
		power = Abs(current_wind);
		if(power < 5) power = 0;
		else power = Max(((power + 5) / 25), 1) * 50;
	}
	if(last_wind != power)
	{
		last_wind = power;
		MakePowerProducer(last_wind);
	}
	// Fade linearly in time until next timer call
	var start = 0;
	var end = GetAnimationLength(this->TurnAimation());
	if(current_wind < 0)
	{
		start = end;
		end = 0;
	}

	// Number of frames for one revolution: the more wind the more
	// revolutions per frame.
	var wind = Abs(current_wind);
	if(wind == 0) wind = 1;
	var l = this->MinRevolutionTime()/wind;
	
	// current animation position
	var animation_position = GetAnimationPosition(wind_anim);
	
	// adjust wheel speed
	if(!stopped)
		this.wheel->SetRotation((100 * animation_position) / Max(start, end), l, BoundBy(current_wind, -1, 1));
	
	// Note ending is irrelevant since this is called again after 35 frames
	if((l > 0) && (wind > 1))
	{
		SetAnimationPosition(wind_anim, Anim_Linear(animation_position, start, end, l, ANIM_Loop));
	}
	else
	{
		SetAnimationPosition(wind_anim, Anim_Const(animation_position));
	}
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description = "$Description$";
