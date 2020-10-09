/**
	PieProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	size: size of the pie, 1000 = 100%
	color: foreground color
	back_color: background color
*/

#include GUI_RingProgressBar

local Name = "$Name$";
local Description = "$Description$";

local size;
local color, back_color;

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

func Init(to, max, cur, timeout, offset, visibility, data)
{
	data.color = data.color ?? RGBa(255, 255, 255, 200);
	data.back_color = data.back_color ?? RGBa(50, 50, 50, 50);
	
	size = data.size ?? 1000;
	color = data.color;
	back_color = data.back_color;

	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	ring = [];
	
	if (timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}
	
	var amount = 24;
	
	ring[0] = this;
	
	for (var i = 1; i < amount; ++i)
		ring[i] = CreateObjectAbove(GetID(), 0, 0, GetOwner());

	var cnt = 0;
	for (var obj in ring)
	{
		obj->Set(to, 180 / amount + ((cnt * 360) / amount), offset, visibility, size);
		++cnt;
	}
	Update();
}

func Update()
{
	var l = GetLength(ring);
	var p = (current * 100) / maximum;
	var last_colored = (l * p) / 100;
	
	
	for (var i = 0; i < l; ++i)
	{
		var obj = ring[i];
		if (i >= last_colored)
		{
			obj->SetClrModulation(back_color);
		}
		else
		{
			obj->SetClrModulation(color);
		}
	}
}

func Set(to, angle, offset, visibility, size)
{
	SetAction("Attach", to);
	var distance = (10 * size) / 1000;
	
	var x = -Sin(angle, distance) - offset.x;
	var y = +Cos(angle, distance) - offset.y;
	SetPosition(GetX() - x, GetY() - y + 8); // for good position in first frame
	SetVertexXY(0, x + to->GetVertex(0, VTX_X), y + to->GetVertex(0, VTX_Y));
	my_angle = -angle;
	Rotate(my_angle, 0, 0, (2500 * size) / 1000);
	
	this.Visibility = visibility;
}

func Rotate (int r, int xoff, int yoff, size) {
  var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
  size = size ?? 1000;
  // set matrix values
  SetObjDrawTransform (
    (+fcos) * size / 1000, (+fsin) * size / 1000, ((1000-fcos)*xoff - fsin*yoff) * size / 1000,
    (-fsin) * size / 1000, (+fcos) * size / 1000, ((1000-fcos)*yoff + fsin*xoff) * size / 1000
  );
}
