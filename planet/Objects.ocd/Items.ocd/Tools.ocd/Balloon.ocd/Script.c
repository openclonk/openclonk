/*-- Balloon --*/

local user;

public func RejectUse(object clonk)
{
	// Disallow if directly above ground or water or if the Clonk is already holding onto something.
	return GBackSemiSolid(0,15) || clonk->GetActionTarget() != nil;
}

func ControlUseStart(object clonk, int ix, int iy)
{
	var balloon = CreateObjectAbove(BalloonDeployed,0,5);
	balloon->SetSpeed(clonk->GetXDir(),clonk->GetYDir());

	//sound
	Sound("BalloonInflate");

	//Lots of object pointers
	user = clonk;
	clonk->SetAction("Ride",balloon);
	balloon["rider"] = clonk;
	balloon["parent"] = this;

	//make sure clonk is not diving
	var side = "R";
	if(Random(2)) side = "L";
	user->PlayAnimation(Format("Jump.%s",side), 5, Anim_Linear(user->GetAnimationLength("Jump.L"), 0,
		user->GetAnimationLength("Jump.L"), 36, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	AddEffect("NoDrop",this,1,1,this);
	return 1;
}

func FxNoDropTimer(object target, effect, int timer)
{
	if(target->Contained() != user)
	{
		target->Enter(user);
	}
}

func Hit()
{
	Sound("GeneralHit?");
}

func IsInventorProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
