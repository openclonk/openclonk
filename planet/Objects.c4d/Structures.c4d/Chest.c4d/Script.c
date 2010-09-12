/*
	Chest
	Author: Maikel

	Storage for items.
*/

#include Library_ItemContainer

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
}

/*-- Contents --*/

private func MenuOnInteraction() { return true; }

private func MaxContentsCount()
{
	return 5;
}

func GetInteractionMetaInfo(object clonk)
{
	if(content_menu)
		return { Description = "$CloseChest$", IconName = nil, IconID = nil, Selected = true };
	else
		return { Description = "$OpenChest$", IconName = nil, IconID = nil, Selected = false };
}

// callback: menu was closed
func MenuClosed()
{
	Close();
}

private func OnContentMenuOpen() { Open(); }

private func Open()
{
	PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 22, ANIM_Remove), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
	Sound("Open.ogg");
}

private func Close()
{
	PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 15, ANIM_Remove), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
	Sound("Close.ogg");
}

protected func Definition(def) 
{
		SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(0,-3000,-5000), Trans_Rotate(-30,1,0,0), Trans_Rotate(30,0,1,0), Trans_Translate(1000,1,0)),def);
}

local Name = "$Name$";
