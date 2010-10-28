// Give the player a new boompack 2.5 seconds after launch.

#appendto Boompack

func Launch(int angle, object clonk)
{
	if (clonk) 
		clonk->AddEffect("RespawnBoom", clonk, 100, 90, nil, nil);
	return _inherited(angle, clonk);
}

