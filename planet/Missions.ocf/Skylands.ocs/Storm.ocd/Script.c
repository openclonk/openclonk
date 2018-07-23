/**
	Storm
*/

local Name = "$Name$";
local Description = "$Description$";

local storm_debug = false;
local streams, n_streams;
local execution_index = 0, n_exec_per_loop = 1;
local max_stream_len = 1000; // maximum number of intervals to be calculated for each stream of this storm
local exec_interval = 10; // exec every n frames
local find_mask; // Find_-condition for objects to be flung by wind
local strength; // storm strength
local stream_density = 20;
local stream_border_dist = 20;
local map, map_res1, map_res2, map_size1, map_size2, map_off1, map_off2;
//local debug_map;
local storm_particles; // storm particle definition, for pretty visuals

local StormStream;

static g_storm;

func Initialize()
{
	// singleton
	g_storm = this;
	SetPosition();
	// defaults
	storm_particles = 
	{
		Size = 1,
		Stretch = PV_Speed(PV_Linear(4000, 0), 0),
		Alpha = PV_KeyFrames(0, 0, 0, 100, 255, 1000, 255),
		Rotation = PV_Direction(),
		CollisionVertex = 1000,
		OnCollision = PC_Die()
	};
	StormStream = {
		max_segment_stretch = 100, // maximum number of pixels per segment that can be deviated from dir in either direction
		max_segment_stretch_want = 5, // maximum movement back into original position that is preferred (i.e.: speed at which gaps behind sky islands close)
		search_steps = 10,        // steps in pixels in which to search for holes to blow through
		search_steps_mult = 200,  // multiplyer, in percent, by which search steps get larger with each iteration
	};
	find_mask = Find_And(Find_Category(C4D_Vehicle | C4D_Living | C4D_Object), Find_Not(Find_Func("IsEnvironment")));
	SetStorm(20,0, 2000);
}

func Clear()
{
	// timer
	RemoveEffect("IntExecute", this);
	// helper objects
	for (var i=0; i<n_streams; ++i)
		if (streams[i].debug) streams[i].debug->RemoveObject();
	streams = nil;
	n_streams = 0;
	map = nil;
}

private func InitMap()
{
	// init empty wind map according to parameters
	// determine coordinate borders
	var wdt=LandscapeWidth()-1, hgt = LandscapeHeight()-1;
	var w1_min = Min(Min(MapXYToW1(0,0),MapXYToW1(0,hgt)),Min(MapXYToW1(wdt,0),MapXYToW1(wdt,hgt)));
	var w1_max = Max(Max(MapXYToW1(0,0),MapXYToW1(0,hgt)),Max(MapXYToW1(wdt,0),MapXYToW1(wdt,hgt)));
	var w2_min = Min(Min(MapXYToW2(0,0),MapXYToW2(0,hgt)),Min(MapXYToW2(wdt,0),MapXYToW2(wdt,hgt)));
	var w2_max = Max(Max(MapXYToW2(0,0),MapXYToW2(0,hgt)),Max(MapXYToW2(wdt,0),MapXYToW2(wdt,hgt)));
	// implement to cover complete border range
	map_res1 = StormStream.dir_len;
	map_res2 = stream_density;
	map_off1 = w1_min - map_res1/2;
	map_off2 = w2_min - map_res2/2;
	map_size1 = (w1_max - map_off1) / map_res1 + 1;
	map_size2 = (w2_max - map_off2) / map_res2 + 1;
	// allocate map
	map = CreateArray(map_size1 * map_size2);
	//debug_map = CreateArray(map_size1 * map_size2);
	return true;
}

private func MapXYToIdx(int x, int y)
{
	if (x<0 || x>=LandscapeWidth() || y<0 || y>=LandscapeHeight()) return -1;
	return (MapXYToW1(x, y)-map_off1)/map_res1 + ((MapXYToW2(x, y)-map_off2)/map_res2) * map_size1;
}

private func MapXYToW1(int x, int y)
{
	// coordinate transform from x/y space to in-wind-direction coordinate
	return (x*StormStream.dir_x + y*StormStream.dir_y) / StormStream.dir_len;
}

