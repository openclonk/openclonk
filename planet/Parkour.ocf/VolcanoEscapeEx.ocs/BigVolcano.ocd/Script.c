/*
	BigVolcano
	Author: Sven2

	DESC
*/

static const BigVolcanoBehaviour_Fill = 0,
             BigVolcanoBehaviour_Advance = 1,
             BigVolcanoBehaviour_AdvanceLava = 2,
             BigVolcanoBehaviour_Stop = 3,
             BigVolcanoBehaviour_Underground = DMQ_Sub;
             
static const BigVolcano_XRes = 25; // step size of segments of lava_y array

local lava_y; // array for current vertical lava positions
local lava_y_endpoint; // volcano is done when lava reaches this point
local n_lava_y; // size of lava_y array
local n_branches;
local mat_behaviours; // array of BigVolcanoBehaviour_*, indexed by GetMaterial(x, y)+1: How to behave in materials
local mat_advancespeeds; // array of ints from 0 to 100, indexed by GetMaterial(x, y)+1: How to behave in materials
local speed_multiplier = 1; // number of pixels by which the volcano advances when it does advance

func Activate(int start_y, int end_y)
{
	if (lava_y) return; // already active
	if (!start_y) start_y = LandscapeHeight();
	if (!end_y) end_y = 0;
	lava_y_endpoint = end_y;
	ScheduleCall(this, this.Execute, 10, 99999);
	n_branches = 0;
	mat_behaviours = CreateArray(20);
	mat_advancespeeds = CreateArray(20);
	mat_behaviours[0] = BigVolcanoBehaviour_Fill;
	mat_advancespeeds[0] = 80;
	for (var mat_idx = 0, mat_name; mat_name = MaterialName(mat_idx); ++mat_idx)
	{
		var behaviour, density = GetMaterialVal("Density", "Material", mat_idx), speed;
		if (WildcardMatch(mat_name, "*Lava")) // pass through lava
		{
			behaviour = BigVolcanoBehaviour_AdvanceLava;
			speed = 100;
		}
		else if (WildcardMatch(mat_name, "*Water")) // fill into water to remove it
		{
			behaviour = BigVolcanoBehaviour_Fill;
			speed = 10;
		}
		else if (density <= 0) // fill tunnels
		{
			behaviour = BigVolcanoBehaviour_Fill;
			speed = 80;
		}
		else if (density <= 25) // pass through any other liquids
		{
			behaviour = BigVolcanoBehaviour_Advance;
			speed = 60;
		}
		else if (GetMaterialVal("DigFree", "Material", mat_idx)) // pass through diggable materials
		{
			behaviour = BigVolcanoBehaviour_Advance;
			speed = 50;
		}
		else // stop at rocks + minerals
		{
			behaviour = BigVolcanoBehaviour_Stop;
			speed = 10;
		}
		mat_behaviours[mat_idx + 1] = behaviour;
		mat_advancespeeds[mat_idx + 1] = speed;
	}
	n_lava_y = (LandscapeWidth()-1)/BigVolcano_XRes + 2;
	lava_y = CreateArray(n_lava_y);
	for (var i = 0; i<n_lava_y; ++i) lava_y[i] = start_y;
	return true;
}

