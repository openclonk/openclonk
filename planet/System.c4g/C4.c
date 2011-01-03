/*--
		C4.c
		Authors:
		
		Old stuff which does not belong in the engine anymore.
--*/


// Stuff for the proplist changes.
static const DFA_NONE    = nil;
static const DFA_WALK    =  "WALK";
static const DFA_FLIGHT  =  "FLIGHT";
static const DFA_KNEEL   =  "KNEEL";
static const DFA_SCALE   =  "SCALE";
static const DFA_HANGLE  =  "HANGLE";
static const DFA_DIG     =  "DIG";
static const DFA_SWIM    =  "SWIM";
static const DFA_THROW   =  "THROW";
static const DFA_BRIDGE  =  "BRIDGE";
static const DFA_PUSH    =  "PUSH";
static const DFA_CHOP    =  "CHOP";
static const DFA_LIFT    =  "LIFT";
static const DFA_FLOAT   =  "FLOAT";
static const DFA_ATTACH  =  "ATTACH";
static const DFA_CONNECT =  "CONNECT";
static const DFA_PULL    =  "PULL";
static const Action = {
	Length = 1,
	Directions = 1,
	Step = 1,
	Procedure = DFA_NONE,
};

global func GetActMapVal(string entry, string action, id def, int num)
{
	if (!def)
		def = GetID();
	if (entry == "Facet")
		entry = ["X", "Y", "Wdt", "Hgt", "OffX", "OffY"][num];
	return GetProperty(entry, GetProperty(action, def));
}

global func ShowNeededMaterial(object of_obj)
{
	MessageWindow(GetNeededMatStr(of_obj), GetOwner(), nil, of_obj->GetName());
	return true;
}

// Removes a material pixel from the specified location, if the material is flammable.
// \par x X coordinate. Offset if called in object context.
// \par y Y coordinate. Offset if called in object context.
// \returns \c true if material was removed, \c false otherwise.
global func FlameConsumeMaterial(int x, int y)
{
	var mat = GetMaterial(x, y);
	if (mat == -1)
		return false;
	if (!GetMaterialVal("Inflammable", "Material", mat))
		return false;
	return !!ExtractMaterialAmount(x, y, mat, 1);
}

// Removes a material pixel from the specified location, if the material is a liquid.
// \par x X coordinate. Offset if called in object context.
// \par y Y coordinate. Offset if called in object context.
// \returns The material index of the removed pixel, or -1 if no liquid was found.
global func ExtractLiquid(int x, int y)
{
	var mat = GetMaterial(x, y);
	var density = GetMaterialVal("Density", "Material", mat);
	if (density < C4M_Liquid || density >= C4M_Solid)
		return -1;
	ExtractMaterialAmount(x, y, mat, 1);
	return mat;
}

// Splits the calling object into its components.
global func Split2Components()
{
	if (!this)
		return false;
	var ctr = Contained();
	// Transfer all contents to container.
	while (Contents())
		if (!ctr || !Contents()->Enter(ctr))
			Contents()->Exit();
	// Split components.
	for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(compid); ++j)
		{
			var comp = CreateObject(compid, nil, nil, GetOwner());
			if (OnFire()) comp->Incinerate();
			if (!ctr || !comp->Enter(ctr))
			{
				comp->SetR(Random(360));
				comp->SetXDir(Random(3) - 1);
				comp->SetYDir(Random(3) - 1);
				comp->SetRDir(Random(3) - 1);
			}
		}
	RemoveObject();
	return;
}
