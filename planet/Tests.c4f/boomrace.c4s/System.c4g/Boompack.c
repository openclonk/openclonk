#appendto BOOM

func Launch(int angle, object clonk)
{
	if(clonk) clonk->AddEffect("RespawnBoom",clonk,1,90,nil,nil);
	return _inherited(angle, clonk);
}