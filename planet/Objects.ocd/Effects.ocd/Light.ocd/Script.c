/*-- Light --*/

local Visibility = VIS_None;
local Name = "$Name$";

// Light types to be used for light_type parameter in CreateLight
local LGT_Constant = 0;
local LGT_Blast    = 1; // light up and remove again after some time
local LGT_Temp     = 2; // light up and remove again after some time
local range, ltype, t;

global func CreateLight(int x, int y, int range, int light_type, player, int fadeout, int time)
{
	// create light object. player may be nil
	var light = CreateObjectAbove(Fx_Light, x, y, player);
	if (light) light->Init(range, light_type, fadeout, time);
	return light;
}

func Init(int lrange, int light_type, int fadeout, int time)
{
	// Init depending on type
	range = lrange;
	ltype = light_type;
	t = 0;
	if (ltype == LGT_Constant)
	{
		// Just a fixed light
		SetLightRange(range);
	}
	else if (ltype == LGT_Blast)
	{
		SetLightRange(range);
		time = Sqrt(range)+10;
		ScheduleCall(this, Global.RemoveObject, time, 1);
	}
	else if (ltype == LGT_Temp)
	{
		SetLightRange(range, fadeout);
		ScheduleCall(this, Global.RemoveObject, time, 1);
	}
	else
	{
		// Invalid light type
		FatalError(Format("Light init: Invalid light type %v", light_type));
		return false;
	}
}

// Temp or helper object not stored
func SaveScenarioObject() { return false; }

