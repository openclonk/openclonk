/*
	Bullet trail
	Author: Newton

	Draws a trail to any object. That object should fly straight or so fast
	that it looks like it because the trail itself is only drawn straight.
	Any projectile can use the trail by calling (after creating it):
	
	Set(int width, int length, object projectile)
	
	while width is the width of the trail, length the length of the trail and
	projectile itself. The trail will be removed when the projectile is gone.
	The color is always a light-grey. However each frame, ~TrailColor(int time)
	is called in the projectile which can return the color modulation.
*/

local fRemove, iSpeed, pShot, w, l, r, x, y;

public func Set(int iWidth, int iLength, object pSht)
{
	pShot = pSht;

	w = 1000*iWidth/ActMap.Travel.Wdt;
	l = 1000*iLength/ActMap.Travel.Hgt;

	var iXDir = pShot->GetXDir(1000);
	var iYDir = pShot->GetYDir(1000);

	iSpeed = Sqrt(iXDir*iXDir/1000+iYDir*iYDir/1000);

	SetAction("Travel");
	SetXDir(iXDir,1000);
	SetYDir(iYDir,1000);

	r = Angle(0,0,iXDir,iYDir);
	x = GetX();
	y = GetY();

	SetPosition(pShot->GetX(),pShot->GetY());
	Traveling();
}

/* Timer */

private func Traveling()
{
	// The shot is gone -> remove
	if(!fRemove)
		if(!pShot)
			Remove();

	// on the borders
	if(GetX() <= 0 && GetXDir() < 0
	|| GetX() >= LandscapeWidth() && GetXDir() > 0
	|| GetY() <= 0 && GetYDir() < 0
	|| GetY() >= LandscapeHeight() && GetYDir() > 0)
		Remove();
	
	if(pShot)
		SetPosition(pShot->GetX(),pShot->GetY());
			
	// Display
	DrawTransform();
	if(pShot)
		if(pShot->~TrailColor(GetActTime()) != nil)
			SetClrModulation(pShot->~TrailColor(GetActTime()));

	if(l == 0) RemoveObject();
}

public func Hit()
{
	Remove();
}

public func Remove() {
	SetXDir();
	SetYDir();
	l = Min(l,1000*Distance(x,y,GetX(),GetY())/ActMap.Travel.Hgt);
	fRemove = true;
	pShot = nil;
}

public func DrawTransform() {

	var distance = Distance(x,y,GetX(),GetY());
	var relative_length = 1000*distance/ActMap.Travel.Hgt;

	// skip because nothing has to be transformed
	if(!fRemove && l < relative_length) return;

	// stretch >-<
	if(fRemove) l = Max(0,l-iSpeed);

	// stretch <->
	var h = Min(l,relative_length);

	var fsin = -Sin(r, 1000), fcos = Cos(r, 1000);

	var xoff = -(ActMap.Travel.Wdt*w/1000)/2;
	var yoff = 0;

	var width = +fcos*w/1000, height = +fcos*h/1000;
	var xskew = +fsin*h/1000, yskew = -fsin*w/1000;

	var xadjust = +fcos*xoff + fsin*yoff;
	var yadjust = -fsin*xoff + fcos*yoff;

	// set matrix values
	SetObjDrawTransform (
		width, xskew, xadjust,
		yskew, height, yadjust
	);
}

public func SaveScenarioObject() { return false; }

local ActMap = {

Travel = {
	Prototype = Action,
	Name = "Travel",
	Procedure = DFA_FLOAT,
	Speed = 100000,
	Accel = 16,
	Decel = 16,
	NextAction = "Travel",
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 5,
	Hgt = 25,
	OffX = 0,
	OffY = 2,
	StartCall = "Traveling"
},
};
