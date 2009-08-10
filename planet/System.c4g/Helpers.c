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

global func RemoveAll()
{
	var cnt;
	for(var obj in FindObjects(...))
	{
		if(obj) 
		{
			RemoveObject(obj);
			cnt++;
		}
	}
	return cnt;
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
  return FindObject(crit, Sort_Distance());
}
global func GetAvailableObjectCheck(int plr)
{
  // Object is not connected to a pipe (for line construction kits)
  if (FindObject (Find_ActionTarget(this), Find_Or(Find_ID(SPIP), Find_ID(DPIP)), Find_Action("Connect"))) return false;
  // Not chosen by another friendly clonk
  if (GetEffect("IntNotAvailable",this) && !Hostile(plr,GetOwner(EffectVar(0,this,GetEffect("IntNotAvailable",this))))) return false;
  return true;
}

