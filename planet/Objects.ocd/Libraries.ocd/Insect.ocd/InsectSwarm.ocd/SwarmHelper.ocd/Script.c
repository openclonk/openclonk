/*
	For every swarm one of these will be created @0,0 to help manage the swarm.
	Keeps track of the swarm master (aka swarm center).

	@author Clonkonaut
--*/

local swarm_master;
local swarm_count;

private func Initialize()
{
	SetPosition(0,0);
}

public func SetMaster(object new_master)
{
	if (!new_master) return;

	swarm_master = new_master;
}

public func GetMaster()
{
	return swarm_master;
}

public func MakeNewMaster(object next)
{
	if (!next) // Swarm destroyed
		return RemoveObject();
	swarm_count--;
	swarm_master = next;
	swarm_master->SetPreviousInLine(nil);
}

public func SetSwarmCount(int count)
{
	swarm_count = count;
}

public func GetSwarmCenter(proplist coordinates)
{
	coordinates.x = swarm_master->GetX();
	coordinates.y = swarm_master->GetY();
}

/* Saving */

// Place(int amount, int swarm_members, proplist rectangle)
public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	if (!swarm_master) return false;
	// Save as a swarm
	props->RemoveCreation();
	// Could use current swarm spread for area.
	// Seems like overkill. Rectangle is just to ensure placement finds a good location.
	var swarm_range = 50;
	props->Add(SAVEOBJ_Creation, "%v->Place(1, %d, Rectangle(%d, %d, 50, 50))",
		swarm_master->GetID(),
		swarm_count + 1,
		BoundBy(swarm_master->GetX()-swarm_range, 0, LandscapeWidth()-swarm_range),
		BoundBy(swarm_master->GetY()-swarm_range, 0, LandscapeHeight()-swarm_range));
	return true;
}


local Name = "$Name$";
