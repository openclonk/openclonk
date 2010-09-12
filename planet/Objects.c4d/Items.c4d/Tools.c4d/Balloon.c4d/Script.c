/*-- Balloon --*/

local user;

func ControlUseStart(object clonk, int ix, int iy)
{
	if(GBackSolid(0,15) || GBackLiquid(0,15) || clonk->GetActionTarget() != nil) return 1;
	var balloon = CreateObject(BalloonDeployed,0,-5);
	balloon->SetSpeed(clonk->GetXDir(),clonk->GetYDir());

	//Lots of object pointers
	user = clonk;
	clonk->SetAction("Ride",balloon);
	balloon["rider"] = clonk;
	balloon["parent"] = this;

	AddEffect("NoDrop",this,1,1,this);
	return 1;
}

func FxNoDropTimer(object target, int num, int timer)
{
	if(target->Contained() != user)
	{
		target->Enter(user);
	}
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
