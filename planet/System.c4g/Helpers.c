/* Some useful helper functions */

#strict 2

global func MessageWindow(string pMsg, int iForPlr, id idIcon, string pCaption)
  {
  // get icon
  if (!idIcon) idIcon=GetID();
  // get caption
  if (!pCaption) pCaption=GetName();
  // create msg window (menu)
  var pCursor = GetCursor(iForPlr);
  if (!CreateMenu(idIcon, pCursor, pCursor, 0, pCaption, 0, 2)) return;
  AddMenuItem(pCaption, "", TIM1, pCursor,0,0,pMsg);
  return 1;
  }

global func RemoveAll(id idDef, int dwOCF)
  {
  var Cnt, obj = FindObject(idDef, 0,0,0,0, dwOCF), next;
  while (obj)
    {
    // Get the next object in case obj->Destruction does funny things
    next = FindObject(idDef, 0,0,0,0, dwOCF, 0,0,0, obj);
    if(RemoveObject(obj))
      ++Cnt;
    obj = next;
    }
  return Cnt;
  }

global func CastlePanic()
  {
  return ResortObjects("CastlePanicResort", 1);
  }

global func CastlePanicResort(object pObj1, object pObj2)
  {
  return GetDefBottom(pObj1)-GetDefBottom(pObj2);
  }

global func SetBit(int iOldVal, int iBitNr, bool iBit)
  {
  if(GetBit(iOldVal, iBitNr) != (iBit != 0)) 
    return ToggleBit(iOldVal, iBitNr);
  return iOldVal;
  }

global func GetBit(int iValue, int iBitNr)
  {
  return (iValue & (1 << iBitNr)) != 0;
  }

global func ToggleBit(int iOldVal, int iBitNr)
  {
  return iOldVal ^ (1 << iBitNr);
  }

global func RGB(int r, int g, int b) { return (r & 255)<<16 | (g & 255)<<8 | (b & 255); }
global func RGBa (int r, int g, int b, int a) { return (a & 255)<<24 | (r & 255)<<16 | (g & 255)<<8 | (b & 255); }

global func DrawParticleLine (szKind, x0, y0, x1, y1, prtdist, a, b0, b1, ydir)
  {
  // Parameter gültig?
  if (!prtdist) return 0;
  // Anzahl der benötigten Partikel berechnen
  var prtnum = Max(Distance(x0, y0, x1, y1) / prtdist, 2);
  var i=prtnum;
  // Partikel erzeugen!
  while (i>-1)
    {
    var i1,i2,b; i2 = i*256/prtnum; i1 = 256-i2;

    b =   ((b0&16711935)*i1 + (b1&16711935)*i2)>>8 & 16711935
        | ((b0>>8&16711935)*i1 + (b1>>8&16711935)*i2) & -16711936;
    if (!b && (b0 | b1)) ++b;
    CreateParticle(szKind, x0+(x1-x0)*i/prtnum, y0+(y1-y0)*i--/prtnum, 0,ydir, a, b);
    }
  // Erfolg; Anzahl erzeugter Partikel zurückgeben
  return (prtnum);
  }

//Such nach einem Objekt, dass ein Acquire-Command holen könnte
global func GetAvailableObject (def, xobj)
{
  var crit = Find_And(Find_OCF(OCF_Available),
    Find_InRect(-500,-250,1000,500),
    Find_ID(def),
    Find_OCF(OCF_Fullcon),
    Find_Not(Find_OCF(OCF_OnFire)),
    Find_Func("GetAvailableObjectCheck", GetOwner()),
    Find_Not(Find_Container(xobj)));
  if (!xobj) SetLength(crit, GetLength(crit) - 1);
  return FindObject2(crit, Sort_Distance());
}
global func GetAvailableObjectCheck(int plr)
{
  // Object is not connected to a pipe (for line construction kits)
  if (FindObject2 (Find_ActionTarget(this), Find_Or(Find_ID(SPIP), Find_ID(DPIP)), Find_Action("Connect"))) return false;
  // Not chosen by another friendly clonk
  if (GetEffect("IntNotAvailable",this) && !Hostile(plr,GetOwner(EffectVar(0,this,GetEffect("IntNotAvailable",this))))) return false;
  return true;
}

// Einen Script zeitverzögert und ggf. wiederholt ausführen
global func Schedule(string strScript, int iInterval, int iRepeat, object pObj)
{
  
  // Default
  if(!iRepeat) iRepeat = 1;
  if(!pObj) pObj = this;
  // Effekt erzeugen
  var iEffect = AddEffect("IntSchedule", pObj, 1, iInterval, pObj);
  if(iEffect <= 0) return false;
  // Variablen setzen
  EffectVar(0, pObj, iEffect) = strScript;
  EffectVar(1, pObj, iEffect) = iRepeat;
  return true;
}

global func FxIntScheduleTimer(object pObj,  int iEffect)
{
  // Nur eine bestimmte Anzahl Ausführungen
  var fDone = (--EffectVar(1, pObj, iEffect) <= 0);
  // Ausführen
  eval(EffectVar(0, pObj, iEffect));
  return -fDone;
}

// Eine Funktion zeitverzögert und ggf. wiederholt aufrufen
global func ScheduleCall(object pObj, string strFunction, int iInterval, int iRepeat, par0, par1, par2, par3, par4)
{
  // Default
  if(!iRepeat) iRepeat = 1;
  if(!pObj) pObj = this;
  // Effekt erzeugen
  var iEffect = AddEffect("IntScheduleCall", pObj, 1, iInterval, pObj);
  if(iEffect <= 0) return false;
  // Variablen setzen
  EffectVar(0, pObj, iEffect) = strFunction;
  EffectVar(1, pObj, iEffect) = iRepeat;
  // EffectVar(2): Nur zur Abwärtskompatibilität reserviert
  EffectVar(2, pObj, iEffect) = pObj; 
  for(var i = 0; i < 5; i++)
    EffectVar(i + 3, pObj, iEffect) = Par(i + 4);
  return true;
}

global func FxIntScheduleCallTimer(object pObj, int iEffect)
{
  // Nur eine bestimmte Anzahl Ausführungen
  var fDone = (--EffectVar(1, pObj, iEffect) <= 0);
  // Ausführen
  Call(EffectVar(0, pObj, iEffect), EffectVar(3, pObj, iEffect), EffectVar(4, pObj, iEffect), EffectVar(5, pObj, iEffect), EffectVar(6, pObj, iEffect), EffectVar(7, pObj, iEffect));
  // Nur eine bestimmte Anzahl Ausführungen
  return (-fDone);
}

global func ClearScheduleCall(object pObj, string strFunction)
{
  var i, iEffect;
  // Von Effektzahl abwärts zählen, da Effekte entfernt werden
  i = GetEffectCount("IntScheduleCall", pObj);
  while (i--)
    // Alle ScheduleCall-Effekte prüfen
    if (iEffect = GetEffect("IntScheduleCall", pObj, i))
      // Gesuchte Zielfunktion
      if (EffectVar(0, pObj, iEffect) == strFunction)      
        // Effekt löschen
        RemoveEffect(0, pObj, iEffect);
}
