/**
	BarProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	bars: number of bars
	color: color of filled bars
	back_color: color of empty bars
	size: size of the bar 1000 = 100%
*/

local Name = "$Name$";
local Description = "$Description$";

local maximum, current, timeout_time;
local bars;
local color, back_color, number_of_bars, size;

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
	
	bars = [];
	
	number_of_bars = data.bars ?? 10;
	color = data.color ?? RGB(1, 255, 1);
	back_color = data.back_color ?? RGBa(1, 1, 1, 150);
	size = data.size ?? 1000;
	
	if(timeout_time)
	{
		var e = AddEffect("TimeOut", this, 1, BoundBy(timeout_time/2, 5, 35), this);
		e.t = timeout_time;
	}

	bars[0] = this;
	
	for(var i = 1; i < number_of_bars; ++i)
		bars[i] = CreateObject(GetID(), 0, 0, GetOwner());
		
	var cnt = 0;
	for(var obj in bars)
	{
		obj->Set(to, cnt, number_of_bars, size, offset, visibility);
		++cnt;
	}
	
	AddEffect("LifeCheck", to, 1, 0, this);
	Update();
}

func FxLifeCheckStop(target, effect, cause, temp)
{
	if(temp) return;
	if(this)
		this->RemoveObject();
}

func FxTimeOutTimer(target, effect, time)
{
	effect.t -= effect.Interval;
	if(effect.t > 0) return 1;
	Close();
	return -1;
}

func Update()
{
	var l = GetLength(bars);
	var p = (current * 100) / maximum;
	var last_colored = (l * p) / 100;
	
	
	for(var i = 0; i < l; ++i)
	{
		var obj = bars[i];
		if(i >= last_colored)
		{
			obj->SetClrModulation(back_color);
			continue;
		}		
		obj->SetClrModulation(color);
	}
}

func Close()
{
	RemoveObject();
}

func Destruction()
{
	if(GetType(bars) == C4V_Array)
	for(var i = GetLength(bars) - 1; i > 0; --i) // off-by-one on purpose
	{
		var obj = bars[i];
		if(obj)
			obj->RemoveObject();
	}
}

func SetValue(int to)
{
	current = BoundBy(to, 0, maximum);;
	var e = GetEffect("TimeOut", this);
	if(e)
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
	
	SetObjDrawTransform(size, 0, 0, 0, size, (6 * (1000 - size)));
	this.Visibility = visibility;
}
