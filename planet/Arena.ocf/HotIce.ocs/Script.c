/* Hot ice */

func Initialize()
{
	// Materials: Chests
	var i,pos;
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var top_area_hgt = ls_hgt*[50,80][SCENPAR_MapType]/100;
	for (i=0; i<6; ++i)
		if (pos=FindLocation(Loc_InRect(0,0,ls_wdt,top_area_hgt-100), Loc_Wall(CNAT_Bottom))) // Loc_Wall adds us 100 pixels...
		{
			var chest = CreateObjectAbove(Chest,pos.x,pos.y);
			if (chest)
			{
				chest->CreateContents(Firestone,5);
				chest->CreateContents(Bread,1);
				chest->CreateContents(Bow,1);
				//chest->CreateContents(Arrow,1); - avoid extra layer in ring menu
				chest->CreateContents(FireArrow,1);
				chest->CreateContents(BombArrow,1)->SetStackCount(5);
				chest->CreateContents(Shield,1);
				chest->CreateContents(Sword,1);
			}
		}
	// Materials: Firestones
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,0,ls_wdt,top_area_hgt), Loc_Solid()))
			if (IsFirestoneSpot(pos.x,pos.y))
				CreateObjectAbove(Firestone,pos.x,pos.y-1);
	// Some firestones in lower half. For ap type 1, more firestones in lower than upper half.
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,ls_hgt/2,ls_wdt,ls_hgt/3), Loc_Solid()))
			if (IsFirestoneSpot(pos.x,pos.y))
				CreateObjectAbove(Firestone,pos.x,pos.y-1);
	return true;
}

func InitializePlayer(int plr)
{
	// everything visible
	SetFoW(false, plr);
	// player positioning. In lower area for both maps becuase starting high is an an advantage.
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var crew = GetCrew(plr);
	var start_pos = FindLocation(Loc_InRect(ls_wdt/5,ls_hgt/2,ls_wdt*3/5,ls_hgt/3), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
	if (!start_pos) start_pos = FindLocation(Loc_InRect(ls_wdt/10,0,ls_wdt*8/10,ls_hgt*4/5), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
	if (!start_pos) start_pos = {x=Random(ls_wdt*6/10)+ls_wdt*2/10, y=ls_hgt*58/100};
	crew->SetPosition(start_pos.x, start_pos.y-10);
	// initial material
	crew->CreateContents(Shovel);
	crew->CreateContents(Club);
	crew->CreateContents(WindBag);
	crew->CreateContents(Firestone,3);
	crew.MaxEnergy = 120000;
	crew->DoEnergy(1000);
	return true;
}

// Horizontal Loc_Space doesn't work with Loc_Wall because it checks inside the ground.
func IsStartSpot(int x, int y)
{
	// Don't spawn just at the border of an island.
	if (!GBackSolid(x-3,y+2)) return false;
	if (!GBackSolid(x+3,y+2)) return false;
	// Spawn with some space.
	return PathFree(x-5, y, x+5, y) && PathFree(x, y-21, x, y-1);
}

func IsFirestoneSpot(int x, int y)
{
// Very thorough ice surrounding check so they don't explode right away or when the first layer of ice melts
	return GBackSolid(x,y-1) && GBackSolid(x,y+4) && GBackSolid(x-2,y) && GBackSolid(x+2,y);
}
