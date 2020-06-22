/**
	Structure Library
	Basic library for structures, handles:
	* Damage
	* Info dialogue
	* Energy bar if rule active
	* Basements
	* Value
	
	@author Maikel
*/

// All structure related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See Initialize
// for which variables are being used.
local lib_structure;

protected func Construction()
{
	// Initialize the single proplist for the structure library.
	if (lib_structure == nil)
		lib_structure = {};
	lib_structure.basement = nil;
	// This contains a list of materials that are needed for repairing damage. (proplist with {id, count})
	lib_structure.repair_materials = [];
	// Total value of the components - used to calculate the required material for repairing.
	lib_structure.total_component_value = nil;
	// Make a writable copy of the editor action.
	if (this.EditorActions == GetID().EditorActions)
		MakePropertyWritable("EditorActions");
	return _inherited(...);
}

// This object is a structure.
public func IsStructure() { return true; }


/*-- Damage Handling --*/

public func GetHitPoints()
{
	return this.HitPoints;
}

public func GetRemainingHitPoints()
{
	return this.HitPoints - GetDamage();
}

public func Damage(int change, int cause, int cause_plr)
{
	// Only do stuff if the object has the HitPoints property.
	if (this && this.HitPoints != nil)
	{
		if (GetDamage() >= this.HitPoints)
		{
			// Remove contents from the building depending on the type of damage.
			EjectContentsOnDestruction(cause, cause_plr);
			// Handle detruction with a custom callback? If not, remove the object.
			if (!this->~OnNoHitPointsRemaining(cause, cause_plr))
			{
				// Destruction callback is made by the engine.
				return RemoveObject();
			}
		}
		// Update all interaction menus with the new hitpoints.
		UpdateInteractionMenus(this.GetDamageMenuEntries);
	}
	return _inherited(change, cause, cause_plr, ...);
}

private func EjectContentsOnDestruction(int cause, int by_player)
{
	// Exit all objects in this structure.
	for (var obj in FindObjects(Find_Container(this)))
	{
		// For a non-blast destruction just place the objects at the bottom of the structure.
		var angle = Random(360);
		var x = RandomX(GetLeft(), GetRight());
		var y = GetBottom();
		var dx = 0;
		var dy = 0;
		var dr = 0;
		// Scatter objects around if the destruction is caused by a blast.
		if (cause == FX_Call_DmgBlast)
		{
			var speed = RandomX(3, 4);
			x = RandomX(-4, 4);
			y = RandomX(-4, 4);
			dx = Cos(angle, speed);
			dy = Sin(angle, speed);
			dr = RandomX(-20, 20);
		}
		obj->Exit(x, y, Random(360), dx, dy, dr);
		obj->SetController(by_player);	
	}
	return;
}


/*-- Basement Handling --*/

public func SetBasement(object to_basement)
{
	lib_structure.basement = to_basement;
	if (this.EditorActions)
	{
		if (lib_structure.basement)
			this.EditorActions.basement = nil;
		else
			this.EditorActions.basement = new GetID().EditorActions.basement {};
	}
	return;
}

public func GetBasement()
{
	if (lib_structure) return lib_structure.basement;
	return nil;
}

public func IsStructureWithoutBasement()
{
	return IsStructure() && !(lib_structure && lib_structure.basement);
}

public func AddBasement()
{
	var offset = this->~GetBasementOffset() ?? [0, 0];
	var basement = CreateObject(this->~GetBasementID() ?? Basement, offset[0], GetBottom() + 4 + offset[1]);
	basement->SetParent(this);
	return;
}


/*-- SolidMask --*/

// Move objects out of the solid mask of a structure: it assumes the structure's shape is fully covered by it solid mask (e.g. basement, bridge).
private func MoveOutOfSolidMask()
{
	// Find all objects inside the basement which are stuck.
	var wdt = GetObjWidth();
	var hgt = GetObjHeight();
	var lying_around = FindObjects(Find_Or(Find_Category(C4D_Vehicle), Find_Category(C4D_Object), Find_Category(C4D_Living)), Find_AtRect(-wdt / 2, -hgt / 2, wdt, hgt), Find_NoContainer());
	// Move up these objects.
	for (var obj in lying_around)
	{
		var x = obj->GetX();
		var y = obj->GetY();
		var delta_y = 0;
		var max_delta = obj->GetObjHeight() + hgt;
		// Move up object until it is not stuck any more.
		while (obj->Stuck() || obj->GetContact(-1, CNAT_Bottom))
		{
			// Only move up the object by at most its height plus the basements height.
			if (delta_y > max_delta)
			{
				obj->SetPosition(x, y);
				break;
			}
			delta_y++;
			obj->SetPosition(x, y - delta_y);
		}
	}
	return;
}


