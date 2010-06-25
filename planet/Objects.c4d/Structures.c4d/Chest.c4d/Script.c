/*
	Chest
	Author: Maikel

	Storage for items.
*/

local content_menu;

protected func Construction()
{
	PlayAnimation("Open", 1, Anim_Linear(0, 0, 1, 20, ANIM_Hold), Anim_Const(1000));
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(20,80),0,1,0));
}

func IsInteractable() { return true; }

// Opens a menu when the clonk interacts with the chest
func Interact(object clonk)
{
	// not opened yet
	if (!content_menu && clonk->GetProcedure() == "WALK") 
	{
		content_menu = clonk->CreateRingMenu(GetID(), this);
		// all contents into the menu
		for (var i=0; i<5; ++i)
			content_menu->AddItem(Contents(i));
		content_menu->Show();
		Open();
	}
	else if(clonk == content_menu->GetMenuObject())
	{
		content_menu->Close();
	}
	// otherwise, fail
	else
		return false;
		
	return true;
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

// Callback from ringmenu.
public func Selected(object menu, object selected, bool alt)
{
	var chest_content = selected->GetSymbol();
	var clonk_content = menu->GetMenuObject()->GetItem(alt);

	Exchange(chest_content, clonk_content, menu->GetMenuObject(), alt);
	selected->SetSymbol(clonk_content);
	menu->Show();
	return false;
}

private func Exchange(object chest_content, object clonk_content, object clonk, int alt)
{
	if(chest_content)
		chest_content->Exit();
	if (clonk_content)
		clonk_content->Exit();

	if(chest_content)
		clonk->Collect(chest_content,nil,alt);
	if (clonk_content)
		clonk_content->Enter(this);
}

/*-- Contents --*/

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


protected func Definition(def) 
{
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation",Trans_Mul(Trans_Rotate(-30,1,0,0),Trans_Rotate(30,0,1,0),Trans_Translate(1000,1,0,0)),def);
}