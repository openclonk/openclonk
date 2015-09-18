/**
	BoilingMagma
	Causes Lava on the map to boil

	@author Win 
*/

local Name = "$Name$";
local Description = "$Description$";

local intensity;
// Magic number by which the total map size (in pixels) is divided to get the amount of tries per frame.
local intensity_quotient = 10000;

public func Place(int amount)
{
	amount = amount ?? 1;
	
	// The amount directly controls the intensity. More objects would not help.
	var obj = CreateObject(this, 0, 0, NO_OWNER);
	obj.intensity *= amount;
	return [obj];
}

public func Construction()
{
	SetPosition(0, 0);
	intensity = GetDefaultIntensity();
	if (intensity <= 0) intensity = 1;
	AddTimer("Boiling", 1);
}

private func GetDefaultIntensity()
{
	return (LandscapeWidth() * LandscapeHeight()) / this.intensity_quotient;
}

private func Boiling()
{
	for(var i = 0; i < intensity; i++)
	{
		// Checks if there is a deep enough pool of lava at a random location of the map, then creates spawner on the surface
		var x_rand = Random(LandscapeWidth());	
		var y_rand = Random(LandscapeHeight());
		var mat = MaterialName(GetMaterial(x_rand, y_rand));
		var above_mat = MaterialName(GetMaterial(x_rand, y_rand - 1));
		var depth_check_math = MaterialName(GetMaterial(x_rand, y_rand + 30));

		if (mat == "DuroLava" || mat == "Lava")
			if (above_mat == nil || above_mat == "Tunnel")
				if (depth_check_math == "DuroLava" || depth_check_math == "Lava")
					if (PathFree(x_rand, y_rand, x_rand, y_rand + 30))
					{
						CreateObject(BoilingLava_Spawner, x_rand, y_rand);
					}
	}
}

/*
	Sets the intensity of the lava spawner. An intensity of 3 means that a new random spawn position is tested 3 times per frame.
*/
public func SetIntensity(int intensity)
{
	this.intensity = intensity ?? GetDefaultIntensity();
	return true;
}

// Save the intensity IFF it is different from the definition's.
public func SaveScenarioObject(props, ...)
{
	if (!_inherited(props, ...)) return false;
	if (this.intensity != GetDefaultIntensity())
		props->AddCall("Intensity", this, "SetIntensity", this.intensity);
	return true;
}
