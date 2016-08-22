/**
	Bullet trail
	Draws a trail to any object. That object should fly straight or so fast
	that it looks like it because the trail itself is only drawn straight.
	Any projectile can use the trail by calling (after creating it):
	
	Set(object bullet, int width, int length)
	
	while width is the width of the trail, length the length of the trail and
	projectile itself. The trail will be removed when the projectile is gone.
	The color is always a light-grey. However each frame, ~TrailColor(int time)
	is called in the projectile which can return the color modulation.
	
	@author Newton, Maikel
*/

local for_bullet;
local width, length;
local speed, rotation;
local orig_x, orig_y;

public func Set(object bullet, int wdt, int lgt)
{
	// Store the bullet for which this trail is made.
	for_bullet = bullet;

	// Change plane to be just behind bullet.
	this.Plane = for_bullet.Plane - 1;

	// Store trail properties.
	width = 1000 * wdt / ActMap.Travel.Wdt;
	length = 1000 * lgt / ActMap.Travel.Hgt;
	var xdir = for_bullet->GetXDir(1000);
	var ydir = for_bullet->GetYDir(1000);
	speed = Distance(0, 0, xdir, ydir) / Sqrt(1000);
	rotation = Angle(0, 0, xdir, ydir);
	orig_x = for_bullet->GetX();
	orig_y = for_bullet->GetY();

	// Copy bullet's motion.
	SetAction("Travel");
	SetComDir(COMD_None);
	SetPosition(orig_x, orig_y);
	SetXDir(xdir, 1000);
	SetYDir(ydir, 1000);
	return;
}

private func UpdateTrail()
{
	// Remove trail if the bullet is gone or the trail is out of the landscape borders.
	if (!for_bullet)
		return Remove();
	if ((GetX() <= 0 && GetXDir() < 0) || (GetX() >= LandscapeWidth() && GetXDir() > 0) || (GetY() <= 0 && GetYDir() < 0) || (GetY() >= LandscapeHeight() && GetYDir() > 0))
		return Remove();
	
	// Display.
	DrawTransform();
	// Adjust trail color as specified in bullet.
	if (for_bullet)
	{
		var trail_color = for_bullet->~TrailColor(GetActTime());
		if (trail_color != nil)
			SetClrModulation(for_bullet->~TrailColor(GetActTime()));			
	}
	return;
}

public func RemoveTrail()
{
	// Reduce trail length by its speed every frame.
	length = Max(0, length - speed);
	// Display.
	DrawTransform();
	// Remove trail if its length is zero.
	if (length <= 0)
		return RemoveObject();
	return;
}

public func Hit()
{
	// Remove trail if it hit something.
	return Remove();
}

private func Remove()
{
	SetXDir(0);
	SetYDir(0);
	return SetAction("Remove");
}

public func DrawTransform()
{
	// Determine length, width and position of the trail.
	var distance = Distance(orig_x, orig_y, GetX(), GetY());
	var relative_length = 1000 * distance / ActMap.Travel.Hgt;

	var h = Min(length, relative_length);

	var fsin = -Sin(rotation, 1000);
	var fcos = Cos(rotation, 1000);

	var xoff = -ActMap.Travel.Wdt * width / 2 + 750;
	var yoff = 0;

	// Set object draw transformation.
	var draw_width = +fcos * width / 1000;
	var draw_height = +fcos * h / 1000;
	var xskew = +fsin * h / 1000;
	var yskew = -fsin * width / 1000;
	var xadjust = (+fcos * xoff + fsin * yoff) / 1000;
	var yadjust = (-fsin * xoff + fcos * yoff) / 1000;
	return SetObjDrawTransform(draw_width, xskew, xadjust, yskew, draw_height, yadjust);
}

public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local Plane = 300;

local ActMap = {
	Travel = {
		Prototype = Action,
		Name = "Travel",
		Procedure = DFA_FLOAT,
		NextAction = "Travel",
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 5,
		Hgt = 25,
		OffX = 0,
		OffY = 2,
		StartCall = "UpdateTrail"
	},
	Remove = {
		Prototype = Action,
		Name = "Remove",
		Procedure = DFA_FLOAT,
		NextAction = "Remove",
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 5,
		Hgt = 25,
		OffX = 0,
		OffY = 2,
		StartCall = "RemoveTrail"
	}
};
