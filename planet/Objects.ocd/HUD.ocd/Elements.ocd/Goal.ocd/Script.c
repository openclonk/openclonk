/**
	HUD Goal
	Displays the current goal in the HUD.
	
	@authors Newton
*/

local goal;

protected func Initialize()
{
	// Set parallaxity
	this.Parallaxity = [0, 0];
	// Set visibility
	this.Visibility = VIS_Owner;
	return;
}

public func SetGoal(object to_goal)
{
	goal = to_goal;
	Update();
	return;
}

public func Update()
{
	if (!goal)
		return;	
	// Display short description under the goal.
	var hudinfo = goal->~GetShortDescription(GetOwner());
	if (hudinfo)
		CustomMessage(Format("@%s",hudinfo), this, GetOwner(), 0, 90);
	else
		CustomMessage("", this, GetOwner(), 0, 90);
	// Set goal graphics to current goal.
	SetGraphics(goal->GetGraphics(), goal->GetID(), 1, GFXOV_MODE_IngamePicture);
	return;
}

public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (!goal)
		return;
	// Let the goal decide on how to inform the player.
	goal->Activate(plr);
	return;
}
