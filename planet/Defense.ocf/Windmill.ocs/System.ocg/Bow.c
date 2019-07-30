#appendto Bow

local shooting_strength = 100;

public func FinishedAiming(object clonk, int angle)
{
	clonk->DetachMesh(iArrowMesh);
	iArrowMesh = nil;

	// shoot
	if (Contents(0))
	{
		if (Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle, shooting_strength, clonk);
			Sound("Objects::Weapons::Bow::Shoot?");
		}
	}

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold));
	clonk->PlayAnimation("Close1Hand", 11, Anim_Const(0));
	clonk->StartShoot(this);
	return true;
}