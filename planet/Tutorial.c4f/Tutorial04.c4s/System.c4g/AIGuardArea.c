// Makes the clonk guard an certain area.

#appendto Clonk

public func AI_GuardArea(int x, int y, int wdt, int hgt)
{
	var effect = AddEffect("IntAIGuardArea", this, 100, 20, this);
	EffectVar(0, this, effect) = x;
	EffectVar(1, this, effect) = y;
	EffectVar(2, this, effect) = wdt;
	EffectVar(3, this, effect) = hgt;
	return;
}


// Area is saved in effect vars 0 to 4 (x, y, wdt, hgt)
protected func FxIntAIGuardAreaTimer(object target, int num, int time)
{
	var x = EffectVar(0, this, num);
	var y = EffectVar(1, this, num);
	var wdt = EffectVar(2, this, num);
	var hgt = EffectVar(3, this, num);
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



