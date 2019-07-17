/**
	Status Symbol
	Shows a certain state of an object.
	
	@author Zapper, Maikel
*/


local symbol;

public func Init(object to)
{
	SetAction("Be", to);
	
	// Above the object.
	var hgt = to->GetDefCoreVal("Height", "DefCore") / 2;
	SetVertex(0, VTX_Y, hgt);
	var x = to->GetVertex(0, VTX_X);
	SetVertex(0, VTX_X, x);
	// Set plane to be higher than the object attached to.
	this.Plane = Max(to.Plane + 1, 1000);	
	return;
}

public func GetStatusSymbolHelper(object to)
{
	var obj = FindObject(Find_ID(StatusSymbol), Find_ActionTarget(to));
	if (obj) 
		return obj;
	obj = CreateObject(StatusSymbol, 0, 0, to->GetOwner());
	obj->Init(to);
	return obj;
}

global func ShowStatusSymbol(id symbol)
{
	if (!this) 
		return false;
	var h = StatusSymbol->GetStatusSymbolHelper(this);
	if (!h) 
		return false;
	
	h->AddSymbol(symbol);
	return true;
}

global func RemoveStatusSymbol(id symbol)
{
	if (!this) 
		return false;
	var h = StatusSymbol->GetStatusSymbolHelper(this);
	if (!h) 
		return false;
	h->RemoveSymbol(symbol);
	return true;
}

public func AddSymbol(id symbol_id)
{
	symbol = symbol_id;
	Update();
}

public func RemoveSymbol(id symbol_id)
{
	if (symbol != symbol_id)
		return;
	symbol = nil;
	Update();
}

public func Update()
{
	if (!symbol)
	{
		SetGraphics();
		this.Visibility = VIS_None;
		return;
	}
	SetShape(symbol->GetDefOffset(0), symbol->GetDefOffset(1), symbol->GetDefWidth(), symbol->GetDefHeight());
	SetGraphics(nil, symbol);
	Blink();
	return;
}

public func Blink()
{
	if (GetEffect("Blinking", this))
		RemoveEffect("Blinking", this);
	AddEffect("Blinking", this, 1, 16, this);
	return;
}

protected func FxBlinkingStart(object target, proplist effect, int temp)
{
	if (temp) 
		return FX_OK;
	// Set interval to a fixed number of frames.
	effect.Interval = 16;
	// Set initial visibility to visible.
	this.Visibility = this->GetActionTarget().Visibility;
	effect.visible = true;	
	return FX_OK;
}

protected func FxBlinkingTimer(object target, proplist effect)
{
	if (effect.visible)
	{
		effect.visible = false;	
		this.Visibility = VIS_None;
	}
	else
	{
		effect.visible = true;	
		this.Visibility = this->GetActionTarget().Visibility;
	}
	return FX_OK;
}

// Callback from the engine: this symbol has lost its parent.
protected func AttachTargetLost()
{
	return RemoveObject();
}
	
public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 1000;

local ActMap =
{
	Be = 
	{
		Prototype = Action,
		Name = "Be",
		Procedure = DFA_ATTACH,
		NextAction = "Be",
		Length = 1,
		FacetBase = 1,
		AbortCall = "AttachTargetLost"
	}
};
