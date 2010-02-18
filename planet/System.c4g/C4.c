/*-- Altes Zeug, das nicht mehr in die Engine muss --*/

// stuff for the proplist changes
static const DFA_NONE    =-1;
static const DFA_WALK    = 0;
static const DFA_FLIGHT  = 1;
static const DFA_KNEEL   = 2;
static const DFA_SCALE   = 3;
static const DFA_HANGLE  = 4;
static const DFA_DIG     = 5;
static const DFA_SWIM    = 6;
static const DFA_THROW   = 7;
static const DFA_BRIDGE  = 8;
static const DFA_BUILD   = 9; 
static const DFA_PUSH    =10;
static const DFA_CHOP    =11;
static const DFA_LIFT    =12;
static const DFA_FLOAT   =13;
static const DFA_ATTACH  =14;
static const DFA_FIGHT   =15;
static const DFA_CONNECT =16;
static const DFA_PULL    =17;
static Action;

global func GetActMapVal(string strEntry, string strAction, id idDef, int iEntryNr) {
  if (!idDef) idDef = GetID();
  if (strEntry == "Facet") strEntry = ["X", "Y", "Wdt", "Hgt", "OffX", "OffY"][iEntryNr];
  return GetProperty(strEntry, GetProperty(strAction, idDef));
}
 
global func ShowNeededMaterial(object pOfObject)
{
  MessageWindow(GetNeededMatStr(pOfObject), GetOwner(),CXCN,pOfObject->GetName());
  return 1;
}

/// Removes a material pixel from the specified location, if the material is flammable.
/// \par x X coordinate. Offset if called in object context.
/// \par y Y coordinate. Offset if called in object context.
/// \returns \c true if material was removed, \c false otherwise.
global func FlameConsumeMaterial(int x, int y)
{
	var mat = GetMaterial(x, y);
	if (mat == -1) return false;
	if (!GetMaterialVal("Inflammable", "Material", mat)) return false;
	return !!ExtractMaterialAmount(x, y, mat, 1);
}


/// Removes a material pixel from the specified location, if the material is a liquid.
/// \par x X coordinate. Offset if called in object context.
/// \par y Y coordinate. Offset if called in object context.
/// \returns The material index of the removed pixel, or -1 if no liquid was found.
global func ExtractLiquid(int x, int y)
{
	var mat = GetMaterial(x, y);
	var density = GetMaterialVal("Density", "Material", mat);
	if (density < C4M_Liquid || density >= C4M_Solid) return -1;
	ExtractMaterialAmount(x, y, mat, 1);
	return mat;
}

/*-- Special effects --*/
global func LaunchEffect(id type, int x, int y /*, ... */)
{
	var fx = CreateObject(type, AbsX(x), AbsY(y));
	return fx && fx->Activate(x, y, ...);
}

/// Splits the calling object into its components.
global func Split2Components()
{
	if (!this) return false;
	var ctr = Contained();
	// Transfer all contents to container
	while (Contents())
		if (!ctr || !Contents()->Enter(ctr))
			Contents()->Exit();
	// Split
	for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(compid); ++j)
		{
			var comp = CreateObject(compid, nil, nil, GetOwner());
			if (OnFire()) comp->Incinerate();
			if (!ctr || !comp->Enter(ctr))
			{
				comp->SetR(Random(360));
				comp->SetXDir(Random(3)-1);
				comp->SetYDir(Random(3)-1);
				comp->SetRDir(Random(3)-1);
			}
		}
	RemoveObject();
}
