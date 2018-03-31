/**
	Flag Library
	The flagpoles mark the area a player owns. It also serves as an energy transmitter.
	A structure that serves as a flagpole, which is an object which defines an ownership
	radius, should include this library.
	
	The ownership radius can be changed via the function SetFlagRadius(int to_radius).
	The power system uses the flag library to determine which structures belong to the
	same network, which is given by a number of connected flags.
	
	Important notes when including this library:
	 * The object including this library should return _inherited(...) in the
	   Construction, Initialize and Destruction callback if overloaded.
	   
	The flag library and its components Library_Flag_Marker, Library_Ownable, 
	Library_Flag_ConstructionPreviewer and Library_Flag_ConstructionPreviewer_Arrow
	depend on the following other definitions:
	 * ConstructionPreviewer
	
	@author Zapper, Maikel
*/


// A static variable is used to keep track of all flags.
static LIB_FLAG_FlagList;

// Radius of new flag of this type, unless overwritten by SetFlagRadius().
local DefaultFlagRadius = 200;

// All flag related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Construction for which variables are being used.
local lib_flag;

protected func Construction()
{
	// Initialize the flag list if not done already.
	if (GetType(LIB_FLAG_FlagList) != C4V_Array)
		LIB_FLAG_FlagList = [];
	// Make this object also a C4D_Environment for hostility change callbacks.
	SetCategory(GetCategory() | C4D_Environment);
	// Initialize the single proplist for the flag library.
	if (lib_flag == nil)
		lib_flag = {};
	// Set some variables corresponding to this flag.
	lib_flag.construction_time = FrameCounter();
	lib_flag.radius = GetID()->GetFlagRadius();
	lib_flag.range_markers = [];
	lib_flag.linked_flags = [];
	lib_flag.power_helper = nil;
	return _inherited(...);
}

protected func Initialize()
{
	AddOwnership();
	AddEffect("IntFlagMovementCheck", this, 100, 12, this);
	return _inherited(...);
}

protected func Destruction()
{
	// Important: first process other libraries like the power library which removes links
	// from the network and then handle ownership removal.
	_inherited(...);
	RemoveOwnership();
	return;
}

// This object is a flagpole.
public func IsFlagpole() { return true; }


/*-- Library Code --*/

// Redraws the ownership markers of this flag according to the current circumstances.
public func RedrawFlagRadius()
{
	// Debugging logs.
	//Log("FLAG - RedrawFlagRadius(): flag = %v", this);
	// A flag with no radius is not drawn.
	if (!lib_flag.radius)
	{
		ClearFlagMarkers();
		return;
	}
	
	// Make a list of all other flags.
	var other_flags = [];
	for (var flag in LIB_FLAG_FlagList) 
	{
		if (!flag || flag == this) 
			continue;
		PushBack(other_flags, flag);
	}
		
	// Draw the flag border.
	var count = Max(5, lib_flag.radius / 10);
	var marker_index = -1;
	for (var i = 0; i < 360; i += 360 / count)
	{
		++marker_index;

		// Determine the position of the flag marker.
		var x = Sin(i, lib_flag.radius);
		var y = -Cos(i, lib_flag.radius);	
		
		// Check for other flags which were constructed earlier and determine whether to draw the marker.
		var draw = true;
		for (var other_flag in other_flags)
		{
			if (other_flag)
			{
				if (other_flag->HasCoordinatesInControlArea(GetX() + x, GetY() + y))
				{
					// For equal construction times, increase this construction time.
					// TODO: this feels rather hacky?!
					if (other_flag->GetFlagConstructionTime() == GetFlagConstructionTime())
						lib_flag.construction_time += 1;
					
					// Do not draw the marker if the construction time is too large or flags are allied.
					if (other_flag->GetFlagConstructionTime() < GetFlagConstructionTime())
						draw = false;
					if (IsAllied(GetOwner(), other_flag->GetOwner()))
						draw = false;
				}
			}
		}
		// If the marker should not be drawn: move to center and fade out.
		var marker = lib_flag.range_markers[marker_index];
		if (!draw)
		{
			if (marker_index < GetLength(lib_flag.range_markers))
			{
				if (marker)
				{
					marker->FadeOut();
					marker->MoveTo(GetX(), GetY(), -marker->GetR());
				}
			}
			continue;
		}
		if (!marker)
		{
			marker = CreateObject(GetFlagMarkerID(), 0, 0, GetOwner());
			lib_flag.range_markers[marker_index] = marker;
			marker->SetR(Angle(0, 0, x, y));
		}
		marker->FadeIn();
		marker->MoveTo(GetX() + x, GetY() + y, Angle(0, 0, x, y));
	}
	
	// Check whether there were any unnecessary markers.
	if (marker_index < GetLength(lib_flag.range_markers) - 1)
	{
		var old = marker_index;
		while (++marker_index < GetLength(lib_flag.range_markers))
		{
			var marker = lib_flag.range_markers[marker_index];
			marker->RemoveObject();
			lib_flag.range_markers[marker_index] = nil;
		}
		SetLength(lib_flag.range_markers, old + 1);
	}
	return;
}

