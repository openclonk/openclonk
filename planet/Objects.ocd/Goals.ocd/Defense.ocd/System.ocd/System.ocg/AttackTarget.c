// Used by the AI to find targets to attack.

global func GetRandomAttackTarget(object attacker)
{
	// First let the scenario tell what to do by a gamecall.
	var target = GameCall("GiveRandomAttackTarget", attacker);
	if (!target)
	{
		// Attack structures owned by the enemy of the attacker.
		var controller = attacker->GetController();
		var target = FindObject(Find_Category(C4D_Structure), Find_Hostile(controller), Sort_Random());
	}
	return target;
}