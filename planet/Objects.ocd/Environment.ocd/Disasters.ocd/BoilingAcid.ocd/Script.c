/**
	BoilingMagma
	Causes Acid on the map to bubble

	@author 
*/

#include BoilingLava

local Name = "$Name$";
local Description = "$Description$";

// Magic number by which the total map size (in pixels) is divided to get the amount of tries per frame.
local intensity_quotient = 50000;

private func Boiling()
{
	for (var i = 0; i < intensity; i++)
	{
		// Checks if there is a deep enough pool of acid at a random location of the map, then creates spawner at a random depth into the pool
		var x_rand = Random(LandscapeWidth());	
		var y_rand = Random(LandscapeHeight());
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
