#strict 2

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}

func Departure(object pContainer)
{
  Enter(pContainer);
}

local pMaster;
local iNumber;

func SetMaster(pNewMaster, iNewNumber)
{
  SetCategory(C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax | C4D_MouseSelect);
  this["Visibility"] = VIS_None;
  this["Parallaxity"] = [0,0];
  pMaster = pNewMaster;
  iNumber = iNewNumber;
  SetGraphics(Format("%d", iNumber+1),NUMB,3,GFXOV_MODE_IngamePicture);
  SetObjDrawTransform(300, 0,-1000*18, 0, 300, 1000*18, 3);
  SetGraphics("Spot", GetID(), 1, GFXOV_MODE_IngamePicture);
}

func SetImage(pObj, bool fSelected)
{
  if(fSelected)
  {
    SetClrModulation(RGB(255), 3); SetGraphics("Spot", GetID(), 1, GFXOV_MODE_IngamePicture);
  }
  else
  {
    SetClrModulation(HSL(0,0,180), 3); SetGraphics(nil, nil, 1, 0);
  }
  if(!pObj)
  {
    SetGraphics(nil, nil, 2, 0);
    SetName(GetID()->GetName());
    return;
  }
  var idID = pObj->GetID();
  SetGraphics(nil, idID, 2, GFXOV_MODE_IngamePicture);
  if(fSelected)
    SetObjDrawTransform(1000, 0, 0, 0, 1000, 0, 2);
  else
    SetObjDrawTransform( 600, 0, 0, 0,  600, 0, 2);
  SetName(pObj->GetName());
}

func MouseSelection(int iPlr)
{
  if(iPlr == GetOwner()) pMaster->SelectNumber(iNumber);
}