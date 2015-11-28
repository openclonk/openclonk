/**
	Flag Library: Construction Previewer
	Construction preview object for objects implementing flagpole behavior.
	This object is used in the flag library and shows helper arrows around 
	the preview to tell the player which neighbouring flags will be connected.

	@author Sven2
*/

// This builds on the normal construction previewer.
#include ConstructionPreviewer


// All flag related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See 
// Construction for which variables are being used.
local lib_flag;

protected func Initialize()
{
	var return_value = _inherited(...);
	// Initialize the single proplist for the flag library.
	if (lib_flag == nil)
		lib_flag = {};
	// Initialize the list for neighbouring arrows.
	lib_flag.neighbour_arrows = [];
	return return_value;
}

// Preview update: Called on initial creation and when position is updated.
public func AdjustPreview(bool below_surface, bool look_up, bool no_call)
{
	// Regular preview behaviour (check site, color object, etc.)
	inherited(below_surface, look_up, no_call, ...);
	if (no_call) 
		return true;
	// Update arrows pointing to neighboured flags.
	var neighboured_flags = Library_Flag->FindFlagsInRadius(this, structure->GetFlagRadius(), clonk->GetOwner());
	var index = 0;
	for (var flag in neighboured_flags)
	{
		var arrow = lib_flag.neighbour_arrows[index];
		if (!arrow)
		{
			arrow = CreateObject(Library_Flag_ConstructionPreviewer_Arrow, 0, 0, GetOwner());
			lib_flag.neighbour_arrows[index] = arrow;
			if (!arrow) 
			{
				++index; 
				continue;
			}
			arrow->SetAction("Show", this);
		}
		arrow->SetR(Angle(GetX(), GetY(), flag->GetX(), flag->GetY()));
		++index;
	}
	// Kill any leftover arrows.
	for (var j = index; j < GetLength(lib_flag.neighbour_arrows); ++j)
	{
		if (lib_flag.neighbour_arrows[j]) 
			lib_flag.neighbour_arrows[j]->RemoveObject();
	}
	SetLength(lib_flag.neighbour_arrows, index);
	return true;
}

// Callback from the engine: preview gone, remove arrows.
protected func Destruction()
{
	for (var arrow in lib_flag.neighbour_arrows) 
		if (arrow)
			arrow->RemoveObject();
	return _inherited(...);
}


/*-- Saving --*/

// The UI is not saved.
public func SaveScenarioObject() { return false; }
