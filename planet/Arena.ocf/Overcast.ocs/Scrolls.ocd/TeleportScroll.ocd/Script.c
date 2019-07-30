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
	DrawParticleLine("Flash", 0, 0,-GetX()+x,-GetY()+y, 3, 0, 0, 8, {Prototype = Particles_Flash(), Size = 20, R = 50, G = 50, B = 255});

	// Make sure the clonk loses the attach procedure.
	var action = clonk->GetAction();
	if (action && clonk.ActMap[action].Procedure == DFA_ATTACH)
		clonk->SetAction("Jump");
	clonk->SetPosition(x, y);
	clonk->SetXDir(0);
	clonk->SetYDir(-5);
	
	RemoveObject();
	return 1;
}


local Name = "$Name$";
local Description = "$Description$";

local Collectible = 1;
