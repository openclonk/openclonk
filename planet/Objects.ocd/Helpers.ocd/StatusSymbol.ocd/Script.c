/**
	Status Symbol
	Shows a certain state of an object.
	
	@author Zapper, Maikel
*/


local symbol;

func AttachTargetLost(){return RemoveObject();}

func Init(object to)
{
	SetAction("Be", to);
	
	// Above the object.
	var hgt = to->GetDefCoreVal("Height", "DefCore") / 2;
	SetVertex(0, VTX_Y, hgt);
	var x = to->GetVertex(0, VTX_X);
	SetVertex(0, VTX_X, x);
	// Set plane to be high.
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
	if(!h) 
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

func AddSymbol(id symbol_id)
{
	symbol = symbol_id;
	Update();
}

func RemoveSymbol(id symbol_id)
{
	if (symbol != symbol_id)
		return;
	symbol = nil;
	Update();
}

func Update()
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

func Blink()
{
	if (GetEffect("Blinking", this))
		RemoveEffect("Blinking", this);
	AddEffect("Blinking", this, 1, 24, this);
}

func FxBlinkingStart(target, effect, temp)
{
	if (temp) 
		return;
	effect.cycle = 1;
}

func FxBlinkingTimer(target, effect)
{
	effect.cycle++;
	if ((effect.cycle % 2) == 0)
	{
		this.Visibility = VIS_None;
		return 1;
	}
	this.Visibility = this->GetActionTarget().Visibility;
	return 1;
}
	
func SaveScenarioObject() { return false; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";

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
