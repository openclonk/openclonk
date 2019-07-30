/**
	Chest
	Storage for items.
	
	@author Maikel
*/


#include Library_Structure
#include Library_Ownable

local is_open;
local has_interaction_menu_open = false;

protected func Construction()
{
	// On Construction, the object does not contain anything, so we default to keeping the chest open.
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 1, ANIM_Hold));
	is_open = true;
	
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20, 80),0, 1, 0));
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

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
	{
		Open();
		has_interaction_menu_open = true;
	}
}

public func OnShownInInteractionMenuStop(bool last)
{
	if (last)
	{
		if (ContentsCount() > 0)
			Close();
		has_interaction_menu_open = false;
	}
}

public func Collection2(object obj)
{
	if (is_open && !has_interaction_menu_open)
		Close();
	return _inherited(obj, ...);
}

public func Ejection(object obj)
{
	if (ContentsCount() == 0 && !has_interaction_menu_open)
		Open();
	return _inherited(obj, ...);
}

public func ContentsDestruction(object destroyed)
{
	if (ContentsCount() <= 1 && !has_interaction_menu_open)
		Open();
	return _inherited(destroyed, ...);
}

private func Open()
{
	if (is_open)
		return;
	is_open = true;	
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold));
	Sound("Structures::Chest::Open");
}

private func Close()
{
	if (!is_open)
		return;
	is_open = false;	
	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold));
	Sound("Structures::Chest::Close");
}

public func NoConstructionFlip() { return true; }

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30, 1, 0, 0), Trans_Rotate(30, 0, 1, 0), Trans_Translate(1000, 1, 0)),def);
	return _inherited(def, ...);
}

local Name = "$Name$";
local Description = "$Description$";
local ContainBlast = true;
local HitPoints = 50;
local Components = {Wood = 2};