func Execute()
{
	//Log("tic");
	// Create branch
	var ls_w = LandscapeWidth();
	if (n_branches<20 && !Random(1))
	{
		var x = Random(ls_w);
		LaunchBranch(x, GetLavaY(x), Random(5)-2, -15-Random(5));
	}
	// Rise lava
	if (true)
	{
		var lavamat = Material("DuroLava");
		var any_nonlava, last_move, this_move;
		for (var i = 0; i<n_lava_y; ++i)
		{
			if (lava_y[i] > lava_y_endpoint)
			{
				any_nonlava = true;
				var x = BoundBy(i*BigVolcano_XRes-BigVolcano_XRes/2, 0, ls_w-BigVolcano_XRes) + Random(BigVolcano_XRes);
				var y = GetLavaY(x);
				var speed = mat_advancespeeds[GetMaterial(x, y-5)+1];
				if (i) if (lava_y[i] > lava_y[i-1]+BigVolcano_XRes*2) speed += 50;
				if (i<n_lava_y-1) if (lava_y[i] > lava_y[i + 1]+BigVolcano_XRes*2) speed += 50;
				this_move = (Random(100) < speed);
				if (this_move)
					lava_y[i] -= speed_multiplier;
				else if (!Random(3))
				{
					if (speed<=10)
					{
						// Blast away solid
						var blast_size = 5 + Random(5);
						BlastFree(x, y-3, blast_size, GetController());
						// gfx
						var particle_speed = blast_size * 3;
						CreateParticle("FireDense", PV_Random(x - 1, x + 1), PV_Random(y - 4, y - 2), PV_Random(-particle_speed, particle_speed), PV_Random(-particle_speed, particle_speed), PV_Random(30, 40), Particles_Fire(), 5);
						if (!Random(5)) SoundAt("Hits::Materials::Rock::RockHit*", x, y-3, 100);
					}
					else if (speed <=50)
					{
						// Crumble away diggable
						ShakeFree(x, y-Random(20),5);
					}
				}
				if (last_move || (this_move && i))
				{
					var x1=(i-1)*BigVolcano_XRes, x2 = i*BigVolcano_XRes;
					var y1 = lava_y[i-1]+speed_multiplier + 1, y2 = lava_y[i]+speed_multiplier + 1;
					var limit = 50;
					while (GetMaterial(x1, y1 + 1) != lavamat && --limit) ++y1;
					limit = 50;
					while (GetMaterial(x1, y2 + 1) != lavamat && --limit) ++y2;
					DrawMaterialQuad("DuroLava-lava_red", x1, lava_y[i-1], x2, lava_y[i], x2, y2, x1, y1, BigVolcanoBehaviour_Underground);
				}
				last_move = this_move;
			}
			
		}
		// Lava risen to the max?
		if (!any_nonlava)
		{
			ClearScheduleCall(this, this.Execute);
			return;
		}
	}
	return true;
}

func GetLavaY(int x)
{
	var i = BoundBy(x/BigVolcano_XRes, 0, n_lava_y-2), ix = x%BigVolcano_XRes;
	return (lava_y[i]*(BigVolcano_XRes-ix) + lava_y[i + 1]*ix) / BigVolcano_XRes;
}


/* Branch effects */
// draws a lava branch that walks ahead of the rising main lava lake
// branch effect variables:
// x, y: Branch tip position
// tip_progress: Progress from trace_x/y[end] towards dir_x/dir_y. From 0 to 100.
// dir_x, dir_y: Advance direction per segment
// len: Length of branch
// trace_x, trace_y: Array of previous positions. Index from oldest to newest. Currenct tip x/y not included.
// trace_len: length of trace_x, trace_y arrays

local dbg_counter;

func LaunchBranch(int x, int y, int dir_x, int dir_y, int parent_level)
{
	var fx = AddEffect("VolcanoBranch", this, 1, 3, this);
	fx.x = x; fx.y = y; fx.dir_x = dir_x; fx.dir_y = dir_y;
	fx.tip_progress = 0;
	fx.trace_x=[x]; fx.trace_y=[y]; fx.trace_len = fx.len = 1;
	fx.counter = ++dbg_counter;
	fx.level = parent_level + 1;
	//Log("LaunchBranch %d from %d/%d towards %d/%d", fx.counter, x, y, dir_x, dir_y);
	++n_branches;
	return fx;
}

