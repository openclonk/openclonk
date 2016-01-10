/**
	Chest
	Storage for items.
	
	@author Maikel
*/


#include Library_Structure
#include Library_Ownable

local is_open;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
	is_open = false;
	return _inherited(...);
}

/*-- Contents --*/

public func IsContainer() { return true; }

local MaxContentsCount = 50;

protected func RejectCollect()
{
	if (ContentsCount() >= MaxContentsCount)
		return true;
	return false;
}

public func OnShownInInteractionMenuStart(bool first)
{
	if (first)
		Open();
}

public func OnShownInInteractionMenuStop(bool last)
{
	if (last)
		Close();
}

private func Open()
{
	if (is_open)
		return;
	is_open = true;	
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold), Anim_Const(1000));
	Sound("Structures::Chest::Open");
}

private func Close()
{
	if (!is_open)
		return;
	is_open = false;	
	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold), Anim_Const(1000));
	Sound("Structures::Chest::Close");
}

public func NoConstructionFlip() { return true; }

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local ContainBlast = true;
local HitPoints = 50;
