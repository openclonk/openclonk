#appendto JarOfWinds
local charges;


protected func ControlUse(object pClonk, iX, iY)
{
	
	if(!GetEffect("JarReload",this))
	{
		if(!GBackLiquid())
		{
			FireWeapon(pClonk, iX, iY);
			charges++;
			if(charges>5)
			{
				CastParticles("Magic",30,18,0,0,16,32,RGB(0,0,255),RGB(128,128,255));
				RemoveObject();
			}
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
