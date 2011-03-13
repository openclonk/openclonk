// Executes ranged attacks on target.

#appendto Clonk

protected func AI_RangedAttack(object clonk, int x, int y, object target)
{
	clonk->AI_LogCommandStack();
	clonk->AI_Log("Ranged attack on %v", target);
	// target in range?
	if (ObjectDistance(clonk, target) > 600 || target->Contained())
		return clonk->AI_Log("Target %v out of range", target);
		
	// target still alive?
	if (!(target->GetOCF() & OCF_Alive))
		return clonk->AI_Log("Target %v has died", target);
		
	// Look for ranged weapon in contents.
	for (var weapon in FindObjects(Find_Container(clonk), Find_Func("AI_IsRangedWeapon"), Find_Func("AI_CanHitTarget", target)))
	{
		// Check if weapon is loaded.
		if (weapon->~AI_IsLoaded())
		{
			clonk->AI_Log("Use ranged weapon %v", weapon);
			clonk->AppendCommand("Call", weapon, nil, nil, target, nil, weapon->AI_CommandString()); //add+finish
			return;
		}		
	}
	// Maybe an ranged weapon can be acquired easily.
	//for (var weapon in FindObjects(Find_Distance(100), Find_Func("AI_IsRangedWeapon"), Find_Not(Find_Container(clonk))))
	//{
	//	clonk->AppendCommand("Get", weapon);
	//	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_RangedAttack");
	//	return;
	//}
	
	// Return to basic attacks.

	return;
}

protected func AI_HasRangedWeapon()
{
	return !!FindObject(Find_Container(this), Find_Func("AI_IsRangedWeapon"));
}