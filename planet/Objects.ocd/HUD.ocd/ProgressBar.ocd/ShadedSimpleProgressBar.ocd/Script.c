/**
	ShadedSimpleProgressBar
	Shows progress.
	
	additional data the bar takes through the "data" parameter:
	color: color of the inside
	back_color: color of the background
	width: length of the bar in pixels
	height: height of the bar in pixels
	
	graphics_name: name of the filled graphics
	back_graphics_name: name of the empty graphics
	
	image: id to get the graphics from
*/

#include GUI_SimpleProgressBar


local graphics_name, back_graphics_name, image;

func Init(to, max, cur, timeout, offset, visibility, proplist data)
{
	inherited(to, max, cur, timeout, offset, visibility, data);
	
	image = data.image ?? GUI_ShadedSimpleProgressBar;
	graphics_name = data.graphics_name ?? "Bar";
	back_graphics_name = data.back_graphics_name ?? "Empty";
	
	SetGraphics(back_graphics_name, image, 0, GFXOV_MODE_Base, nil, GFX_BLIT_Custom);
	SetGraphics(graphics_name, image, 1, GFXOV_MODE_Base, nil, GFX_BLIT_Custom);
	SetBarColor(data.color, data.back_color);
	Update();
}

func SetBarColor(c, b)
{
	color = c ?? RGB(255, 255, 255);
	back_color = b ?? RGB(255, 255, 255);
	
	SetClrModulation(color, 1);
	SetClrModulation(back_color, 0);
}

func Update()
{
	var p = (current * 100) / maximum;
	var w = (width * 1000) / 110;
	var l = (width * p * 10) / 110;
	SetObjDrawTransform((width * 1000) / 110, 0, 0, 0, (height * 1000) / 19, 0, 0);
	SetObjDrawTransform(l, 0, -(w-l) * 55, 0, (height * 1000) / 19, 100, 1);
}
