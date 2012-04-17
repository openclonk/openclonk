/*
	Chest
	Author: Maikel

	Storage for items.
*/


local chestanim;

protected func Construction()
{
	chestanim = PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
	return _inherited(...);
}

/*-- Contents --*/

public func IsContainer() { return true; }

private func MaxContentsCount()
{
	return 5;
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
	chestanim = PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Hold), Anim_Const(1000));
	Sound("ChestOpen");
}

private func Close()
{
	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Hold), Anim_Const(1000));
	Sound("ChestClose");
}

public func NoConstructionFlip() { return true; }

protected func Definition(def)
{
		SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

local Name = "$Name$";
