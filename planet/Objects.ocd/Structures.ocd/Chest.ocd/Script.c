/*
	Chest
	Author: Maikel

	Storage for items.
*/


#include Library_Structure

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
public func IsInteractable() { return true; }

private func MaxContentsCount()
{
	return 5;
}


// Open contentsmenu via interaction
public func Interact(object clonk, int mode)
{
	// Interaction does the same as the content control.
	clonk->ObjectControl(clonk->GetOwner(), CON_Contents);
}
	
protected func RejectCollect()
{
	if (ContentsCount() >= MaxContentsCount())
		return true;
	return false;
}

public func OnContentMenuOpened()
{
	return Open();
}


public func OnContentMenuClosed()
{
	return Close();
}

private func Open()
{
	if (is_open)
		return;
	is_open = true;	
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold), Anim_Const(1000));
	Sound("ChestOpen");
}

private func Close()
{
	if (!is_open)
		return;
	is_open = false;	
	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold), Anim_Const(1000));
	Sound("ChestClose");
}

public func NoConstructionFlip() { return true; }

protected func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

local Name = "$Name$";
local HitPoints = 50;
