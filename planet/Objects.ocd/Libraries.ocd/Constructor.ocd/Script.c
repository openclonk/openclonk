/**
	Constructor
	Library for objects which are able to construct structures.
	
	@author Maikel
*/


public func IsConstructor() { return true; }

func RejectUse(object clonk)
{
	return !clonk->IsWalking();
}

public func ControlUseStart(object clonk, int x, int y)
{
	// Is the clonk at an construction site?
	// TODO: check for multiple objects
	var structure = FindObject(Find_Category(C4D_Structure), Find_Or(Find_Distance(20), Find_AtPoint()), Find_Layer(GetObjectLayer()));
	if (structure)
	{
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
	}
	
	// Otherwise create a menu with possible structures to build.
	OpenConstructionMenu(clonk);
	//clonk->CreateConstructionMenu(this, true);
	clonk->CancelUse();
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int x, int y)
{
	// Is the clonk still at an construction site?
	var structure = FindObject(Find_Category(C4D_Structure), Find_Or(Find_Distance(20), Find_AtPoint()), Find_Layer(GetObjectLayer()));
	if (structure)
	{	
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
	}	
	return true;
}

private func ShowConstructionMaterial(object clonk, object structure)
{
	var mat_msg = "$TxtNeeds$";
	var structure_id = structure->GetID();
	var comp, index = 0;
	while (comp = structure->GetComponent(nil, index))
	{
		var current_amount = structure->GetComponent(comp);
		var max_amount = GetComponent(comp, nil, nil, structure_id);
		mat_msg = Format("%s %dx{{%i}}", mat_msg, Max(0, max_amount - current_amount), comp);
		index++;
	}
	clonk->Message(mat_msg);
	return;
}


private func Repair(object clonk, object structure)
{

}

/** Gives a list of ids of the players knowledge.
*/
public func GetConstructionPlans(int plr)
{
	var construction_plans = [];
	var construct_id, index = 0;
	while (construct_id = GetPlrKnowledge(plr, nil, index++, C4D_Structure))
		construction_plans[index-1] = construct_id;
	return construction_plans;
}

/* Construction preview */

func ShowConstructionPreview(object clonk, id structure_id)
{
	AddEffect("ControlConstructionPreview", clonk, 1, 0, this, nil, structure_id, clonk);
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, true);
	return true;
}

func FxControlConstructionPreviewStart(object clonk, effect, int temp, id structure_id, object clonk)
{
	if (temp) return;

	effect.structure = structure_id;
	effect.flipable = !structure_id->~NoConstructionFlip();
	effect.preview = structure_id->~CreateConstructionPreview(clonk);
	if (!effect.preview) effect.preview = CreateObject(ConstructionPreviewer, AbsX(clonk->GetX()), AbsY(clonk->GetY()), clonk->GetOwner());
	effect.preview->Set(structure_id, clonk);
}

// Called by Control2Effect
func FxControlConstructionPreviewControl(object clonk, effect, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (ctrl != CON_Aim)
	{
		// CON_Use is accept
		if (ctrl == CON_Use)
			CreateConstructionSite(clonk, effect.structure, AbsX(effect.preview->GetX()), AbsY(effect.preview->GetY() + effect.preview.dimension_y/2), effect.preview.blocked, effect.preview.direction, effect.preview.stick_to);
		// movement is allowed
		else if (IsMovementControl(ctrl))
			return false;
		// Flipping
		// this is actually realized twice. Once as an Extra-Interaction in the clonk, and here. We don't want the Clonk to get any non-movement controls though, so we handle it here too.
		// (yes, this means that actionbar-hotkeys wont work for it. However clicking the button will.)
		else if (IsInteractionControl(ctrl))
		{
			if (release)
				effect.preview->Flip();
			return true;
		}

		// everything else declines
		RemoveEffect("ControlConstructionPreview", clonk, effect);
		return true;
	}
		
	effect.preview->Reposition(x, y);
	return true;
}

func FxControlConstructionPreviewStop(object clonk, effect, int reason, bool temp)
{
	if (temp) return;

	effect.preview->RemoveObject();
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, false);
}

/* Construction */

