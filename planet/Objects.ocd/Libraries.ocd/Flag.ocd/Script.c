/**
	Flag Library
	The flagpoles mark the area a player owns. It also serves as an energy transmitter.
	
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
	// Initialize the single proplist for the power library.
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

private func RefreshAllFlagLinks()
{
	// Schedule a refresh for all flags.
	for (var flag in LIB_FLAG_FlagList) 
		if (flag)
			flag->ScheduleRefreshLinkedFlags();
	
	// Update power balance for power helpers after refreshing the linked flags.
	// TODO: find out why this is called twice in the same frame in different ways.
	Schedule(nil, "Library_Flag->RefreshAllPowerNetworks()", 2, 0);
	AddEffect("ScheduleRefreshAllPowerNetworks", nil, 1, 2, nil, Library_Flag);
	
}

protected func FxScheduleRefreshAllPowerNetworksTimer()
{
	Library_Flag->RefreshAllPowerNetworks();
	return -1;
}

public func RedrawFlagRadius()
{
	// A flag with no radius is not drawn.
	if (!lib_flag.radius)
	{
		ClearFlagMarkers();
		return;
	}
	
	//var flags = FindObjects(Find_ID(FlagPole),Find_Exclude(target), Find_Not(Find_Owner(GetOwner())), /*Find_Distance(FLAG_DISTANCE*2 + 10,0,0)*/Sort_Func("GetLifeTime"));
	// Make a list of all other flags.
	var other_flags = [];
	var i = 0;
	for (var flag in LIB_FLAG_FlagList) 
		if (flag)
		{
			//if(f->GetOwner() == GetOwner()) continue;
			if (flag == this) 
				continue;
			other_flags[i++] = flag;
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
				if (Distance(GetX() + x, GetY() + y, other_flag->GetX(), other_flag->GetY()) <= other_flag->GetFlagRadius())
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
		
		if (!draw)
		{
			if (marker_index < GetLength(lib_flag.range_markers))
				if (lib_flag.range_markers[marker_index])
				{
					lib_flag.range_markers[marker_index]->FadeOut();
					lib_flag.range_markers[marker_index]->MoveTo(GetX(), GetY(), -lib_flag.range_markers[marker_index]->GetR());
				}
			continue;
		}
		var marker = lib_flag.range_markers[marker_index];
		if (!marker)
		{
			marker = CreateObject(GetFlagMarkerID(), 0, 0, GetOwner());
			marker->SetR(Angle(0, 0, x, y));
		}
		marker->FadeIn();
		marker->MoveTo(GetX() + x, GetY() + y, Angle(0, 0, x, y));
		lib_flag.range_markers[marker_index] = marker;
	}
	
	// there were unnecessary markers?
	if(marker_index < GetLength(lib_flag.range_markers) - 1)
	{
		var old = marker_index;
		while(++marker_index < GetLength(lib_flag.range_markers))
		{
			var marker = lib_flag.range_markers[marker_index];
			marker->RemoveObject();
			lib_flag.range_markers[marker_index] = nil;
		}
		SetLength(lib_flag.range_markers, old + 1);
	}
	
	return true;
}

func RefreshOwnershipOfSurrounding()
{
	for(var obj in FindObjects(Find_Distance(lib_flag.radius), Find_Func("CanBeOwned")))
	{
		var o = GetOwnerOfPosition(AbsX(obj->GetX()), AbsY(obj->GetY()));
		if(obj->GetOwner() == o) continue;
		var old = obj->GetOwner();
		obj->SetOwner(o);
	}
}

private func AddOwnership()
{
	if (GetIndexOf(LIB_FLAG_FlagList, this) == -1)
		LIB_FLAG_FlagList[GetLength(LIB_FLAG_FlagList)] = this;

	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	
	// Linked flags - optimization for the power system.
	RefreshAllFlagLinks();
	return;
}

