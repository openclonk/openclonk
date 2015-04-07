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

public func IsContainer()		{ return !full_material; }
// disallow taking stuff out
public func RefuseTransfer(object toMove) { return true; }
// disallow site cancellation. Useful e.g. for sites that are pre-placed for a game goal
public func MakeUncancellable() { no_cancel = true; return true; }

// we have 2 interaction modes
public func IsInteractable(object obj)	{ return definition != nil && !full_material; }
public func GetInteractionCount() { return 1 + !no_cancel; }
public func GetInteractionMetaInfo(object obj, int num)
{
	if(num == 0)
		return {IconName=nil, IconID=Hammer, Description="$TxtTransfer$"};
	if(num == 1 && !no_cancel)
		return {IconName=nil, IconID=Icon_Cancel, Description="$TxtAbort$"};
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

	var xw = (1 - dir * 2) * 1000;
	var w, h;
	w = def->GetDefWidth();
	h = def->GetDefHeight();
	// Draw the building with a wired frame and large alpha unless site graphics is overloaded by definition
	if (!def->~SetConstructionSiteOverlay(this, direction, stick_to))
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
		SetGraphics("", GetID(), 3, GFXOV_MODE_ExtraGraphics);
	}
	SetObjDrawTransform(xw,0,0,0,1000, -h*500,1);
	SetObjDrawTransform(xw,0,0,0,1000, -h*500,2);
	// Height of construction site needs to exceed 12 pixels for the clonk to be able to add materials.
	h = Max(12, h);
	SetShape(-w/2, -h, w, h);
	// Increase shape for below surface constructions to allow for adding materials.
	if (definition->~IsBelowSurfaceConstruction())
		SetShape(-w/2, -2 * h, w, 2 * h);
	
	SetName(Format(Translate("TxtConstruction"),def->GetName()));
	
	this.visibility = VIS_Owner | VIS_Allies;
	
	ShowMissingComponents();
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
	var max = GetComponent(def, nil, nil, definition);
	
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
	
	// Update preview image
	if (definition) definition->~SetConstructionSiteOverlay(this, direction, stick_to, obj);
	
	// check if we're done?
	if(full_material)
		StartConstructing();
}

// component removed (e.g.: Contained wood burned down or some externel scripts went havoc)
// Make sure lists are updated
public func ContentsDestruction(object obj) { return Collection2(nil); }
public func Ejection(object obj) { return Collection2(nil); }

// Interacting removes the Construction site
public func Interact(object clonk, int num)
{
	// Open Contents-Menu
	if(num == 0)
	{
		clonk->CreateContentsMenus();
	}
	// Remove Site
	if(num == 1 && !no_cancel)
	{
		// test
		for(var obj in FindObjects(Find_Container(this)))
			obj->Exit();
	
		RemoveObject();
	}
}

private func ShowMissingComponents()
{
	if(definition == nil)
	{
		Message("");
		return;
	}
		
	var stuff = GetMissingComponents();
	//var msg = "Construction Needs:";
	var msg = "@";
	for(var s in stuff)
		if(s.count > 0)
			msg = Format("%s %dx{{%i}}", msg, s.count, s.id);
	
	//Message("@%s",msg);
	CustomMessage(msg, this, NO_OWNER, 0, 23);
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
	while (comp = GetComponent(nil, index, nil, definition))
	{
		// find material
		var max_amount = GetComponent(comp, nil, nil, definition);
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

private func StartConstructing()
{
	if(!definition)
		return;
	if(!full_material)
		return;
	
	is_constructing = true;
	
	// find all objects on the bottom of the area that are not stuck
	var wdt = GetObjWidth();
	var hgt = GetObjHeight();
	var lying_around = FindObjects(Find_Or(Find_Category(C4D_Vehicle), Find_Category(C4D_Object), Find_Category(C4D_Living)),Find_InRect(-wdt/2 - 2, -hgt, wdt + 2, hgt + 12), Find_Not(Find_OCF(OCF_InFree)),Find_NoContainer());
	
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
		Interact(nil, 1);
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