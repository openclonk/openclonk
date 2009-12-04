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
	layer 2 - actionicon
	layer 3,4 - hands
	
	layer 12 - hotkey

*/

local isSelected, crew, hotkey, myobject, actiontype, hand;

static const ACTIONTYPE_INVENTORY = 0;
static const ACTIONTYPE_VEHICLE = 1;
static const ACTIONTYPE_STRUCTURE = 2;

private func HandSize() { return 400; }
private func IconSize() { return 500; }

protected func Construction()
{
	_inherited();
	
	isSelected = false;
	hotkey = 0;
	myobject = nil;
	hand = 0;
	
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
		hand = 0;
		crew->SelectItem(hotkey-1);
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

public func Clear()
{
	myobject = nil;
	actiontype = -1;
	hotkey = 0;
	this["Visibility"] = VIS_None;
}

public func SetObject(object obj, int type, int pos)
{
	if(actiontype != ACTIONTYPE_INVENTORY)
		if(obj == myobject)
			if(type == actiontype)
				if(pos+1 == hotkey)
					return;

	this["Visibility"] = VIS_Owner;
				
	actiontype = type;
	myobject = obj;
	hotkey = pos+1;
	
	RemoveEffect("IntRemoveGuard",myobject);
	
	if(!myobject) 
	{	
		SetGraphics(nil,nil,1);
		SetName(Format("$LabelSlot$ %d",hotkey-1));
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

	ShowHotkey();
	UpdateSelectionStatus();
}

public func FxIntRemoveGuardStop(object target, int num, int reason, bool temp)
{
	if(reason == 3)
		if(target == myobject)
			SetObject(nil,0,hotkey-1);
}

public func SetCrew(object c)
{
	if(crew == c) return;
	
	crew = c;
	SetOwner(c->GetOwner());
	
	this["Visibility"] = VIS_Owner;
}

public func ShowHotkey()
{
	if(hotkey > 10 || hotkey <= 0)
	{
		SetGraphics(nil,nil,12);
	}
	else
	{
		var num = hotkey;
		if(hotkey == 10) num = 0;
		var name = Format("%d",num);
		SetGraphics(name,NUMB,12,GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(300,0,16000,0,300,-34000, 12);
		SetClrModulation(RGB(160,0,0),12);
	}
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
		if(crew->GetSelected() == hotkey-1)
			isSelected = true;

	// and set the icon...
	if(isSelected)
	{
		SetClrModulation(RGB(220,0,0),12);
		SetObjDrawTransform(500,0,16000,0,500,-34000, 12);
		
		if(actiontype == ACTIONTYPE_VEHICLE)
			SetGraphics("LetGo",GetID(),2,GFXOV_MODE_Base);
			
		if(actiontype == ACTIONTYPE_STRUCTURE)
			SetGraphics("Exit",GetID(),2,GFXOV_MODE_Base);
	}
	else
	{
		SetClrModulation(RGB(160,0,0),12);
		SetObjDrawTransform(300,0,16000,0,300,-34000, 12);
		
		if(actiontype == ACTIONTYPE_VEHICLE)
			SetGraphics("Grab",GetID(),2,GFXOV_MODE_Base);
			
		if(actiontype == ACTIONTYPE_STRUCTURE)
			SetGraphics("Enter",GetID(),2,GFXOV_MODE_Base);
	}
	SetObjDrawTransform(IconSize(),0,-16000,0,IconSize(),20000, 2);
	
	// the hands...
	var hands = isSelected;
	// .. are not displayed for inventory if the clonk is inside
	// a building or is pushing something because the controls
	// are redirected to those objects
	if(actiontype == ACTIONTYPE_INVENTORY)
		if(crew->Contained() || crew->GetProcedure() == "PUSH")
			hands = false;
			
	if(hands)
	{
		if(hand == 0 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("One",GetID(),3,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,-16000,0,HandSize(),-12000, 3);
		}
		if(hand == 1 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("Two",GetID(),4,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,8000,0,HandSize(),-12000, 4);
		}
	}
	else
	{
		SetGraphics(nil,nil,3);
		SetGraphics(nil,nil,4);
	}
}
