/**
	Jet Stream
	Fast flowing, narrow air current. 
	
	@author Maikel	
*/


/*-- Control --*/

// Create a jet stream from (sx, sy) to (ex, ey) with a given width. The duration can
// be nil for an infinite jet stream. Range for width is [10, 80], for speed [10, 200].
public func Create(int sx, int sy, int ex, int ey, int duration, int width, int speed)
{
	if (this != JetStream)
		return;
	var stream = [[sx, sy], [ex, ey]];
	AddEffect("IntJetStream", nil, 100, 1, nil, JetStream, stream, duration, width, speed);
	return;
}

// Create a jet stream along the given line with a given width. The duration can
// be nil for an infinite jet stream. Range for width is [10, 80], for speed [10, 200].
public func CreateLine(array line, int duration, int width, int speed)
{
	if (this != JetStream)
		return;
	AddEffect("IntJetStream", nil, 100, 1, nil, JetStream, line, duration, width, speed);
	return;
} 


/*-- Jet Stream --*/

public func FxIntJetStreamStart(object target, effect, int temp, array stream, int duration, int width, int speed)
{
	if (temp)
		return FX_OK;
	effect.stream = stream;
	effect.duration = duration;
	effect.width = width ?? 40;
	effect.width = BoundBy(effect.width, 10, 80);
	effect.speed = speed ?? 40;
	effect.speed = BoundBy(effect.speed, 10, 200);
	effect.particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Random(2, 5)
	};
	return FX_OK;
}

public func FxIntJetStreamTimer(object target, proplist effect, int time)
{
	// Check the duration and remove if needed.
	if (effect.duration != nil && time >= effect.duration)
		return FX_Execute_Kill;
		
	// The speed in the first and the last stages of the stream is slower.
	var speed = effect.speed;
	
	// Loop over the stream coordinates and perform the stream mechanics.
	for (var index = 0; index <= GetLength(effect.stream) - 2; index++)
	{
		var start = effect.stream[index];
		var end = effect.stream[index + 1];
		var sx = start[0];
		var sy = start[1];
		var ex = end[0];
		var ey = end[1];
		var width = effect.width / 2;
		var d = Distance(sx, sy, ex, ey);
		var wind_d = Max(3 * d / 4, d - speed / 2);
		var angle = Angle(sx, sy, ex, ey);
		var xdir = Sin(angle, speed / 2);
		var ydir = -Cos(angle, speed / 2);	
		// Create air particles.
		var particle_count = d * width / 200;
		for (var count = 0; count < particle_count; count++)
		{
			var x = RandomX(-width, width);
			var y = RandomX(0, wind_d);
			var point_angle = Angle(0, 0, x, y);
			var point_d = Distance(x, y);
			x = -Sin(angle + point_angle, point_d);
			y = Cos(angle + point_angle, point_d);	
			CreateParticle("Air", sx + x - xdir / 4, sy + y - ydir / 4, xdir, ydir, PV_Random(14, 26), effect.particles, 1);
		}
		// Move objects in the stream.
		var lines = [];
		for (var count = - 3; count <= 3; count++)
		{
			var x = Cos(angle, count * (width - 2) / 3);
			var y = Sin(angle, count * (width - 2) / 3);
			PushBack(lines, Find_OnLine(sx + x, sy + y, ex + x, ey + y));
		}
		var find_lines = Find_Or(lines[0], lines[1], lines[2], lines[3], lines[4], lines[5], lines[6]);
		for (var obj in FindObjects(Find_Category(C4D_Object | C4D_Vehicle | C4D_Living), Find_Layer(), find_lines))
		{
			if (obj->Stuck())
				continue;
			var factor = 6;
			var dx = factor * xdir - obj->GetXDir(100);
			var dy = factor * ydir - obj->GetYDir(100);
			// Only move the object if it is not already moving faster than the stream.
			if (Sign(xdir) * dx > 0)
				obj->SetXDir(obj->GetXDir(100) + factor * xdir, 100);
			if (Sign(ydir) * dy > 0)
				obj->SetYDir(obj->GetYDir(100) + factor * ydir, 100);
		}
	}
	return FX_OK;
}


/*-- Proplist --*/

local Name = "$Name$";
