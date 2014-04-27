/**
	SplitTerraflint
	Split a terraflint into three flints.

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
	Contained()->CreateContents(Firestone);
	Contained()->CreateContents(Firestone);
	Contained()->CreateContents(Firestone);
	RemoveObject();
}

func IsChemicalProduct() { return true; }