/*-- Value --*/

public func CalcDefValue()
{
	var value = 0;
	var comp, index = 0;
	while (comp = this->GetComponent(nil, index++))
	{
		var comp_value = comp->GetValue();
		var comp_amount = this->GetComponent(comp);
		value += comp_value * comp_amount;
	}
	return value;
}

/* Repair Materials */

// Adds one unit of a material to the internal repair list.
public func AddRepairMaterial(id material_id)
{
	// Sort into list of already contained
	for (var old_material in lib_structure.repair_materials)
	{
		if (old_material.id == material_id)
		{
			old_material.count += 1;
			return true;
		}
	}
	
	// New material?
	PushBack(lib_structure.repair_materials, {id = material_id, count = 1});
	return true;
}

// Calculates and caches the total component value.
private func GetTotalComponentValue()
{
	if (lib_structure.total_component_value == nil)
	{
		lib_structure.total_component_value = 0;
		
		var component, i = 0;
		while (component = GetComponent(nil, i++))
		{
			var count = GetComponent(component);
			lib_structure.total_component_value += count * component->GetValue();
		}
	}
	return lib_structure.total_component_value;
}

// Returns all materials that are currently needed to repair the structure fully.
public func GetRepairMaterials()
{
	// Safety.
	if (!GetComponent()) return [];
	
	var damage = GetDamage();
	
	// Reset repair materials if structure was magically repaired to 100%.
	if (damage <= 0)
	{
		lib_structure.repair_materials = [];
		return [];
	}
	
	// Otherwise, figure out materials that could be used to repair the structure.
	
	// Initialize total component value (calculate only once for speed).
	var total_component_value = GetTotalComponentValue();
	
	// Check if the material list already contains enough materials to repair all the damage.
	var material_value = 0;
	for (var material in lib_structure.repair_materials)
	{
		material_value += material.count * material.id->GetValue();
	}
	// Multiply by 1000 to prevent issues with rounding.
	var remaining_damage_value = 1000 * total_component_value * GetDamage() / GetHitPoints() - 1000 * material_value;
	
	if (remaining_damage_value <= 0)
	{
		return lib_structure.repair_materials;
	}
	
	// Otherwise, we need to fill the remaining space with additional materials.
	while (remaining_damage_value > 0)
	{
		// Do a random sample of components weighted by value and amount.
		var random_sample = Random(total_component_value);
		
		var current_offset = 0;
		var component, i = 0;
		var found = false;
		while (component = GetComponent(nil, i++))
		{
			var count = GetComponent(component);
			var value = component->GetValue();
			var weight = count * value;
			current_offset += weight;
			
			if (random_sample <= current_offset)
			{
				remaining_damage_value -= 1000 * value;
				AddRepairMaterial(component);
				found = true;
				break;
			}
		}
		
		// Failsafe. No components?
		if (!found)
			break;
	}
	
	return lib_structure.repair_materials;
}

/*-- Interaction --*/

// Always show an interaction menu with at least the damage entry.
public func HasInteractionMenu() { return true; }

public func RejectInteractionMenu(object clonk)
{
	if (GetCon() < 100) return Format("$MsgNotFullyConstructed$", GetName());
	return _inherited(clonk, ...);
}

// Show damage and allow a player to repair the building when damaged.
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];		
	var damage_menu =
	{
		title = "$Damage$",
		entries_callback = this.GetDamageMenuEntries,
		callback = "OnRepairSelected",
		callback_hover = "OnRepairMenuHover",
		callback_target = this,
		BackgroundColor = RGB(75, 50, 0),
		Priority = 90
	};
	PushBack(menus, damage_menu);
	return menus;
}

