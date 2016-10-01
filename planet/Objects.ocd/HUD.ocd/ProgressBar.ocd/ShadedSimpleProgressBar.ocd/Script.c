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
	var p = (current * 100) / Max(1, maximum);
	var w = (width * 1000) / 110;
	var l = (width * p * 10) / 110;
	SetObjDrawTransform((width * 1000) / 110, 0, 0, 0, (height * 1000) / 19, 0, 0);
	SetObjDrawTransform(l, 0, -(w-l) * 55, 0, (height * 1000) / 19, 100, 1);
}

/*
	adds an energy bar above the object.
	The energy bar uses either target.HitPoints & GetDamage() or target->GetMaxEnergy() & target->GetEnergy().
*/
global func AddEnergyBar()
{
	var e = AddEffect("ShowEnergyBar", this, 1, 0, nil, nil);
	if (e)
		return e.bar;
}

global func FxShowEnergyBarStart(target, effect, temp)
{
	if (temp) return;
	var attachpoint = { x = 0, y = target->GetDefOffset(1) - 5};
	var current, max;
	if (target->GetCategory() & C4D_Living)
	{
		max = target->~GetMaxEnergy();
		current = target->GetEnergy();
	}
	else
	{
		max = target.HitPoints;
		current = max - target->GetDamage();
	}
	
	if (current == nil || max == nil)
		return -1;
	
	effect.bar = target->CreateProgressBar(GUI_ShadedSimpleProgressBar, max, current, 0, target->GetOwner(), attachpoint, nil, { width = 28, height = 6, color = RGB(200, 1, 1) });
	effect.bar->SetPlane(750);
	// update once
	effect.Interval = 1;
	
	return 1;
}

global func FxShowEnergyBarTimer(target, effect, time)
{
	var value;
	if (target->GetCategory() & C4D_Living) value = target->GetEnergy();
	else value = target.HitPoints - target->GetDamage();
	
	effect.bar->SetValue(value);
	effect.Interval = 0;
	return 1;
}

global func FxShowEnergyBarDamage(target, effect, dmg, cause)
{
	effect.Interval = 1;
	return dmg;
}

global func FxShowEnergyBarStop(target, effect, reason, temp)
{
	if (temp) return;
	if (effect.bar)
		effect.bar->Close();
}