/**
	ConstructionSite
	Needs material put into it, then constructs the set building.

	@author boni
*/

local definition;		// this definition is being built here
local direction;
local stick_to;
local full_material;	// true when all needed material is in the site
local no_cancel;		// if true, site cannot be cancelled
local is_constructing;

// This should be recongnized as a container by the interaction menu independent of its category.
public func IsContainer() { return !full_material; }
// Disallow site cancellation. Useful e.g. for sites that are pre-placed for a game goal
public func MakeUncancellable() { no_cancel = true; return true; }

/*-- Testing / Development --*/

// Builds the construction even if the required materials isn't there.
// Use for debugging purposes (or maybe cool scenario effects)
public func ForceConstruct()
{
	full_material = true;
	StartConstructing();
}

/*-- Interaction --*/

public func HasInteractionMenu() { return true; }

public func GetInteractionMenuEntries(object clonk)
{
	// Default design of a control menu item
	var custom_entry = 
	{
		Right = "100%", Bottom = "2em",
		BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
		image = {Right = "2em"},
		text = {Left = "2em"}
	};
	
	return [{symbol = Icon_Cancel, extra_data = "abort", 
			custom =
			{
				Prototype = custom_entry,
				Priority = 1,
				text = {Prototype = custom_entry.text, Text = "$TxtAbort$"},
				image = {Prototype = custom_entry.image, Symbol = Icon_Cancel}
			}}];
}

public func GetMissingMaterialMenuEntries(object clonk)
{
	var material = GetMissingComponents();
	if (!material) return [];
	
	var entries = [];
	for (var mat in material)
	{
		var text = nil;
		if (mat.count > 1) text = Format("x%d", mat.count);
		PushBack(entries, {symbol = mat.id, text = text});
	}
	return entries;
}

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];		
	var comp_menu =
	{
		title = "$TxtMissingMaterial$",
		entries_callback = this.GetMissingMaterialMenuEntries,
		BackgroundColor = RGB(50, 0, 0),
		Priority = 15
	};
	PushBack(menus, comp_menu);
	var prod_menu =
	{
		title = "$TxtAbort$",
		entries_callback = this.GetInteractionMenuEntries,
		callback = "OnInteractionControl",
		callback_hover = "OnInteractionControlHover",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 50),
		Priority = 20
	};
	PushBack(menus, prod_menu);
	return menus;
}

public func OnInteractionControlHover(id symbol, string action, desc_menu_target, menu_id)
{
	var text = "";
	if (action == "abort")
	{
		if (no_cancel)
			text = "$TxtNoAbortDesc$";
		else
			text = "$TxtAbortDesc$";
	}
	GuiUpdateText(text, menu_id, 1, desc_menu_target);
}

public func OnInteractionControl(id symbol, string action, object clonk)
{
	if (action == "abort")
	{
		if (!Deconstruct())
			Sound("UI::Click*", false, nil, clonk->GetOwner());
	}
}

/*-- Engine callbacks --*/

public func Deconstruct()
{
	// Remove site
	if (no_cancel)
	{
		return false;
	}
	else
	{
		for (var contained in FindObjects(Find_Container(this)))
		{
			contained->Exit();
		}
		RemoveObject();
		return true;
	}
}

public func Construction()
{
	this.visibility = VIS_None;
	definition = nil;
	full_material = false;
	
	return true;
}

// Scenario saving
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Name");
	if (definition) props->AddCall("Definition", this, "Set", definition, direction, stick_to);
	if (no_cancel) props->AddCall("NoCancel", this, "MakeUncancellable");
	return true;
}

// Only allow collection if needed
public func RejectCollect(id def, object obj)
{
	var max = definition->GetComponent(def);
	
	// Not a component?
	if (max == 0)
	{
		return true;
	}
	// Reject collection if full
	return GetAvailableComponentCount(def) >= max;
}

// Check if full
public func Collection2(object obj)
{
	UpdateStatus(obj);
}

// Component removed (e.g.: Contained wood burned down or some externel scripts went havoc)
// Make sure lists are updated
public func ContentsDestruction(object obj) { return UpdateStatus(); }
public func Ejection(object obj) { return UpdateStatus(); }

/*-- Internals --*/

