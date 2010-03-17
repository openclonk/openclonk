/*-- Wind generator --*/

#include Library_PowerGenerator

public func GetCapacity() { return 500; }
public func GetGeneratorPriority() { return 256; }

/* Initialisierung */

local wind_anim;

protected func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(5,25)*(Random(2)*2-1),0,1,0));
	return _inherited(...);
}

protected func Initialize()
{
	wind_anim = PlayAnimation("Turn", 5, Anim_Const(0), Anim_Const(1000));
	// Set initial position
	Wind2Turn();
	return _inherited(...);
}

func Wind2Turn()
{
	DoPower(Abs(GetWind()/3));

	// Fade linearly in time until next timer call
	var start = 0;
	var end = GetAnimationLength("Turn");
	if(GetWind() < 0)
	{
		start = end;
		end = 0;
	}

	// Number of frames for one revolution: the more wind the more
	// revolutions per frame.
	var wind = Abs(GetWind());
	if(wind == 0) wind = 1;
	var l = 4500/wind;

	// Note ending is irrelevant since this is called again after 35 frames
	if(l > 0)
	{
		SetAnimationPosition(wind_anim, Anim_Linear(GetAnimationPosition(wind_anim), start, end, l, ANIM_Loop));
	}
	else
	{
		SetAnimationPosition(wind_anim, Anim_Const(GetAnimationPosition(wind_anim)));
	}
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