private func MapXYToW2(int x, int y)
{
	// coordinate transform from x/y space to perpendicular-to-wind-direction coordinate
	return (x*StormStream.dir_y - y*StormStream.dir_x) / StormStream.dir_len;
}

// dir_*: vector pointing in storm direction. vector length is equal to segment intervals.
// strength: how much to fling objects
func SetStorm(int dir_x, int dir_y, int astrength)
{
	// clear old
	Clear();
	// add new
	var d = Distance(dir_x, dir_y);
	if (!astrength || !d) return;
	strength = astrength;
	StormStream.dir_x = dir_x;
	StormStream.dir_y = dir_y;
	StormStream.dir_len = d;
	// init map
	InitMap();
	// create streams
	n_streams = ((Abs(LandscapeWidth()*dir_y) + Abs(LandscapeHeight()*dir_x))/d - 2*stream_border_dist) / stream_density;
	streams = CreateArray(n_streams);
	var i_stream = 0, x0, y0, sgn_x=1, sgn_y=1;
	var wdt = LandscapeWidth()-1, hgt = LandscapeHeight()-1;
	if (dir_y<0) { y0 = hgt; sgn_y = -1; }
	if (dir_x<0) { x0 = wdt; sgn_x = -1; }
	//Log("creating %d streams", n_streams);
	for (var i = 0; i<n_streams; ++i)
	{
		var pos = stream_border_dist + i * stream_density;
		var x0,y0;
		if (dir_y)
		{
			var dpos=Abs(pos*d/dir_y);
			if (dpos<=wdt)
			{
				// streams from horizontal border of landscape
				var s = CreateStream(wdt - x0 - dpos * sgn_x, y0);
				if (s) streams[i_stream++] = s;
				continue;
			}
			pos -= Abs(wdt*dir_y/d);
		}
		// streams from vertical border of landscape
		pos=Abs(pos*d/dir_x);
		//Log("@%d", pos);
		var s = CreateStream(x0, y0 + pos * sgn_y);
		if (s) streams[i_stream++] = s;
	}
	n_streams=i_stream;
	//Log("%d total", n_streams);
	streams = streams[0:n_streams];
	// create timer for stream execution
	n_exec_per_loop = n_streams;
	AddEffect("IntExecute", this, 1, exec_interval, this);
}

func FxIntExecuteTimer()
{
	for (var i_exec=0; i_exec<n_exec_per_loop; ++i_exec)
	{
		// exec current stream
		//Log("exec %d", execution_index);
		ExecuteStream(streams[execution_index]);
		// next stream (++execution_index %= n_streams may or may not work)
		++execution_index;
		execution_index %= n_streams;
	}
}

private func CreateStream(int x0, int y0)
{
	//Log("stream at %d/%d", x0,y0);
	// Not in earth
	if (GBackSolid(x0, y0)) return nil;
	// Determine length
	var len_x = max_stream_len, len_y = max_stream_len;
	if (StormStream.dir_x < 0)
		len_x = -(x0-1) / StormStream.dir_x + 1;
	else if (StormStream.dir_x)
		len_x = (LandscapeWidth()-x0-1) / StormStream.dir_x + 1;
	if (StormStream.dir_y < 0)
		len_y = -(y0-1) / StormStream.dir_y + 1;
	else if (StormStream.dir_y)
		len_y = (LandscapeHeight()-y0-1) / StormStream.dir_y + 1;
	var len = Min(len_x, len_y);
	// Initialize as laminar stream along desired path
	var x = CreateArray(len), y = CreateArray(len);
	var is_blocked = CreateArray(len);
	for (var i=0; i<len; ++i)
	{
		x[i] = x0+i*StormStream.dir_x;
		y[i] = y0+i*StormStream.dir_y;
		is_blocked[i] = (i>0); // initial stream is blocked and will be unblocked on first execution
	}
	// Create stream data struct
	var stream_debug;
	if (storm_debug) stream_debug = CreateObjectAbove(Storm_DebugDisplay,0,0,NO_OWNER);
	var new_stream = {
		Prototype = StormStream,
		"x0" = x0, "y0" = y0, // "a"=a because Guenther said so
		"len" = len,
		"x" = x, "y" = y,
		"is_blocked" = is_blocked,
		"debug" = stream_debug,
	};
	return new_stream;
}

