/** 
	Balloon
	Inflatable balloon which acts like a parachute.
*/


public func RejectUse(object clonk)
{
	// Disallow if directly above ground or water or if the Clonk is already holding onto something.
	return GBackSemiSolid(0, 15) || clonk->GetActionTarget() != nil;
}

public func ControlUseStart(object clonk)
{
	// Create the balloon and set its speed and rider.
	var balloon = CreateObjectAbove(BalloonDeployed, 0, 5);
	balloon->SetSpeed(clonk->GetXDir(), clonk->GetYDir());
	balloon->SetRider(clonk);
	balloon->SetParent(this);

	// Sound.
	Sound("Objects::Balloon::Inflate");

	// Make the clonk ride the balloon.
	clonk->SetAction("Ride", balloon);

	// Make sure clonk is not diving.
	var side = ["L", "R"][Random(2)];
	clonk->PlayAnimation(Format("Jump.%s", side), CLONK_ANIM_SLOT_Movement, Anim_Linear(clonk->GetAnimationLength("Jump.L"), 0, clonk->GetAnimationLength("Jump.L"), 36, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	return true;
}

// Ensure the balloon is not dropped or thrown.
public func QueryRejectDeparture(object clonk)
{
	if (!clonk)
		return false;
	if (!clonk->GetActionTarget())
		return false;
	if (clonk->GetAction() == "Ride" && clonk->GetActionTarget()->~GetParent() == this)
		return true;
	return false;
}

public func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func IsInventorProduct() { return true; }


/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