func FxVolcanoBranchTimer(object q, fx, int time)
{
	// Progress tip
	var next_tip_progress = fx.tip_progress + 10;
	var last_segment = fx.trace_len-1;
	var sx = fx.trace_x[last_segment], sy = fx.trace_y[last_segment];
	var nx = sx + next_tip_progress*(fx.dir_x)/100, ny = sy + next_tip_progress*(fx.dir_y)/100;
	//Log("Go %d from %d/%d towards %d/%d", fx.counter, sx, sy, nx-sx, ny-sy);
	if (nx == sx && ny == sy) { fx.tip_progress = next_tip_progress; return FX_OK; }
	if (ny<0 || fx.len>20) return FX_Execute_Kill; // End here?
	var behaviour = mat_behaviours[GetMaterial(nx, ny)+1];
	var i, dx, dy;
	if (behaviour == BigVolcanoBehaviour_Fill)
	{
		CastPXS("DuroLava", 4 + Cos(fx.fill_time*40, 2), 30 + Cos(fx.fill_time*40, 25), nx, ny-1, 0, 50);
		if (!(fx.fill_time%20) && !GBackSemiSolid(nx, ny-6)) SoundAt("BigVolcano::BigVolcanoBubble*", nx, ny, 10);
		if (fx.fill_time++ > 31) return FX_Execute_Kill; // Done?
	}
	else if (behaviour != BigVolcanoBehaviour_Stop)
	{
		fx.fill_time = 0;
		//if (behaviour == BigVolcanoBehaviour_AdvanceLava && !Random(5)) return FX_Execute_Kill; // prevent too many overlapping branches
		DrawVerticalBranch(fx.x, fx.y, nx, ny, 1);
		fx.x = nx; fx.y = ny;
		// Extend width of old segments
		for (i = fx.tip_progress*last_segment/100; i<next_tip_progress*last_segment/100; ++i)
		{
			var old_half_wdt = (last_segment-i + 1)/3 + 1;
			var new_half_wdt = (last_segment-i + 2)/3 + 1;
			if (new_half_wdt != old_half_wdt)
				ExtendVerticalBranch(fx.trace_x[i],fx.trace_y[i], fx.trace_x[i + 1],fx.trace_y[i + 1], old_half_wdt, new_half_wdt);
		}
		fx.tip_progress = next_tip_progress;
	}
	else
	{
		// Do not bounce too much
		if (++fx.bounces>6) return FX_Execute_Kill;
		// Try to find a new spot to advance to
		if (fx.tip_progress)
		{
			// Start searching from last pos we went to
			sx += fx.tip_progress*(fx.dir_x)/100;
			sy += fx.tip_progress*(fx.dir_y)/100;
		}
		dx = Sign(fx.dir_x);
		if (!dx) dx = Random(2)*2-1;
		dx *= 5;
		var n_valid_branches = 0;
		for (i = 0; i<2; ++i)
		{
			for (dy=-10; dy<10; dy += 2)
			{
				var b = mat_behaviours[GetMaterial(sx + dx, sy + dy)+1];
				if (b != BigVolcanoBehaviour_Stop && b != BigVolcanoBehaviour_AdvanceLava)
				{
					if (!n_valid_branches++)
					{
						// First valid branch: Take over this one
						nx = sx + dx;
						ny = sy + dy;
						// Do not branch too deeply
						if (fx.level>1) ++i;
					}
					else
					{
						// Second possible position: Branch off this one
						LaunchBranch(sx, sy, dx, dy, fx.level++);
					}
					break;
				}
			}
			dx*=-1;
		}
		// Caught in a corner?
		if (!n_valid_branches) return FX_Execute_Kill;
		// Spot found. Add segment if we already progressed in the last segment
		/*if (fx.tip_progress)
		{
			fx.trace_x[fx.trace_len] = sx; fx.trace_y[fx.trace_len] = sy;
			++fx.trace_len; ++fx.len;
			fx.tip_progress = 0;
		}*/
		// Move to new spot
		fx.dir_x = nx-fx.trace_x[last_segment];
		fx.dir_y = ny-fx.trace_y[last_segment];
	}
	// End pos reached?
	if (fx.tip_progress == 100)
	{
		// Add new tip
		fx.trace_x[fx.trace_len] = nx; fx.trace_y[fx.trace_len] = ny;
		++fx.trace_len; ++fx.len;
		// Launch next branch
		fx.tip_progress = 0;
		fx.dir_x = BoundBy(fx.dir_x + Random(5)-2,-8, 8);
		fx.dir_y = BoundBy(fx.dir_y + Random(5)-2,-15,-20);
		//Log("%v %v",fx.trace_y, fx.trace_len);
	}
	// Remove segments that fell below the main lake (max one at a time)
	if (fx.trace_y[1] >= GetLavaY(fx.trace_x[1]))
	{
		fx.trace_x = fx.trace_x[1:fx.trace_len];
		fx.trace_y = fx.trace_y[1:fx.trace_len];
		--fx.trace_len;
	}
	return FX_OK;
}

