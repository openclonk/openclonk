/**
	LibraryFlag_ConstructionPreviewer
	Construction preview object for objects implementing flagpole
	Shows helper arrows around the preview to tell the player which neighbouring flags will be connected

	@author Sven2
*/

#include ConstructionPreviewer

local neighbour_flag_arrows;

func Initialize()
{
	var r = _inherited(...);
	neighbour_flag_arrows = [];
	return r;
}

// Preview update: Called on initial creation and when position is updated
func AdjustPreview(bool look_up, bool no_call)
{
	// Regular preview behaviour (check site, color object, etc.)
	inherited(look_up, no_call, ...);
	if (no_call) return true;
	// Update arrows pointing to neighboured flags
	var neighboured_flags = Library_Flag->FindFlagsInRadius(this, structure->GetFlagRadius(), clonk->GetOwner());
	var i;
	for (var flag in neighboured_flags)
	{
		var arrow = neighbour_flag_arrows[i];
		if (!arrow)
		{
			neighbour_flag_arrows[i] = arrow = CreateObject(Library_Flag_ConstructionPreviewer_Arrow, 0,0, GetOwner());
			if (!arrow) { ++i; continue; }
			arrow->SetAction("Show", this);
		}
		arrow->SetR(Angle(GetX(), GetY(), flag->GetX(), flag->GetY()));
		++i;
	}
	// Kill any leftover arrows
	for (var j=i; j<GetLength(neighbour_flag_arrows); ++j)
	{
		if (neighbour_flag_arrows[j]) neighbour_flag_arrows[j]->RemoveObject();
	}
	SetLength(neighbour_flag_arrows, i);
	return true;
}

func Destruction()
{
	// Preview gone. Remove arrows.
	for (var arrow in neighbour_flag_arrows) if (arrow) arrow->RemoveObject();
	return _inherited(...);
}

// UI not saved.
func SaveScenarioObject() { return false; }