func CreateConstructionSite(object clonk, id structure_id, int x, int y, bool blocked, int dir, object stick_to)
{
	// Only when the clonk is standing and outdoors
	if (clonk->GetAction() != "Walk")
		return false;
	if (clonk->Contained())
		return false;
	// Check if the building can be build here
	if (structure_id->~RejectConstruction(x, y, clonk)) 
		return false;
	if (blocked)
	{
		CustomMessage("$TxtNoSiteHere$", this, clonk->GetOwner(), nil,nil, RGB(255,0,0)); 
		return false;
	}
	// intersection-check with all other construction sites... bah
	for (var other_site in FindObjects(Find_ID(ConstructionSite)))
	{
		if(!(other_site->GetLeftEdge()   > GetX()+x+structure_id->GetDefWidth()/2  ||
		     other_site->GetRightEdge()  < GetX()+x-structure_id->GetDefWidth()/2  ||
		     other_site->GetTopEdge()    > GetY()+y+structure_id->GetDefHeight()/2 ||
		     other_site->GetBottomEdge() < GetY()+y-structure_id->GetDefHeight()/2 ))
		{
			CustomMessage(Format("$TxtBlocked$",other_site->GetName()), this, clonk->GetOwner(), nil,nil, RGB(255,0,0));
			return false;
		}
	}
	
	// Set owner for CreateConstruction
	SetOwner(clonk->GetOwner());
	// Create construction site
	var site;
	site = CreateObject(ConstructionSite, x, y, Contained()->GetOwner());
	/* note: this is necessary to have the site at the exact position x,y. Otherwise, for reasons I don't know, the
	   ConstructionSite seems to move 2 pixels downwards (on ConstructionSite::Construction() it is still the
	   original position) which leads to that the CheckConstructionSite function gets different parameters later
	   when the real construction should be created which of course could mean that it returns something else. (#1012)
	   - Newton
	*/
	site->SetPosition(GetX()+x,GetY()+y);
	
	// Randomize sign rotation
	site->SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-30, 30), 0, 1, 0), Trans_Rotate(RandomX(-10, 10), 1, 0, 0)));
	site->PlayAnimation("LeftToRight", 1, Anim_Const(RandomX(0, GetAnimationLength("LeftToRight"))), Anim_Const(500));
	
	site->Set(structure_id, dir, stick_to);
	//if(!(site = CreateConstruction(structure_id, x, y, Contained()->GetOwner(), 1, 1, 1)))
		//return false;
	
	// check for material
	var comp, index = 0;
	var mat;
	var w = structure_id->GetDefWidth()+10;
	var h = structure_id->GetDefHeight()+10;

	while (comp = GetComponent(nil, index, nil, structure_id))
	{
		// find material
		var count_needed = GetComponent(comp, nil, nil, structure_id);
		index++;
		
		mat = CreateArray();
		// 1. look for stuff in the clonk
		mat[0] = FindObjects(Find_ID(comp), Find_Container(clonk));
		// 2. look for stuff lying around
		mat[1] = clonk->FindObjects(Find_ID(comp), Find_NoContainer(), Find_InRect(-w/2, -h/2, w,h));
		// 3. look for stuff in nearby lorries/containers
		var i = 2;
		for(var cont in clonk->FindObjects(Find_Or(Find_Func("IsLorry"), Find_Func("IsContainer")), Find_InRect(-w/2, -h/2, w,h)))
			mat[i] = FindObjects(Find_ID(comp), Find_Container(cont));
		// move it
		for(var mat2 in mat)
		{
			for(var o in mat2)
			{
				if(count_needed <= 0)
					break;
				o->Exit();
				o->Enter(site);
				count_needed--;
			}
		}
	}
	
	// Message
	clonk->Message("$TxtConstructions$", structure_id->GetName());
	return true;
}


/*-- Construction Menu --*/

// Local variable to keep track of the menu properties.
local menu, menu_id, menu_target, menu_controller;

