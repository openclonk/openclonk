/*--- The Base ---*/

// Author: Randrian

// Determines it the Building is acutally a base
local fIsBase;
local iEnergy;

// ---------------- Settings for base funcionallity --------------------
// --- these functions can be overloaded for vendors or special bases ---

// Determines if the base can heal allied clonks
public func CanHeal() { return true; }
// The amount of energy the base can store, if none the base can't heal
public func GetHeal() { return 100; }
// The money one filling of the bases energy storage costs
public func GetHealCost() { return 5;}

// Determines if the base can extinguish allied clonks
public func CanExtinguish() { return true; }

// Does the base block enemies?
public func CanBlockEnemies() { return true; }


// ------------- Base states -------------------------------

// This Building can be a base
public func IsBaseBuilding() { return true; }

// This Building is a base at the moment
public func IsBase() { return fIsBase; }

// Makes this building a base or removes the base functionallity
public func MakeBase(bool fRemoveBase)
{
	if(fRemoveBase)
	{
		fIsBase = 0;
		RemoveEffect("IntBase", this);
	}
	else
	{
		fIsBase = 1;
		AddEffect("IntBase", this, 1, 10, this);
		if(!FindObject(Find_ID(BaseMaterial), Find_Owner(GetOwner())))
			CreateObjectAbove(BaseMaterial,AbsX(10),AbsY(10),GetOwner());
	}
}

// ---------- Healing, Extinguishing and Autosell -----------

func FxIntBaseTimer(pThis, effect, iTime)
{
	var pObj;
	// Can this base heal? Then look for clonks that need some
	if(CanHeal() && GetHeal())
		for(pObj in FindObjects(Find_Container(this), Find_OCF(OCF_CrewMember), Find_Allied(GetOwner())))
		{
			if(pObj->GetEnergy() < pObj->GetMaxEnergy() && !GetEffect("IntBaseHeal", pObj))
				AddEffect("IntBaseHeal", pObj, 1, 1, this);
		}
	// Can this base extinguish? Then look for something on fire
	if(CanExtinguish())
		for(pObj in FindObjects(Find_Container(this), Find_OCF(OCF_OnFire), Find_Allied(GetOwner())))
			pObj->Extinguish();
}

func FxIntBaseHealTimer(pClonk, effect)
{
	// The clonk has left the base? Stop!
	if(pClonk->Contained() != this) return -1;
	// Full energy? Stop too.
	if(pClonk->GetEnergy() >= pClonk->GetMaxEnergy()) return -1;

	// No energy left? Buy some
	if(!iEnergy)
	{
		if(GetWealth(GetOwner()) >= GetHealCost())
		{
			DoWealth(GetOwner(), -GetHealCost());
			Sound("UnCash", 0, 100, pClonk->GetOwner()+1); // TODO: get sound
			iEnergy = GetHeal()*5;
		}
	}
	// Some energy in the storage? heal clonk
	if(iEnergy)
	{
		pClonk->DoEnergy(200, 1, FX_Call_EngBaseRefresh, GetOwner()+1);
		iEnergy--;
	}
}

local Name = "$Name$";
