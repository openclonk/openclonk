/**
	Acid Bubble Spawner
	Spawns acid bubbles for some time.

	@author Win
*/

local Name = "Acid Bubble Spawner";

local max_time = 12;
local timer = 0;
local count = 7;

public func Construction()
{
	AddTimer("Boil", 1);
}

/*
	Periodically spawns bubbles until count runs out
*/
private func Boil()
{
	if (++timer > max_time)
	{
		timer = 0;
		var amount = RandomX(1, 3);
		count -= amount;
		
		var bubbles = CastAcidBubbles(amount, RandomX(10, 30), 0, 0);
		for (var bubble in bubbles)
			bubble->SetCon(RandomX(30, 40));
	}
	
	if (!GBackLiquid(0, 0) || count <= 0)
	{
		RemoveObject();
	}
}

// This is a helper object. Do not save!
func SaveScenarioObject() { return false; }
