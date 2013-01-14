/**
	StatusSymbol.ocd
	Shows a certain state of an object.
*/

local Name = "$Name$";
local Description = "$Description$";

local symbols;
local message;

local ActMap=
{
	Be = 
	{
		Prototype = Action,
		Name="Be",
		Procedure=DFA_ATTACH,
		NextAction="Be",
		Length=1,
		FacetBase=1,
		AbortCall = "AttachTargetLost"
	}
};

func AttachTargetLost(){return RemoveObject();}

func Init(to)
{
	symbols=[];
	SetAction("Be", to);
	
	// above the object
	var hgt = to->GetDefCoreVal("Height", "DefCore") / 2;
	SetVertex(0, VTX_Y, hgt);
	var x = to->GetVertex(0, VTX_X);
	SetVertex(0, VTX_X, x);
}

public func GetStatusSymbolHelper(to)
{
	var obj = FindObject(Find_ID(StatusSymbol), Find_ActionTarget(to));
	if(obj) return obj;
	obj = CreateObject(StatusSymbol, 0, 0, to->GetOwner());
	obj->Init(to);
	return obj;
}

global func AddStatusSymbol(ID)
{
	if(!this) return false;
	var h = StatusSymbol->GetStatusSymbolHelper(this);
	if(!h) return false;
	
	h->Add(ID);
	return true;
}

global func RemoveStatusSymbol(ID)
{
	if(!this) return false;
	var h = StatusSymbol->GetStatusSymbolHelper(this);
	if(!h) return false;
	h->Remove(ID);
	return true;
}

func Add(what)
{
	Blink();
	var e = -1;
	for(var i=0;i<GetLength(symbols);++i)
	{
		if(symbols[i] == nil) {e = i; continue;}
		if(symbols[i].ID != what) continue;
		++symbols[i].cnt;
		Update();
		return;
	}
	
	if(e == -1)
		e = GetLength(symbols);
		
	symbols[e] = {ID=what, cnt=1};
	Update();
}

func Remove(what)
{
	Blink();
	for(var i=0;i<GetLength(symbols);++i)
	{
		if(symbols[i] == nil) continue;
		if(symbols[i].ID != what) continue;
		--symbols[i].cnt;
		
		if(symbols[i].cnt <= 0)
			symbols[i] = nil;
	}
	Update();
}

func Update()
{
	message = "@";
	for(var i=0;i<GetLength(symbols);++i)
	{
		if(symbols[i] == nil) continue;
		message = Format("%s{{%i}}", message, symbols[i].ID);
	}
	this->Message(message);
}

func Blink()
{
	if(GetEffect("Blinking", this))
		RemoveEffect("Blinking", this);
	AddEffect("Blinking", this, 1, 36, this);
}

func FxBlinkingStart(target, effect, temp)
{
	if(temp) return;
	effect.cycle = 1;
}

func FxBlinkingTimer(target, effect)
{
	this.Visibility = this->GetActionTarget().Visibility;
	
	if(((++effect.cycle) % 2) == 0)
	{
		this->Message("");
		return 1;
	}
	this->Message(message);
	return 1;
}
	