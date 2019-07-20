/**
	CustomRingProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	radius: radius of the bar in pixels, the amount of points is dynamically adjusted
	image: definition to show when filled
	back_image: definition to show when not filled, defaults to image
	color: foreground color
	back_color: background color
*/

#include GUI_RingProgressBar

local Name = "$Name$";
local Description = "$Description$";

local image, back_image;
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
	data.image = data.image ?? GUI_RingProgressBar;
	data.back_image = data.back_image ?? data.image;
	data.color = data.color ?? RGBa(255, 255, 255, 200);
	data.back_color = data.back_color ?? RGBa(50, 50, 50, 50);
	
	image = data.image;
	back_image = data.back_image;
	color = data.color;
	back_color = data.back_color;
	
	return inherited(to, max, cur, timeout, offset, visibility, data);
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
			obj->SetGraphics(nil, back_image, 0, GFXOV_MODE_ObjectPicture, nil, GFX_BLIT_Custom);
			obj->SetClrModulation(back_color);
		}
		else
		{
			obj->SetGraphics(nil, image, 0, GFXOV_MODE_ObjectPicture, nil, GFX_BLIT_Custom);
			obj->SetClrModulation(color);
		}
		obj->Rotate(obj.my_angle + 180, 0, 0);
	}
}