private func RemoveOwnership()
{
	ClearFlagMarkers();
	
	// Remove the flag from the global flag list.
	for (var i = 0; i < GetLength(LIB_FLAG_FlagList); ++i)
	{
		if (LIB_FLAG_FlagList[i] != this) 
			continue;
		LIB_FLAG_FlagList[i] = LIB_FLAG_FlagList[GetLength(LIB_FLAG_FlagList)-1];
		SetLength(LIB_FLAG_FlagList, GetLength(LIB_FLAG_FlagList)-1);
		break;
	}
	
	// Redraw radiuses of all flags.
	RedrawAllFlagRadiuses();
	
	// Refresh the ownership of the flag's surroundings.
	RefreshOwnershipOfSurrounding();
	
	// Linked flags - optimization for the power system.
	RefreshAllFlagLinks();
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

public func ScheduleRefreshLinkedFlags()
{
	if (!GetEffect("RefreshLinkedFlags", this)) 
		AddEffect("RefreshLinkedFlags", this, 1, 1, this);
	return;
}

public func StopRefreshLinkedFlags()
{
	if (GetEffect("RefreshLinkedFlags", this))
		RemoveEffect("RefreshLinkedFlags", this);
	return;
}

protected func FxRefreshLinkedFlagsTimer()
{
	this->RefreshLinkedFlags();
	return -1;
}

// Returns all flags allied to owner of which the radius intersects the given circle
func FindFlagsInRadius(object center_object, int radius, int owner)
{
	var flag_list = [];
	if (LIB_FLAG_FlagList) for(var flag in LIB_FLAG_FlagList)
	{
		if(!IsAllied(flag->GetOwner(), owner)) continue;
		if(flag == center_object) continue;
		if(ObjectDistance(center_object, flag) > radius + flag->GetFlagRadius()) continue;
		flag_list[GetLength(flag_list)] = flag;
	}
	return flag_list;
}

public func RefreshLinkedFlags()
{
	// failsafe - the array should exist
	if(GetType(LIB_FLAG_FlagList) != C4V_Array) return;
	
	var current = [];
	var new = [this];
	
	var owner = GetOwner();
	
	while (GetLength(new))
	{
		for (var f in new) 
			if(f != this) 
				current[GetLength(current)] = f;
		var old = new;
		new = [];
		
		for (var oldflag in old)
			for (var flag in LIB_FLAG_FlagList)
			{
				if(!IsAllied(flag->GetOwner(), owner)) continue;
				if(GetIndexOf(current, flag) != -1) continue;
				if(GetIndexOf(new, flag) != -1) continue;
				if(flag == this) continue;
				
				if(ObjectDistance(oldflag, flag) > oldflag->GetFlagRadius() + flag->GetFlagRadius()) continue;
				
				new[GetLength(new)] = flag;
			}
	}
	
	lib_flag.linked_flags = current;
	
	// update flag links for all linked flags - no need for every flag to do that
	// meanwhile, adjust power helper. Merge if necessary
	// since we don't know whether flag links have been lost we will create a new power helper and possibly remove old ones
	Library_Power->Init(); // make sure the power system is set up
	var old = lib_flag.power_helper;
	lib_flag.power_helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
	LIB_POWR_Networks[GetLength(LIB_POWR_Networks)] = lib_flag.power_helper;
	
	// list of helpers yet to merge
	var to_merge = [old];
	
	// copy from me
	lib_flag.linked_flags = current;
	for (var other in lib_flag.linked_flags)
	{
		other->CopyLinkedFlags(this, lib_flag.linked_flags);
		
		if (GetIndexOf(to_merge, other.lib_flag.power_helper) == -1)
			to_merge[GetLength(to_merge)] = other.lib_flag.power_helper;
		other.lib_flag.power_helper = lib_flag.power_helper;
	}
	
	// for every object in to_merge check if it actually (still) belongs to the group
	for (var network in to_merge)
	{
		if (network == nil)
			continue;
		RefreshPowerNetwork(network);
	}
}

public func CopyLinkedFlags(object from, array flaglist)
{
	lib_flag.linked_flags = flaglist[:];
	for (var i = GetLength(lib_flag.linked_flags)-1; i >= 0; --i)
		if (lib_flag.linked_flags[i] == this)
			lib_flag.linked_flags[i] = from;
	StopRefreshLinkedFlags();
	return;
}

private func ClearFlagMarkers()
{
	for(var obj in lib_flag.range_markers)
		if (obj) obj->RemoveObject();
	lib_flag.range_markers = [];
}

// Engine callback: reassign owner of flag markers for correct color.
protected func OnOwnerChanged(int new_owner, int old_owner)
{
	for (var marker in lib_flag.range_markers)
	{
		if (!marker) 
			continue;
		marker->SetOwner(new_owner);
		marker->ResetColor();
	}
	return _inherited(new_owner, old_owner, ...);
}

// Callback from construction library: create a special preview that gives extra info about affected buildings / flags.
public func CreateConstructionPreview(object constructing_clonk)
{
	CreateObjectAbove(Library_Flag_ConstructionPreviewer, constructing_clonk->GetX()-GetX(), constructing_clonk->GetY()-GetY(), constructing_clonk->GetOwner());
	return;
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

public func GetFlagMarkerID() { return LibraryFlag_Marker; }

public func GetLinkedFlags() {return lib_flag.linked_flags; }


/*-- Power System --*/

public func GetPowerHelper() { return lib_flag.power_helper; }

public func SetPowerHelper(object to) 
{
	lib_flag.power_helper = to; 
	return;
}

// Refreshes all power networks (Library_Power objects).
public func RefreshAllPowerNetworks()
{
	// Don't do anything if there are no power helpers created yet.
	if (GetType(LIB_POWR_Networks) != C4V_Array)
		return;
	
	// Special handling for neutral networks of which there should be at most one.
	for (var network in LIB_POWR_Networks)
	{
		if (!network || !network.lib_power.neutral_network) 
			continue;
		RefreshPowerNetwork(network);
		break;
	}
	
	// Do the same for all other helpers: delete / refresh.
	for (var index = GetLength(LIB_POWR_Networks) - 1; index >= 0; index--)
	{
		var network = LIB_POWR_Networks[index];
		if (!network) 
			continue;
		
		if (network->IsEmpty())
		{
			network->RemoveObject();
			RemoveArrayIndex(LIB_POWR_Networks, index);
			continue;
		}
		//network->CheckPowerBalance();
	}
	return;
}

private func RefreshPowerNetwork(object network)
{
	// Merge all the producers and consumers into their actual networks.
	for (var link in Concatenate(network.lib_power.idle_producers, network.lib_power.active_producers))
	{
		if (!link)
			continue;
		var actual_network = Library_Power->GetPowerNetwork(link.obj);
		if (!actual_network || actual_network == network)
			continue;
		// Remove from old network and add to new network.
		network->RemovePowerProducer(link.obj);
		actual_network->AddPowerProducer(link.obj, link.prod_amount, link.priority);
	}
	for (var link in Concatenate(network.lib_power.waiting_consumers, network.lib_power.active_consumers))
	{
		if (!link)
			continue;
		var actual_network = Library_Power->GetPowerNetwork(link.obj);
		if (!actual_network || actual_network == network)
			continue;
		// Remove from old network and add to new network.
		network->RemovePowerConsumer(link.obj);
		actual_network->AddPowerConsumer(link.obj, link.cons_amount, link.priority);
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
