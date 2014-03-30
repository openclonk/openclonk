/**
	ChargeShower
	Shows stuff.
*/

local Name = "$Name$";
local Description = "$Description$";

local current, to;

local ActMap = {
		Turn = {
			Prototype = Action,
			Name = "Turn",
			Procedure = DFA_ATTACH,
			Length=6,
			Delay=4,
			X = 0,
			Y = 0,
			Wdt = 11,
			Hgt = 19,
			NextAction = "Turn"
		}
	};

func AttachTargetLost()
{
	return RemoveObject();
}

func To(int i)
{
	to = i;
	if(!GetEffect("Adjust", this))
		AddEffect("Adjust", this, 1, 2, this);
}

func FxAdjustTimer()
{
	if(current == to)
		return -1;
	
	if(current < to)
		++current;
	else --current;
	
	Set(current);
	
	return 1;
}

func Set(int i)
{
	SetObjDrawTransform((800 * i)/100, 0, -500, 0, 900, -150);
}

func Init(to)
{
	var x = GetX() - to->GetX();
	var y = GetY() - to->GetY();
	SetVertexXY(0, -x, -y);
	SetAction("Turn", to);
	SetPhase(Random(5));
	this.Plane = to.Plane + 1;
}

public func Initialize()
{
	to=0;
	current=0;
	Set(to);
	return true;
}

public func SaveScenarioObject() { return false; }
