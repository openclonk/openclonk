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

// The autosell function
public func ExecAutoSell()
{
	// Search all objects for objects that want to be sold automatically
	for(pObj in FindObjects(Find_Container(this), Find_Func("AutoSell")))
		Sell(pObj);
}

// Does the base block enemies?
public func CanBlockEnemies() { return true; }

// ---------------------------------------------------------

// ----------------- Settings for the trading of objects ----------------
// --- these functions can be overloaded for vendors or special bases ---

// returns an array with the definition, the amount
func GetBuyObject(int iIndex)
{
	var aBuy = [0,0];
	var idDef = GetHomebaseMaterial(GetOwner(), nil, iIndex, C4D_All);
	aBuy[0] = idDef;
	aBuy[1] = GetHomebaseMaterial(GetOwner(), idDef, 0);
	if(!idDef) return 0;
	// The default implementation returns the Homebasemaerial of the playeer
	return aBuy;
}

// returns an array with all buyable objects
func GetBuyObjects()
{
	var aBuyObjects = [];
	var iIndex, aBuy;
	while(aBuy = GetBuyObject(iIndex++))
		aBuyObjects[iIndex-1] = aBuy;
	return aBuyObjects;
}

// returns the value of the object if sold in this base
func GetSellValue(object pObj)
{
	// By default call the engine function
	return pObj->GetValue();
}

func GetBuyValue(id idObj)
{
	// By default call the engine function
	return idObj->GetValue();
}

// change the amount of buyable material
func DoBaseMaterial(id idDef, int iCount)
{
	// by default use Homebase engine function
	DoHomebaseMaterial(GetOwner(), idDef, iCount);
	// this should also call UpdateClonkBuyMenus() if the standart function isn't used
}

public func OnHomebaseMaterialChange()
{
	// and update the buy menu
	UpdateClonkBuyMenus();
}

// ---------------------------------------------------------


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
			CreateObject(BaseMaterial,AbsX(10),AbsY(10),GetOwner());
	}
}

// ---------- Healing, Extinguishing and Autosell -----------

func FxIntBaseTimer(pThis, iEffect, iTime)
{
	var pObj;
	// Can this base heal? Then look for clonks that need some
	if(CanHeal() && GetHeal())
		for(pObj in FindObjects(Find_Container(this), Find_OCF(OCF_CrewMember), Find_Allied(GetOwner())))
		{
			if(pObj->GetEnergy() < pObj->GetPhysical("Energy")/1000 && !GetEffect("IntBaseHeal", pObj))
				AddEffect("IntBaseHeal", pObj, 1, 1, this);
		}
	// Can this base extinguish? Then look for something on fire
	if(CanExtinguish())
		for(pObj in FindObjects(Find_Container(this), Find_OCF(OCF_OnFire), Find_Allied(GetOwner())))
			pObj->Extinguish();
	// Sell objects
	ExecAutoSell();
	// Update the sell menu of clonks (if someone knows a way to directly get info if the contents of the base change this coult be imporved)
	if(aClonkSellList)
	{
		// Only if there are clonks with menus
		var fFound;
		for(pClonk in aClonkSellList)
			if(pClonk)
			{
				fFound = 1;
				break;
			}
		if(fFound)
			UpdateSellList();
	}
}

