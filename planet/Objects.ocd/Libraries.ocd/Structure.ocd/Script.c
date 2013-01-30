/**
	Structure Library
	Basic library for structures, handles:
	* Damage
	* Info dialogue
	
	@author Maikel
*/

public func Damage(int change, int cause, int cause_plr)
{
	// Only do stuff if the object has the HitPoints property.
	if (this && this.HitPoints != nil)
		if (GetDamage() >= this.HitPoints)
		{		
			// Destruction callback is made by the engine.
			return RemoveObject();
		}
	return _inherited(change, cause, cause_plr);
}

// provide information dialogue, see HUD.ocd/ObjectInfoDisplay.ocd and PlayerControl.c
public func HasObjectInformationDialogue()
{
	return true;
}

public func GetObjectInformationDialogue() { return nil; } // use custom dialogue
func GetMaxDamage(){return this.HitPoints;}
public func OnObjectInformationDialogueOpen(object dialogue)
{
	dialogue->SetDisplayData(
	[
		{type = HUD_OBJECTINFODISPLAY_CUSTOM, width = 200, lines = 0},
		{type = HUD_OBJECTINFODISPLAY_BORDER},
		{type = HUD_OBJECTINFODISPLAY_NAME, owner = true, lines = 1},
		{type = HUD_OBJECTINFODISPLAY_BAR, name = "$HitPoints$", lines = 1, cur = this.GetDamage, max = this.GetMaxDamage}
	]
	);
}