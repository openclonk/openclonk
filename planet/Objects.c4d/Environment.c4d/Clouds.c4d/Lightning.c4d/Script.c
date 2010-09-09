// Lightning bolt

local xDir, yDir;
local xDev, yDev;
local gamma;
local size;

/// Creates a lightning bolt at the specified location.
/// \par x X coordinate. Always global.
/// \par y Y coordinate. Always global.
/// \par xdir Average horizontal speed of the bolt.
/// \par xdev Maximum deviation from the average horizontal speed.
/// \par ydir Average vertical speed of the bolt.
/// \par ydev Maximum deviation from the average vertical speed.
/// \par doGamma If \c true, the lightning bolt will flash the screen.
/// \returns \c true if the lightning could be launched, \c false otherwise.
global func LaunchLightning(int x, int y, int xdir, int xdev, int ydir, int ydev, bool doGamma)
{
	return LaunchEffect(Lightning, x, y, xdir, xdev, ydir, ydev, doGamma == nil || doGamma);
}

public func Activate(int x, int y, int xdir, int xdev, int ydir, int ydev, bool doGamma, int startSize)
{
	xDir = xdir; yDir = ydir;
	xDev = xdev; yDev = ydev;
	gamma = doGamma;
	size = startSize || 10;
	AddVertex(x-GetX(), y-GetY());
	//Log("Lightning %d: Launching at %d/%d (offset %d/%d)", ObjectNumber(), GetX(), GetY(), GetVertex(0,0), GetVertex(0,1));
	AddEffect("LightningMove", this, 1, 1, this);
	return true;
}

protected func FxLightningMoveTimer()
{
	var vertices = GetVertexNum();
	var oldx = GetVertex(vertices - 1, 0);
	var oldy = GetVertex(vertices - 1, 1);
	var newx = oldx + xDir + xDev - Random(2 * xDev);
	var newy = oldy + yDir + yDev - Random(2 * yDev);
	var pathCheckX = oldx+GetX(), pathCheckY = oldy+GetY();
	//Log("Lightning %d: Moving from %d/%d to %d/%d", ObjectNumber(), pathCheckX, pathCheckY, newx+GetX(), newy+GetY());
	var strike_solid = PathFree2(pathCheckX, pathCheckY, newx+GetX(), newy+GetY());
	if (strike_solid)
	{
		newx = strike_solid[0] - GetX();
		newy = strike_solid[1] - GetY();
		//Log("Lightning %d: Move blocked, moving to %d/%d instead", ObjectNumber(), newx+GetX(), newy+GetY());
	}
	AddVertex(newx, newy);
	DrawRotatedParticleLine("LightningBolt", oldx, oldy, newx, newy, size/5, size*2, 0xa0f0f0f0);
	// Strike objects on the line
	for (var obj in FindObjects(Find_OnLine(oldx, oldy, newx, newy)))
	{
		if (!obj->~LightningStrike(size))
			Punch(obj, size);
	}
	if (strike_solid)
	{
		RemoveObject();
	}
	else if (size > 2 && !Random(20/size))
	{
		// Branch
		LaunchEffect(Lightning, newx+GetX(), newy+GetY(), xDir, xDev, yDir, yDev, false, size - 2);
	}
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
		DrawRotatedParticleLine("LightningBolt", oldx, oldy, newx, newy, size/5, size*2, 0xa0f0f0f0);
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
