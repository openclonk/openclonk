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

public func ControlUse(object clonk, int x, int y)
{
	// Otherwise create a menu with possible structures to build.
	OpenConstructionMenu(clonk);
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
		var max_amount = structure_id->GetComponent(comp);
		mat_msg = Format("%s %dx{{%i}}", mat_msg, Max(0, max_amount - current_amount), comp);
		index++;
	}
	clonk->Message(mat_msg);
	return;
}

// Gives a list of ids of the players knowledge.
public func GetConstructionPlans(int plr)
{
	var construction_plans = [];
	var construct_id, index = 0;
	while (construct_id = GetPlrKnowledge(plr, nil, index++, C4D_Structure)) // Structures
		if (CanBuild(construct_id))
			construction_plans[GetLength(construction_plans)] = construct_id;
	index = 0;
	while (construct_id = GetPlrKnowledge(plr, nil, index++, C4D_Vehicle)) // Vehicles
		if (CanBuild(construct_id))
			construction_plans[GetLength(construction_plans)] = construct_id;
	index = 0;
	while (construct_id = GetPlrKnowledge(plr, nil, index++, C4D_Object)) // Objects
		if (CanBuild(construct_id))
			construction_plans[GetLength(construction_plans)] = construct_id;
	return construction_plans;
}

// Checks whether a certain construction plans can be built by this constructor.
// Overload as seem fit.
func CanBuild(id construction_plan)
{
	return true;
}

/*-- Construction preview --*/

public func ShowConstructionPreview(object clonk, id structure_id)
{
	AddEffect("ControlConstructionPreview", clonk, 1, 0, this, nil, structure_id, clonk);
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, true);
	return true;
}

public func FxControlConstructionPreviewStart(object clonk, effect, int temp, id structure_id, object clonk)
{
	if (temp) return;

	effect.structure = structure_id;
	effect.flipable = !structure_id->~NoConstructionFlip();
	effect.preview = structure_id->~CreateConstructionPreview(clonk);
	if (!effect.preview) effect.preview = CreateObjectAbove(ConstructionPreviewer, AbsX(clonk->GetX()), AbsY(clonk->GetY()), clonk->GetOwner());
	effect.preview->Set(structure_id, clonk);
}

// Called by Control2Effect
public func FxControlConstructionPreviewControl(object clonk, effect, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (ctrl != CON_Aim)
	{
		// CON_Use is accept, but don't remove the preview, this is done on releasing the button.
		if (ctrl == CON_Use && status == CONS_Down)
		{
			var ok = CreateConstructionSite(clonk, effect.structure, AbsX(effect.preview->GetX()), AbsY(effect.preview->GetY() + effect.preview.dimension_y/2), effect.preview.blocked, effect.preview.direction, effect.preview.stick_to);
			if (ok)
				clonk->~PlaySoundConfirm();
			else
				clonk->~PlaySoundDecline();
			return true;	
		}
		// movement is allowed
		else if (IsMovementControl(ctrl))
			return false;
		// Flipping
		// this is actually realized twice. Once as an Extra-Interaction in the clonk, and here. We don't want the Clonk to get any non-movement controls though, so we handle it here too.
		// (yes, this means that actionbar-hotkeys wont work for it. However clicking the button will.)
		else if (IsInteractionControl(ctrl))
		{
			if (status == CONS_Up)
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

public func FxControlConstructionPreviewStop(object clonk, effect, int reason, bool temp)
{
	if (temp) return;
	effect.preview->RemoveObject();
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, false);
}


/*-- Construction --*/

public func CreateConstructionSite(object clonk, id structure_id, int x, int y, bool blocked, int dir, object stick_to)
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
		if (!(other_site->GetLeftEdge()   > GetX()+x+structure_id->GetDefWidth()/2  ||
		     other_site->GetRightEdge()  < GetX()+x-structure_id->GetDefWidth()/2  ||
		     other_site->GetTopEdge()    > GetY()+y+structure_id->GetDefHeight()/2 ||
		     other_site->GetBottomEdge() < GetY()+y-structure_id->GetDefHeight()/2 ))
		{
			CustomMessage(Format("$TxtBlocked$", other_site->GetName()), this, clonk->GetOwner(), nil, nil, RGB(255, 0, 0));
			return false;
		}
	}
	
	// Set owner for CreateConstruction
	SetOwner(clonk->GetOwner());
	// Create construction site
	var site;
	site = CreateObjectAbove(ConstructionSite, x, y, Contained()->GetOwner());
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

	site->TakeConstructionMaterials(clonk);		
	
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
	
	// Size of the grid items in em.
	var item_size = 4; 
	
	// Construction menu proplist.
	menu = CreateConstructionMenuBackground(menu_target);
	menu.structures = CreateStructureGrid(clonk, item_size);
	menu.struct_info = CreateStructureInfo();
	menu.separator = CreateConstructionMenuSeparator();

	// Menu ID.
	menu_id = GuiOpen(menu);
	clonk->SetMenu(menu_id);
	return;
}

