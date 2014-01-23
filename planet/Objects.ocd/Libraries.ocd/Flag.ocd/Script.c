/*
	The flagpoles mark the area a player owns.
	It also serves as an energy transmitter.
*/

static LibraryFlag_flag_list;

local DefaultFlagRadius = 200; // Radius of new flag of this type, unless overwritten by SetFlagRadius().
local lflag;

public func IsFlagpole(){return true;}

func RefreshAllFlagLinks()
{
	for(var f in LibraryFlag_flag_list) if (f)
	{
		f->ScheduleRefreshLinkedFlags();
	}
	
	// update power balance for power helpers after refreshing the linked flags
	Schedule(nil, "Library_Flag->RefreshAllPowerHelpers()", 2, 0);
	AddEffect("ScheduleRefreshAllPowerHelpers", nil, 1, 2, nil, Library_Flag);
}

func FxScheduleRefreshAllPowerHelpersTimer()
{
	Library_Flag->RefreshAllPowerHelpers();
	return -1;
}

func RefreshAllPowerHelpers()
{
	// no power helpers created yet
	if(GetType(Library_Power_power_compounds) != C4V_Array)
		return;
	
	// special handling for neutral
	var neutral = nil;
	for(var obj in Library_Power_power_compounds)
	{
		if(!obj || !obj.neutral) continue;
		neutral = obj;
		break;
	}
	
	if(neutral)
	{
		RefreshPowerHelper(neutral);
	}
	
	// same for all helpers - delete / refresh
	for(var i = GetLength(Library_Power_power_compounds); --i >= 0;)
	{
		var obj = Library_Power_power_compounds[i];
		if (!obj) continue;
		if(GetLength(obj.power_links) == 0 && GetLength(obj.sleeping_links) == 0)
		{
			obj->RemoveObject();
			Library_Power_power_compounds[i] = Library_Power_power_compounds[GetLength(Library_Power_power_compounds) - 1];
			SetLength(Library_Power_power_compounds, GetLength(Library_Power_power_compounds) - 1);
			continue;
		}
		
		obj->CheckPowerBalance();
	}
}

func RedrawFlagRadius()
{
	//ClearFlagMarkers();
	
	//var flags = FindObjects(Find_ID(FlagPole),Find_Exclude(target), Find_Not(Find_Owner(GetOwner())), /*Find_Distance(FLAG_DISTANCE*2 + 10,0,0)*/Sort_Func("GetLifeTime"));
	var other_flags = [];
	var i = 0;
	for(var f in LibraryFlag_flag_list) if (f)
	{
		//if(f->GetOwner() == GetOwner()) continue;
		if(f == this) continue;
		other_flags[i++] = f;
	}
	// inner border
	var count = Max(5, lflag.radius / 10);
	var marker_index = -1;
	for(var i=0; i<360; i+= 360 / count)
	{
		++marker_index;
		var draw=true;
		var f=nil;
		var x= Sin(i, lflag.radius);
		var y=-Cos(i, lflag.radius);	
		//var inEnemy = false;
		
		for(var f in other_flags) if (f)
		{
			if(Distance(GetX()+x,GetY()+y,f->GetX(),f->GetY()) <= f->GetFlagRadius())
			{
				if(f->GetFlagConstructionTime() == GetFlagConstructionTime())
					lflag.construction_time += 1;
					
				if(f->GetFlagConstructionTime() < GetFlagConstructionTime())
				{	
					draw=false;
				}
				//else inEnemy=true;
				if(IsAllied(GetOwner(), f->GetOwner()))
					draw = false;
			}
		}
		
		if(!draw)
		{
			if(marker_index < GetLength(lflag.range_markers))
				if(lflag.range_markers[marker_index])
				{
					lflag.range_markers[marker_index]->FadeOut();
					lflag.range_markers[marker_index]->MoveTo(GetX(), GetY(), -lflag.range_markers[marker_index]->GetR());
				}
			continue;
		}
		var marker = lflag.range_markers[marker_index];
		if(!marker)
		{
			marker = CreateObject(GetFlagMarkerID(), 0, 0, GetOwner());
			marker->SetR(Angle(0, 0, x, y));
		}
		marker->FadeIn();
		marker->MoveTo(GetX() + x, GetY() + y, Angle(0, 0, x, y));
		lflag.range_markers[marker_index] = marker;
	}
	
	// there were unnecessary markers?
	if(marker_index < GetLength(lflag.range_markers) - 1)
	{
		var old = marker_index;
		while(++marker_index < GetLength(lflag.range_markers))
		{
			var marker = lflag.range_markers[marker_index];
			marker->RemoveObject();
			lflag.range_markers[marker_index] = nil;
		}
		SetLength(lflag.range_markers, old + 1);
	}
}

