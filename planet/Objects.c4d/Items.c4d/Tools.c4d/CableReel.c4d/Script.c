/*-- Cable reel --*/

protected func Hit()
{
	Sound("RockHit*");
}

public func IsToolProduct() { return 1; }

/*-- Line connection --*/

// Called with double dig: will connect power line to building at the clonk's position.
protected func Activate(object pClonk)
{
	[$TxtConnectLine$]
	
	pClonk->SetComDir(COMD_Stop);
	
	// Is there an object which accepts power lines?
	var pObj = FindObject(Find_AtPoint(), Find_Func("CanPowerConnect"));
	// No such object -> message.
	if(!pObj)
		return pClonk->Message("$TxtNoNewLine$");
	// Is there a power line connected to this wire roll?
	var pLine = FindObject(Find_PowerLine());
		
	if(pLine) // There already is a power line.
	{
		if(pObj == pLine->GetActionTarget(0) || pObj == pLine->GetActionTarget(1)) // Power line is already connected to pObj -> remove line.
		{
			pLine->RemoveObject();
			Sound("Connect");
			pClonk->Message("$TxtLineRemoval$");
			return true;
		}
		else // Connect existing power line to pObj.
		{
			if(pLine->GetActionTarget(0) == this)
				pLine->SetActionTargets(pObj, pLine->GetActionTarget(1));
			else if(pLine->GetActionTarget(1) == this)
				pLine->SetActionTargets(pLine->GetActionTarget(0), pObj);
			else
				return;
			Sound("Connect");
			pClonk->Message("$TxtConnect$", pObj->GetName());
			RemoveObject();
			return true;
		}
	}
	else // A new power line needs to be created.
	{
		pLine = CreateObject(PowerLine, 0, 0, NO_OWNER);
		pLine->SetActionTargets(this, pObj);
		Sound("Connect");
		pClonk->Message("$TxtConnect$", pObj->GetName());
		return true;
	}
	return true;
}

// Finds all power lines connected to pObject (can be nil in local calls).
private func Find_PowerLine(object pObject)
{
	if(!pObject) pObject = this;
	return [C4FO_Func, "IsConnectedTo", pObject];
}

local Name = "$Name$";
local Description = "$Description$";