// Callback from the global function that determines the ownership for a position.
// The coordinates x and y are global.
public func HasCoordinatesInControlArea(int x, int y)
{
	return Distance(x, y, GetX(), GetY()) <= GetFlagRadius();
}

// Removes all the ownership markers for this flag.
private func ClearFlagMarkers()
{
	for (var marker in lib_flag.range_markers)
		if (marker) 
			marker->RemoveObject();
	lib_flag.range_markers = [];
	return;
}

// Changes the ownership of the structures within this flag's radius.
private func RefreshOwnershipOfSurrounding()
{
	for (var obj in FindObjects(Find_Distance(lib_flag.radius), Find_Func("CanBeOwned")))
	{
		var owner = GetOwnerOfPosition(AbsX(obj->GetX()), AbsY(obj->GetY()));
		if (obj->GetOwner() == owner)
			continue;
		obj->SetOwner(owner);
	}
	return;
}

private func AddOwnership()
{
	// Debugging logs.
	//Log("FLAG - AddOwnership(): flag = %v", this);
	// Add this flag to the global list of flags.
	if (GetIndexOf(LIB_FLAG_FlagList, this) == -1)
		PushBack(LIB_FLAG_FlagList, this);
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	// Linked flags - refresh links for this flag and update the power system.
	RefreshLinkedFlags();
	return;
}

private func RemoveOwnership()
{
	// Debugging logs.
	//Log("FLAG - RemoveOwnership(): flag = %v", this);
	// Remove all the flag markers.
	ClearFlagMarkers();
	// Remove the flag from the global flag list.
	RemoveArrayValue(LIB_FLAG_FlagList, this);
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	// Linked flags - refresh links for this flag and update the power system.
	RefreshLinkedFlags();
	return;
}

protected func FxIntFlagMovementCheckStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.moving = false;
	effect.Interval = 12;
	effect.x = target->GetX();
	effect.y = target->GetY();
	return FX_OK;
}

protected func FxIntFlagMovementCheckTimer(object target, proplist effect)
{
	// Check if flag started moving.
	if (!effect.moving)
	{
		if (effect.x != target->GetX() || effect.y != target->GetY())
		{
			effect.moving = true;
			RemoveOwnership();
		}	
	}
	// Check if flag stopped moving.
	else
	{
		if (effect.x == target->GetX() && effect.y == target->GetY())
		{
			effect.moving = false;
			AddOwnership();
		}	
	}
	// Update coordinates.
	effect.x = target->GetX();
	effect.y = target->GetY();
	return FX_OK;
}

