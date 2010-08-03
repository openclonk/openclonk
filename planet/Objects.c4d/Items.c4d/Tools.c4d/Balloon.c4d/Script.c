/*-- Balloon --*/

local user;

func ControlUseStart(object clonk, int ix, int iy)
{
	if(MaterialDepthCheck(0,0, "Sky") < 15) return 1;
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

func Definition(def) {
	SetProperty("Name", "$Name$" ,def);
	SetProperty("Collectible", 1, def);
}