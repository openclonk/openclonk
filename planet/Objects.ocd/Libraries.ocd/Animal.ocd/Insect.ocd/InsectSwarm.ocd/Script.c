/**
	Insect Swarm Library
	Superimpose to any (flying) insect to create an insect swarm.

	@author Clonkonaut
--*/

#include Library_Insect

// Places amount swarms with swarm_members individuals each
public func Place(int amount, int swarm_members, proplist area)
{
	// No calls to objects, only definitions
	if (GetType(this) == C4V_C4Object) return;
	if (!swarm_members)
		swarm_members = lib_swarm_standard;

	for (var insect in _inherited(amount, area))
		insect->CreateSwarm(swarm_members - 1); // -1 because one insect already exists
}

/** Call something for every swarm member
*/
public func SwarmCall(string fn, par1, par2, par3, par4, par5, par6, par7, par8, par9)
{
	if (!lib_swarm_helper) return;
	lib_swarm_helper->SwarmCall(fn, par1, par2, par3, par4, par5, par6, par7, par8, par9);
}

/** Standard swarm size (utilised in Place()). Default 10.
*/
local lib_swarm_standard = 10;

/** Maximum distance one individual can be away from another. Default 10 pixels.
*/
local lib_swarm_density = 10;

// The managing object (see SwarmHelper.ocd)
local lib_swarm_helper;
// The next insect that gets promoted to swarm master when the swarm master dies
local lib_swarm_nextinline;
// The previous insect that gets promoted to swarm master before this one
local lib_swarm_previnline;

/** Promotes this to swarm master and will create amount minions
*/
public func CreateSwarm(int amount)
{
	if (amount <= 0) return;

	// Create a swarm helper
	lib_swarm_helper = CreateObject(Library_Swarm_Helper, AbsX(0), AbsY(0), NO_OWNER);
	lib_swarm_helper->SetMaster(this);
	lib_swarm_helper->SetSwarmCount(amount);

	var last_created = this, insect;
	while (amount)
	{
		insect = CreateObject(GetID(), 0, 0, GetOwner());
		insect->SetPreviousInLine(last_created);
		insect->SetSwarmHelper(lib_swarm_helper);
		insect->SetCommand("None");
		last_created->SetNextInLine(insect);
		last_created = insect;
		amount--;
	}

	return lib_swarm_helper;
}

public func SetNextInLine(object next)
{
	lib_swarm_nextinline = next;
}

public func SetPreviousInLine(object next)
{
	lib_swarm_previnline = next;
}

public func SetSwarmHelper(object helper)
{
	lib_swarm_helper = helper;
	lib_swarm_helper->AddSwarmMember(this);
}

// Swarm insect need randomized activities to not look odd
private func Initialize()
{
	AddTimer("Activity", 10 + Random(25));
	SetComDir(COMD_None);
}

// On death or destruction we need to update the line
private func Death()
{
	// Death of the master
	if (lib_swarm_helper)
		if (lib_swarm_helper->GetMaster() == this)
			lib_swarm_helper->MakeNewMaster(lib_swarm_nextinline);
	// Death of a slave
	if (lib_swarm_previnline && lib_swarm_nextinline)
	{
		lib_swarm_nextinline->SetPreviousInLine(lib_swarm_previnline);
		lib_swarm_previnline->SetNextInLine(lib_swarm_nextinline);
	}
	PurgeLine(); // Don't do everything twice in case Destruction() follows
	_inherited(...);
}

private func Destruction()
{
	// Destruction of the master
	if (lib_swarm_helper)
		if (lib_swarm_helper->GetMaster() == this)
			lib_swarm_helper->MakeNewMaster(lib_swarm_nextinline);
	// Destruction of a slave
	if (lib_swarm_previnline && lib_swarm_nextinline)
	{
		lib_swarm_nextinline->SetPreviousInLine(lib_swarm_previnline);
		lib_swarm_previnline->SetNextInLine(lib_swarm_nextinline);
	}
}

private func PurgeLine()
{
	lib_swarm_nextinline = nil;
	lib_swarm_previnline = nil;
}

private func MoveToTarget()
{
	if (!lib_swarm_helper || (lib_swarm_helper->GetMaster() == this))
		return _inherited(...);

	var x, y;
	// Follow previous in line
	if (lib_swarm_previnline && Random(3))
	{
		x = lib_swarm_previnline->GetX();
		y = lib_swarm_previnline->GetY();
	}
	else // Go wild and stay around the master!
	{
		var point = {};
		lib_swarm_helper->GetSwarmCenter(point);
		x = point.x;
		y = point.y;
	}
	x = BoundBy(x + Random(lib_swarm_density * 2) - lib_swarm_density, 10, LandscapeWidth() - 10);
	y = BoundBy(y + Random(lib_swarm_density * 2) - lib_swarm_density, 10, LandscapeHeight() - 10);
	// Only move if there is no solid or liquid at this coordinate, otherwise just wait for next activity update.
	if (GBackSemiSolid(x - GetX(), y - GetY()))
		return;	
	SetCommand("MoveTo", nil, x, y, nil, true);
	AppendCommand("Call", this, nil, nil, nil, nil, "MissionComplete");
	return;
}

/* Saving */

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...)) return false;
	// Do not save swarms - this will be done by the swarm helper instad
	return !lib_swarm_helper;
}