// Returns all flags allied to owner of which the radius intersects the given circle.
public func FindFlagsInRadius(object center_object, int radius, int owner)
{
	var flag_list = [];
	if (LIB_FLAG_FlagList)
	{
		for (var flag in LIB_FLAG_FlagList)
		{
			if (!IsAllied(flag->GetOwner(), owner)) 
				continue;
			if (flag == center_object) 
				continue;
			if (ObjectDistance(center_object, flag) > radius + flag->GetFlagRadius()) 
				continue;
			PushBack(flag_list, flag);
		}
	}
	return flag_list;
}

// Refreshes the linked flags for this flags and also updates the linked flags of the flages linked to this flag.
// TODO: Maybe there is a need to update the links of the flags which were linked before but are not now.
public func RefreshLinkedFlags()
{
	// Debugging logs.
	//Log("FLAG - RefreshLinkedFlags(): flag = %v", this);
	//LogFlags();
	
	// Safety check: the global flag list should exist.
	if (GetType(LIB_FLAG_FlagList) != C4V_Array) 
		return;
		
	// Construct a list fo currently linked flags (to this flag).
	var current_linked_flags = [];
	var owner = GetOwner();
	// Do this by iterating over all directly linked flags and go outward from this flag.
	var iterate_flags = [this];
	// Once the list of iterated flags is empty we are done.
	while (GetLength(iterate_flags))
	{
		// Store all the flags found in the last iteration which are not this flag.
		for (var flag in iterate_flags) 
			if (flag != this)
				PushBack(current_linked_flags, flag);
		// Find the new iteration of flags which are connected to the flags in the previous iteration.
		var previous_iterate_flags = iterate_flags;
		iterate_flags = [];
		for (var prev_flag in previous_iterate_flags)
		{
			for (var flag in LIB_FLAG_FlagList)
			{
				if (!flag)
					continue;				
				// A new connected flag must be allied.
				if (!IsAllied(flag->GetOwner(), owner)) 
					continue;
				// And must not be an already found flag or this flag.
				if (GetIndexOf(current_linked_flags, flag) != -1 || flag == this) 
					continue;
				// Neither may it be an already found flag in this loop.	
				if (GetIndexOf(iterate_flags, flag) != -1) 
					continue;
				// Last, check whether the new flag is really connected to the previous flag.
				if (ObjectDistance(prev_flag, flag) > prev_flag->GetFlagRadius() + flag->GetFlagRadius()) 
					continue;
				PushBack(iterate_flags, flag);
			}
		}
	}
	
	// Update the linked flags of this flag.
	lib_flag.linked_flags = current_linked_flags;
	
	// Update the linked flags for all other linked flags as well.
	for (var other in lib_flag.linked_flags)
		other->CopyLinkedFlags(this, lib_flag.linked_flags);

	// Since the connected flags have been updated it is necessary to update the power helper as well.
	// Create a new power network for ths flag since we don't know whether flag links have been lost.
	// We then just possibly remove the old ones if they exist.
	SetPowerHelper(GetPowerSystem()->CreateNetwork(), true, false);
	// Now merge all networks into the newly created network.
	GetPowerSystem()->RefreshAllPowerNetworks();
	// Debugging logs.
	//LogFlags();
	return;
}

// Copy the linked flags from another flag (from) and its flaglist.
public func CopyLinkedFlags(object from, array flaglist)
{
	lib_flag.linked_flags = flaglist[:];
	for (var i = GetLength(lib_flag.linked_flags) - 1; i >= 0; --i)
		if (lib_flag.linked_flags[i] == this)
			lib_flag.linked_flags[i] = from;
	return;
}

