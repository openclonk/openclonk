/*
	Progress-Bar element
	Author: Newton
	
	This object can show an unlimited amount of progress bars in different
	locations, sizes and colors. Using this basic funcionality, one could create
	floating health bars attached to clonks or include the bars as layers into
	(HUD) objects.
	This object is used by the crew selector.
*/

local offsx, offsy, layer, width, height;

protected func Construction()
{
	offsx = CreateArray();
	offsy = CreateArray();
	width = CreateArray();
	height = CreateArray();
	layer = CreateArray();
}

public func SetBarOffset(int x, int y, int num)
{
	offsx[num] = x;
	offsy[num] = y;
}

public func RemoveBarLayers(int la)
{
	// remove layers
	SetGraphics(nil, nil, la);
	SetGraphics(nil, nil, la + 1);
}

public func SetBarLayers(int la, int num)
{
	RemoveBarLayers(la);

	// new layers
	layer[num] = la;
	SetGraphics("Empty",Library_Bars, layer[num],GFXOV_MODE_Base);
	SetGraphics("Bar",Library_Bars, layer[num]+1, GFXOV_MODE_Base);
}

public func SetBarDimensions(int wdt, int hgt, int num)
{
	width[num] = 1000 * wdt / Library_Bars->GetDefWidth();
	height[num] = 1000 * hgt / Library_Bars->GetDefHeight();
}

public func SetBarProgress(int promille, int num)
{
	// not existing
	if (GetLength(layer) <= num) return false;

	// width/height not set == 1000
	if (!width[num]) width[num] = 1000;
	if (!height[num]) height[num] = 1000;

	var w = Library_Bars->GetDefWidth()/2;
	
	// the bar does not start on the left side of the graphics... correct this
	var graphicscorrect = 100;
	
	var baroffset = offsx[num]*1000 - width[num]*w * (1000-promille)/(1000 + graphicscorrect);

	SetObjDrawTransform(width[num],0, offsx[num]*1000, 0, height[num], offsy[num]*1000, layer[num]);
	SetObjDrawTransform((promille * width[num])/1000, 0, baroffset, 0, height[num], offsy[num]*1000, layer[num]+1);

	return true;
}

// UI not saved.
func SaveScenarioObject() { return false; }
