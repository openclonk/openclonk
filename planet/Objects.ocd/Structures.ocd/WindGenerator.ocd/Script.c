/*-- Wind generator --*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerProducer
#include Library_Flag

local DefaultFlagRadius = 90;

/* Initialisierung */

local wind_anim;
local last_wind;

func TurnAnimation(){return "Turn";}
func MinRevolutionTime(){return 4500;} // in frames

protected func Construction()
{
	SetProperty("MeshTransformation",Trans_Mul(Trans_Rotate(RandomX(-15,15),0,1,0), Trans_Translate(1200,0,0)));
	return _inherited(...);
}

protected func Initialize()
{
	// create wheel
	(this.wheel = CreateObject(WindGenerator_Wheel, 0, 0, NO_OWNER))->Set(this);

	// Start animation
	wind_anim = PlayAnimation(TurnAnimation(), 5, this.wheel->Anim_R(0, GetAnimationLength(TurnAnimation())), Anim_Const(1000));
	
	// Set initial position
	AddTimer("Wind2Turn");
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
	if(this.wheel->Stuck() || this.wheel->HasStopped())
	{
		power = 0;
	}
	else
	{
		power = Abs(this.wheel->GetRDir(this->MinRevolutionTime()/90));
		if(power < 5) power = 0;
		else power = Max(((power + 5) / 25), 1) * 50;
	}

	if(last_wind != power)
	{
		last_wind = power;
		MakePowerProducer(last_wind);
	}

	// adjust wheel speed
	this.wheel->SetRDir(current_wind*90, this->MinRevolutionTime());
	// make sounds
	if (Abs(current_wind) >= 10 && Random(15 - Abs(current_wind / 10)) < 5)
	{
		if (!Random(2))
			Sound("WoodCreak?",false,nil,nil,nil, 75);
		else
			Sound("HingeCreak?",false,nil,nil,nil, 75);
	}
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 60;
local ContactIncinerate = 5;
local HitPoints = 50;