// Engine callback: owner of the flag has changed.
protected func OnOwnerChanged(int new_owner, int old_owner)
{
	// Debugging logs.
	//Log("FLAG - OnOwnerChanged(): flag = %v, new_owner = %d, old_owner = %d", this, new_owner, old_owner);
	// Reassign owner of flag markers for correct color.
	for (var marker in lib_flag.range_markers)
	{
		if (!marker) 
			continue;
		marker->SetOwner(new_owner);
		marker->ResetColor();
	}
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	// Also change the ownership of the surrounding buildings.
	RefreshOwnershipOfSurrounding();
	// Linked flags - refresh links for this flag.
	RefreshLinkedFlags();
	return _inherited(new_owner, old_owner, ...);
}

// Engine callback: a player has changed its hostility.
protected func OnHostilityChange(int player1, int player2, bool hostile, bool old_hostility)
{
	// Debugging logs.
	//Log("FLAG - OnHostilityChange(): flag = %v, player1 = %d, player2 = %d, hostile = %v, old_hostility = %v", this, player1, player2, hostile, old_hostility);
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	// Linked flags - refresh links for this flag.
	RefreshLinkedFlags();
	return _inherited(player1, player2, hostile, old_hostility);
}

// Engine callback: a player has switched its team.
protected func OnTeamSwitch(int player, int new_team, int old_team)
{
	// Debugging logs.
	//Log("FLAG - OnTeamSwitch(): flag = %v, player = %d, new_team = %d, old_team = %d", this, player, new_team, old_team);
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	// Linked flags - refresh links for this flag.
	RefreshLinkedFlags();		
	return _inherited(player, new_team, old_team, ...);
}


/*-- Construction --*/

// Callback from construction library: create a special preview that gives extra info about affected buildings / flags.
public func CreateConstructionPreview(object constructing_clonk)
{
	// Return the specific previewer for the flag.
	return CreateObjectAbove(Library_Flag_ConstructionPreviewer, constructing_clonk->GetX() - GetX(), constructing_clonk->GetY() - GetY(), constructing_clonk->GetOwner());
}


/*-- Flag properties --*/

public func SetFlagRadius(int to_radius)
{
	lib_flag.radius = to_radius;
	
	// Redraw the flag markers.
	RedrawAllFlagRadiuses();
	
	// Refresh the ownership in the new area.
	RefreshOwnershipOfSurrounding();	
	return;
}

public func GetFlagRadius()
{
	if (lib_flag)
		return lib_flag.radius;
 	return DefaultFlagRadius;
}

public func GetFlagConstructionTime() { return lib_flag.construction_time; }

public func GetFlagMarkerID() { return Library_Flag_Marker; }

public func GetLinkedFlags() {return lib_flag.linked_flags; }


/*-- Power System --*/

public func GetPowerHelper() { return lib_flag.power_helper; }

public func SetPowerHelper(object to, bool update_linked_flags, bool report_inconsistency) 
{
	var old_network = GetPowerHelper();
	lib_flag.power_helper = to;
	// Update linked flags
	if (update_linked_flags)
	{
		for (var linked_flag in GetLinkedFlags())
		{
			if (!linked_flag)
				continue;
			// Assert different power helpers for the same network.
			if (report_inconsistency && linked_flag->GetPowerHelper() != old_network)
			{
				FatalError("Flags in the same network have different power helpers.");
			}
			linked_flag->SetPowerHelper(to);
		}
	}
	return;
}


/*-- Logging --*/

private func LogFlags()
{
	for (var flag in LIB_FLAG_FlagList)
	{
		Log("FLAG - State for flag (%v): owner = %d, con_time = %d, radius = %d, power_network = %v", flag, flag->GetOwner(), flag->GetFlagConstructionTime(), flag->GetFlagRadius(), flag->GetPowerHelper());
		Log("\tlinked flags = %v", flag->GetLinkedFlags());
	}
	return;
}


/*-- Saving --*/

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	// Category is always set to StaticBack.
	props->Remove("Category");
	// Store the flag radius correctly.
	if (lib_flag && lib_flag.radius != DefaultFlagRadius) 
		props->AddCall("Radius", this, "SetFlagRadius", lib_flag.radius);
	return true;
}