private func ExecuteStream(proplist s)
{
	//Log("ExecStream %v", s);
	// Execute stream against wind direction, so changes dont propagate immediately but only segment-by-segment
	var do_particles = !Random(3);
	for (var i_segment = s.len-2; i_segment>=0; --i_segment)
	{
		// propagate block
		if (s.is_blocked[i_segment])
		{
			//Log("segment %d", i_segment);
			if (!s.is_blocked[i_segment+1]) StreamBlockVertex(s, i_segment+1);
			if (storm_debug)
				CreateParticle("SphereSpark", s.x[i_segment], s.y[i_segment], 0, 0, 36, {Size =  12});
			continue;
		}
		// current segment base point
		var x = s.x[i_segment], y = s.y[i_segment];
		var tx = s.x[i_segment+1], ty = s.y[i_segment+1];
		// determine direction of current segment
		var vx = tx - x;
		var vy = ty - y;

		// determine where we want to go
		var want_vx = s.x0+(i_segment+1)*s.dir_x - x;
		var want_vy = s.y0+(i_segment+1)*s.dir_y - y;

		var want_stretch = (s.dir_x*want_vy-s.dir_y*want_vx) / s.dir_len;
		//if (i_segment==8) Log("%v", want_stretch);
		// can turn?
		if (Abs(want_stretch) > s.max_segment_stretch_want)
		{
			// We cannot go all the way...turn as much as we can
			var stretch_dir = Abs(want_stretch)/want_stretch; // sign of direction
			want_stretch = s.max_segment_stretch_want * stretch_dir;
		}
		// check from want_v alternating in both directions for a free path
		var search_range = (Abs(want_stretch) + s.max_segment_stretch);
		var search_off, has_found = false;
		for (var search_offset = 0; search_offset <= search_range; search_offset = search_offset * s.search_steps_mult/100 + s.search_steps)
		{
			// search up
			search_off = want_stretch - search_offset;
			if (search_off >= -s.max_segment_stretch)
				if (StreamCheckPathFree(s,x,y,search_off)) { has_found=true; break; }
			if (!search_offset) continue; // don't check direction -0 and +0 twice
			// search down
			search_off = want_stretch + search_offset;
			if (search_off <= s.max_segment_stretch)
				if (StreamCheckPathFree(s,x,y,search_off)) { has_found=true; break; }
		}
		// did we find a path?
		if (has_found)
		{
			// path found
			if (s.is_blocked[i_segment+1]) StreamUnblockVertex(s, i_segment+1);
			var new_tx = x + s.dir_x - search_off * s.dir_y / s.dir_len;
			var new_ty = y + s.dir_y + search_off * s.dir_x / s.dir_len;
			if (new_tx != tx || new_ty != ty) StreamMoveVertex(s, i_segment+1, tx, ty, new_tx, new_ty);
			tx = new_tx; ty = new_ty;
			// determine storm density at this position
			var map_idx = MapXYToIdx(tx, ty), local_strength;
			if (map_idx>=0) local_strength = map[map_idx]; else local_strength=1;
			// fling objects along path
			vx = vx * strength / s.dir_len;
			vy = vy * strength / s.dir_len; // - 20;
			var fling_objs = FindObjects(find_mask, Find_OnLine(x,y,new_tx,new_ty)), obj;
			for (obj in fling_objs) if (obj->GetID()==ElevatorCase) { fling_objs = []; break; } // do not fling stuff in elevator case
			for (obj in fling_objs)
			{
				// check if object can be pushed
				if (obj->Stuck()) continue;
				if (!PathFree(x,y,obj->GetX(),obj->GetY())) continue; // don't push through solid
				// determine push strength. subsequent pushes of overlapping storm pathes stack diminishingly
				var push_strength = strength/20;
				var pushfx = GetEffect("StormPush",obj);
				if (pushfx)
				{
					push_strength /= pushfx.count++;
					if (!push_strength) continue;
				}
				else
				{
					pushfx=AddEffect("StormPush", obj, 1, 5, this);
					if (pushfx) pushfx.count = 1;
				}
				// now push
				var ovx = obj->GetXDir(100);
				var ovy = obj->GetYDir(100);
				// check max speed
				if (Distance(ovx,ovy,vx,vy) > push_strength*6)
				{
					if (Distance(ovx,ovy) > 500)
						obj->Fling(BoundBy(vx-ovx,-push_strength,push_strength),BoundBy(vy-ovy,-push_strength,push_strength),100,true);
					else
					{
						obj->SetXDir(ovx+BoundBy(vx-ovx,-push_strength,push_strength),100);
						obj->SetYDir(ovy+BoundBy(vy-ovy,-push_strength,push_strength),100);
					}
				}
			}
			// Gfx
			if (do_particles && map_idx>=0)
			{
				if (local_strength >= 1)
				{
					// Two streams coincide here. Gfx!
					vx = tx-x; vy = ty-y;
					var v = Distance(vx,vy);
					vx = vx * s.dir_len / v;
					vy = vy * s.dir_len / v / 2;
					CreateParticle("Dust", PV_Random(x - 10, x + 10), PV_Random(y - 10, y + 10), PV_Random(vx * 80 / 100, vx * 120 / 100), PV_Random(vy, vy * 140 / 100), PV_Random(20, 40), storm_particles,local_strength); 
				}
			}
		}
		else
		{
			// path not found. segment blocked.
			if (!s.is_blocked[i_segment+1]) StreamBlockVertex(s, i_segment+1);
		}
	}
	if (s.debug) s.debug->ShowData(s.x, s.y);
}

