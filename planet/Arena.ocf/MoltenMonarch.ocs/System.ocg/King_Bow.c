#appendto Bow

local king_size;

public func MakeKingSize() { king_size = true; SetMeshMaterial("Kingwood",1); return(SetMeshMaterial("KingLeather", 0)); }
public func MakeNormalSize() { king_size = false; SetMeshMaterial("wood",0); return(SetMeshMaterial("Leather", 1)); }
public func Departure() { MakeNormalSize(); }

// Callback from the clonk, when he actually has stopped aiming
public func FinishedAiming(object clonk, int angle)
{
	clonk->DetachMesh(iArrowMesh);
	iArrowMesh = nil;

	// shoot
	if(Contents(0))
	{
		if(Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle,100,clonk);
			if(king_size)
			{
				AddEffect("ExplosiveArrow",arrow,100,1,this);
				arrow->SetClrModulation(RGB(255,128,0));
			}
			Sound("BowShoot?");
		}
	}

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold), Anim_Const(1000));
	clonk->PlayAnimation("Close1Hand", 11, Anim_Const(0), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

public func FxExplosiveArrowTimer(pTarget, effect, iEffectTime)
{
	pTarget->CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), Particles_Glimmer(), 5);
	effect.timer++;
	if(!pTarget->GetXDir() && !pTarget->GetYDir()) effect.timer = Max(effect.timer,65);
	if(effect.timer>90) pTarget->Explode(15+Random(7));
	
}

