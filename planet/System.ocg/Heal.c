/**
    Heal.c
    Function to heal livings over time.

    Author: Armin
*/

/**
	Heals the object over time for /amount/ HP.
	Calling the function multiple times results in faster healing (as opposed to longer healing).
*/
global func Heal(int amount)
{
	// Add effect.
	var fx = this->AddEffect("HealingOverTime", this, 1, 36);
	fx.healing_amount = amount;
	fx.done = 0;
	return fx;
}

global func FxHealingOverTimeTimer(object target, effect fx)
{
	// Stop healing the Clonk if he reached full health.
	if (target->GetEnergy() >= target.MaxEnergy/1000  || fx.done >= fx.healing_amount)
		return -1;
	target->DoEnergy(1);
	fx.done++;
	return FX_OK;
}
