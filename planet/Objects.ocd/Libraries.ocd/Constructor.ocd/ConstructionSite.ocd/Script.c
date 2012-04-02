/**
	ConstructionSite
	Needs material put into it, then constructs the set building.

	@author boni
*/

local definition;
local full_material; // true when all needed material is in the site

public func IsContainer()		{ return true; }
public func IsInteractable()	{ return definition != nil; }

public func Construction()
{
	this.visibility = VIS_None;
	definition = nil;
	full_material = false;
	
	return true;
}

public func Set(id def)
{
	definition = def;
	
	var w,h;
	w = def->GetDefWidth();
	h = def->GetDefHeight();
	
	SetGraphics(nil, def, 1, GFXOV_MODE_Base, nil, 1);
	SetObjDrawTransform(1000,0,0,0,1000, -h*500,1);
	SetShape(-w/2, -h, w, h);
	
	SetName(Format("Construction Site: %s",def->GetName()));
	
	this.visibility = VIS_Owner | VIS_Allies;
	
	ShowMissingComponents();
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
	// update message
	ShowMissingComponents();
	
	// check if we're done?
	if(full_material)
		StartConstructing();
}

// Interacting removes the Construction site
public func Interact(object clonk)
{
	// test
	for(var obj in FindObjects(Find_Container(this)))
		obj->Exit();
	
	RemoveObject();
}

private func ShowMissingComponents()
{
	if(definition == nil)
	{
		Message("");
		return;
	}
		
	var stuff = GetMissingComponents();
	var msg = "Construction Needs:";
	for(var s in stuff)
		if(s.count > 0)
			msg = Format("%s|%dx{{%i}}", msg, s.count, s.id);
	
	Message("@%s",msg);
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
	
	// create the construction
	var site;
	if(!(site = CreateConstruction(definition, 0, 0, GetOwner(), 1, 1, 1)))
	{
		Log("Can't build here anymore");
		Interact();
		return;
	}
	
	// Autoconstruct 2.0!
	Schedule(site, "DoCon(2)",1,50);
	Schedule(this,"RemoveObject()",1);
}