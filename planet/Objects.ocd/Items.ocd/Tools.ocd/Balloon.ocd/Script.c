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
	Sound("BalloonInflate");

	// Make the clonk ride the balloon.
	clonk->SetAction("Ride", balloon);

	// Make sure clonk is not diving.
	var side = ["L", "R"][Random(2)];
	clonk->PlayAnimation(Format("Jump.%s", side), 5, Anim_Linear(clonk->GetAnimationLength("Jump.L"), 0, clonk->GetAnimationLength("Jump.L"), 36, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Ensure the balloon is not dropped or thrown.
	var effect = AddEffect("NoDrop", this, 1, 1, this);
	effect.user = clonk;
	return true;
}

// Replace me by a callback which blocks departure.
public func FxNoDropTimer(object target, proplist effect, int timer)
{
	if (target->Contained() != effect.user)
		target->Enter(effect.user);
	return FX_OK;
}

public func Hit()
{
	Sound("GeneralHit?");
}

public func IsInventorProduct() { return true; }


/*-- Properties --*/

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
