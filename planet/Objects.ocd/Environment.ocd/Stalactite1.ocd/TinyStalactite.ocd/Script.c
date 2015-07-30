/**
	TinyStalactite
	Covers up the base of stalactites

	@author 
*/

local Name = "$Name$";
local Description = "$Description$";
local parent;

func Initialize()
{
}

func SetParent(object target)
{
	parent = target;
	AddEffect("Parent",this,1,1,this);
}

func FxParentTimer(object target, effect, int time)
{
	SetPosition(parent->GetX(), parent->GetY());
}