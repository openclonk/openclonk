/*-- Bed --*/

local iAttachedMesh;
local iAnimation;

/* Initialize */
protected func Initialize()
{
//  SetEntrance(1);
  SetProperty("MeshTransformation", Trans_Rotate(20, 0, 1, 0));
}

func IsAssassinable() { return 1; }
func OnAssassination() { return 0; }

private func IsOccupied()
{
  var obj;
  if (obj = FindObject(Find_Container(this), Find_Func("IsClonk")))
  {
    if (!obj->GetAlive())
      return -1;
    return 1;
  }
  return 0;
}

public func IsFree() { return !IsOccupied(); }

protected func ActivateEntrance(object pClonk)
{
  // Entry query
  if (!pClonk->Contained() )
    if (pClonk->GetOCF() & OCF_CrewMember)
      return OnEntrance(pClonk);
  // Exit query
  if (pClonk->Contained() == this )
    return OnExit(pClonk);
  return 0; 
}

private func OnEntrance(object pClonk)
{
    // Already occupied
    if (IsOccupied()) return(CheckChallenge(pClonk));
    // Put corps in
    if (pClonk->~IsCarryingCorps())
    {
        var pCorps = pClonk->~GetCorps(1);
        pCorps->Enter(this);
        if (iAttachedMesh)
            DetachMesh(iAttachedMesh);
        iAttachedMesh = AttachMesh(pCorps, "Clonk", "Master", Trans_Scale(1100));
        FinishCommand(pClonk, 1);
        return 1;
    }
    // Enter
    pClonk->Enter(this);
    if (iAttachedMesh)
        DetachMesh(iAttachedMesh);
    iAttachedMesh = AttachMesh(pClonk, "Clonk", "Master", Trans_Scale(1100));
    iAnimation = pClonk->PlayAnimation("Sleep", CLONK_ANIM_SLOT_Death, Anim_Const(pClonk->GetAnimationLength("Sleep")), Anim_Const(1000));
    return 1;
}

private func OnExit(object pClonk)
{
  if (iAttachedMesh)
  {
    DetachMesh(iAttachedMesh);    
    pClonk->StopAnimation(iAnimation);
    iAttachedMesh = nil;
    iAnimation = nil;
  }
  pClonk->Exit(0,+15);
  
}

func Zzz()
{
	var iTime = GetActTime();
	CreateParticle("Zzz",8*(1-2*GetDir()),-3, 2+(iTime%50)*3/25 + RandomX(-1,+1),-5, 60,
                 RGBa(255, 255, 255, 128));
}
  
private func CheckChallenge(object pClonk)
{
  // Ein feindlicher Clonk von au�en fordert den Insassen
  var pEnemy = FindObject(Find_OCF(OCF_Alive), Find_Container(this));
  if (pEnemy)
	{
    if (!pEnemy->GetAlive())
    {
      OnExit(pEnemy);
      if (!Hostile(pClonk->GetOwner(), pEnemy->GetOwner()))
        pClonk->AI_LookForFriend(pEnemy);
    }
    else if (Hostile(pClonk->GetOwner(), pEnemy->GetOwner()))
		{
      OnExit(pEnemy);
			pEnemy->~DoShow(200);
			pClonk->~DoShow(200);
			if (pEnemy->~IsAI())
			  pEnemy->AITimer(pClonk);
      if (pClonk->~IsAI())
			  pClonk->AITimer(pEnemy);
//			pEnemy->FightWith(pClonk);
		}
	}
  return(0);
}

/* Bett l�dt Energie auf */  
  
private func RefillEnergy()
{
  var clonk = FindObject(0, 0, 0, 0, 0, OCF_CrewMember, 0, 0, this);
  if (clonk) DoEnergy(1, clonk);
}

local ActMap = {
Attach = {
    Prototype = Action,
    Procedure = DFA_ATTACH,
    Delay = 0,
},
};

local Name = "$Name$";
local Description = "$Description$";