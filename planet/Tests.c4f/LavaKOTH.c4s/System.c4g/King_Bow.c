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
			Sound("BowShoot*.ogg");
		}
	}

	// Open the hand to let the string go and play the fire animation
	PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), animation_set["ShootTime"], ANIM_Hold), Anim_Const(1000));
	clonk->PlayAnimation("Close1Hand", 11, Anim_Const(0), Anim_Const(1000));
	clonk->StartShoot(this);
	return true;
}

public func FxExplosiveArrowTimer(pTarget, iEffectNumber, iEffectTime)
{
	CastParticles("Spark",1,20,pTarget->GetX()-GetX(),pTarget->GetY()-GetY(),8+(EffectVar(0, pTarget, iEffectNumber)/2),12+(EffectVar(0, pTarget, iEffectNumber)/2),RGB(255,200,0),RGB(255,255,150));
	EffectVar(0, pTarget, iEffectNumber)++;
	if(!pTarget->GetXDir() && !pTarget->GetYDir()) EffectVar(0, pTarget, iEffectNumber) = Max(EffectVar(0, pTarget, iEffectNumber),65);
	if(EffectVar(0, pTarget, iEffectNumber)>90) pTarget->Explode(15+Random(7));
	
}

