#appendto JarOfWinds

protected func ControlUse(object pClonk, iX, iY)
{
	if(!GetEffect("JarReload",this))
	{
		if(!GBackLiquid())
		{
			FireWeapon(pClonk, iX, iY);
			Amount=0;
			AddEffect("JarReload",this,100,2,this);
			Sound("WindCharge.ogg",false,nil,nil,1);
		}
		
		return true;
	}
	else
	{
		pClonk->Message("Reloading!");
		return true;
	}
	ChargeSoundStop();
}
