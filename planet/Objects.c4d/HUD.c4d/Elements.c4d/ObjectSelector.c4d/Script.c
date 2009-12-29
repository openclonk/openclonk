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

local selected, crew, hotkey, myobject, actiontype;

static const ACTIONTYPE_INVENTORY = 0;
static const ACTIONTYPE_VEHICLE = 1;
static const ACTIONTYPE_STRUCTURE = 2;

private func HandSize() { return 400; }
private func IconSize() { return 500; }

protected func Construction()
{
	_inherited();
	
	selected = 0;
	hotkey = 0;
	myobject = nil;
	
	// parallaxity
	this["Parallaxity"] = [0,0];
	
	// visibility
	this["Visibility"] = VIS_None;
}

public func MouseSelectionAlt(int plr)
{
	if(!crew) return false;
	if(plr != GetOwner()) return false;

	// object is in inventory
	if(actiontype == ACTIONTYPE_INVENTORY)
	{
		crew->SelectItem(hotkey-1,true);
		return true;
	}
}

public func MouseSelection(int plr)
{
	if(!crew) return false;
	if(plr != GetOwner()) return false;
	
	// object is in inventory
	if(actiontype == ACTIONTYPE_INVENTORY)
	{
		crew->SelectItem(hotkey-1);
		return true;
	}

	// no object
	if(!myobject) return false;
	
	// object is a pushable vehicle
	if(actiontype == ACTIONTYPE_VEHICLE)
	{
		var proc = crew->GetProcedure();
		
		// object is inside building -> activate
		if(crew->Contained() && myobject->Contained() == crew->Contained())
		{
			crew->SetCommand("Activate",myobject);
			return true;
		}
		// crew is currently pushing vehicle
		else if(proc == "PUSH")
		{
			// which is mine -> let go
			if(crew->GetActionTarget() == myobject)
				PlayerObjectCommand(plr, false, "UnGrab");
			else
				PlayerObjectCommand(plr, false, "Grab", myobject);
				
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
					return UpdateSelectionStatus();

	this["Visibility"] = VIS_Owner;

	actiontype = type;
	myobject = obj;
	hotkey = pos+1;
	
	RemoveEffect("IntRemoveGuard",myobject);
	
	if(!myobject) 
	{	
		SetGraphics(nil,nil,1);
		SetName(Format("$TxtSlot$",hotkey));
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
		
		SetName(Format("$TxtSelect$",myobject->GetName()));
		
		// create an effect which monitors whether the object is removed
		if(actiontype == ACTIONTYPE_INVENTORY)
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

public func Selected()
{
	return selected;
}

public func UpdateSelectionStatus()
{

	// determine...
	var sel = 0;
	
	if(actiontype == ACTIONTYPE_VEHICLE)
		if(crew->GetProcedure() == "PUSH" && crew->GetActionTarget() == myobject)
			sel = 1;
			
	if(actiontype == ACTIONTYPE_STRUCTURE)
		if(crew->Contained() == myobject)
			sel = 1;

	if(actiontype == ACTIONTYPE_INVENTORY)
	{
		if(crew->GetSelected() == hotkey-1)
			sel = 1;
		if(crew->GetSelected(true) == hotkey-1)
			sel = 2;
	}
			
	selected = sel;
	
	// and set the icon...
	if(selected)
	{
		SetClrModulation(RGB(220,0,0),12);
		SetObjDrawTransform(500,0,16000,0,500,-34000, 12);
		
		if(actiontype == ACTIONTYPE_VEHICLE)
		{
			SetGraphics("LetGo",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtUnGrab$",myobject->GetName()));
		}
			
		if(actiontype == ACTIONTYPE_STRUCTURE)
		{
			SetGraphics("Exit",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtExit$",myobject->GetName()));
		}
	}
	else
	{
		SetClrModulation(RGB(160,0,0),12);
		SetObjDrawTransform(300,0,16000,0,300,-34000, 12);
		
		if(actiontype == ACTIONTYPE_VEHICLE)
		{
			if(!(myobject->Contained()))
			{
				SetGraphics("Grab",GetID(),2,GFXOV_MODE_Base);
				SetName(Format("$TxtGrab$",myobject->GetName()));
			}
			else
			{
				SetGraphics("Exit",GetID(),2,GFXOV_MODE_Base);
				SetName(Format("$TxtPushOut$",myobject->GetName()));
			}
		}
		if(actiontype == ACTIONTYPE_STRUCTURE)
		{
			SetGraphics("Enter",GetID(),2,GFXOV_MODE_Base);
			SetName(Format("$TxtEnter$",myobject->GetName()));
		}
	}
	SetObjDrawTransform(IconSize(),0,-16000,0,IconSize(),20000, 2);
	
	UpdateHands();
}

public func UpdateHands()
{
	// the hands...
	var hands = selected;
	// .. are not displayed for inventory if the clonk is inside
	// a building or is pushing something because the controls
	// are redirected to those objects
	if(actiontype == ACTIONTYPE_INVENTORY)
		if(crew->Contained() || crew->GetProcedure() == "PUSH")
			hands = 0;
			
	if(hands)
	{
		if(hands == 1 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("One",GetID(),3,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,-16000,0,HandSize(),-12000, 3);
		}
		else SetGraphics(nil,nil,3);
		if(hands == 2 || actiontype != ACTIONTYPE_INVENTORY)
		{
			SetGraphics("Two",GetID(),4,GFXOV_MODE_Base);
			SetObjDrawTransform(HandSize(),0,8000,0,HandSize(),-12000, 4);
		}
		else SetGraphics(nil,nil,4);
	}
	else
	{
		SetGraphics(nil,nil,3);
		SetGraphics(nil,nil,4);
	}
}
