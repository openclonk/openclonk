/*--
	Scroll: Teleport
	Author: Mimmo

	Teleports you to a safe position.
--*/


public func ControlUse(object clonk, int x, int y)
{
	var pos = GetRandomSpawn();
	x = pos[0];
	y = pos[1];	
	
	DrawParticleLine("Magic",0,0,-GetX()+x,-GetY()+y,3,64,RGB(0,128,255),RGB(0,200,255),-1);
	DrawParticleLine("MagicFire",0,0,-GetX()+x,-GetY()+y,4,64,RGB(0,255,128),RGB(0,255,20),-1);
	clonk->SetPosition(x, y);
	clonk->SetXDir(0);
	clonk->SetYDir(-5);
	
	RemoveObject();
	return 1;
}


local Name = "$Name$";
local Description = "$Description$";

local Collectible = 1;
