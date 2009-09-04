/*--- The Base ---*/

#strict 2

// Determines it the Building is acutally a base
local fIsBase;
local iEnergy;

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}

// ---------- Settings for base funcionallity --------------

// Determines if the base can heal allied clonks
public func CanHeal() { return true; }
// The amount of energy the base can store, if none the base can't heal
public func GetHeal() { return 100; }
// The money one filling of the bases energy storage costs
public func GetHealCost() { return 5;}

// Determines if the base can extinguish allied clonks
public func CanExtinguish() { return true; }

// Gives an array of the id's that the base shall sell automatically
public func GetAutoSell() { return [GOLD]; }

// Does the base block enemies?
public func CanBlockEnemies() { return true; }

// ---------------------------------------------------------

// ---------- Settings for the trading of objects ----------

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
  // Are there autosell ids?
  var aList = GetAutoSell();
  if(aList)
    for(var idDef in aList)
      for(pObj in FindObjects(Find_Container(this), Find_ID(idDef)))
	Sell(pObj);
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
  [$Sell$|Image=BASM|Condition=IsBase]
  return OpenSellMenu(pClonk);
}

func ContextBuy(object pClonk)
{
  [$Buy$|Image=BASE|Condition=IsBase]
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
    if(pClonk->GetMenu() != BASE)
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
  pClonk->CreateMenu (BASE, this, C4MN_Extra_Value, "$TxtNothingToBuy$", 0, C4MN_Style_Normal, 0, BUY_);
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
      PlayerMessage(iForPlr, "$TxtNotEnoughtMoney$", this);
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
  if(pObj->GetOCF() & OCF_CrewMember) pObj->MakeCrewMember(iForPlr);
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
    if(GetDefCoreVal("NoSell", "DefCore", pObj->GetID())) continue;
    iIndex = 0;
    // Have a look in the list if you could stack this item
    for(aArray in aSellList)
    {      
      if(CanStack(aArray[2], pObj))
      {
          // Ok, the objects are similar? than stack them
	  aSellList[iIndex][1]++;
          iIndex = -1;
          break;
      }
      iIndex++;       
    }
    if(iIndex != -1)
      // They are different? Than create a new entry
      aSellList[GetLength(aSellList)] = [pObj->GetID(), 1, pObj];
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
    if(pClonk->GetMenu() != BASM)
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
  pClonk->CreateMenu (BASM, this, C4MN_Extra_Value, "$TxtNothingToSell$", 0, C4MN_Style_Normal, 1);
  var iIndex;
  for(aArray in aSellList)
  {
    pClonk->AddMenuItem("$TxtSell$", "SellDummy", aArray[0], aArray[1], pClonk, nil, 128, 0, GetSellValue(aArray[2]));
    iIndex++;
  }
  if(iSelection == iIndex) iSelection--;
  pClonk->SelectMenuItem(iSelection);
}

func CanStack(object pFirst, object pSecond)
{
  // Test if these Objects differ from each other
  if(pFirst->GetID() != pSecond->GetID()) return false;
  if(GetSellValue(pFirst) != GetSellValue(pSecond)) return false;
  
  // Test if different color matters
//  if(!(GetDefCoreVal("AllowPictureStack", "DefCore", pFirst->GetID()) & APS_Color)) TODO Find out why the engine doesn't like APS_Color
  if(1)
  {
    if(pFirst->GetColor() != pSecond->GetColor()) return false;
    if(pFirst->GetClrModulation() != pSecond->GetClrModulation()) return false;
  }
  // TODO: Check differen overlays
  
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
  if(GetDefCoreVal("NoSell", "DefCore", pObj->GetID()) || pObj->GetOCF() & OCF_CrewMember)
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
  return FindObjects(Find_Owner(iPlr), Find_Category(C4D_Structure), Find_Func("IsBase"))[iIndex];
}

global func GetBase (object pObj)
{
  return GetOwner();
}