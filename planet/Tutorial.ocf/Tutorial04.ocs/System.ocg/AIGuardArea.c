// Makes the clonk guard an certain area.

#appendto Clonk

public func AI_GuardArea(int x, int y, int wdt, int hgt)
{
	var effect = AddEffect("IntAIGuardArea", this, 100, 20, this);
	effect.x = x;
	effect.y = y;
	effect.wdt = wdt;
	effect.hgt = hgt;
	return;
}


// Area is saved in effect vars 0 to 4 (x, y, wdt, hgt)
protected func FxIntAIGuardAreaTimer(object target, effect, int time)
{
	var x = effect.x;
	var y = effect.y;
	var wdt = effect.wdt;
	var hgt = effect.hgt;
	var enemy = FindObject(Find_Owner(0), Find_Exclude(target), Find_OCF(OCF_CrewMember), Find_InRect(AbsX(x), AbsY(y), wdt, hgt), Sort_Distance());
 	if (enemy && !target->GetCommand(0, 0))
 	{ 
		if (target->AI_HasRangedWeapon())	
			target->SetCommand("Call", target, nil, nil, enemy, "AI_RangedAttack");
		else if (target->AI_HasMeleeWeapon())
			target->SetCommand("Call", target, nil, nil, enemy, "AI_MeleeAttack");
	}
	return 1;
}