public func OpenConstructionMenu(object clonk)
{
	// If the menu is already open, don't open another instance.
	if (clonk->GetMenu() && clonk->GetMenu().ID == menu_id)
		return;	
		
	// Create a menu target for visibility.
	menu_target = CreateObject(Dummy, 0, 0, clonk->GetOwner());
	menu_target.Visibility = VIS_Owner;
	menu_controller = clonk;
	
	// Number of items, square shape.
	var grid_size = 5; 
	// Size of the grid items in em.
	var item_size = 10; 
	// Calculate menu size.
	var menu_width = grid_size * item_size * 2 + 2; 
	var menu_height = grid_size * item_size;
	
	// Construction menu proplist.
	menu =
	{
		Target = menu_target,
		Style = GUI_Multiple,
		Decoration = GUI_MenuDeco,
		Left = Format("50%%-%dem", menu_width / 2),
		Right = Format("50%%+%dem", menu_width / 2),
		Top = Format("50%%-%dem", menu_height / 2),
		Bottom = Format("50%%+%dem", menu_height / 2),
		BackgroundColor = {Std = 0}
	};
	
	menu.Structures = CreateStructureGrid(clonk, grid_size, item_size);
	menu.StructInfo = CreateStructureInfo(grid_size * item_size);
	menu.Separator =
	{
		Target = menu_target,
		ID = 9,
		Left = "50%-1em",
		Right = "50%+1em",
		Top = "0%",
		Bottom = "100%",
		BackgroundColor = {Std = 0x50888888}	
	};

	// Menu ID.
	menu_id = CustomGuiOpen(menu);
	clonk->SetMenu(menu_id);
	return;
}

public func CreateStructureGrid(object clonk, int grid_size, int item_size)
{
	var structures = 
	{
		Target = menu_target,
		ID = 1,
		// Add a small amount of em to make the items fit in the grid.
		Left = "0%",
		Right = "50%+0.1em", 
		Top = "0%",
		Bottom = "100%em",
		Style = GUI_GridLayout,
		BackgroundColor = {Std = 0}
	};
	structures = MenuAddStructures(structures, clonk, item_size);
	return structures;
}

public func CreateStructureInfo(int size)
{
	var structinfo = 
	{
		Target = menu_target,
		ID = 2,
		Left = Format("100%%-%dem", size),
		Right = "100%",
		Top = "0%",
		Bottom = "100%",
		BackgroundColor = {Std = 0}
	};
	// Bottom 20% for description and other written information.
	structinfo.Description = 
	{
		Target = menu_target,
		ID = 3,
		Priority = 0x0fffff,
		Left = "0%",
		Right = "100%",
		Top = "80%",
		Bottom = "100%",	
		Text = nil // will be updated
	};
	// Upright 80% is for the picture, though only 60% used.
	structinfo.Picture = 
	{
		Target = menu_target,
		ID = 4,
		Left = "10%",
		Right = "70%",
		Top = "10%",
		Bottom = "70%",	
		Symbol = nil // will be updated
	};
	structinfo.PowerConsumer =
	{
		Target = menu_target,
		ID = 5,
		Left = "10%",
		Right = "20%",
		Top = "10%",
		Bottom = "20%",	
		Symbol = nil // will be updated
	};
	structinfo.PowerProducer = 
	{
		Target = menu_target,
		ID = 6,
		Left = "60%",
		Right = "70%",
		Top = "10%",
		Bottom = "20%",	
		Symbol = nil // will be updated
	};
	// Materials and exit button are shown on left 20%.
	structinfo.Materials = 
	{
		Target = menu_target,
		ID = 7,
		Left = "80%",
		Right = "100%",
		Top = "0%",
		Bottom = "80%"
	};
	structinfo.CloseButton = 
	{
		Target = menu_target,
		ID = 8,
		Left = "90%", 
		Right = "100%", 
		Top = "0%",
		Bottom = "10%",
		Symbol = Icon_Cancel,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "CloseConstructionMenu")
	};
	structinfo = MenuMaterialCosts(structinfo, nil);
	return structinfo;
}

