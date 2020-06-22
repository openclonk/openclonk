#appendto Axe

// EnemyAI calls this function when trying to attack with an axe
// Useful because circumvents tree chopping
public func ControlUse(object clonk, int iX, int iY)
{
	// Combat
	if (!CanStrikeWithWeapon(clonk) || !clonk->HasHandAction())
	{
		return false;
	}

	var rand = Random(2)+1;
	var arm = "R";
	var animation = Format("SwordSlash%d.%s", rand, arm);
	carry_bone = "pos_hand2";

	var length = this.StrikingLength;

	if (clonk->IsWalking())
	{
		if (!GetEffect("AxeStrikeStop", clonk, 0))
			AddEffect("AxeStrikeStop", clonk, 2, length, this);
	}
	if (clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
		animation  = Format("SwordSlash%d.%s", rand, arm);
	}
	if (clonk->IsJumping())
	{
		rand = 1;
		if (clonk->GetYDir() < -5) rand = 2;
		animation = Format("SwordJump%d.%s",rand, arm);
	}

	PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();

	magic_number=((magic_number + 1)%10) + (ObjectNumber()*10);
	StartWeaponHitCheckEffect(clonk, length, 1);

	return true;
}