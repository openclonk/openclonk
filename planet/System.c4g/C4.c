/*-- Altes Zeug, das nicht mehr in die Engine muss --*/

#strict 2

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

// Für Szenarien ohne Objects.c4d...
global func EmptyBarrelID() { return BARL; }

// Fügt das Material in ein Fass im Objekt ein
global func ObjectInsertMaterial(int imat, object pTarget)
{
  if (!pTarget || imat == -1) return; // Kein Zielobjekt / Material?
  // Fasstyp ermitteln
  var idBarl;
  if (idBarl = GetBarrelType(imat))
  {
    // Fass suchen
    var pBarl = FindFillBarrel(pTarget, idBarl);
    if (pBarl)
      // Fass auffüllen
      return pBarl->BarrelDoFill(1, imat+1);
  }
  // Kein Fass? Dann Objekt überlaufen lassen
  return InsertMaterial(imat, GetX(pTarget)-GetX(), GetY(pTarget)-GetY());
}
  
// Auffüllbares Fass im Objekt suchen
global func FindFillBarrel(object pInObj, id type)
{
  // Alle Inhaltsobjekte durchlaufen
  var pObj;
  for(var i = 0; pObj = Contents(i, pInObj); i++)
    // ID stimmt?
    if(pObj->GetID() == type)
      // Fass nicht voll?
      if (!pObj->~BarrelIsFull())
        // Nehmen wir doch das
        return pObj;
  // Nix? Dann halt ein leeres Fass suchen und füllen
  if (!(pObj=FindContents(EmptyBarrelID(), pInObj))) return;
  ChangeDef(type, pObj);
  return pObj;
}

// Flüssigkeit aus Fässern im Objekt extrahieren
global func ObjectExtractLiquid(object pFrom)
{
  // Alle Inhaltsobjekte durchlaufen
  var pObj;
  for(var i = 0; pObj = Contents(i, pFrom); i++)
  {
    // Extrahieren
    var iRet = pObj->~BarrelExtractLiquid();
    if(iRet != -1) return iRet;
  }
  // Extrahieren nicht möglich
  return -1;
}
  
global func ShowNeededMaterial(object pOfObject)
{
  MessageWindow(GetNeededMatStr(pOfObject), GetOwner(),CXCN,GetName(pOfObject));
  return 1;
}

// Fasskonfiguration
// Kann z.B. durch eine Spielregel überladen werden (Shamino)
// Bit 0 (1): Wasserfässer sind auch im Verkauf 8 Clunker wert
// Bit 1 (2): Fässer werden beim Verkaufen nicht entleert (sind wieder voll kaufbar)
// Bit 2 (4): Nur Wasserfässer werden beim Verkaufen nicht entleert (sind wieder voll kaufbar)
global func BarrelConfiguration() { return 5; }

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
	var fx = CreateObject(type, x, y);
	return fx && fx->Activate(AbsX(x), AbsY(y), ...);
}

/// Creates an earthquake at the specified location.
/// \par x X coordinate. Always global.
/// \par y Y coordinate. Always global.
/// \returns \c true if the earthquake could be launched, \c false otherwise.
global func LaunchEarthquake(int x, int y)
{
	return LaunchEffect(FXQ1, x, y);
}

/// Creates a lightning bolt at the specified location.
/// \par x X coordinate. Always global.
/// \par y Y coordinate. Always global.
/// \par xdir Average horizontal speed of the bolt.
/// \par xrange Maximum deviation from the average horizontal speed.
/// \par ydir Average vertical speed of the bolt.
/// \par yrange Maximum deviation from the average vertical speed.
/// \par fDoGamma If \c true, the lightning bolt will flash the screen.
/// \returns \c true if the lightning could be launched, \c false otherwise.
global func LaunchLightning(int x, int y, int xdir, int xrange, int ydir, int yrange, bool fDoGamma)
{
	return LaunchEffect(FXL1, x, y, xdir, xrange, ydir, yrange, fDoGamma);
}

