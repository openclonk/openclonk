/**
	SplitFirestone
	Split a firestone to get its natural components, coal and sulphur.

	@author Clonkonaut
*/

local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	ScheduleCall(this, "Split", 1);
}
func Split()
{
	if (!Contained()) return RemoveObject();
	Contained()->CreateContents(Coal);
	Contained()->CreateContents(Sulphur);
	RemoveObject();
}

func IsChemicalProduct() { return true; }