public func CreateConstructionMenuBackground(object target)
{
	return
	{
		Target = target,
		Decoration = GUI_MenuDeco,
		BackgroundColor = 0xee403020
	};
}

public func CreateConstructionMenuSeparator()
{
	return 
	{
		Left = "60%",
		Right = "60%+0.5em",
		BackgroundColor = {Std = 0x50888888}	
	};
}

public func CreateStructureGrid(object clonk, int item_size)
{
	var structures = 
	{
		Target = menu_target,
		ID = 1,
		Right = "60%",
		Style = GUI_GridLayout
	};
	structures = MenuAddStructures(structures, clonk, item_size);
	return structures;
}

public func CreateStructureInfo()
{
	var structinfo = 
	{
		Target = menu_target,
		ID = 2,
		Left = "60%+0.5em"
	};
	// Bottom part for material costs, description and other written information.
	structinfo.description = 
	{
		Target = menu_target,
		Priority = 0x0fffff,
		Left = "0.25em",
		Right = "100%-0.25em",
		Top = "100%-3em",
		Bottom = "100%",	
		Text = nil // will be updated
	};
	structinfo.materials = 
	{
		Target = menu_target,
		Priority = 0x0fffff,
		Left = "0.25em",
		Right = "100%-0.25em",
		Top = "100%-5em",
		Bottom = "100%-3em",	
		Text = nil // will be updated
	};
	// Upper part is for the picture and power display.
	structinfo.picture = 
	{
		Target = menu_target,
		Bottom = "100%-5em",
		Margin = ["10%+0.5em"],
		Symbol = nil, // will be updated
		power_consumer =
		{
			Target = menu_target,
			Right = "1.5em",
			Bottom = "1.5em",	
			Symbol = nil // will be updated
		},
		power_producer = 
		{
			Target = menu_target,
			Left = "1.5em",
			Right = "3em",
			Bottom = "1.5em",	
			Symbol = nil // will be updated
		}	
	};
	structinfo.close_button = 
	{
		Target = menu_target,
		Left = "100%-2em", 
		Bottom = "2em",
		Symbol = Icon_Cancel,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "CloseConstructionMenu")
	};
	return structinfo;
}

public func MenuAddStructures(proplist struct, object clonk, int item_size)
{
	var plans = GetConstructionPlans(clonk->GetOwner());
	for (var structure in plans)
	{
		var str =
		{
			Target = menu_target,
			Right = Format("%dem", item_size),
			Bottom = Format("%dem", item_size),
			BackgroundColor = {Std = 0, Hover = 0x50ffffff},
			OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "OnConstructionHover", structure)],
			OnMouseOut = GuiAction_SetTag("Std"), 
			OnClick = GuiAction_Call(this, "OnConstructionSelection", {struct = structure, constructor = clonk}),
			Priority = structure->GetValue(),
			picture = 
			{
				Left = "8%",
				Right = "92%",
				Top = "8%",
				Bottom = "92%",
				Symbol = structure
			}
		};
		GuiAddSubwindow(str, struct);
	}
	return struct;
}

public func OnConstructionSelection(proplist par)
{
	ShowConstructionPreview(par.constructor, par.struct);
	CloseConstructionMenu();
	return;
}

public func OnConstructionHover(id structure)
{
	var struct_info = menu.struct_info;
	
	// Update the description and costs of this part of the menu.
	struct_info.description.Text = Format("%s: %s", structure->GetName(), structure.Description);
	struct_info.materials.Text = GetStructureMaterialsString(structure);
	
	// Update the picture of the structure.
	struct_info.picture.Symbol = structure;
	
	// Update also power consumption/production.
	if (structure->~IsPowerConsumer())
		struct_info.picture.power_consumer.Symbol = Library_PowerConsumer;
	else
		struct_info.picture.power_consumer.Symbol = nil;
		
	if (structure->~IsPowerProducer())
		struct_info.picture.power_producer.Symbol = Library_PowerProducer;
	else
		struct_info.picture.power_producer.Symbol = nil;
	
	// update everything - close the old info first to clean up possible remainers and then re-open it
	menu.struct_info = struct_info;
	GuiClose(menu_id, menu.struct_info.ID, menu.struct_info.Target);
	GuiUpdate({struct_info = menu.struct_info}, menu_id);
	return;
}

private func GetStructureMaterialsString(id structure) 
{
	var comp, index = 0;
	var components = [];
	while (comp = structure->GetComponent(nil, index++))
		components[GetLength(components)] = [comp, structure->GetComponent(comp)];

	var materials_string = "Costs: ";
	for (comp in components)
		materials_string = Format("%s %dx {{%i}}", materials_string, comp[1], comp[0]);
	return materials_string;
}

public func CloseConstructionMenu()
{
	GuiClose(menu_id, nil, menu_target);
	menu_id = nil;
	menu_target->RemoveObject();
	menu_target = nil;
	if (menu_controller)
		menu_controller->MenuClosed();
	menu_controller = nil;
	return;
}
