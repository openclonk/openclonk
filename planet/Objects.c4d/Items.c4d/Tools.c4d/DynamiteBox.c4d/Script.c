/*-- Dynamite box --*/

static const DYNA_MaxLength = 400;
static const DYNA_MaxCount  = 5;

public func Initialize()
{
	iCount = DYNA_MaxCount;
	aDynamites = CreateArray(iCount);
	aWires = CreateArray(iCount);
	for(var i = 0; i < iCount; i++)
	{
		aDynamites[i] = 0;
		aWires[i] = 0;
	}
	SetGraphics(Format("%d", DYNA_MaxCount), Fuse, 1, GFXOV_MODE_Picture);
}

private func Hit()
{
	Sound("WoodHit"); //TODO Some metal sond
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 450; }

local iCount;
local aDynamites;
local aWires;

local pWire;

public func ControlUse(object clonk, int x, int y)
{
	if(clonk->GetAction() != "Walk") return true;

	var pDyna = aDynamites[iCount-1] = CreateContents(Dynamite);
	if(!pDyna->ControlUse(clonk, x, y, 1))
	{
		pDyna->RemoveObject();
		return true;
	}
	if(pWire)
		pWire->Connect(aDynamites[iCount], pDyna);
	// First? then add Timer
	else
		AddEffect("IntLength", this, 1, 10, this);

	pWire = CreateObject(Fuse);
	pWire->Connect(pDyna, this);
	Sound("Connect");
	aWires[iCount-1] = pWire;
	
	iCount--;
	SetGraphics(Format("%d", iCount), Fuse, 1, GFXOV_MODE_Picture);

	Message("%d left", clonk, iCount);

	if(iCount == 0)
	{
		var pos = clonk->GetItemPos(this);
		ChangeDef(Igniter);
		SetGraphics("0", Fuse, 1, GFXOV_MODE_Picture);
		clonk->UpdateAttach();
		clonk->OnSlotFull(pos);
	}

	return true;
}

local fWarning;
local fWarningColor;

func FxIntLengthTimer(pTarget, iNumber, iTime)
{
	var iLength = 0;
	var i = GetLength(aWires)-1;
	while(i >= 0 && aWires[i])
	{
		iLength += aWires[i]->GetLineLength();
		i--;
	}
	if(iLength > DYNA_MaxLength*4/5)
	{
		fWarning = 1;
		Message("Line too long! %d%%", this, iLength*100/DYNA_MaxLength);
		if( (iTime % 5) == 0)
		{
			var fOn = 1;
			if( fWarningColor == 1) fOn = 0;
			fWarningColor = fOn;
			for(var i = 0; i < GetLength(aWires); i++)
				if(aWires[i]) aWires[i]->SetColorWarning(fOn);
		}
	}
	else if(fWarning)
	{
		fWarning = 0;
		for(var i = 0; i < GetLength(aWires); i++)
			if(aWires[i]) aWires[i]->SetColorWarning(0);
	}
	if(iLength > DYNA_MaxLength)
	{
		var iMax = GetLength(aWires)-1;
		aWires[iMax]->RemoveObject();
		aDynamites[iMax]->Reset();
		SetLength(aWires, iMax);
		SetLength(aDynamites, iMax);
		Message("Line too long,|lost dynamite!|%d left.", this, iMax);
		if(iMax == 0)
		{
			Message("Line broken.", Contained());
			RemoveObject();
		}
	}
}

func FxIntLengthStop(pTarget, iNumber, iReason, fTmp)
{
	for(var i = 0; i < GetLength(aWires); i++)
			if(aWires[i]) aWires[i]->SetColorWarning(0);
}

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
}
