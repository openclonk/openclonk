/*-- Balloon --*/

local user;

func ControlUseStart(object clonk, int ix, int iy)
{
	if(GBackSolid(0,15) || GBackLiquid(0,15) || clonk->GetActionTarget() != nil) return 1;
	var balloon = CreateObject(BalloonDeployed,0,-5);
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

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
