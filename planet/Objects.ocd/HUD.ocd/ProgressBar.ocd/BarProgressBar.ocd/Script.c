/**
	BarProgressBar
	Shows progress.
	Can also show a custom ID.
	
	additional data the bar takes through the "data" parameter:
	bars: number of bars
	color: color of filled bars
	back_color: color of empty bars
	graphics_name: graphics name of filled bars
	back_graphics_name: graphics name of empty bars
	size: size of the bar 1000 = 100%
	image: id to use as bar graphics
	fade_speed: after the time-out the bar starts to fade, this can specify the speed. standard: 5
*/

local Name = "$Name$";
local Description = "$Description$";

local maximum, current, timeout_time;
local bars;
local color, back_color, number_of_bars, size;
local graphics_name, back_graphics_name;
local image, fade_speed;

local current_clr;
local active_overlay;

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


func BarID()
{
	return GetID();
}

func Init(to, max, cur, timeout, offset, visibility, data)
{
	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	bars = [];
	
	number_of_bars = data.bars ?? 10;
	
	graphics_name = data.graphics_name;
	back_graphics_name = data.back_graphics_name;
	color = data.color;
	back_color = data.back_color; 
	
	if (!color) 
	{
		if (graphics_name)
			color = RGB(255, 255, 255);
		else
			color = RGB(1, 255, 1);
	}
	if (!back_color)
	{
		if (back_graphics_name)
			back_color = RGB(255, 255, 255);
		else
			back_color = RGBa(1, 1, 1, 150);
	}
	
	size = data.size ?? 1000;
	image = data.image ?? nil;
	fade_speed = data.fade_speed ?? 5;
	
	if (timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, 5, this);
		e.t = timeout_time;
	}

	bars[0] = this;
	
	for (var i = 1; i < number_of_bars; ++i)
	{
		bars[i] = CreateObjectAbove(GetID(), 0, 0, GetOwner());
	}
		
	var cnt = 0;
	for (var obj in bars)
	{
		if (image != nil)
		{
			obj->SetObjDrawTransform(1, 0, 0, 0, 1); // deactivate overlay 0
			obj->SetGraphics(nil, image, 1, GFXOV_MODE_IngamePicture, nil, nil);
			obj.active_overlay = 1;
		}
		obj->Set(to, cnt, number_of_bars, size, offset, visibility);
		++cnt;
	}
	Update();
}

func FxTimeOutTimer(target, effect, time)
{
	effect.t -= effect.Interval;
	if (effect.t > 0) return 1;
	var a = 255 - fade_speed * Abs(effect.t);
	if (a <= 20) {Close(); return -1;}
	else SetFade(a);
	
	return 1;
}

func Update()
{
	var l = GetLength(bars);
	var p = (current * 100) / maximum;
	var last_colored = (l * p) / 100;
	
	
	for (var i = 0; i < l; ++i)
	{
		var obj = bars[i];
		if (i >= last_colored)
		{
			obj.current_clr = back_color;
			obj->SetClrModulation(back_color, active_overlay);
			obj->SetGraphics(back_graphics_name, image, active_overlay, GFXOV_MODE_IngamePicture, nil, nil);
			continue;
		}		
		
		obj.current_clr = color;
		obj->SetClrModulation(color, active_overlay);
		obj->SetGraphics(graphics_name, image, active_overlay, GFXOV_MODE_IngamePicture, nil, nil);
	}
}

func Close()
{
	RemoveObject();
}

func Destruction()
{
	if (GetType(bars) == C4V_Array)
	for (var i = GetLength(bars) - 1; i > 0; --i) // off-by-one on purpose
	{
		var obj = bars[i];
		if (obj)
			obj->RemoveObject();
	}
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

func Initialize()
{
}	

func AttachTargetLost()
{
	return RemoveObject();
}

func Set(to, number, max_num, size, offset, visibility)
{
	SetAction("Attach", to);
	var x = 0 - offset.x;
	var y = (6 * number * size) / 1000 - offset.y;
	SetPosition(GetX() - x, GetY() - y + 8); // for good position in first frame
	SetVertexXY(0, x + to->GetVertex(0, VTX_X), y + to->GetVertex(0, VTX_Y));
	
	SetObjDrawTransform(size, 0, 0, 0, size, (6 * (1000 - size)), active_overlay);
	this.Visibility = visibility;
}

func SetFade(int a)
{
	for (var bar in bars)
	{
		var t_a = BoundBy(((bar.current_clr >> 24)) + a, 0, 255);
		var clr = (bar.current_clr & 0xffffff) + (t_a << 24);
		bar->SetClrModulation(clr, active_overlay);
	}
}

func SetPlane(int to)
{
	// called on a slave?
	if (GetType(bars) != C4V_Array) return;
	
	for (var bar in bars)
		bar.Plane = to;
}