func RefreshOwnershipOfSurrounding()
{
	for(var obj in FindObjects(Find_Distance(lflag.radius), Find_Func("CanBeOwned")))
	{
		var o = GetOwnerOfPosition(AbsX(obj->GetX()), AbsY(obj->GetY()));
		if(obj->GetOwner() == o) continue;
		var old = obj->GetOwner();
		obj->SetOwner(o);
	}
}
public func Initialize()
{
	// no falling down anymore
	SetCategory(C4D_StaticBack);
	
	if(GetIndexOf(LibraryFlag_flag_list, this) == -1)
		LibraryFlag_flag_list[GetLength(LibraryFlag_flag_list)] = this;

	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	// linked flags - optimization for power system
	RefreshAllFlagLinks();
	
	return _inherited(...);
}

public func Construction()
{
	if(GetType(LibraryFlag_flag_list) != C4V_Array)
		LibraryFlag_flag_list = [];
	
	lflag =
	{
		construction_time = FrameCounter(),
		radius = GetID()->GetFlagRadius(),
		range_markers = [],
		linked_flags = [],
		power_helper = nil
	};
		
	return _inherited(...);
}

public func Destruction()
{
	ClearFlagMarkers();
	
	// remove from global array
	for(var i = 0; i < GetLength(LibraryFlag_flag_list); ++i)
	{
		if(LibraryFlag_flag_list[i] != this) continue;
		LibraryFlag_flag_list[i] = LibraryFlag_flag_list[GetLength(LibraryFlag_flag_list)-1];
		SetLength(LibraryFlag_flag_list, GetLength(LibraryFlag_flag_list)-1);
		break;
	}
	
	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	// refresh all flag links
	RefreshAllFlagLinks();
	
	return _inherited(...);
}

func ScheduleRefreshLinkedFlags()
{
	if(GetEffect("RefreshLinkedFlags", this)) return;
	AddEffect("RefreshLinkedFlags", this, 1, 1, this);
}

func StopRefreshLinkedFlags()
{
	if(!GetEffect("RefreshLinkedFlags", this)) return;
	RemoveEffect("RefreshLinkedFlags", this);
}

func FxRefreshLinkedFlagsTimer()
{
	this->RefreshLinkedFlags();
	return -1;
}

// Returns all flags allied to owner of which the radius intersects the given circle
func FindFlagsInRadius(object center_object, int radius, int owner)
{
	var flag_list = [];
	if (LibraryFlag_flag_list) for(var flag in LibraryFlag_flag_list)
	{
		if(!IsAllied(flag->GetOwner(), owner)) continue;
		if(flag == center_object) continue;
		if(ObjectDistance(center_object, flag) > radius + flag->GetFlagRadius()) continue;
		flag_list[GetLength(flag_list)] = flag;
	}
	return flag_list;
}

