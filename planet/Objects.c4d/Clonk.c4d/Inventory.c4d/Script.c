#strict 2

static const INVT_MaxItems = 3;
static const INVT_MaxTools = 1;

static const INVT_SymbolSize = 64;

local aInventory;
local aTools;
local aPictures;
local iSelection;

func Initialize()
{
  // 3 Inventory spaces
  aInventory = [0,0,0];
  // 1 Tool space
  aTools = [0];
  iSelection = 0;
  // parallaxity
  this["Parallaxity"] = [0,0];

  this["Visibility"] = VIS_None;
  
  SetPosition(30,-90);
  
  SetClrModulation(RGBa(0,0,20,128));
  SetAction("Inventory");
  
  // Create pictures
  var pPict;
  var OffX = INVT_SymbolSize/2, OffY = INVT_SymbolSize/2;
  aPictures = [];
  for(var iIndex = 0; iIndex < GetLength(aInventory)+GetLength(aTools); iIndex++)
  {
    pPict = CreateObject(EMPT, 0, 0, GetOwner());
    pPict->SetMaster(this, iIndex);
    aPictures[iIndex] = pPict;
    pPict->SetPosition(GetX()+OffX, GetY()+OffY);
    OffX += INVT_SymbolSize;
    if(iIndex == GetLength(aInventory)-1) OffX += INVT_SymbolSize;
  }
  
  UpdateImage();
}

func Show(fHide)
{
  if(fHide)
  {
    for(var pPict in aPictures)
      pPict["Visibility"] = VIS_None;
    this["Visibility"] = VIS_None;
  }
  else
  {
    for(var pPict in aPictures)
      pPict["Visibility"] = VIS_Owner;
    this["Visibility"] = VIS_Owner;
  }
}

local iOffset;
func UpdateImage()
{
  var iIndex = 0;
  var pObj;
  var idID;
  for(pObj in aInventory)
  {
    aPictures[iIndex]->SetImage(pObj, iSelection == iIndex);
    iIndex++;
  }
  for(pObj in aTools)
  {
    aPictures[iIndex]->SetImage(pObj, -1-iSelection+GetLength(aInventory)  == iIndex);
    iIndex++;
  }
  return;
}

func AddTool(object pObj)
{
  var iIndex;
  // Find the first free slot
  while(iIndex < GetLength(aTools) && aTools[iIndex]) iIndex++;
  if(iIndex == GetLength(aTools)) return 0;
  // Add item
  aTools[iIndex] = pObj;
  UpdateImage();
}

func RemTool(object pObj)
{
  // Get Index
  var iIndex;
  for(var pTest in aTools)
  {
    if(pTest == pObj) break;
    iIndex++;
  }
  if(iIndex == GetLength(aTools)) return 0;
  aTools[iIndex] = 0;
  UpdateImage();
}

func AddItem(object pObj)
{
  if(pObj->~IsTool()) return AddTool(pObj);
  var iIndex;
  // Find the first free slot
  while(iIndex < GetLength(aInventory) && aInventory[iIndex]) iIndex++;
  if(iIndex == GetLength(aInventory)) return 0;
  // Add item
  aInventory[iIndex] = pObj;
  UpdateImage();
}

func FreeSpace(object pObj)
{
  var iIndex;
  if(pObj->~IsTool())
  {
    // Find the first free slot
    while(iIndex < GetLength(aTools) && aTools[iIndex]) iIndex++;
    if(iIndex == GetLength(aTools)) return 0;
  }
  else
  {
    // Find the first free slot
    while(iIndex < GetLength(aInventory) && aInventory[iIndex]) iIndex++;
    if(iIndex == GetLength(aInventory)) return 0;
  }
  return true;
}

func RemItem(object pObj)
{
  if(pObj->~IsTool()) return RemTool(pObj);
  // Get Index
  var iIndex;
  for(var pTest in aInventory)
  {
    if(pTest == pObj) break;
    iIndex++;
  }
  if(iIndex == GetLength(aInventory)) return 0;
  aInventory[iIndex] = 0;
  UpdateImage();
}

func GetSelectedObj()
{
  // Positiv counts are normal items
  if(iSelection >= 0) return aInventory[iSelection];
  // negativ counts are tools
  else return aTools[-1-iSelection];
}

func SelectNumber(iIndex)
{
  if(iIndex >= GetLength(aInventory)) iIndex = GetLength(aInventory)-iIndex-1;
  if(iIndex > 0)
  {
    if(iIndex >= GetLength(aInventory)) return false;
    iSelection = iIndex;
    UpdateImage();
    return true;
  }
  if(-1-iIndex >= GetLength(aTools)) return false;
  iSelection = iIndex;
  UpdateImage();
  return true;
}

func SelectNext()
{
  iSelection++;
  if(iSelection >= GetLength(aInventory)) iSelection = -GetLength(aTools);
  UpdateImage();
}

func Definition(def) {
    SetProperty("ActMap", {
Inventory = {
Prototype = Action,
Name = "Inventory",
Procedure = DFA_NONE,
Length = 1,
Delay = 0,
X = 0,
Y = 0,
Wdt = 330,
Hgt = 74,
OffX = -5,
OffY = -5,
},  }, def);
  SetProperty("Name", "$Name$", def);
}
