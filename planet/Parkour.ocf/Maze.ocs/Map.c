/*-- 
	Dynamic maze
	
	Author: Sven2
--*/

#include Library_Map

static g_caves;

local caves, n_caves, start_cave, end_cave;

func FindCaves(int n)
{ //n=6;
	var mask = CreateLayer();
	mask->Draw("Rock");
	caves = [];
	var min_cave_dist = 12, border = 5;
	while (n--)
	{
		var cave = {};
		if (!mask->FindPosition(cave, "Rock", [border,border,this.Wdt-border*2,this.Hgt-border*2])) continue;
		mask->Draw("Tunnel", {Algo=MAPALGO_Ellipsis, X=cave.X, Y=cave.Y, Wdt=min_cave_dist, Hgt=min_cave_dist});
		cave.links = [];
		cave.dirs = 0;
		cave.rand = Random(65536);
		cave.depth = -1;
		caves[n_caves++] = cave;
	}
/*	caves[0].X = 10; caves[0].Y = 10;
	caves[1].X = 30; caves[1].Y = 10;
	caves[2].X = 10; caves[2].Y = 30;
	caves[3].X = 30; caves[3].Y = 70;
	caves[4].X = 50; caves[4].Y = 10;
	caves[5].X = 50; caves[5].Y = 30;*/
	return n_caves;
}

func GetCaveLinkDir(c1, c2)
{
	// Returns if c2 is left (1), right (2), atop (4) or below (8) c1. Returns only one direction.
	var dx=c2.X-c1.X, dy=c2.Y-c1.Y, adx=Abs(dx), ady=Abs(dy);
	//Log("%d,%d  to   %d,%d  dx=%d  dy=%d", c1.X, c1.Y, c2.X, c2.Y, dx, dy);
	return (dx<-ady) | (dx>ady)<<1 | (dy<=-adx)<<2 | (dy>=adx)<<3;
}

func IsLineOverlap(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
	// Check if line from x1,y1 to x2,y2 crosses the line from x3,y3 to x4,y4
	var d1x=x2-x1, d1y=y2-y1, d2x=x4-x3, d2y=y4-y3, d3x=x3-x1, d3y=y3-y1;
	var a = d1y*d3x-d1x*d3y;
	var b = d2y*d3x-d2x*d3y;
	var c = d2y*d1x-d2x*d1y;
	if (!c) return !a && Inside(x3, x1,x2) && Inside(y3, y1,y2); // lines are parallel
	return a*c>=0 && !(a*a/(c*c+1)) && b*c>=0 && !(b*b/(c*c+1));
}

func FindCaveConnections()
{
	var i, j, cave, cave2, caves2, dir, dir2, n, check_link, all_links = [];
	// Connect all caves to neighbours
	for (i=0; i<n_caves; ++i)
	{
		cave = caves[i];
		caves2 = caves[i+1:n_caves];
		for (cave2 in caves2) cave2.d = Distance(cave.X, cave.Y, cave2.X, cave2.Y);
		SortArrayByProperty(caves2, "d");
		for (cave2 in caves2)
		{
			// Make sure there's a max of one connection per direction (up/left/right/down)
			dir = GetCaveLinkDir(cave, cave2);
			//Log("Cave %d to %d direction %d.", GetIndexOf(caves, cave), GetIndexOf(caves, cave2), dir);
			if (cave.dirs & dir) continue;
			dir2 = dir >>  1& 5|10 &dir  << 1;
			//Log("dir %d opposite to %d.", dir2, dir);
			if (cave2.dirs & dir2) continue;
			// Make sure the connectors don't cross each other
			// Note that connectors may still overlap the edges of caves, effectively connecting them
			// But since nothing really "breaks" on this occasion, just stick with the simple check for now.
			var has_overlap = false;
			for (check_link in all_links)
				if (check_link[0] !== cave && check_link[1] !== cave)
					if (check_link[0] !== cave2 && check_link[1] !== cave2)
						if (IsLineOverlap(cave.X, cave.Y, cave2.X, cave2.Y, check_link[0].X, check_link[0].Y, check_link[1].X, check_link[1].Y))
							{ has_overlap=true; break; }
			if (has_overlap) continue;
			// Connect these caves
			cave.links[GetLength(cave.links)] = cave2;
			cave2.links[GetLength(cave2.links)] = cave;
			cave.dirs |= dir;
			cave2.dirs |= dir2;
			// Register and count connections
			all_links[n++] = [cave, cave2];
			// All directions connected?
			if (cave.dirs == 0xf) break;
		}
	}
	return n;
}

func FindStart()
{
	SortArrayByProperty(caves, "X");
	start_cave = caves[0];
	return start_cave;
}

