/**
	Action.c
	Stuff for the proplist changes.
	
	@author Günther
*/

static const DFA_NONE    = nil;
static const DFA_WALK    =  "WALK";
static const DFA_FLIGHT  =  "FLIGHT";
static const DFA_KNEEL   =  "KNEEL";
static const DFA_SCALE   =  "SCALE";
static const DFA_HANGLE  =  "HANGLE";
static const DFA_DIG     =  "DIG";
static const DFA_SWIM    =  "SWIM";
static const DFA_THROW   =  "THROW";
static const DFA_BRIDGE  =  "BRIDGE";
static const DFA_PUSH    =  "PUSH";
static const DFA_LIFT    =  "LIFT";
static const DFA_FLOAT   =  "FLOAT";
static const DFA_ATTACH  =  "ATTACH";
static const DFA_CONNECT =  "CONNECT";
static const DFA_PULL    =  "PULL";
static const Action = 
{
	GetName = Global.GetName,
	Length = 1,
	Directions = 1,
	Step = 1,
	Procedure = DFA_NONE,
};

// documented in /docs/sdk/script/fn
global func GameCall(string fn, ...)
{
	if (!fn)
		return;
	var f = Scenario[fn];
	if (!f)
		return;
	return Scenario->Call(f, ...);
}
