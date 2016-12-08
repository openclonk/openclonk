/**
	ConstructionSite
	Needs material put into it, then constructs the set building.

	@author boni
*/

local definition;
local direction;
local stick_to;
local full_material; // true when all needed material is in the site
local no_cancel; // if true, site cannot be cancelled
local is_constructing;

// This should be recongnized as a container by the interaction menu independent of its category.
public func IsContainer() { return !full_material; }
// disallow taking stuff out
public func RefuseTransfer(object toMove) { return true; } // TODO: this is not used by another function, is this a callback?
// disallow site cancellation. Useful e.g. for sites that are pre-placed for a game goal
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
	// default design of a control menu item
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

public func Deconstruct()
{
	// Remove Site
	if (!no_cancel)
	{
		for(var obj in FindObjects(Find_Container(this)))
			obj->Exit();
		RemoveObject();
		return true;
	}
	return false;
}

public func Construction()
{
	this.visibility = VIS_None;
	definition = nil;
	full_material = false;
	
	return true;
}

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

	SetName(Format(Translate("TxtConstruction"), def->GetName()));
	this.visibility = VIS_Owner | VIS_Allies;	
	ShowMissingComponents();
	return;
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

// only allow collection if needed
public func RejectCollect(id def, object obj)
{
	var max = definition->GetComponent(def);
	
	// not a component?
	if(max == 0)
		return true;
	if(ContentsCount(def) < max)
		return false;
	
	return true;
}

// check if full
public func Collection2(object obj)
{
	// Ignore any activity during construction
	if (is_constructing) return;
	
	// update message
	ShowMissingComponents();
	
	// Update possibly open menus.
	UpdateInteractionMenus(this.GetMissingMaterialMenuEntries);
	
	// Update preview image
	if (definition) definition->~SetConstructionSiteOverlay(this, direction, stick_to, obj);
	
	// check if we're done?
	if(full_material)
		StartConstructing(obj->GetController());
}

// component removed (e.g.: Contained wood burned down or some externel scripts went havoc)
// Make sure lists are updated
public func ContentsDestruction(object obj) { return Collection2(nil); }
public func Ejection(object obj) { return Collection2(nil); }

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
	if(definition == nil)
		return;
	
	if(full_material == true)
		return nil;
	
	// set false again as soon as we find a missing component
	full_material = true;
	
	// check for material
	var comp, index = 0;
	var missing_material = CreateArray();
	while (comp = definition->GetComponent(nil, index))
	{
		// find material
		var max_amount = definition->GetComponent(comp);
		var c = ContentsCount(comp);
		var dif = max_amount-c;
		
		if(dif > 0)
		{
			PushBack(missing_material, {id=comp, count=dif});
			full_material = false;
		}		
		
		index++;
	}
	
	return missing_material;
}

private func StartConstructing(int by_player)
{
	if(!definition)
		return;
	if(!full_material)
		return;
	
	is_constructing = true;
	
	// find all objects on the bottom of the area that are not stuck
	var wdt = GetObjWidth();
	var hgt = GetObjHeight();
	var lying_around = FindObjects(Find_Category(C4D_Vehicle | C4D_Object | C4D_Living), Find_AtRect(-wdt/2 - 2, -hgt, wdt + 2, hgt + 12), Find_OCF(OCF_InFree), Find_NoContainer());
	
	// create the construction, below surface constructions don't perform any checks.
	// uncancellable sites (for special game goals) are forced and don't do checks either
	var site;
	var checks = !definition->~IsBelowSurfaceConstruction() && !no_cancel;
	if(!(site = CreateConstruction(definition, 0, 0, GetOwner(), 1, checks, checks)))
	{
		// spit out error message. This could happen if the landscape changed in the meantime
		// a little hack: the message would immediately vanish because this object is deleted. So, instead display the
		// message on one of the contents.
		if(Contents(0))
			CustomMessage("$TxtNoConstructionHere$", Contents(0), GetOwner(), nil,nil, RGB(255,0,0));
		Deconstruct();
		return;
	}
	
	if(direction)
		site->SetDir(direction);
	// Inform about sticky building
	if (stick_to)
		site->CombineWith(stick_to);
	
	// Object provides custom construction effects?
	if (!site->~DoConstructionEffects(this))
	{
		// If not: Autoconstruct 2.0!
		Schedule(site, "DoCon(2)",1,50);
		Schedule(this,"RemoveObject()",1);
		Global->ScheduleCall(nil, Global.GameCallEx, 51, 1, "OnConstructionFinished", site, by_player);
		site->Sound("Structures::FinishBuilding");
	}
	
	// clean up stuck objects
	for(var o in lying_around)
	{
		if (!o) continue;
		
		var x, y;
		var dif = 0;
		
		x = o->GetX();
		y = o->GetY();
		
		// move living creatures upwards till they stand on top.
		if(o->GetOCF() & OCF_Alive)
		{
			while(o->GetContact(-1, CNAT_Bottom))
			{
				// only up to 20 pixel
				if(dif > 20)
				{
					o->SetPosition(x,y);
					break;
				}
				
				dif++;
				o->SetPosition(x, y-dif);
			}
		}
		else {
			while(o->Stuck())
			{
				// only up to 20 pixel
				if(dif > 20)
				{
					o->SetPosition(x,y);
					break;
				}
				
				dif++;
				o->SetPosition(x, y-dif);
			}
		}
	}
}

func TakeConstructionMaterials(object from_clonk)
{
	// check for material
	var comp, index = 0;
	var mat;
	var w = definition->GetDefWidth() + 10;
	var h = definition->GetDefHeight() + 10;

	while (comp = definition->GetComponent(nil, index))
	{
		// find material
		var count_needed = definition->GetComponent(comp);
		index++;
		
		mat = CreateArray();
		// 1. look for stuff in the clonk
		mat[0] = FindObjects(Find_ID(comp), Find_Container(from_clonk));
		// 2. look for stuff lying around
		mat[1] = from_clonk->FindObjects(Find_ID(comp), Find_NoContainer(), Find_InRect(-w/2, -h/2, w,h));
		// 3. look for stuff in nearby lorries/containers
		var i = 2;
		for(var cont in from_clonk->FindObjects(Find_Or(Find_Func("IsLorry"), Find_Func("IsContainer")), Find_InRect(-w/2, -h/2, w,h)))
			mat[i] = FindObjects(Find_ID(comp), Find_Container(cont));
		// move it
		for(var mat2 in mat)
		{
			for(var o in mat2)
			{
				if(count_needed <= 0)
					break;
				o->Exit();
				o->Enter(this);
				count_needed--;
			}
		}
	}
}
