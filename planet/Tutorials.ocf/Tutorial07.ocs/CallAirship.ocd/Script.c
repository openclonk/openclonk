/**
	Call Airship
	Attached to the player which adds an interaction to call the airship.
	
	@author Maikel
*/


local pilot;
local airship;

public func Create(object clonk, object pilot, object airship)
{
	if (this != Helper_CallAirship)
		return;
	var helper = clonk->CreateObject(Helper_CallAirship);
	helper->SetAction("Attach", clonk);
	helper->SetPilot(pilot);
	helper->SetAirship(airship);
	return;
}

public func SetPilot(object to)
{
	pilot = to;
	return;
}

public func SetAirship(object to)
{
	airship = to;
	return;
}


/*-- Interaction --*/

// Players can talk to NPC via the interaction bar.
public func IsInteractable(object clonk) { return pilot && airship && ObjectDistance(clonk, airship) >= this.CallDistance; }

// Adapt appearance in the interaction bar.
public func GetInteractionMetaInfo(object clonk)
{
	if (GetEffect("MoveAirship", this))
		return { Description = "$MsgStopAirship$", IconName = nil, IconID = airship };
	return { Description = "$MsgCallAirship$", IconName = nil, IconID = airship };
}

// Called on player interaction.
public func Interact(object clonk)
{
	if (!clonk || !pilot || !airship)
		return true;
		
	if (GetEffect("MoveAirship", this))
		RemoveEffect("MoveAirship", this);
	else
		AddEffect("MoveAirship", this, 100, 1, this, nil, clonk, pilot, airship);
	return true;
}


/*-- Movement --*/

private func FxMoveAirshipStart(object target, proplist effect, int temp, object clonk, object pilot, object airship)
{
	if (temp)
		return FX_OK;
	effect.clonk = clonk;
	effect.pilot = pilot;
	effect.airship = airship;
	// Let the pilot grab the airship.
	effect.pilot->SetCommand("Grab", airship);
	// Let the pilot play a confirm sound.
	effect.pilot->PlaySoundConfirm();
	return FX_OK;
}

private func FxMoveAirshipTimer(object target, proplist effect)
{
	if (!effect.clonk || !effect.pilot || !effect.airship)
		return FX_Execute_Kill;
		
	if (ObjectDistance(effect.clonk, effect.airship) < this.CallDistance)
		return FX_Execute_Kill;
		
	if (effect.airship->GetCommand())
		return FX_OK;
	// Add a move to command to a bit above the clonk.	
	airship->SetCommand("MoveTo", nil, effect.clonk->GetX(), effect.clonk->GetY() - effect.airship->GetBottom() + effect.clonk->GetTop());
	return FX_OK;
}

private func FxMoveAirshipStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Let the pilot ungrab the airship.	
	if (effect.pilot)
		effect.pilot->SetCommand("UnGrab");
	// Remove the move command from the airship.
	if (effect.airship)
	{
		effect.airship->SetCommand("None");
		effect.airship->SetComDir(COMD_Stop);
	}
	return FX_OK;
}


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		Delay = 0,
		NextAction = "Attach",
	}
};
local Name = "$Name$";
local CallDistance = 60;
