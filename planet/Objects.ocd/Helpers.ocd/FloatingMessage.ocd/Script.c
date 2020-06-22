/**
	FloatingMessage
	Object that serves as a helper for creating floating messages.
	
	setters:
	SetColor(r, g, b, a)
	SetMessage(msg)
	FadeOut(speed, step = 1)
		every 'speed' frames the alpha of the message is increased by 'step'
*/

local ActMap = {
Float = {
	Prototype = Action,
	Name = "Float",
	Procedure = DFA_FLOAT,
	Length = 1,
	Delay = 0,
	FacetBase = 1,
}
};

local Name = "$Name$";
local Description = "$Description$";
local Plane = 300;

local msg;
local r, g, b;
local alpha;

func SetMessage(m)
{
	msg = m;
	Update();
}

func FadeOut(int speed, step)
{
	if (step == nil)step = 1;
	var e = AddEffect("FadeOut", this, 1, speed, this);
	e.step = step;
}

func FxFadeOutTimer(target, effect)
{
	alpha -= effect.step;
	if (alpha < 5) return RemoveObject();
	Update();
	return 1;
}


func SetColor(int r2, g2, b2, a)
{
	r = r2;
	g = g2;
	b = b2;
	if (a != nil) alpha = a;
	Update();
}

func Update()
{
	// the lacking </c> is on purpose
	this->Message(Format("@<c %x><c %x>%s</c>", RGBa(255, 255, 255, alpha), RGBa(r, g, b, alpha), msg));
}

public func Initialize()
{
	SetAction("Float");
	SetComDir(COMD_None);
	SetSpeed(0, -20);
	alpha = 255;
	r = 255;
	g = 255;
	b = 255;
	return true;
}

func SaveScenarioObject() { return false; }
