#strict 2

/*
	Object selector HUD
	Author: Newton
	
	For each inventory item (and each vehicle, house etc on the same pos)
	one of this objects exists in the bottom bar. If clicked, the associated
	object is selected (inventory item is selected, vehicle is grabbed
	or ungrabbed, house is entered or exited...).
	
	HUD elements are passive, they don't update their status by themselves
	but rely on the clonk to update their status.

	This object works only for crew members that included the standard clonk
	controls (see Libraries.c4d/ClonkControl.c4d)
	
*/

/*
	usage of layers:
	-----------------
	layer 0 - unused
	layer 1 - title
	layer 2 - select-marker
	
	layer 12 - hotkey

*/

local isSelected, crew, hotkey, myobject, inventory_pos, actiontype;

static const ACTIONTYPE_INVENTORY = 0;
static const ACTIONTYPE_VEHICLE = 1;
static const ACTIONTYPE_STRUCTURE = 2;

protected func Construction()
{
	_inherited();
	
	isSelected = false;
	hotkey = false;
	myobject = nil;
	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_None;
}

public func MouseSelection(int plr)
{
	if(!crew) return false;
	// invisible...
	if(this["Visibility"] != VIS_Owner) return false;
	
	// object is in inventory
	if(actiontype == ACTIONTYPE_INVENTORY)
	{
		crew->SelectItem(inventory_pos);
		return true;
	}

	// no object
	if(!myobject) return false;
	
	// object is a pushable vehicle
	if(actiontype == ACTIONTYPE_VEHICLE)
	{
		var proc = crew->GetProcedure();
		
		// crew is currently pushing my vehicle -> let go
		if(proc == "PUSH" && crew->GetActionTarget() == myobject)
		{
			PlayerObjectCommand(plr, false, "Ungrab");
			return true;
		}
		// grab
		else if(proc == "WALK")
		{
			PlayerObjectCommand(plr, false, "Grab", myobject);
			return true;
		}
	}
	
	// object is a building
	if(actiontype == ACTIONTYPE_STRUCTURE)
	{
		// inside? -> exit
		if(crew->Contained() == myobject)
		{
			PlayerObjectCommand(plr, false, "Exit");
			return true;
		}
		// outside? -> enter
		else if(crew->CanEnter())
		{
			PlayerObjectCommand(plr, false, "Enter", myobject);
			return true;
		}
	}
	// TODO: more script choices... Selection-Callbacks for  all objects
}

public func SetObject(object obj, int type, int inv_pos)
{
	actiontype = type;
	myobject = obj;
	inventory_pos = inv_pos;
	
	RemoveEffect("IntRemoveGuard",myobject);
	
	if(!myobject) 
	{	
		SetGraphics(nil,nil,1);
		SetName(Format("$LabelSlot$ %d",inventory_pos));
	}
	else
	{
		SetGraphics(nil,myobject->GetID(),1,GFXOV_MODE_IngamePicture);
		if(actiontype == nil)
		{
			if(myobject->Contained() == crew) actiontype = ACTIONTYPE_INVENTORY;
			else if(myobject->GetDefGrab()) actiontype = ACTIONTYPE_VEHICLE;
			else if(myobject->GetDefCoreVal("Entrance","DefCore",2) != nil) actiontype = ACTIONTYPE_STRUCTURE;
		}
		
		SetName(myobject->GetName());
		
		// create an effect which monitors whether the object is removed
		AddEffect("IntRemoveGuard",myobject,1,0,this);
	}
}

public func FxIntRemoveGuardStop(object target, int num, int reason, bool temp)
{
	if(reason == 3)
		if(target == myobject)
			SetObject(nil,0,inventory_pos);
}

public func SetCrew(object c)
{
	crew = c;
	SetOwner(c->GetOwner());
	
	this["Visibility"] = VIS_Owner;
}

public func SetHotkey(int num)
{
	if(num < 0 || num > 9)
	{
		SetGraphics(nil,nil,12);
		hotkey = false;
		return;
	}
	
	hotkey = true;
	var name = Format("%d",num);
	SetGraphics(name,NUMB,12,GFXOV_MODE_IngamePicture);
	SetObjDrawTransform(300,0,16000,0,300,-30000, 12);
	SetClrModulation(RGB(160,0,0),12);
}

public func UpdateSelectionStatus()
{
	// determine...
	isSelected = false;
	
	if(actiontype == ACTIONTYPE_VEHICLE)
		if(crew->GetProcedure() == "PUSH" && crew->GetActionTarget() == myobject)
			isSelected = true;
			
	if(actiontype == ACTIONTYPE_STRUCTURE)
		if(crew->Contained() == myobject)
			isSelected = true;

	if(actiontype == ACTIONTYPE_INVENTORY)
		if(crew->GetSelected() == inventory_pos)
			isSelected = true;
	
	if(!hotkey) return;

	if(isSelected)
	{
		SetClrModulation(RGB(220,0,0),12);
		SetObjDrawTransform(500,0,16000,0,500,-30000, 12);
	}
	else
	{
		SetClrModulation(RGB(160,0,0),12);
		SetObjDrawTransform(300,0,16000,0,300,-30000, 12);
	}
}
