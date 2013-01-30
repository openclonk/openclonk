/*
	adds an energy bar above the object.
	The energy bar uses either target.HitPoints & GetDamage() or target->GetMaxEnergy() & target->GetEnergy().
*/
global func AddEnergyBar()
{
	var e = AddEffect("ShowEnergyBar", this, 1, 0, nil, nil);
	if (e)
		return e.bar;
}

global func FxShowEnergyBarStart(target, effect, temp)
{
	if (temp) return;
	var attachpoint = { x = 0, y = target->GetDefOffset(1) - 5};
	var current, max;
	if (target->GetCategory() & C4D_Living)
	{
		max = target->~GetMaxEnergy();
		current = target->GetEnergy();
	}
	else
	{
		max = target.HitPoints;
		current = max - target->GetDamage();
	}
	
	if (current == nil || max == nil)
		return -1;
	
	effect.bar = target->CreateProgressBar(GUI_ShadedSimpleProgressBar, max, current, 0, target->GetOwner(), attachpoint, nil, { width = 28, height = 6, color = RGB(200, 1, 1) });
	effect.bar->SetPlane(750);
	// update once
	effect.Interval = 1;
	
	return 1;
}

global func FxShowEnergyBarTimer(target, effect, time)
{
	var value;
	if (target->GetCategory() & C4D_Living) value = target->GetEnergy();
	else value = target.HitPoints - target->GetDamage();
	
	effect.bar->SetValue(value);
	effect.Interval = 0;
	return 1;
}

global func FxShowEnergyBarDamage(target, effect, dmg, cause)
{
	effect.Interval = 1;
	return dmg;
}

global func FxShowEnergyBarStop(target, effect, reason, temp)
{
	if (temp) return;
	if (effect.bar)
		effect.bar->Close();
}