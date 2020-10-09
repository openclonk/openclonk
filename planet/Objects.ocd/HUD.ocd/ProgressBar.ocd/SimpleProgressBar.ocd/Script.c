/**
	SimpleProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	color: color of the inside
	back_color: color of the background
	width: length of the bar in pixels
	height: height of the bar in pixels
*/

local Name = "$Name$";
local Description = "$Description$";

local maximum, current, timeout_time;

local width, height, color, back_color;

local ActMap=
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure = DFA_ATTACH,
		NextAction="Be",
		Length = 1,
		FacetBase = 1,
		AbortCall = "AttachTargetLost"
	}
};


func Init(to, max, cur, timeout, offset, visibility, proplist data)
{
	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	width = data.width ?? 40;
	height = data.height ?? 5;

	
	if (timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}
	
	this.Visibility = visibility;
	
	SetGraphics(nil, GetID(), 1, GFXOV_MODE_Base, nil, GFX_BLIT_Custom);
	SetBarColor(data.color, data.back_color);
	
	SetAction("Attach", to);
	SetVertexXY(0, -offset.x + to->GetVertex(0, VTX_X), -offset.y + to->GetVertex(0, VTX_Y));
	Update();
}

func SetBarColor(c, b)
{
	color = c ?? RGB(200, 200, 10);
	back_color = b ?? RGB(1, 1, 1);
	
	SetClrModulation(color, 1);
	SetClrModulation(back_color, 0);
}

func FxTimeOutTimer(target, effect, time)
{
	effect.t -= effect.Interval;
	if (effect.t > 0) return 1;
	Close();
	return -1;
}

func Update()
{
	var p = (current * 100) / maximum;
	var w = (width * 1000) / 16;
	var l = (width * p * 10) / 16;
	SetObjDrawTransform(w, 0, 0, 0, (height * 1000) / 16, 0, 0);
	SetObjDrawTransform(l, 0, -(w-l) * 8, 0, (height * 800) / 16, 100, 1);
}

func Close()
{
	RemoveObject();
}

func Destruction()
{
	
}

func SetValue(int to)
{
	current = BoundBy(to, 0, maximum);;
	var e = GetEffect("TimeOut", this);
	if (e)
		e.t = timeout_time;
	Update();
}

func DoValue(int change)
{
	SetValue(current + change);
}


func AttachTargetLost()
{
	return RemoveObject();
}

func SetPlane(int to)
{
	this.Plane = to;
}
