
/*

	Devils's Hand, a tool to remove objects.
	
	@author: K-Pone

*/


local Name = "$Name$";
local Description = "$Description$";

func Initialize()
{
	
}

func ControlUse(object clonk, x, y)
{
	var dummy = clonk->CreateObject(Dummy, x, y); // Dummy is only needed to have a reference point for Find_Distance
	var remobj = dummy->FindObject(Find_Distance(5), Find_NoContainer(), Find_Not(Find_ID(Clonk)));
	dummy->RemoveObject();
	
	if (!remobj)
	{
		Sound("UI::Error");
		return;
	}
	
	remobj->RemoveObject();
	
	Sound("UI::Click", true, nil, clonk->GetOwner());
}