public func MenuAddStructures(proplist struct, object clonk, int item_size)
{
	var plans = GetConstructionPlans(clonk->GetOwner());
	var nr_plans = GetLength(plans);
	var width = 100 / 5;
	
	var cnt = 0;
	for (var structure in GetConstructionPlans(clonk->GetOwner()))
	{
		var x = cnt % 5;
		var y = cnt / 5;
		var str =
		{
			Target = menu_target,
			ID = cnt + 100,
			Right = Format("%dem", item_size),
			Bottom = Format("%dem", item_size),
			BackgroundColor = {Std = 0, Hover = 0x50ffffff},
			OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "OnConstructionHover", structure)],
			OnMouseOut = GuiAction_SetTag("Std"), 
			OnClick = GuiAction_Call(this, "OnConstructionSelection", {Struct = structure, Constructor = clonk}),
			Picture = 
			{
				ID = cnt + 200,
				Left = "8%",
				Right = "92%",
				Top = "8%",
				Bottom = "92%",
				Symbol = structure
			}
		};
		struct[Format("Struct%d", cnt + 4)] = str;		
		cnt++;
	}
	return struct;
}

public func MenuMaterialCosts(proplist info, id structure)
{
	// Show text "costs:" as a submenu.
	info.MaterialCosts = { 
		Target = menu_target,
		ID = 1000,
		Left = "90%",
		Right = "100%",
		Top = "20%-1.2em",
		Bottom = "20%",
		Style = GUI_TextHCenter,
		Text = "Costs:"
	};	
	
	// Get the different components of the structure.
	var comp, index = 0;
	var components = [];
	if (structure)
		while (comp = GetComponent(nil, index++, nil, structure))
			components[GetLength(components)] = [comp, GetComponent(comp, nil, nil, structure)];
					
	// Add those components to the submenus of the info menu.
	for (var i = 1; i <= 7; i++)
	{
		var amount = "";
		var symbol = nil;
		if (GetLength(components) >= i)
		{
			amount = Format("%dx", components[i - 1][1]);
			symbol = components[i - 1][0];
		}
		info[Format("MaterialCost%d", i)] = 
		{
			Target = menu_target,
			ID = i + 1000,
			Left = "90%",
			Right = "100%",
			Top = Format("%d%", 10 + 10 * i),
			Bottom = Format("%d%", 20 + 10 * i),
			Symbol = symbol,
			Style = GUI_TextRight | GUI_TextBottom,
			Text = amount
		};	
	}
	return info;
}

public func OnConstructionSelection(proplist par)
{
	ShowConstructionPreview(par.Constructor, par.Struct);
	CloseConstructionMenu();
	return;
}

public func OnConstructionHover(id structure)
{
	var structinfo = menu.StructInfo;
	
	// Update the description to this part of the menu.
	structinfo.Description.Text = structure.Description;
	CustomGuiUpdate(structinfo.Description, menu_id, structinfo.Description.ID, menu_target);
	
	// Update the picture of the structure.
	structinfo.Picture.Symbol = structure;
	CustomGuiUpdate(structinfo.Picture, menu_id, structinfo.Picture.ID, menu_target);
	
	// Update also power consumption/production.
	if (structure->~IsPowerConsumer())
		structinfo.PowerConsumer.Symbol = Library_PowerConsumer;
	else
		structinfo.PowerConsumer.Symbol = nil;
	CustomGuiUpdate(structinfo.PowerConsumer, menu_id, structinfo.PowerConsumer.ID, menu_target);
	if (structure->~IsPowerProducer())
		structinfo.PowerProducer.Symbol = Library_PowerProducer;
	else
		structinfo.PowerProducer.Symbol = nil;
	CustomGuiUpdate(structinfo.PowerProducer, menu_id, structinfo.PowerProducer.ID, menu_target);
	
	// Update the material costs of the structure.
	structinfo = MenuMaterialCosts(structinfo, structure);
	for (var i = 1; i <= 7; i++)
		CustomGuiUpdate(structinfo[Format("MaterialCost%d", i)], menu_id, structinfo[Format("MaterialCost%d", i)].ID, menu_target);

	return;
}

public func CloseConstructionMenu()
{
	CustomGuiClose(menu_id, nil, menu_target);
	menu_id = nil;
	menu_target->RemoveObject();
	menu_target = nil;
	if (menu_controller)
		menu_controller->MenuClosed();
	menu_controller = nil;
	return;
}