private func StreamCheckPathFree(proplist s, int x, int y, int offset)
{
	// determine target coordinates
	var tx = x + s.dir_x - offset * s.dir_y / s.dir_len;
	var ty = y + s.dir_y + offset * s.dir_x / s.dir_len;
	// check path
	return PathFree(x,y,tx,ty);
}

private func StreamMoveVertex(proplist s, int i, int old_x, int old_y, int new_x, int new_y)
{
	//Log("moving %d/%d to %d/%d", old_x, old_y, new_x, new_y);
	// adjust vertex
	s.x[i] = new_x; s.y[i] = new_y;
	// adjust map
	var idx = MapXYToIdx(old_x, old_y);
	if (idx>=0) --map[idx];
	//DebugMapAdd(idx, Format("m%d.%d", s.y0, i));
	idx = MapXYToIdx(new_x, new_y);
	if (idx>=0) ++map[idx];
	//DebugMapAdd(idx, Format("M%d.%d", s.y0, i));
	return true;
}

private func StreamBlockVertex(proplist s, int i)
{
	//Log("blocking at %d/%d", s.x[i], s.y[i]);
	// adjust vertex
	s.is_blocked[i] = true;
	// adjust map
	var idx = MapXYToIdx(s.x[i], s.y[i]);
	if (idx>=0) --map[idx];
	//DebugMapAdd(idx, Format("X%d.%d", s.y0, i));
	return true;
}

private func StreamUnblockVertex(proplist s, int i)
{
	//Log("unblocking at %d/%d", s.x[i], s.y[i]);
	// adjust vertex
	s.is_blocked[i] = false;
	// adjust map
	var idx = MapXYToIdx(s.x[i], s.y[i]);
	if (idx>=0) ++map[idx];
	//DebugMapAdd(idx, Format("O%d.%d", s.y0, i));
	return true;
}

private func DumpStreamInfo(int i)
{
	var s = streams[i], q=[], idcs = [];
	for (var j=0; j<s.len; ++j)
	{
		var idx = MapXYToIdx(s.x[j], s.y[j]);
		if (idx<0) q[j] = []; else q[j] = map[idx];
		idcs[j] = idx;
	}
	Log("%v", idcs);
	return q;
}

/*private func DebugMapAdd(int i, string s)
{
	if (i<0) return;
	if (debug_map[i]) debug_map[i] = Format("%s %s", debug_map[i], s); else debug_map[i] = s;
	return true;
}*/

func GetWindEx(int x, int y)
{
	if (!map) return 0; // not initialized or zero storm
	var idx = MapXYToIdx(x, y);
	if (idx<0) return 0; // outside landscape
	// check storm density map
	return -BoundBy(map[idx]*strength/10, 0,100);
}

global func GetWind(int x, int y)
{
	if (g_storm) return g_storm->GetWindEx(x+GetX(),y+GetY());
	return _inherited(x,y,...);
}
