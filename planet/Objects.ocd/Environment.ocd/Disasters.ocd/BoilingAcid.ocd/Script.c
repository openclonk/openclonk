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
		{	
			var nearby_spawner = FindObject(Find_Distance(RandomX(80, 100), x_rand, y_rand), Find_Property("IsAcidSpawner"));
			if (!nearby_spawner)
			{
				var spawner = CreateObject(Dummy, x_rand, y_rand + random_depth);
				spawner.Boil = this.SpawnerBoil;
				spawner.max_time = 12;
				spawner.timer = 0;
				spawner.count = 7;
				spawner->AddTimer("Boil", 1);
				spawner.IsAcidSpawner = true;
			}
		}
	}
}

private func SpawnerBoil()
{
	if (++this.timer > this.max_time)
	{
		this.timer = 0;
		var amount = RandomX(1, 3);
		this.count -= amount;
		
		var bubbles = this->CastAcidBubbles(amount, RandomX(10, 30), 0, 0);
		for (var bubble in bubbles)
			bubble->SetCon(RandomX(30, 40));
	}
	
	if (!this->GBackLiquid(0, 0) || this.count <= 0)
	{
		this->RemoveObject();
	}
}


public func Definition(def, ...)
{
	_inherited(def, ...);
	def.EditorProps.area.Options[1].Delegate.Color = 0x30ff30;
}
