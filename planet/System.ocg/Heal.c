/**
    Heal.c
    Function to heal livings over time.

    @author Armin
*/

/**
	Heals the object over time for /amount/ HP.
	Calling the function multiple times results in faster healing (as opposed to longer healing).
	
	If necessary, a custom interval can be set. The healing effect restores 1 energy per interval.
*/
global func Heal(int amount, int interval)
{
	AssertObjectContext();

	// Add effect.
	var fx = CreateEffect(FxHealingOverTime, 1, interval ?? 36);
	fx.healing_amount = amount;
	fx.done = 0;
	return fx;
}


static const FxHealingOverTime = new Effect
{
	Timer = func ()
	{
		// Stop healing the Clonk if he reached full health.
		if (Target->GetEnergy() >= Target->GetMaxEnergy() || this.done >= this.healing_amount)
		{
			return FX_Execute_Kill;
		}
		Target->DoEnergy(1);
		++this.done;
		return FX_OK;
	}
};
