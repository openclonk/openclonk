/* Hot ice */

func Initialize()
{
	// Materials: Chests
	var i,pos;
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	for (i=0; i<6; ++i)
		if (pos=FindLocation(Loc_InRect(0,0,ls_wdt,ls_hgt/2-100), Loc_Wall(CNAT_Bottom))) // Loc_Wall adds us 100 pixels...
		{
			var chest = CreateObject(Chest,pos.x,pos.y);
			if (chest)
			{
				chest->CreateContents(Firestone,5);
				chest->CreateContents(Bread,2);
				var bonus = [[Bow,Arrow],[Shield,Sword]][Random(2)];
				for (var obj in bonus) chest->CreateContents(obj);
			}
		}
	// Materials: Firestones
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,0,ls_wdt,ls_hgt/2), Loc_Solid()))
			if (GBackSolid(pos.x,pos.y-1))
				CreateObject(Firestone,pos.x,pos.y-1);
	// Some firestones in lower half
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,ls_hgt/2,ls_wdt,ls_hgt/3), Loc_Solid()))
			if (GBackSolid(pos.x,pos.y-1))
				CreateObject(Firestone,pos.x,pos.y-1);
	return true;
}

func InitializePlayer(int plr)
{
	// player positioning
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var crew = GetCrew(plr);
	var start_pos = FindLocation(Loc_InRect(ls_wdt/5,ls_hgt/2,ls_wdt*3/5,ls_hgt/3), Loc_Wall(CNAT_Bottom), Loc_Space(20), Loc_Space(20,true));
	if (!start_pos) start_pos = {x=Random(ls_wdt*6/10)+ls_wdt*2/10, y=ls_hgt*58/100};
	crew->SetPosition(start_pos.x, start_pos.y-10);
	// initial material
	crew->CreateContents(Shovel);
	crew->CreateContents(Club);
	crew->CreateContents(Firestone);
	crew.MaxEnergy = 120000;
	crew->DoEnergy(1000);
	return true;
}
