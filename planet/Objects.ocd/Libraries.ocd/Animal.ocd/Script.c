/*-- Animal reproduction --*/

// Status
public func IsAnimal() { return true; }

// Population control
private func ReproductionAreaSize() { return 800; }   // The area, in which new animals of this kind can appear
private func ReproductionRate()     { return 4000; }  // The chane that reproduction takes place in one timer intervall
private func MaxAnimalCount()       { return 10; }    // The maximal animalcount in the area

// Special reproduction (e.g. with egg)
private func SpecialRepr()
{
}

// Special Conditions (e.g. a fish should have Swim action)
private func SpecialReprodCond()
{
	return 1;
}

// Count animals
private func CountMe()
{
	var ReprodSize = ReproductionAreaSize();
	var ReprodSizeHalb = ReprodSize	/ -2;
	return ObjectCount(Find_ID(GetID()), Find_InRect(ReprodSizeHalb, ReprodSizeHalb, ReprodSize , ReprodSize), Find_OCF(OCF_Alive));
}

/* Reproduction */

public func Reproduction(bool fRepr)
{
	// Already dead
	if (!GetAlive()) return 0;
	// Not full grown up
	if (GetCon() < 100) return 0;
	// Special conditions not fulfilled
	if (!SpecialReprodCond()) return 0;
	// Already to much animals of this kind
	//if(!FindObject(REPR)) { if (CountMe() >= MaxAnimalCount()) return 0; }
	//else if(ObjectCount(Find_ID(GetID()))+1 >= GetComponent(GetID(), 0, FindObject(Find_ID(REPR)))) return 0;
	// Reproduction
	if (!SpecialRepr())
	{
		// Normal reproduction
		var pChild = CreateConstruction(GetID(), 0, 0, -1, 40);
		pChild->~Birth();
	}
	// Success
	return 1;
}

/* Birth */

public func Birth()
{
	SetAction("Walk");
	if (Random(2)) SetComDir(COMD_Left);
	else SetComDir(COMD_Right);
	return 1;
}

/* Collection of animals */

local fForceEnter;

// Force collection
public func ForceEnter(object pContainer)
{
	fForceEnter = 1;
	Enter(pContainer);
	fForceEnter = 0;
	return 1;
}

protected func RejectEntrance(object pContainer)
	{
	// Handing over (z.B. Clonk->Lore) is always ok
	if (Contained()) return;
	// Dead? OK too
	if (!GetAlive()) return;
	// Forced? Well ok.
	if (fForceEnter) return;
	// All other cases depend on the global settings (game rule)
	return 1;// !Library_Animal_IsCollectible(pContainer); TODO create this rule :-)
}
