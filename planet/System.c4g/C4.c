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

global func CastC4ID(x) { return x; }

// Abgelöst durch SetPosition
global func ForcePosition(object obj, int x, int y) { return SetPosition(x, y, obj); }

// Abgelöst durch RemoveObject
global func AssignRemoval(object obj) { return RemoveObject(obj); }

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

global func SetOnlyVisibleToOwner(bool fVisible, object pObj)
{
  var oldVal=GetOnlyVisibleToOwner(pObj);
  if (fVisible) 
    SetVisibility(VIS_Owner | VIS_God, pObj);
  else
    SetVisibility(VIS_All, pObj);
  return oldVal;
}
  
global func GetOnlyVisibleToOwner(object pObj)
{
  return (GetVisibility(pObj) == VIS_Owner | VIS_God);
}

global func MessageBoard(string msg, par0, par1, par2, par3, par4, par5, par6, par7, par8)
{
  return Log(msg, par0, par1, par2, par3, par4, par5, par6, par7, par8);
}

// Fasskonfiguration
// Kann z.B. durch eine Spielregel überladen werden (Shamino)
// Bit 0 (1): Wasserfässer sind auch im Verkauf 8 Clunker wert
// Bit 1 (2): Fässer werden beim Verkaufen nicht entleert (sind wieder voll kaufbar)
// Bit 2 (4): Nur Wasserfässer werden beim Verkaufen nicht entleert (sind wieder voll kaufbar)
global func BarrelConfiguration() { return 5; }
