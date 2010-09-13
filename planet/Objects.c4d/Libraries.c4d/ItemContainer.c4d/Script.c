/*--
	Item container
	Author: Maikel
	
	Basic control object for vehicles and structures which are item containers, which the clonk can access.
	Functions to be overloaded by the container's script:
	MaxContentsCount() - Specifies the maximum number of items this container can hold.
 --*/



/*-- Contents count --*/

private func MaxContentsCount()
{
	return 0;
}

protected func RejectCollect()
{
	if (ContentsCount() >= MaxContentsCount())
		return true;
	return false;
}

/*-- Ring menu --*/

local content_menu;

// Overloadable, specifies when the menu pops up.
private func MenuOnInteraction() { return false; }
private func MenuOnControlUse() { return false; }
private func MenuOnControlUseAlt() { return false; }

public func IsInteractable() { return MenuOnInteraction(); }

// Opens a menu when the clonk interacts with the chest
public func Interact(object clonk)
{
	if (MenuOnInteraction())
		OpenMenu(clonk);
	else
		return _inherited(clonk, ...);
}

public func ControlUse(object clonk)
{
	if (MenuOnControlUse())
		OpenMenu(clonk);
	else
		return _inherited(clonk, ...);
	return true;
}

public func ControlUseAlt(object clonk)
{
	if (MenuOnControlUseAlt())
		OpenMenu(clonk);
	else
		return _inherited(clonk, ...);
	return true;
}

public func OpenMenu(object clonk)
{
	//var proc = clonk->GetProcedure();
	// not opened yet           v- why is this check here? - it belongs in the object script - Newton
	if (!content_menu /* && (proc == "WALK" || proc == "PUSH")*/)
	{
		content_menu = clonk->CreateRingMenu(GetID(), this);
		// all contents into the menu
		for (var i = 0; i < ContentsCount(); i++)
			content_menu->AddItem(Contents(i));
		if (ContentsCount() < MaxContentsCount())
			content_menu->AddItem(nil);
		content_menu->Show();
		OnContentMenuOpen();
	}
	else if(content_menu && clonk == content_menu->GetMenuObject())
	{
		content_menu->Close();
		OnContentMenuClose();
	}
	// otherwise, fail
	else
		return false;
		
	return true;
}

private func OnContentMenuOpen() { return; }
private func OnContentMenuClose() { return; }

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
	if (chest_content)
		chest_content->Exit();
	if (clonk_content)
		clonk_content->Exit();

	if (chest_content)
		clonk->Collect(chest_content,nil,alt);
	if (clonk_content)
		clonk_content->Enter(this);
}

local Name = "$Name$";
