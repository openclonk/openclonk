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

local Name = "$Name$";