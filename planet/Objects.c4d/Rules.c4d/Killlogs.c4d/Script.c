/*-- Kill logs --*/

func Initialize()
{
	// only one at a time
	if(ObjectCount(Find_ID(GetID())) > 1) return RemoveObject();
}

public func Activate(int plr)
{
	MessageWindow(GetProperty("Description"), plr);
	return true;
}

func OnClonkDeath(object clonk, int killed_by)
{
	var plr=clonk->GetOwner();
	if(plr == NO_OWNER) return;
	
	// assert there are three StringTbl entries for each
	var which_one=Random(3)+1;
	
	if(!GetPlayerName(killed_by))
 		return Log(Translate(Format("KilledByGaya%d", which_one)),GetTaggedPlayerName(plr), clonk.Prototype->GetName());
 	if(plr == killed_by)
		return Log(Translate(Format("Selfkill%d", which_one)),GetTaggedPlayerName(plr), clonk.Prototype->GetName());
 	if(!Hostile(plr,killed_by))
  	return Log(Translate(Format("Teamkill%d", which_one)),GetTaggedPlayerName(plr), clonk.Prototype->GetName(), GetTaggedPlayerName(killed_by));
  Log(Translate(Format("KilledByPlayer%d", which_one)),GetTaggedPlayerName(plr), clonk.Prototype->GetName(), GetTaggedPlayerName(killed_by));
}

local Name = "$Name$";
local Description = "$Description$";
