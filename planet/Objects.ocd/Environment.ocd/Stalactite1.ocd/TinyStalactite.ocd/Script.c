/**
	TinyStalactite
	Covers up the base of stalactites

	@author Armin, Win
*/

private func Initialize()
{
}

private func SetChild(object target)
{
	child = target;
	AddEffect("Parent", this, 1, 10, this);
}

private func FxParentTimer(object target, effect, int time)
{
	if (!child)
	{
		child = CreateObject(Stalactite1);
	}
	//SetPosition(parent->GetX(), parent->GetY());
}

private func DrawWaterSource()
{
	var xold = GetX();
	var yold = GetY()-8;
	var xnew = GetX();
	var ynew = GetY()-8;
	for (var i = 0; i < RandomX(30, 140); i++)
	{
		ynew--;
		xnew = xold + RandomX(-1, 1);
		if (GBackSemiSolid(xnew-GetX(), ynew-GetY()))
			DrawMaterialQuad("Water", xold,yold, xold+1,yold, xold+1,yold+1, xold,yold+1);	
		else
			return;
		xold = xnew;
		yold = ynew;
	}
}

local Name = "$Name$";
local Description = "$Description$";
local Plane = 500;
local child;
local mat;