// Returns the contents of the "damage" section in the interaction menu.
public func GetDamageMenuEntries()
{
	var is_invincible = this.HitPoints == nil || this->IsInvincible();
	var damage_text = "$Invincible$";
	var color = RGB(0, 150, 0);
	
	if (!is_invincible)
	{
		if (GetDamage() == 0) damage_text = "$NotDamaged$";
		else if (GetDamage() < this.HitPoints / 2)
		{
			damage_text = "$SlightlyDamaged$";
			color = RGB(200, 150, 0);
		}
		else
		{
			damage_text = "<c ff0000>$HeavilyDamaged$</c>";
			color = RGB(150, 0, 0);
		}
	}
	
	var menu = 
	{
		Bottom = "2em",
		right =
		{
			Left = "2em + 0.2em",
			Right = "100% - 0.2em",
			bottom = {Top = "50%", Margin = "0.1em", BackgroundColor = RGB(0, 0, 0)},
			top = {Text = damage_text, Style = GUI_TextHCenter}
		},
		symbol = 
		{
			Right = "2em",
			Symbol = Hammer
		}
	};
	// Show hit points.
	var percent = "100%";	
	if (!is_invincible)
		percent = Format("%d%%", 100 * GetRemainingHitPoints() / GetHitPoints());
	menu.right.bottom.fill = {BackgroundColor = color, Right = percent, Margin = "0.1em"};	
		
	// Cross out hammer symbol.
	if (is_invincible || GetDamage() == 0)
	{
		menu.symbol.overlay = {Margin = "0.25em", Symbol = Icon_Cancel};
	}	
		
	return [{symbol = Hammer, extra_data = "repair", custom = menu}];
}

public func OnRepairSelected(id symbol, string action, object cursor)
{
	// Repairing requires a hammer or something alike.
	var hammer = FindObject(Find_Container(cursor), Find_Func("IsConstructor"));
	
	if (!hammer)
	{
		PlayerMessage(cursor->GetOwner(), "$YouNeedAHammer$");
		Sound("UI::Click2", {player = cursor->GetOwner()});
		return;
	}
	
	// Check whether some of the required material can be found in the Clonk or the structure.
	var materials = GetRepairMaterials();
	var total_repair_value = 0;
	
	for (var material in materials)
	{
		while (material.count > 0)
		{
			var real_material = FindObject(Find_Or(Find_Container(this), Find_Container(cursor)), Find_ID(material.id));
			if (!real_material) break;
			
			var repair_value = real_material->GetValue();
			total_repair_value += repair_value;
			real_material->RemoveObject();
			material.count -= 1;
		}
	}
	
	if (total_repair_value == 0)
	{
		PlayerMessage(cursor->GetOwner(), "$YouNeedMaterials$");
		Sound("UI::Click2", {player = cursor->GetOwner()});
		return;
	}
	
	// Now repair based on the value fraction. Round up - rather be nice to the player.
	var total_component_value = GetTotalComponentValue();
	var total_damage_repaired = GetHitPoints() * total_repair_value / total_component_value + 1;
	DoDamage(-total_damage_repaired);
	UpdateInteractionMenus(this.GetDamageMenuEntries);
	
	// Clean up the material list and remove nil entries.
	var new_list = [];
	for (var material in lib_structure.repair_materials)
	{
		if (material.count > 0)
			PushBack(new_list, material);
	}
	lib_structure.repair_materials = new_list;
	
	Sound("Structures::Repair");
}

// On hovering, show a list of materials that are needed for repairing the structure.
public func OnRepairMenuHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "$NoRepairNecessary$";
	var is_invincible = this.HitPoints == nil || this->IsInvincible();
	if (!is_invincible && GetDamage() > 0)
	{
		var materials = GetRepairMaterials();
		text = "$RepairRequires$";
		
		var first = true;
		for (var material in materials)
		{
			if (first)
			{
				text = Format("%s %dx%s ({{%i}})", text, material.count, material.id->GetName(), material.id);
				first = false;
			}
			else
			{
				text = Format("%s, %dx%s ({{%i}})", text, material.count, material.id->GetName(), material.id);
			}
		}
	}
	
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func Flip()
{
	// Mirror structure
	if (this->~NoConstructionFlip())
		return false;
	return SetDir(1 - GetDir());
}

private func FlipVertices()
{
	// Flips all vertices around the Y = 0 axis, this can be used to flip the vertices of asymmetric structures.
	for (var cnt = 0; cnt < GetVertexNum(); cnt++)
	{
		SetVertex(cnt, VTX_X, -GetVertex(cnt, VTX_X));
		SetVertex(cnt, VTX_Y, GetVertex(cnt, VTX_Y));
	}
	return;
}


public func Definition(def, ...)
{
	if (!def.EditorProps) def.EditorProps = {};
	if (!def.EditorActions) def.EditorActions = {};
	def.EditorActions.basement = { Name = "$Basement$", EditorHelp = "$BasementHelp$", Command = "AddBasement()" };
	if (!def->~NoConstructionFlip())
		def.EditorActions.flip = { Name = "$Flip$", EditorHelp = "$FlipHelp$", Command = "Flip()" };
	return _inherited(def, ...);
}
