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
	balloon->LocalN("rider") = clonk;
	balloon->LocalN("parent") = this;

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

func Definition(def) {
	SetProperty("Name", "$Name$" ,def);
	SetProperty("Description", "$Description$", def);
	SetProperty("Collectible", 1, def);
}