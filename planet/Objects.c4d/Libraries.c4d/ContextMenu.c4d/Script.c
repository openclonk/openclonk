/*--- Context menu ---*/

#strict 2

/* Control */

protected func ControlSpecial2()
{
  [$CtrlMenuDesc$|Image=CXTX]
  // Inside a building or vehicle: open context menu of the container
  if (Contained())
    if ((Contained()->GetCategory() & C4D_Structure) || (Contained()->GetCategory() & C4D_Vehicle))
    {
      SetCommand(this,"Context",0,0,0,Contained());
      return ExecuteCommand();
    }
  // Is pushing an object: open context menu of the pushed object
  if (GetAction() == "Push")
  {
    SetCommand(this,"Context",0,0,0,GetActionTarget());
    return ExecuteCommand();
  }
  // Carries an object: open context menu of the first carried object
  if (Contents(0))
  {
    SetCommand(this,"Context",0,0,0,Contents(0));
    return ExecuteCommand();
  }
  // Open context menu of the clonk then
  SetCommand(this,"Context",0,0,0,this);
  return ExecuteCommand();
}

/* Context menu entries */

/*public func ContextRelease(pCaller) 
{
  [$CtxRelease$|Image=CXRL|Condition=ReleaseAllowed]
  FindObject(REAC)->Activate(GetOwner());
  return 1;
}*/

public func ContextEnergy(pCaller)
{
  [$TxtEnergysupply$|Image=CXEC|Condition=AtEnergySite]
  var pSite; 
  if (pSite = FindEnergySite())
    SetCommand(this, "Energy", pSite);
  return 1;
}

public func ContextConstructionSite(pCaller)
{
  [$CtxConstructionMaterial$|Image=CXCM|Condition=AtConstructionSite]
  var pSite; 
  if (pSite = FindConstructionSite())
    PlayerMessage(GetOwner(), pSite->GetNeededMatStr(), pSite);
  return 1;
}

public func ContextChop(pCaller)
{
  [$CtxChop$|Image=CXCP|Condition=AtTreeToChop]
  var pTree; 
  if (pTree = FindTree())
    SetCommand(this, "Chop", pTree);
  return 1;
}

public func ContextConstruction(pCaller)
{
  [$CtxConstructionDesc$|Image=CXCN|Condition=HasConstructMenu]
  SetCommand(this, "Construct");
  ExecuteCommand();
  return 1;
}

public func ContextHome(pCaller)
{
  [$CtxHomeDesc$|Image=CXHM|Condition=HasBase]
  SetCommand(this, "Home");
  return 1;
}

public func ContextDescend(pCaller) 
{
  [$TxtDescend$|Image=DSCN|Condition=IsRiding]
  DescendVehicle();
}

/* Conditions */

public func IsRiding() { return (WildcardMatch(GetAction(), "Ride*")); }
public func HasConstructMenu() { return HasKnowledge() && GetPhysical("CanConstruct"); }
public func HasKnowledge() { return GetPlrKnowledge(GetOwner(),nil,0,C4D_Structure); }
public func HasBase()      { return FindBase(GetOwner()) && GetBase(Contained()) != GetOwner(); }
public func ReleaseAllowed() { return ObjectCount(REAC); }
public func AtConstructionSite() { return !Contained() && FindConstructionSite() && ObjectCount(CNMT); }
public func AtEnergySite() { return !Contained() && FindEnergySite(); }
public func AtTreeToChop() { return !Contained() && FindTree() && GetPhysical("CanChop"); }

public func FindConstructionSite()
{
  return FindObject2(Find_AtRect(-1,-16,2,32), Find_OCF(OCF_Construct), Find_Layer(GetObjectLayer()));
}

public func FindEnergySite()
{
  return FindObject2(Find_AtPoint(), Find_OCF(OCF_PowerConsumer), Find_NoContainer(), Find_Layer(GetObjectLayer()), Find_Func("NeedsEnergy"));
}

public func FindTree()
{
  return FindObject2(Find_AtPoint(), Find_OCF(OCF_Chop), Find_Layer(GetObjectLayer()));
}

/* Misc */

protected func ControlCommand(szCommand, pTarget, iTx, iTy, pTarget2, Data)
{
	// Forward MoveTo command to the horse
	if (szCommand == "MoveTo")
		if (IsRiding())
			return GetActionTarget()->~ControlCommand(szCommand, pTarget, iTx, iTy);
	// Other command when riding: descend (exception: Context)
  if (IsRiding() && szCommand != "Context")
	{
		SetComDir(COMD_Stop,GetActionTarget());
		GetActionTarget()->~ControlDownDouble(this);
	}
	// RejectConstruction Callback when constructing via Drag'n'Drop from a building menu
  if(szCommand == "Construct")
  {
    if(Data->~RejectConstruction(iTx - GetX(), iTy - GetY(), this) )
    {
      return 1;
    }
  }
	// Call base implementation
	return _inherited(...);
}

// Callback when selecting the "Construct" menu entry
public func ControlCommandConstruction(target, x, y, target2, def)
{
  // Construction prohibited?
  if(def->~RejectConstruction(x - GetX(), y - GetY(), this) )
    // Finish construction command
    return FinishCommand(this, false, 0) ;
}

// Called when selecting the "Descend" menu entry
public func DescendVehicle()
{
  var pOldVehicle = GetActionTarget();
  SetAction("Walk");
  // Stuck after descending? Descend at the vehicle's position
  if (Stuck()) if (pOldVehicle)
  {
    var x=GetX(), y=GetY();
    SetPosition(GetX(pOldVehicle), GetY(pOldVehicle));
    if (Stuck())
    {
      // Vehicle is stuck as well? Back to the roots.
      SetPosition(x,y);
    }
  }  
}