func RefreshLinkedFlags()
{
	// failsafe - the array should exist
	if(GetType(LibraryFlag_flag_list) != C4V_Array) return;
	
	var current = [];
	var new = [this];
	
	var owner = GetOwner();
	
	while(GetLength(new))
	{
		for(var f in new) if(f != this) current[GetLength(current)] = f;
		var old = new;
		new = [];
		
		for(var oldflag in old)
		for(var flag in LibraryFlag_flag_list)
		{
			if(!IsAllied(flag->GetOwner(), owner)) continue;
			if(GetIndexOf(current, flag) != -1) continue;
			if(GetIndexOf(new, flag) != -1) continue;
			if(flag == this) continue;
			
			if(ObjectDistance(oldflag, flag) > oldflag->GetFlagRadius() + flag->GetFlagRadius()) continue;
			
			new[GetLength(new)] = flag;
		}
	}
	
	lflag.linked_flags = current;
	
	// update flag links for all linked flags - no need for every flag to do that
	// meanwhile, adjust power helper. Merge if necessary
	// since we don't know whether flag links have been lost we will create a new power helper and possibly remove old ones
	Library_Power->Init(); // make sure the power system is set up
	var old = lflag.power_helper;
	lflag.power_helper = CreateObject(Library_Power, 0, 0, NO_OWNER);
	Library_Power_power_compounds[GetLength(Library_Power_power_compounds)] = lflag.power_helper;
	
	// list of helpers yet to merge
	var to_merge = [old];
	
	// copy from me
	
	lflag.linked_flags = current;
	for(var other in lflag.linked_flags)
	{
		other->CopyLinkedFlags(this, lflag.linked_flags);
		
		if(GetIndexOf(to_merge, other.lflag.power_helper) == -1)
			to_merge[GetLength(to_merge)] = other.lflag.power_helper;
		other.lflag.power_helper = lflag.power_helper;
	}
	
	// for every object in to_merge check if it actually (still) belongs to the group
	for(var h in to_merge)
	{
		if(h == nil)
			continue;
		RefreshPowerHelper(h);
	}
}

func RefreshPowerHelper(h)
{
	// merge both power_links and sleeping_links
	for(var o in h.power_links)
	{
		if(o == nil) continue; // possible
		
		var actual = Library_Power->GetPowerHelperForObject(o.obj);
		if (!actual) continue;
		if(actual == h) continue; // right one already
		// remove from old and add to new
		h->RemovePowerLink(o.obj, true);
		actual->AddPowerLink(o.obj, o.amount, true);
	}
		
	for(var i = GetLength(h.sleeping_links); --i >= 0;)
	{
		var o = h.sleeping_links[i];
		var actual = Library_Power->GetPowerHelperForObject(o.obj);
		if(actual == h) continue; // right one already
		// remove from old one and add to new
		actual.sleeping_links[GetLength(actual.sleeping_links)] = o;
			
		h.sleeping_links[i] = h.sleeping_links[GetLength(h.sleeping_links) - 1];
		SetLength(h.sleeping_links, GetLength(h.sleeping_links) - 1);
	}
}

public func CopyLinkedFlags(object from, array flaglist)
{
	lflag.linked_flags = flaglist[:];
	for(var i = GetLength(lflag.linked_flags)-1; i >= 0; --i)
		if(lflag.linked_flags[i] == this)
			lflag.linked_flags[i] = from;
	StopRefreshLinkedFlags();
}

public func GetPowerHelper(){return lflag.power_helper;}
public func SetPowerHelper(object to){lflag.power_helper = to;}

public func GetLinkedFlags(){return lflag.linked_flags;}

private func ClearFlagMarkers()
{
	for(var obj in lflag.range_markers)
		if (obj) obj->RemoveObject();
	lflag.range_markers = [];
}

public func SetFlagRadius(int to)
{
	lflag.radius = to;
	
	// redraw
	RedrawAllFlagRadiuses();
	
	// ownership
	RefreshOwnershipOfSurrounding();
	
	return true;
}

// reassign owner of flag markers for correct color
func OnOwnerChanged(int new_owner, int old_owner)
{
	for(var marker in lflag.range_markers)
	{
		if(!marker) continue;
		marker->SetOwner(new_owner);
		marker->ResetColor();
	}
}

// callback from construction library: Create a special preview that gives extra info about affected buildings / flags
func CreateConstructionPreview(object constructing_clonk)
{
	return CreateObject(Library_Flag_ConstructionPreviewer, constructing_clonk->GetX()-GetX(), constructing_clonk->GetY()-GetY(), constructing_clonk->GetOwner());
}

public func GetFlagRadius(){if (lflag) return lflag.radius; else return DefaultFlagRadius;}
public func GetFlagConstructionTime() {return lflag.construction_time;}
public func GetFlagMarkerID(){return LibraryFlag_Marker;}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (lflag && lflag.radius != DefaultFlagRadius) props->AddCall("Radius", this, "SetFlagRadius", lflag.radius);
	return true;
}
