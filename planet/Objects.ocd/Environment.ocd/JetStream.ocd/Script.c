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
	var host;
	if (this != JetStream) host = this;
	var stream = [[sx, sy], [ex, ey]];
	return AddEffect("IntJetStream", host, 100, 1, nil, JetStream, stream, duration, width, speed);
}

// Create a jet stream along the given line with a given width. The duration can
// be nil for an infinite jet stream. Range for width is [10, 80], for speed [10, 200].
public func CreateLine(array line, int duration, int width, int speed)
{
	var host;
	if (this != JetStream) host = this;
	return AddEffect("IntJetStream", host, 100, 1, nil, JetStream, line, duration, width, speed);
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
	// Editor manipulation 
	if (target)
	{
		//effect.Name = target.Name; - unfortunately this kills the effect
		effect.Description = target.Description;
		effect.target_offset = stream[1];
		effect.EditorProps = {
			speed = { Name="$Speed$", EditorHelp="$SpeedHelp$", Type="int", Min = 1, Max = 200 },
			width = { Name="$Width$", EditorHelp="$WidthHelp$", Type="int", Min = 5, Max = 80 },
			target_offset = { Name="$TargetOffset$", EditorHelp="$TargetOffsetHelp$", Type="point", Relative = true, Color = 0x008fff, Set="SetTargetOffset" },
		};
		effect.SetTargetOffset = JetStream.SetFxTargetOffset;
	}
	return FX_OK;
}

public func SetFxTargetOffset(new_offset)
{
	// Called in effect context
	this.target_offset = new_offset;
	this.stream[1] = new_offset;
	return true;
}

public func FxIntJetStreamTimer(object target, proplist effect, int time)
{
	// Check the duration and remove if needed.
	if (effect.duration != nil && time >= effect.duration)
		return FX_Execute_Kill;
		
	// Global or object context
	var context = target ?? Global;
		
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
			context->CreateParticle("Air", sx + x - xdir / 4, sy + y - ydir / 4, xdir, ydir, PV_Random(14, 26), effect.particles, 1);
		}
		// Move objects in the stream. TODO: This is very inefficient and misses objects for wide streams. We should just give Find_OnLine a width parameter.
		var lines = [];
		for (var count = - 3; count <= 3; count++)
		{
			var x = Cos(angle, count * (width - 2) / 3);
			var y = Sin(angle, count * (width - 2) / 3);
			PushBack(lines, context->Find_OnLine(sx + x, sy + y, ex + x, ey + y));
		}
		var find_lines = Find_Or(lines[0], lines[1], lines[2], lines[3], lines[4], lines[5], lines[6]);
		for (var obj in context->FindObjects(Find_Category(C4D_Object | C4D_Vehicle | C4D_Living), Find_Layer(), find_lines))
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


/* Scenario saving */

public func FxIntJetStreamSaveScen(object obj, proplist fx, proplist props)
{
	// Save stream effect
	props->AddCall("JetStream", obj ?? JetStream, "Create", fx.stream[0][0], fx.stream[0][1], fx.stream[1][0], fx.stream[1][1], fx.duration, fx.width, fx.speed);
	return true;
}

public func SaveScenarioObject(props)
{
	// Save this object only as a host for the jet stream effect
	if (!_inherited(props, ...)) return false;
	if (!GetEffect("IntJetStream", this)) return false;
	return true;
}


/* Editor-created jet streams */
// Regular streams are just global effects, which doesn't work so well in the editor
// So allow creation of streams bound to local objects

public func EditorInitialize()
{
	var ang = Random(360);
	var tx = BoundBy(GetX() + Cos(ang, 70), 10, LandscapeWidth()-10) - GetX();
	var ty = BoundBy(GetY() + Sin(ang, 70), 10, LandscapeHeight()-10) - GetY();
	Create(0, 0, tx, ty);
}

/*-- Proplist --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;