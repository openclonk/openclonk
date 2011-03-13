// Lightning bolt

local xDir, yDir;
local xDev, yDev;
local gamma;
local strength;

/// Creates a lightning bolt at the specified location.
/// \par x X coordinate. Always global.
/// \par y Y coordinate. Always global.
/// \par strength Strength of the bolt, 0 - 100.
/// \par xdir Average horizontal speed of the bolt.
/// \par ydir Average vertical speed of the bolt.
/// \par xdev Maximum deviation from the average horizontal speed.
/// \par ydev Maximum deviation from the average vertical speed.
/// \par do_gamma If \c true, the lightning bolt will flash the screen.
/// \returns \c true if the lightning could be launched, \c false otherwise.
global func LaunchLightning(int x, int y, int to_strength, int xdir, int ydir, int xdev, int ydev, bool do_gamma)
{
	var lightning = CreateObject(Lightning, x - GetX(), y - GetY());
	return lightning && lightning->Launch(x, y, to_strength, xdir, ydir, xdev, ydev, do_gamma);
}

public func Launch(int x, int y, int to_strength, int xdir, int ydir, int xdev, int ydev, bool do_gamma)
{
	xDir = xdir; yDir = ydir;
	xDev = xdev; yDev = ydev;
	gamma = do_gamma;
	strength = to_strength || 20;
	AddVertex(x-GetX(), y-GetY());
	//Log("Lightning %d: Launching at %d/%d (offset %d/%d)", ObjectNumber(), GetX(), GetY(), GetVertex(0,0), GetVertex(0,1));
	AddEffect("LightningMove", this, 1, 1, this);
	return true;
}

protected func FxLightningMoveTimer()
{
	// Calculate new coordinates to move to.
	var vertices = GetVertexNum();
	var oldx = GetVertex(vertices - 1, 0);
	var oldy = GetVertex(vertices - 1, 1);
	var newx = oldx + xDir + xDev - Random(2 * xDev);
	var newy = oldy + yDir + yDev - Random(2 * yDev);
	//Log("Lightning %d: Moving from %d/%d to %d/%d", ObjectNumber(), oldx+GetX(), oldy+GetY(), newx+GetX(), newy+GetY());
	// Check if lightning hits landscape, and adapt new coordinates. Should it penetrate liquids?
	var strike_solid = PathFree2(oldx + GetX(), oldy + GetY(), newx + GetX(), newy + GetY());
	if (strike_solid)
	{
		newx = strike_solid[0] - GetX();
		newy = strike_solid[1] - GetY();
		//Log("Lightning %d: Move blocked, moving to %d/%d instead", ObjectNumber(), newx+GetX(), newy+GetY());
	}
	AddVertex(newx, newy);
	
	// Draw the new line with lightning particles.
	DrawRotatedParticleLine("LightningBolt", oldx, oldy, newx, newy, strength / 30, 2 * strength / 3, 0xa0f0f0f0);
	
	// Strike objects on the line.
	for (var obj in FindObjects(Find_OnLine(oldx, oldy, newx, newy), Find_NoContainer(), Find_Layer(GetObjectLayer())))
	{
		if (!obj->~LightningStrike(3 + strength / 10))
			Punch(obj, 3 + strength / 10);
	}
	
	// Remove lightning, if struck landscape.
	if (strike_solid)
	{
		RemoveObject();
	}
	// Branch with chance inversely proportional to strength.
	else if (strength > 20)
	{
		if (Random(strength) > 5)
		{
			var branch_strength = (strength + Random(strength)) / 3;
			var lightning = CreateObject(Lightning, newx, newy);
			if (lightning)
				lightning->Launch(newx + GetX(), newy + GetY(), branch_strength, xDir, yDir, xDev, yDev, false);
			strength -= branch_strength / 4;
		}
	}
	// Remove lightning if strength is low.
	else if (!Random(strength / 4))
	{
		RemoveObject();		
	}
	return 1;
}

private func Redraw()
{
	var oldx = GetVertex(0, 0);
	var oldy = GetVertex(0, 1);
	for (var vtx = 1; vtx < GetVertexNum(); ++vtx)
	{
		var newx = GetVertex(vtx, 0);
		var newy = GetVertex(vtx, 1);
		//Log("Lightning %d: Redraw vtx %d->%d %d/%d->%d/%d", ObjectNumber(), vtx-1, vtx, oldx, oldy, newx, newy);
		DrawRotatedParticleLine("LightningBolt", oldx, oldy, newx, newy, strength/5, strength*2, 0xa0f0f0f0);
		oldx = newx; oldy = newy;
	}
}

private func DrawRotatedParticleLine(string particle, int x1, int y1, int x2, int y2, int distance, int sizeFifths, int color)
{
	distance = Max(distance, 1);
	var angle = Angle(x1, y1, x2, y2);
	var xdir = Sin(angle, 10);
	var ydir = -Cos(angle, 10);

	// Need at least two particles: start and end
	var count = Max(2, Distance(x1, y1, x2, y2) / distance);
	var deltax = x2 - x1, deltay = y2 - y1;
	//Log("DrawRPL: %s in %d steps, angle %d xdir %d ydir %d", particle, count, angle, xdir, ydir);
	for (var i = count+1; --i; )
	{
		CreateParticle(particle, x1 + deltax * i / count, y1 + deltay * i / count, xdir, ydir, sizeFifths, color);
	}
}

local Name = "$Name$";
