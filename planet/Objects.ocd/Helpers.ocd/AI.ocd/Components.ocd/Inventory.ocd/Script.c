/**
	AI helper functions: Inventory
	
	Contains functions that are related AI inventory.

	@author Sven2, Maikel
*/

/*-- Public interface --*/

// Set the current inventory to be removed when the clonk dies. Only works if clonk has an AI.
public func BindInventory(object clonk)
{
	AssertDefinitionContext(Format("BindInventory(%v)", clonk));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	var cnt = clonk->ContentsCount();
	fx_ai.bound_weapons = CreateArray(cnt);
	for (var i = 0; i < cnt; ++i)
		fx_ai.bound_weapons[i] = clonk->Contents(i);
	return true;
}


/*-- Callbacks --*/

// Callback from the effect Destruction()-call
public func OnRemoveAI(proplist fx_ai, int reason)
{
	_inherited(fx_ai, reason);

	// Remove weapons on death.
	if (reason == FX_Call_RemoveDeath)
	{
		if (fx_ai.bound_weapons)
			for (var obj in fx_ai.bound_weapons)
				if (obj && obj->Contained() == fx_ai.Target)
					obj->RemoveObject();
	}
}
