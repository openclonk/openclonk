/*--- The Base ---*/

// Author: Randrian
// TODO -> torough explanation how to use it

func Initialize()
{
	var iPlrNumber = GetOwner()%4+1;
	var szSection = Format("Player%d", iPlrNumber); // TODO: Check teams and get the fitting player section
	
	aHomebaseMaterial = [];
	aHomabaseProduction = [];
	var iIndex;
	var idID, iCount;
	while(1)
	{
		idID = GetScenarioVal ("HomeBaseMaterial", szSection, iIndex*2);
		iCount = GetScenarioVal ("HomeBaseMaterial", szSection, iIndex*2+1);
		if(!idID && !iCount) break;
		if(idID)
			aHomebaseMaterial[GetLength(aHomebaseMaterial)] = [idID, iCount];
		iIndex++;
	}
	iIndex = 0;
	while(1)
	{
		idID = GetScenarioVal ("HomeBaseProduction", szSection, iIndex*2);
		iCount = GetScenarioVal ("HomeBaseProduction", szSection, iIndex*2+1);
		if(!idID && !iCount) break;
		if(idID)
			aHomabaseProduction[GetLength(aHomabaseProduction)] = [idID, iCount];
		iIndex++;
	}
}

static const BaseMaterial_MaxHomeBaseProduction = 25;

local ProductionUnit;

func ExecHomeBaseProduction()
{
	// Do not exec if no base is around
	if(!FindBase(GetOwner())) return;

	// Called every minute
	ProductionUnit++;
	var aArray;
	// Look at all productions
	for (aArray in aHomabaseProduction)
		// if this id is produced check if it isn't already full
		if (aArray[1]>0)
			if (ProductionUnit % BoundBy(11-aArray[1],1,10) == 0)
				if (DoGetHomebaseMaterial(aArray[0])<BaseMaterial_MaxHomeBaseProduction)
					// Produce Material
					DoDoHomebaseMaterial(aArray[0], 1);
}

local aHomebaseMaterial; // Array filled with [idDef, iCount] arrays
local aHomabaseProduction;

// ---------------------- global Interface ---------------------------

global func GetHomebaseMaterial (int iPlr, id idDef, int iIndex, int dwCategory)
{
	var pObj = FindObject(Find_ID(BaseMaterial), Find_Owner(iPlr));
	if(!pObj) pObj = CreateObject(BaseMaterial,AbsX(10),AbsY(10),iPlr);
	if(pObj) return pObj->DoGetHomebaseMaterial(idDef, iIndex, dwCategory);
}

global func GetHomebaseProduction (int iPlr, id idDef, int iIndex, int dwCategory)
{
	var pObj = FindObject(Find_ID(BaseMaterial), Find_Owner(iPlr));
	if(!pObj) pObj = CreateObject(BaseMaterial,AbsX(10),AbsY(10),iPlr);
	if(pObj) return pObj->DoGetHomebaseProduction(idDef, iIndex, dwCategory);
}

global func DoHomebaseMaterial (int iPlr, id idID, int iChange)
{
	var pObj = FindObject(Find_ID(BaseMaterial), Find_Owner(iPlr));
	if(!pObj) pObj = CreateObject(BaseMaterial,AbsX(10),AbsY(10),iPlr);
	if(pObj) return pObj->DoDoHomebaseMaterial(idID, iChange);
}

global func DoHomebaseProduction (int iPlr, id idID, int iChange)
{
	var pObj = FindObject(Find_ID(BaseMaterial), Find_Owner(iPlr));
	if(!pObj) pObj = CreateObject(BaseMaterial,AbsX(10),AbsY(10),iPlr);
	if(pObj) return pObj->DoDoHomebaseProduction(idID, iChange);
}

// ----------------------------------------------------------------------

public func DoGetHomebaseMaterial (id idDef, int iIndex, int dwCategory)
{
	var aArray;
	var iCount = 0;
	// An ID given? Then try to get the count
	if(idDef)
	{
		for(aArray in aHomebaseMaterial)
			if(aArray[0] == idDef)
				return aArray[1];
		return nil;
	}
	// A index given? Look for the id
	for(aArray in aHomebaseMaterial)
	{
		if(aArray[0]->GetCategory() & dwCategory)
		{
			if(iCount == iIndex) return aArray[0];
			iCount++;
		}
	}
}

public func DoGetHomebaseProduction (id idDef, int iIndex, int dwCategory)
{
	var aArray;
	var iCount = 0;
	// An ID given? Then try to get the count
	if(idDef)
	{
		for(aArray in aHomabaseProduction)
			if(aArray[0] == idDef)
				return aArray[1];
		return nil;
	}
	// A index given? Look for the id
	for(aArray in aHomabaseProduction)
	{
		if(aArray[0]->GetCategory() & dwCategory)
		{
			if(iCount == iIndex) return aArray[0];
			iCount++;
		}
	}
}

public func DoDoHomebaseMaterial (id idID, int iChange)
{
	if(iChange == 0) return;
	var aArray;
	var iIndex = 0;
	for(aArray in aHomebaseMaterial)
	{
		if(aArray[0] == idID)
		{
			aHomebaseMaterial[iIndex][1] += iChange;
			// Callback to the bases of the player
			var i = 0, pBase;
			while(pBase = FindBase(GetOwner(), i++))
				pBase->~OnHomebaseMaterialChange();
					return true;
		}
		iIndex++;
	}
	return false;
}

public func DoDoHomebaseProduction (id idID, int iChange)
{
	if(iChange == 0) return;
	var aArray;
	var iIndex = 0;
	for(aArray in aHomabaseProduction)
	{
		if(aArray[0] == idID)
		{
			aHomabaseProduction[iIndex][1] += iChange;
			return true;
		}
		iIndex++;
	}
	return false;
}

local Name = "$Name$";
