/*-- Trajectory Calculator --*/

static const g_CrosshairID = Trajectory;

protected func Initialize()
{
  SetProperty("Visibility",VIS_Owner);
}

global func RemoveTrajectory(object pObj)
{
  // Find and remove
  var pTrajectory = FindObject(Find_ID(Trajectory), Find_ActionTarget(pObj));
  if(pTrajectory) pTrajectory->RemoveObject();
}

//pObj = Object which has the trajectory. pObj must be owned by a player for the player to see the trajectory.
//int ix = glogal x coordinate
//int iy = global y coordinate
//int iXDir & int iYDir = velocity of simulated shot
//int iColor = What colour the trajectory particles are
//int spacing = distance of pixels between each trajectory particle
global func AddTrajectory(object pObj, int iX, int iY, int iXDir, int iYDir, int iColor, int spacing)
{
  // Delete old trajectory
  RemoveTrajectory(pObj);
  // Create new helper object
  var pTrajectory = CreateObject(Trajectory, pObj->GetX() - GetX(), pObj->GetY() - GetY(), pObj->GetOwner());
  pTrajectory->SetAction("Attach", pObj);
  // Set starting values
  var i = -1, iXOld, iYOld;
  var iFaktor = 100;
  iX *= iFaktor; iY *= iFaktor;
  iYDir *= 5; iXDir *= 5;
  iY -= 4*iFaktor;
  iXOld = iX; iYOld = iY;
  // Trajectory simulation
  while(++i < 500)
  {
    // Speed and gravity offset
    iX += iXDir;
    iY += iYDir + GetGravity() * i / 20;
    // If we are far enough away insert a new point
	if(!spacing) spacing = 10;
    if(Distance((iXOld - iX) / iFaktor, (iYOld - iY) / iFaktor) >= spacing)
    {
      CreateParticle("Trajectory", iX/iFaktor - pTrajectory->GetX(), iY/iFaktor - pTrajectory->GetY(), iXDir/500, iYDir/500, 15, iColor, pTrajectory);
      iXOld = iX; iYOld = iY;
    }
    // Or is it here already?
    if(GBackSolid(iX / iFaktor - GetX(), iY / iFaktor - GetY())) break;
  }
  // So, ready
  return pTrajectory;
}

public func AttachTargetLost()
{
  RemoveObject();
}

local ActMap = {
Attach = {
	Prototype = Action,
	Name = "Attach",
	Procedure = DFA_ATTACH,
	Length = 1,
	Delay = 0,
},
};
local Name = "$Name$";
