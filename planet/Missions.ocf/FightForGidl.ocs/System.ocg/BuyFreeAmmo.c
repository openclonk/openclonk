#appendto Library_Base

func DoBuy(id idDef, int iForPlr, int iPayPlr, object pClonk, bool bRight, bool fShowErrors)
{
	var obj = _inherited(idDef, iForPlr, iPayPlr, pClonk, bRight, fShowErrors, ...);
	if (!obj || (obj==-1) || !pClonk) return obj;
	var idobj = obj->GetID(), idammo;
	     if (idobj == Bow)    idammo = Arrow;
	else if (idobj == Musket) idammo = LeadShot;
	else if (idobj == GrenadeLauncher) idammo = IronBomb;
	if (idammo)
	{
		var ammo = CreateObject(idammo,0,0,iForPlr);
		if (ammo)
		{
			ammo->~SetInfiniteStackCount();
			ammo->Enter(obj);
		}
	}
	return obj;
}