public func Set(id def, int dir, object stick)
{
	definition = def;
	direction = dir;
	stick_to = stick;
	
	// Set the shape of the construction site.
	var w = def->~GetSiteWidth(direction, stick_to) ?? def->GetDefWidth();
	var h = def->~GetSiteHeight(direction, stick_to) ?? def->GetDefHeight();
	// Height of construction site needs to exceed 12 pixels for the clonk to be able to add materials.
	var site_h = Max(12, h);
	SetShape(-w/2, -site_h, w, site_h);
	// Increase shape for below surface constructions to allow for adding materials.
	if (definition->~IsBelowSurfaceConstruction())
		SetShape(-w/2, -2 * site_h, w, 2 * site_h);

	// Draw the building with a wired frame and large alpha unless site graphics is overloaded by definition
	if (!definition->~SetConstructionSiteOverlay(this, direction, stick_to))
	{
		SetConstructionSiteOverlayDefault(def, direction, stick_to, w, h);
	}

	SetName(Format(Translate("TxtConstruction"), def->GetName()));
	this.visibility = VIS_Owner | VIS_Allies;	
	ShowMissingComponents();
	return;
}


private func SetConstructionSiteOverlayDefault(id def, int dir, object stick, int w, int h)
{
	SetGraphics(nil, nil, 0);
	SetGraphics(nil, def, 1, GFXOV_MODE_Base);
	SetClrModulation(RGBa(255, 255, 255, 128), 1);
	// If the structure is a mesh, use wire frame mode to show the site.
	// TODO: use def->IsMesh() once this becomes available.
	if (def->GetMeshMaterial())
	{
		SetClrModulation(RGBa(255, 255, 255, 50), 1);
		SetGraphics(nil, def, 2, GFXOV_MODE_Base, nil, GFX_BLIT_Wireframe);
	}
	SetObjDrawTransform((1 - dir * 2) * 1000, 0, 0, 0, 1000, -h * 500, 1);
	SetObjDrawTransform((1 - dir * 2) * 1000, 0, 0, 0, 1000, -h * 500, 2);
}


private func UpdateStatus(object item)
{
	// Ignore any activity during construction
	if (is_constructing) return;
	
	// Update message
	ShowMissingComponents();
	
	// Update possibly open menus.
	UpdateInteractionMenus(this.GetMissingMaterialMenuEntries);
	
	// Update preview image
	if (definition) definition->~SetConstructionSiteOverlay(this, direction, stick_to, item);
	
	// Check if we're done?
	if (full_material)
	{
		var controller = GetOwner();
		if (item) controller = item->GetController();
		StartConstructing(controller);
	}
}


private func ShowMissingComponents()
{
	if (definition == nil)
	{
		Message("");
		return;
	}
		
	var stuff = GetMissingComponents();
	var msg = "@";
	for (var s in stuff)
		if (s.count > 0)
			msg = Format("%s %dx{{%i}}", msg, s.count, s.id);
	// Ensure that the message is not below the bottom of the map.
	var dy = 23 - Max(23 + GetY() - LandscapeHeight(), 0) / 2;
	CustomMessage(msg, this, NO_OWNER, 0, dy);
}


private func GetMissingComponents()
{
	if (definition == nil)
		return [];
	
	if (full_material == true)
		return [];
	
	// Set false again as soon as we find a missing component
	full_material = true;
	
	// Check for material
	var component, index = 0;
	var missing_material = CreateArray();
	while (component = definition->GetComponent(nil, index))
	{
		// Find material
		var max_amount = definition->GetComponent(component);
		var current_amount = GetAvailableComponentCount(component);
		var diff = max_amount - current_amount;
		
		if (diff > 0)
		{
			PushBack(missing_material, {id=component, count=diff});
			full_material = false;
		}		
		
		index++;
	}
	
	return missing_material;
}


private func StartConstructing(int by_player)
{
	if (!definition || !full_material)
		return;

	is_constructing = true;

	// Find all objects on the bottom of the area that are not stuck
	var lying_around = GetObjectsLyingAround();

	// Create the site?
	var site = CreateConstructionSite();
	if (site)
	{
		StartConstructionEffect(site, by_player);
	}
	
	// Clean up stuck objects
	EnsureObjectsLyingAround(lying_around);
}