func FxIntBaseHealTimer(pClonk, iEffect)
{
	// The clonk has left the base? Stop!
	if(pClonk->Contained() != this) return -1;
	// Full energy? Stop too.
	if(pClonk->GetEnergy() >= pClonk->GetPhysical("Energy")/1000) return -1;

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

// ---------------------- Context controls -----------------------------

// Add buy and sell entries in the context menu

func ContextSell(object pClonk)
{
	[$Sell$|Image=BaseMaterial|Condition=IsBase]
	return OpenSellMenu(pClonk);
}

func ContextBuy(object pClonk)
{
	[$Buy$|Image=Library_Base|Condition=IsBase]
	return OpenBuyMenu(pClonk);
}
// ------------------------ Buying -------------------------------------

local aClonkBuyList;

func AddClonkBuyList(object pClonk)
{
	if(!aClonkBuyList) aClonkBuyList = [];
	var iIndex = 0;
	// find free slot
	while(aClonkBuyList[iIndex] && aClonkBuyList[iIndex] != pClonk) iIndex++;
	aClonkBuyList[iIndex] = pClonk;
}

func UpdateClonkBuyMenus()
{
	if(!aClonkBuyList) aClonkBuyList = [];
	// Reopen the menu for all clonks
	var pClonk;
	var iIndex;
	for(pClonk in aClonkBuyList)
	{
		iIndex++;
		if(!pClonk) continue;
		if(pClonk->GetMenu() != Library_Base)
		{
			aClonkBuyList[iIndex-1] = 0;
			continue;
		}
		OpenBuyMenu(pClonk, nil, pClonk->GetMenuSelection(), 1);
	}
}

// Buy
func OpenBuyMenu(object pClonk, id idDef, int iSelection)
{
	var aBuy = [0,0,0];
	var iIndex, iSelection;
	AddClonkBuyList(pClonk);
	pClonk->CreateMenu (Library_Base, this, C4MN_Extra_Value, "$TxtNothingToBuy$", 0, C4MN_Style_Normal, 0, C4Id("BuyMenu"));
	for(aBuy in GetBuyObjects())
	{
		if(aBuy[0] == idDef) iSelection = iIndex;
		pClonk->AddMenuItem("$TxtBuy$", "BuyDummy", aBuy[0], aBuy[1], pClonk, nil, 128, 0, GetBuyValue(aBuy[0]));
		iIndex++;
	}
	if(idDef || iSelection) pClonk->SelectMenuItem(iSelection);
}

func BuyDummy(id idDef, object pClonk, bool bRight, int iValue)
{
	var iPlr = pClonk->GetOwner();
	DoBuy(idDef, iPlr, GetOwner(), pClonk, bRight, 1);
	OpenBuyMenu(pClonk, idDef);
}

func DoBuy(id idDef, int iForPlr, int iPayPlr, object pClonk, bool bRight, bool fShowErrors)
{
	if(!GetHomebaseMaterial(iPayPlr, idDef)) return; //TODO
	var iValue = GetBuyValue(idDef);
	// Has the clonk enought money?
	if(iValue > GetWealth(iPayPlr))
	{
		// TODO: get an errorsound
		if(fShowErrors)
		{
			Sound("Error", 0, 100, iForPlr+1);
			PlayerMessage(iForPlr, "$TxtNotEnoughtMoney$");
		}
		return -1;
	}
	// Take the cash
	DoWealth(iPayPlr, -iValue);
	Sound("UnCash", 0, 100, iForPlr+1); // TODO: get sound
	// Decrease the Basematerial
	DoBaseMaterial(idDef, -1);
	// Deliver the object
	var pObj = CreateContents(idDef);
	pObj->SetOwner(iForPlr);
	if(pObj->GetOCF() & OCF_CrewMember) pObj->SetCrewStatus(iForPlr,true);
	if(pObj->GetOCF() & OCF_Collectible) pClonk->Collect(pObj);
	// is right clicked? then buy another object
	if(bRight)
		DoBuy(idDef, iForPlr, iPayPlr, pClonk, bRight, iValue, 1);
	return pObj;
}

// -------------------------- Selling -------------------------------------

func GetSellableContents()
{
	return FindObjects(Find_Container(this), Find_Or(Find_Category(C4D_Object), Find_Category(C4D_Vehicle), Find_Category(65536/*C4D_TradeLiving*/)));
}

local aSellList;
local aClonkSellList;

func UpdateSellList()
{
	aSellList = [];
	var iIndex;
	// Create a list of the sellable objects
	for(pObj in GetSellableContents())
	{
		// Are we allowed to sell the object?
		if(GetID()->GetDefCoreVal("NoSell", "DefCore")) continue;
		// Only check the last item to stack, the engine normally sorts the objects so this should be enought to check
		if(iIndex && CanStack(aSellList[iIndex-1][2], pObj))
			aSellList[iIndex-1][1]++;
		else
			aSellList[iIndex++] = [pObj->GetID(), 1, pObj];
	}
	UpdateClonkSellMenus();
}

func AddClonkSellList(object pClonk)
{
	if(!aClonkSellList) aClonkSellList = [];
	var iIndex = 0;
	// find free slot
	while(aClonkSellList[iIndex] && aClonkSellList[iIndex] != pClonk) iIndex++;
	aClonkSellList[iIndex] = pClonk;
}

func UpdateClonkSellMenus()
{
	if(!aClonkSellList) return;
	// Reopen the menu for all clonks
	var pClonk;
	var iIndex;
	for(pClonk in aClonkSellList)
	{
		iIndex++;
		if(!pClonk) continue;
		if(pClonk->GetMenu() != BaseMaterial)
		{
			aClonkSellList[iIndex-1] = 0;
			continue;
		}
		OpenSellMenu(pClonk, pClonk->GetMenuSelection(), 1);
	}
}

// Sell Object
func OpenSellMenu(object pClonk, int iSelection, bool fNoListUpdate)
{
	// Filled with [idDef, iCount, pObj] arrays
	var aList = [];
	var aArray;
	var pObj;
	var iIndex;
	if(!fNoListUpdate)
		UpdateSellList();
	AddClonkSellList(pClonk);
	pClonk->CreateMenu (BaseMaterial, this, C4MN_Extra_Value, "$TxtNothingToSell$", 0, C4MN_Style_Normal, 1);
	var iIndex;
	for(aArray in aSellList) // aArray contains [idDef, iCount, pObj]
	{
		pClonk->AddMenuItem(Format("$TxtSell$", aArray[2]->GetName()), "SellDummy", aArray[0], aArray[1], pClonk, nil, 128+4, aArray[2], GetSellValue(aArray[2]));
		iIndex++;
	}
	if(iSelection == iIndex) iSelection--;
	pClonk->SelectMenuItem(iSelection);
}

func CanStack(object pFirst, object pSecond)
{
	// Test if these Objects differ from each other
	if(!pFirst->CanConcatPictureWith(pSecond)) return false;
	if(GetSellValue(pFirst) != GetSellValue(pSecond)) return false;
	
	// ok they can be stacked
	return true;
}

func SellDummy(id idDef, object pClonk, bool bRight)
{
	var iIndex = pClonk->GetMenuSelection();
	var aArray = aSellList[iIndex];
	DoSell(aArray[2], GetOwner(), bRight);
	OpenSellMenu(pClonk, iIndex);
}

func DoSell(object pObj, int iPlr, bool bRight)
{
	// Test the object
	if(pObj->GetID()->GetDefCoreVal("NoSell", "DefCore") || pObj->GetOCF() & OCF_CrewMember)
	{
		// Enter base (needed for NoSell objects in other objects which are sold)
		if(pObj->Contained() != this)
			pObj->Enter(this);
		return 0;
	}
	// Sell contents first
	var pContents;
	for(pContents in FindObjects(Find_Container(pObj)))
		DoSell(pContents, iPlr);
	// Give the player the cash
	DoWealth(iPlr, GetSellValue(pObj));
	Sound("Cash", 0, 100, iPlr+1); // TODO: get sound
	if(pObj->GetID()->GetDefCoreVal("Rebuy", "DefCore"))
		DoBaseMaterial(pObj->GetID(), 1);
	// right clicked? then sell other objects too
	var pNewObj;
	var bFound;
	if(bRight)
	{
		for(var pNewObj in GetSellableContents())
			if(CanStack(pObj, pNewObj) && pObj != pNewObj)
			{
				bFound = 1;
				break;
			}
	}
	// And remove the object
	pObj->RemoveObject();
	if(bFound) return DoSell(pNewObj, iPlr, bRight);
	return true;
}

// ------------------------ global functions ---------------------------
global func Buy (id idBuyObj, int iForPlr, int iPayPlr, object pToBase, bool fShowErrors)
{
	// if no base is given try this
	if(!pToBase) pToBase = this;
	// not a base?
	if( !pToBase->~IsBase() )
		return 0;
	return pToBase->DoBuy(idBuyObj, iForPlr, iPayPlr, 0, 0, fShowErrors);
}

global func Sell (int iPlr, object pObj, object pToBase)
{
	// if no base is given try this
	if(!pToBase) pToBase = this;
	// not a base?
	if( !pToBase->~IsBase() )
		return 0;
	return pToBase->DoSell(pObj, iPlr);
}

global func FindBase (int iPlr, int iIndex)
{
	return FindObjects(Find_Owner(iPlr), Find_Func("IsBase"))[iIndex];
}

global func GetBase ()
{
	if(!(this->~IsBase())) return NO_OWNER;
	return GetOwner();
}

local Name = "$Name$";
