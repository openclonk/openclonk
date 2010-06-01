/*
	Chest
	Author: Maikel

	Storage for items.
*/

local content_menu;
local ctrl_clonk;

protected func Initialize()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
}

// Opens a menu when the clonk grabs this chest.
protected func Grabbed(object by_object, bool grab)
{
	//if (FindObject(Find_Action("Push"), Find_ActionTarget(this)))
	//	return;//by_object->SetCommand("UnGrab"); 
	ctrl_clonk = by_object;
	if (grab) // If grabbed show content menu.
	{
		content_menu = by_object->CreateRingMenu(GetID(), this);
		for (var content in FindObjects(Find_Container(this)))
			content_menu->AddItem(content->GetID());
		content_menu->AddItem(GetID());
		content_menu->Show();    
		OpenClose(true);
	}
	else // If let go close content menu.
	{
		if (content_menu) 
			content_menu->Close();
		OpenClose();
	}
	return;
}

private func OpenClose(bool open)
{
	if(open == true) //On open
	{
		PlayAnimation("Open", 5, Anim_Linear(0, 0, GetAnimationLength("Open"), 40, ANIM_Remove), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		Sound("Open.ogg");
	}

	if(open != true) //On close
	{
		PlayAnimation("Close", 5, Anim_Linear(0, 0, GetAnimationLength("Close"), 40, ANIM_Remove), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		Sound("Close.ogg");
	}
}

// Callback from ringmenu.
public func Selected(object menu, object selected, bool alt)
{
	if (!selected) 
		return false;
	var content_id = selected->GetSymbol();
	if (content_id == GetID()) 
		return ctrl_clonk->SetCommand("UnGrab");
	var chest_content = FindObject(Find_Container(this), Find_ID(content_id));
	var clonk_content = ctrl_clonk->GetItem(alt);
	Exchange(chest_content, clonk_content, ctrl_clonk);
	selected->RemoveObject();
	menu->Show();

	ctrl_clonk->SetCommand("UnGrab");
	return true;
}

private func Exchange(object chest_content, object clonk_content, object clonk)
{
	if (chest_content)
		chest_content->Exit();
	if (clonk_content)
		clonk_content->Exit();

	if (chest_content)
		chest_content->Enter(clonk);
	if (clonk_content)
		clonk_content->Enter(this);
	return;
}

/*-- Contents --*/

private func MaxContentsCount()
{
	return 20;
}

protected func RejectCollect(id def, object obj)
{
	if (def == GetID())
		return true;
	if (ObjectCount(Find_Container(this)) >= MaxContentsCount())
		return true;
	return false;
}


protected func Definition(def) 
{
	SetProperty("Name", "Chest", def);
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-20,1,0,0),Trans_Rotate(20,0,1,0),Trans_Scale(950),Trans_Translate(1000,1,0,0)),def);
}