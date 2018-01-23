// Used by the AI to find targets to attack.

global func GetRandomAttackTarget(object attacker)
{
	// First let the scenario tell what to do by a gamecall.
	var target = GameCall("GiveRandomAttackTarget", attacker);
	if (target)
		return target;
	// Attack structures owned by the enemy of the attacker.
	var controller = attacker->GetController();
	for (var target in attacker->FindObjects(Find_Category(C4D_Structure), Find_Hostile(controller), attacker->Sort_Distance()))
		if (target && PathFree(attacker->GetX(), attacker->GetY(), target->GetX(), target->GetY()))
			return target;
	// Otherwise return random enemy structure.
	return FindObject(Find_Category(C4D_Structure), Find_Hostile(controller), Sort_Random());
}

global func GetRandomSiegeTarget(object attacker)
{
	// First let the scenario tell what to do by a gamecall.
	var target = GameCall("GiveRandomSiegeTarget", attacker);
	if (target)
		return target;
	// Attack structures owned by the enemy of the attacker.
	var controller = attacker->GetController();
	for (var target in attacker->FindObjects(Find_Category(C4D_Structure), Find_Hostile(controller), attacker->Sort_Distance()))
		if (target && PathFree(attacker->GetX(), attacker->GetY(), target->GetX(), target->GetY()))
			return target;
	// Otherwise return random enemy structure.
	return attacker->FindObject(Find_Category(C4D_Structure), Find_Hostile(controller), Sort_Random());
}