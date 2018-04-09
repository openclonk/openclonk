/**
	CustomImageProgressBar
	Shows progress.
	Takes an image-definition that is used as the source definition for SetGraphics.
	The definition can have the callback "GetBarGraphicsName(int percent)" to return a string for a graphics file that is used for SetGraphics.
	It can also have GetBarColor(int percent) to return a proplist with {r = ?, g = ?, b = ?} to get the color shown.
	
	additional data the bar takes through the "data" parameter:
	image: definition to use as graphics
	size: size of the progress bar 1000 = 100%
*/

local Name = "$Name$";
local Description = "$Description$";

local current, maximum, timeout_time;
local image, size;
local lr, lg, lb, la, graphics_name;

local ActMap=
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure=DFA_ATTACH,
		NextAction="Be",
		Length=1,
		FacetBase=1,
		AbortCall = "AttachTargetLost"
	}
};


func Init(to, max, cur, timeout, offset, visibility, data)
{
	maximum = max;
	current = cur;
	timeout_time = timeout;
	
	la = 255;
	
	size = data.size ?? 1000;
	image = data.image ?? Rock;
	
	if(timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}
	
	SetAction("Attach", to);
	var x = -offset.x;
	var y = -offset.y;
	SetPosition(GetX() - x, GetY() - y + 32); // for good position in first frame
	SetVertexXY(0, x + to->GetVertex(0, VTX_X), y + to->GetVertex(0, VTX_Y));

	this.Visibility = visibility;
	
	Update();
}

func FxTimeOutTimer(target, effect, time)
{
	effect.t -= effect.Interval;
	if(effect.t > 0) return 1;
	if(!GetEffect("FadeOut", this))
		FadeOut();
	return 1;
}

func FadeOut()
{
	AddEffect("FadeOut", this, 1, 3, this);
}

func FxFadeOutTimer(target, effect, time)
{
	if(la <= 20) return Close();
	la -= 15;
	SetClrModulation(RGBa(lr, lg, lb, la));
}

func Update()
{
	var p = (current * 100) / maximum;
	var charge = (255 * p) / 100;
	var clr = image->~GetBarColor(p);
	if (clr)
	{
		lr = clr.r;
		lg = clr.g;
		lb = clr.b;
	}
	else
	{
		lr = charge;
		lg = 255 - charge;
		lb = 255;
	}
	
	SetGraphics(image->~GetBarGraphicsName(p), image, 0, GFXOV_MODE_IngamePicture, nil, GFX_BLIT_Custom);
	SetClrModulation(RGB(lr, lg, lb));
	SetObjDrawTransform(size, 0, 0, 0, size);
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
	if(e)
		e.t = timeout_time;
	if(GetEffect("FadeOut", this))
		RemoveEffect("FadeOut", this);
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
	if(f)
	{
		SetCategory(GetCategory() | C4D_Parallax);
		this.Parallaxity = [0, 0];
	}
	else
	{
		SetCategory(GetCategory() & ~C4D_Parallax);
		this.Parallaxity = nil;
	}
	return true;
}

func SetPlane(int to)
{
	if(to == nil) return;
	this.Plane = to;
	return true;
}


func MakeHUDElement()
{
	SetCategory(C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax);
	SetParallax();
}

func AttachTargetLost()
{
	return RemoveObject();
}
