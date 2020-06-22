/**
	RingProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	radius: radius of the bar in pixels, the amount of points is dynamically adjusted
	amount: number of segments in the ring, usually calculates form radius
*/

local Name = "$Name$";
local Description = "$Description$";

local maximum, current, timeout_time;
local my_angle;
local ring;

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
	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	ring = [];
	
	if (timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}
	
	
	var radius = data.radius ?? 20;
	var amount = data.amount ?? BoundBy(radius, 8, 30);
	
	ring[0] = this;
	
	for (var i = 1; i < amount; ++i)
		ring[i] = CreateObjectAbove(GetID(), 0, 0, GetOwner());
		
	var cnt = 0;
	for (var obj in ring)
	{
		obj->Set(to, radius, ((cnt * 360) / amount), offset, visibility);
		++cnt;
	}
	Update();
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
	var l = GetLength(ring);
	var p = (current * 100) / maximum;
	var last_colored = (l * p) / 100;
	
	
	for (var i = 0; i < l; ++i)
	{
		var obj = ring[i];
		if (i > last_colored)
		{
			obj->SetClrModulation(RGBa(10, 10, 10, 200));
			continue;
		}
		var p = (i * 100) / l;
		var charge = (255 * p) / 100;
		
		var r = 255, g = 255;
		if (p > 50) r = BoundBy(255 - charge * 2, 0, 255);
		if (p < 50) g = BoundBy(charge * 2, 0, 255);
		obj->SetClrModulation(RGBa(r, g, 1, 200));
	}
}

func Close()
{
	RemoveObject();
}

func Destruction()
{
	if (GetType(ring) == C4V_Array)
	for (var i = GetLength(ring) - 1; i > 0; --i) // off-by-one on purpose
	{
		var obj = ring[i];
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

func SetParallax(f)
{
	f = f ?? true;
	for (var obj in ring)
	{
		if (f)
		{
			obj->SetCategory(obj->GetCategory() | C4D_Parallax);
			obj.Parallaxity = [0, 0];
		}
		else
		{
			obj->SetCategory(obj->GetCategory() & ~C4D_Parallax);
			obj.Parallaxity = nil;
		}
	}
	return true;
}

func SetPlane(int to)
{
	if (to == nil) return;
	
	for (var obj in ring)
	{
		obj.Plane = to;
	}
	return true;
}


func MakeHUDElement()
{
	for (var obj in ring)
	{
		obj->SetCategory(C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax);
	}
	SetParallax();
}

func AttachTargetLost()
{
	return RemoveObject();
}

func Rotate (int r, int xoff, int yoff) {
  var fsin = Sin(r, 1000), fcos = Cos(r, 1000);
  // set matrix values
  SetObjDrawTransform (
    +fcos, +fsin, (1000-fcos)*xoff - fsin*yoff,
    -fsin, +fcos, (1000-fcos)*yoff + fsin*xoff
  );
}

func Set(to, distance, angle, offset, visibility)
{
	SetAction("Attach", to);
	var x = -Sin(angle, distance) - offset.x;
	var y = +Cos(angle, distance) - offset.y;
	SetPosition(GetX() - x, GetY() - y + 8); // for good position in first frame
	SetVertexXY(0, x + to->GetVertex(0, VTX_X), y + to->GetVertex(0, VTX_Y));
	my_angle = -angle;
	Rotate(my_angle, 0, 0);
	
	this.Visibility = visibility;
}