// Create the construction, below surface constructions don't perform any checks.
// Uncancellable sites (for special game goals) are forced and don't do checks either
private func CreateConstructionSite()
{
	var checks = !definition->~IsBelowSurfaceConstruction() && !no_cancel;
	var site = CreateConstruction(definition, 0, 0, GetOwner(), 1, checks, checks);

	if (!site)
	{
		// Spit out error message. This could happen if the landscape changed in the meantime
		// a little hack: the message would immediately vanish because this object is deleted. So, instead display the
		// message on one of the contents.
		if (Contents(0))
		{
			CustomMessage("$TxtNoConstructionHere$", Contents(0), GetOwner(), nil,nil, RGB(255, 0, 0));
		}
		Deconstruct();
		return nil;
	}

	// Apply direction
	if (direction)
	{
		site->SetDir(direction);
	}
	// Inform about sticky building
	if (stick_to)
	{
		site->CombineWith(stick_to);
	}
	
	return site;
}


private func StartConstructionEffect(object site, int by_player)
{
	// Object provides custom construction effects?
	if (!site->~DoConstructionEffects(this))
	{
		// If not: Autoconstruct 2.0!
		Schedule(site, "DoCon(2)", 1, 50);
		Schedule(this, "RemoveObject()", 1);
		Global->ScheduleCall(nil, Global.GameCallEx, 51, 1, "OnConstructionFinished", site, by_player);
		site->Sound("Structures::FinishBuilding");
	}
}


private func TakeConstructionMaterials(object from_clonk)
{
	// Check for material
	var component, index = 0;
	var materials;
	var w = definition->GetDefWidth() + 10;
	var h = definition->GetDefHeight() + 10;

	while (component = definition->GetComponent(nil, index))
	{
		// Find material
		var count_needed = definition->GetComponent(component);
		index++;
		
		materials = CreateArray();
		// 1. Look for stuff in the clonk
		materials[0] = FindObjects(Find_ID(component), Find_Container(from_clonk));
		// 2. Look for stuff lying around
		materials[1] = from_clonk->FindObjects(Find_ID(component), Find_NoContainer(), Find_InRect(-w/2, -h/2, w,h));
		// 3. Look for stuff in nearby lorries/containers
		var i = 2;
		for (var container in from_clonk->FindObjects(Find_Or(Find_Func("IsLorry"), Find_Func("IsContainer")), Find_InRect(-w/2, -h/2, w,h)))
			materials[i] = FindObjects(Find_ID(component), Find_Container(container));
		// Move it
		for (var material_list in materials)
		{
			for (var material in material_list)
			{
				if (count_needed <= 0)
				{
					break;
				}
				material->Exit();
				material->Enter(this);
				count_needed--;
			}
		}
	}
}


// Gets the number of available components of a type.
// This defaults to ContentsCount(), but can be overloaded
// for implementations of the construction site.
private func GetAvailableComponentCount(id component)
{
	return ContentsCount(component);
}


// Find all objects on the bottom of the area that are not stuck
private func GetObjectsLyingAround()
{
	var wdt = GetObjWidth();
	var hgt = GetObjHeight();
	return FindObjects(Find_Category(C4D_Vehicle | C4D_Object | C4D_Living), Find_AtRect(-wdt/2 - 2, -hgt, wdt + 2, hgt + 12), Find_OCF(OCF_InFree), Find_NoContainer());
}


// Clean up stuck objects
private func EnsureObjectsLyingAround(array lying_around)
{
	for (var thing in lying_around)
	{
		if (!thing) continue;
		
		var x, y;
		var moved = 0;
		
		x = thing->GetX();
		y = thing->GetY();
		
		// Move living creatures upwards till they stand on top.
		if (thing->GetOCF() & OCF_Alive)
		{
			while (thing->GetContact(-1, CNAT_Bottom))
			{
				// Only up to 20 pixel
				if (moved > 20)
				{
					thing->SetPosition(x, y);
					break;
				}
				
				moved++;
				thing->SetPosition(x, y - moved);
			}
		}
		else
		{
			while (thing->Stuck())
			{
				// Only up to 20 pixel
				if (moved > 20)
				{
					thing->SetPosition(x, y);
					break;
				}
				
				moved++;
				thing->SetPosition(x, y - moved);
			}
		}
	}
}

