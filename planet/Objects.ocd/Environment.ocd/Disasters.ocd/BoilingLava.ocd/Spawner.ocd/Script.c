/**
	Lava Bubble Spawner
	Spawns lava bubbles for some time.

	@author Win 
*/

local Name = "Lava Bubble Spawner";

local max_time = 2;
local timer = 0;
local count = 10;

public func Construction()
{
	AddTimer("Boil", 1);
}

/*
	Periodically spawns bubbles until count runs out.
*/
private func Boil()
{
	if (++timer > max_time)
	{
		timer = 0;
		var amount = RandomX(1, 3);
		count -= amount;
		
		var bubbles = CastLavaBubbles(amount, RandomX(10, 30), RandomX(-30, 30), 0);
		for (var bubble in bubbles)
			bubble->SetCon(RandomX(105, 115));
		if (count <= 0)
			RemoveObject();
	}
}

// This is a helper object. Do not save!
func SaveScenarioObject() { return false; }