func MakeMaze()
{
	// Make maze by removing unnecesseriy links
	start_cave.path = [];
	var open = [start_cave], n_open = 1;
	while (n_open)
	{
		var i_cave = n_open-1-Random(1+Random(n_open)); // Prefer depth-first generation so stray paths are deeper
		var cave = open[i_cave];
		open[i_cave] = open[--n_open]; // SetLength(open, n_open) not nessessery because length is stored in n_open
		var path_length = GetLength(cave.path);
		var path_to_cave = cave.path[:];
		path_to_cave[path_length] = cave;
		cave.depth = path_length;
		for (var cave2 in cave.links[:]) // force a copy because cave.links is modified in the loop
		{
			if (path_length && cave2 === path_to_cave[path_length-1]) continue;
			// Only first path survives
			if (cave2.path)
			{
				// Remove circular path
				RemoveCaveLinks(cave, cave2);
			}
			else
			{
				// Remember path to this cave
				cave2.path = path_to_cave;
				open[n_open++] = cave2;
			}
		}
	}
	// Sort from start to goal
	SortArrayByProperty(caves, "depth");
	// Close one connection 2/3rds of the way to the goal
	var main_path = caves[n_caves-1].path;
	var main_path_length = GetLength(main_path);
	if (main_path_length > 5) RemoveCaveLinks(main_path[main_path_length*2/3], main_path[main_path_length*2/3+1]);
	// Kill unreachable caves
	var i;
	for (i=0; i<n_caves; ++i) if (caves[i].depth>=0) break;
	caves = caves[i:n_caves];
	n_caves -= i;
	end_cave = caves[n_caves-1];
	return true;
}

func RemoveCaveLinks(c1, c2)
{
	var i = GetIndexOf(c1.links, c2), n = GetLength(c1.links) - 1;
	if (i>=0)
	{
		c1.links[i] = c1.links[n];
		SetLength(c1.links, n);
		c1.dirs &= ~GetCaveLinkDir(c1,c2);
	}
	i = GetIndexOf(c2.links, c1); n = GetLength(c2.links) - 1;
	if (i>=0)
	{
		c2.links[i] = c2.links[n];
		SetLength(c2.links, n);
		c2.dirs &= ~GetCaveLinkDir(c2,c1);
	}
	return true;
}

func DrawVariations(string mat, int ratio, int sx, int sy)
{
	var rand_algo = {Algo=MAPALGO_RndChecker, Ratio=ratio, Wdt=sx, Hgt=sy};
	var turb_algo = {Algo=MAPALGO_Turbulence, Amplitude=12, Scale=8, Op=rand_algo};
	return Draw(mat, turb_algo);
}

func DrawBackground()
{
	Draw("Rock");
	DrawVariations("Rock-rock_cracked", 50, 5,15);
	DrawVariations("Ore", 10, 8,8);
	DrawVariations("Firestone", 8, 12,3);
	DrawVariations("Coal", 8, 8,3);
	DrawVariations("Gold", 5, 4,4);
	DrawVariations("Granite", 14, 15,5);
	DrawVariations("Granite", 14, 5,15);
	return true;
}

func DrawCaves()
{
	for (var cave in caves)
	{
		var cave_algo = {Algo=MAPALGO_Ellipsis, X=cave.X, Y=cave.Y, Wdt=4, Hgt=4};
		var turb_algo = {Algo=MAPALGO_Turbulence, Amplitude=8, Scale=15, Op=cave_algo};
		Draw("Tunnel", turb_algo);
		//var src = cave.path;
		//if (src && GetLength(src)) src = Format("%d,%d", src[GetLength(src)-1].X, src[GetLength(src)-1].Y);
		//Log("Cave at %d,%d src %v", cave.X, cave.Y, src);
	}
	return true;
}

func DrawTunnels()
{
	for (var cave in caves)
	{
		for (var cave2 in cave.links)
		{
			if (cave2.done) continue;
			var tunnel_algo = {Algo=MAPALGO_Polygon, X=[cave.X, cave2.X], Y=[cave.Y, cave2.Y], Wdt=2, Open=1, Empty=1 };
			var turb_algo = {Algo=MAPALGO_Turbulence, Amplitude=9, Scale=10, Op=tunnel_algo};
			Draw("Tunnel", turb_algo);
		}
		cave.done = true;
	}
	return true;
}

func DrawStart()
{
	Draw("Tunnel", nil, [0, start_cave.Y - 4, start_cave.X, 4]);
	Draw("Brick", nil, [0, start_cave.Y, start_cave.X-2, 1]);
	return true;
}

func DrawEnd()
{
	Draw("Ruby", {Algo=MAPALGO_Ellipsis, X=end_cave.X, Y=end_cave.Y, Wdt=4, Hgt=4});
	return true;
}

protected func InitializeMap(map)
{
	map->Resize(300,300);

	FindCaves(200);
	FindCaveConnections();
	FindStart();
	MakeMaze();
	
	DrawBackground();
	DrawTunnels();
	DrawCaves();
	DrawStart();
	DrawEnd();
	
	// Caves to global var: Need to clean up circular prop list references because they cause crashes
	var cave;
	for (cave in end_cave.path)
	{
		cave.is_main_path = true;
	}
	for (cave in caves)
	{
		cave.n_links = GetLength(cave.links);
		cave.links = cave.path = nil;
	}
	g_caves = caves;
	return true;
}

