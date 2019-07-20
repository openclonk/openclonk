
public func Set(int sx, int sy, int ex, int ey) {
	//SetObjectBlitMode(GFX_BLIT_Additive);
	SetAction("Vis");
	SetPosition(sx, sy);
	SetClrModulation(RGB(255, 0, 0));

	var cl = 1000*Distance(sx, sy, ex, ey)/256;
	var w = 650;
	var r = Angle(sx, sy, ex, ey)-180;

	var fsin = -Sin(r, 1000), fcos = Cos(r, 1000);

	var xoff = -4;
	var yoff = 0;

	var width = +fcos*w/1000, height = +fcos*cl/1000;
	var xskew = +fsin*cl/1000, yskew = -fsin*w/1000;

	var xadjust = +fcos*xoff + fsin*yoff;
	var yadjust = -fsin*xoff + fcos*yoff;

	// set matrix values
	SetObjDrawTransform (
		width, xskew, xadjust,
		yskew, height, yadjust
	);

}

local ActMap = {
	Vis = {
		Prototype = Action,
		Name = "Vis",
		Procedure = DFA_FLOAT,
		Length = 1,
		X = 0,
		Y = 0,
		Wdt = 32,
		Hgt = 256,
		OffX = -16,
		OffY = 0,
		NextAction = "Hold"
	},
};
local Name = "Path";
