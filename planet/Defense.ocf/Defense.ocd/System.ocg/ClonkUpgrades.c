/* Apply tech upgrades to clonk */

#appendto Clonk

func Construction(...)
{
	if (g_homebases)
	{
		var base = g_homebases[GetOwner()];
		if (base)
		{
			// Apply max energy
			this.MaxEnergy = this.Prototype.MaxEnergy * base.tech_life;
		}
	}
	return _inherited(...);
}
