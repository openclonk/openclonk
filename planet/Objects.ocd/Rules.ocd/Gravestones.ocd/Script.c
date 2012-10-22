/*-- Gravestones --*/


protected func Initialize()
{	
	// Under no circumstance there may by multiple copies of this rule.
	if (ObjectCount(Find_ID(Rule_Gravestones)) > 1)
		return RemoveObject();
	return;
}


public func OnClonkDeath(object clonk)
{
	if (!clonk->GetAlive())
		AddEffect("AddGravestone", clonk, 1, 1, this);
	return;
}

public func FxAddGravestoneTimer(object target, proplist effect, int timer)
{	
	// Wait for the death animation to be over.
	if (timer >= 20)
	{
		AddEffect("IntGravestone", target, 1, nil, this);
		return -1;
	}
}

public func FxIntGravestoneStart(object clonk, proplist effect)
{
	effect.grave = clonk->CreateObject(Clonk_Grave, 0, 0, clonk->GetController());
	clonk->Enter(effect.grave);
	
	//smoke effect
	effect.grave->CastParticles("ExploSmoke", RandomX(10,15), 0, 0, 6, 200, 250, HSLa(0,0,255,64), HSLa(0,0,255,64));
}

public func FxIntGravestoneStop(object clonk, proplist effect, int reason)
{
	if (reason != FX_Call_RemoveClear)
	{
		clonk->Exit();
		effect.grave->RemoveObject();
	}
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetProperty("Description"), iByPlayer);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
