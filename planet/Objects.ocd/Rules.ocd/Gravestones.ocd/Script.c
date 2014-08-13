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
	effect.grave->SetInscription(clonk);
	clonk->RemoveObject();
	//smoke effect
	var particles =
	{
		Prototype = Particles_Dust(),
		R = 200,
		G = 100,
		B = 50,
		Size = PV_KeyFrames(0, 0, 0, 300, 40, 1000, 15)
	};
	effect.grave->CreateParticle("Dust", 0, 0, PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(18, 1 * 36), particles, 6);
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
