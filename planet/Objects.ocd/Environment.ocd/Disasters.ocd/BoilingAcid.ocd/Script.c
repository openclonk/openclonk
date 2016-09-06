/**
	BoilingAcid
	Causes Acid on the map to bubble

	@author 
*/

#include BoilingLava

local Name = "$Name$";
local Description = "$Description$";

// Magic number by which the total map size (in pixels) is divided to get the amount of tries per frame.
local intensity_quotient = 2500;

private func Boiling()
{
	for (var i = 0; i < intensity; i++)
	{
		// Checks if there is a deep enough pool of acid at a random location of the map, then creates spawner at a random depth into the pool
		area->GetRandomPoint(last_boilpos);
		var x_rand = last_boilpos.x - GetX();
		var y_rand = last_boilpos.y - GetY();
		var mat = MaterialName(GetMaterial(x_rand, y_rand));
		var random_depth = RandomX(30, 100);
		var depth_check_mat = MaterialName(GetMaterial(x_rand, y_rand + random_depth));

		if (mat == "Acid" && depth_check_mat == "Acid")
			if (PathFree(x_rand, y_rand, x_rand, y_rand + random_depth))
			{	
				var nearbySpawner = FindObject(Find_Distance(RandomX(80, 100), x_rand, y_rand), Find_ID(BoilingAcid_Spawner));
			
				if (nearbySpawner == nil)
				{
					CreateObject(BoilingAcid_Spawner, x_rand, y_rand + random_depth);
				}
			}
	}
}

public func Definition(def, ...)
{
	_inherited(def, ...);
	def.EditorProps.area.Options[1].Delegate.Color=0x30ff30;
}