func FxVolcanoBranchStop(object q, fx, int reason, bool temp)
{
	if (!temp)
	{
		--n_branches;
		//Log("DieBranch %d", fx.counter);
	}
	return FX_OK;
}

func DrawVerticalBranch(int x1, int y1, int x2, int y2, int half_wdt)
{
	// Draw branch from x1/y1 to x2/y2 with width half_wdt*2
	//Log("BRANCH %d,%d,%d,%d, %d",x1, y1, x2, y2, half_wdt);
	if (Abs(x2-x1)>Abs(y2-y1))
		return DrawMaterialQuad("DuroLava-lava_red",x1, y1-half_wdt, x1, y1 + half_wdt, x2, y2 + half_wdt, x2, y2-half_wdt, BigVolcanoBehaviour_Underground);
	else
		return DrawMaterialQuad("DuroLava-lava_red",x1-half_wdt, y1, x1 + half_wdt, y1, x2 + half_wdt, y2, x2-half_wdt, y2, BigVolcanoBehaviour_Underground);
}

func ExtendVerticalBranch(int x1, int y1, int x2, int y2, int from_half_wdt, int to_half_wdt)
{
	//Log("EXTBRANCH %d,%d,%d,%d, %d->%d",x1, y1, x2, y2, from_half_wdt, to_half_wdt);
	// Extend width of drawn branch from x1/y1 to x2/y2 from width from_half_wdt*2 to to_half_wdt*2
	// Effectively draws extra branch left and right, unless the branch had been pretty thin before
	if (from_half_wdt <= 2) return DrawVerticalBranch(x1, y1, x2, y2, to_half_wdt);
	if (Abs(x2-x1)>Abs(y2-y1))
	{
		DrawMaterialQuad("DuroLava-lava_red",x1, y1-to_half_wdt, x1, y1-from_half_wdt, x2, y2-from_half_wdt, x2, y2-to_half_wdt, BigVolcanoBehaviour_Underground);
		DrawMaterialQuad("DuroLava-lava_red",x1, y1 + to_half_wdt, x1, y1 + from_half_wdt, x2, y2 + from_half_wdt, x2, y2 + to_half_wdt, BigVolcanoBehaviour_Underground);
	}
	else
	{
		DrawMaterialQuad("DuroLava-lava_red",x1-to_half_wdt, y1, x1-from_half_wdt, y1, x2-from_half_wdt, y2, x2-to_half_wdt, y2, BigVolcanoBehaviour_Underground);
		DrawMaterialQuad("DuroLava-lava_red",x1 + to_half_wdt, y1, x1 + from_half_wdt, y1, x2 + from_half_wdt, y2, x2 + to_half_wdt, y2, BigVolcanoBehaviour_Underground);
	}
	return true;
}

// Get highest point of lava surface
func GetLavaPeak()
{
	var y; for (var ly in lava_y) y += ly;
	return y / n_lava_y;
}

// Update speed of rising lava
func SetSpeedMultiplier(int new_multiplier)
{
	speed_multiplier = new_multiplier;
	return true;
}

local Name = "BigVolcano";
local Description = "Volcano helper object";
