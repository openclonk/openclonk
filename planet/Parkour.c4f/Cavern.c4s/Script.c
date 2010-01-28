/*-- The Summit --*/

protected func Initialize()
{
	// Create the race goal.
	var pGoal = CreateObject(Core_Goal_Parkour, 0, 0, NO_OWNER);
	// Set start point.
	var x, y;
	var d = 100;
	while(!FindPosInMat(x, y, "Sky", 0, LandscapeHeight()-d-40, LandscapeWidth(), 40, 20) && d < LandscapeHeight())
		d += 10;
	pGoal->SetStartpoint(x, y);
	// Set some checkpoints.
	for (var i = 0; d < LandscapeHeight()-400; i++)
	{
		var mode = RACE_CP_Check;
		d += RandomX(150,300);
		if (!FindPosInMat(x, y, "Tunnel", 0, LandscapeHeight()-d-80, LandscapeWidth(), 80, 20) || !Random(3))
			FindPosInMat(x, y, "Sky", 0, LandscapeHeight()-d-80, LandscapeWidth(), 80, 20);
		else
			mode = mode | RACE_CP_Respawn;
		// All checkpoints ordered.
		mode = mode | RACE_CP_Ordered;
		pGoal->AddCheckpoint(x, y, mode);
	}
	// Set finish point.
	d = 0;
	while(!FindPosInMat(x, y, "Sky", 0, 20+d, LandscapeWidth(), 40, 20) && d < LandscapeHeight())
		d += 10;
	pGoal->SetFinishpoint(x, y);
	// Done.

	// Create Rockfall.
	// CreateObject(RCKF, 0, 0, NO_OWNER)->SetDisaster(100);
	return;
}

protected func FindPosInMat(int &iToX, int &iToY, string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
{
	var iX, iY;
	for (var i = 0; i < 500; i++)
	{
		iX = iXStart+Random(iWidth);
		iY = iYStart+Random(iHeight);
		if(GetMaterial(AbsX(iX),AbsY(iY))==Material(sMat) &&
			GetMaterial(AbsX(iX+iSize),AbsY(iY+iSize))==Material(sMat) &&
			GetMaterial(AbsX(iX+iSize),AbsY(iY-iSize))==Material(sMat) &&
			GetMaterial(AbsX(iX-iSize),AbsY(iY-iSize))==Material(sMat) &&
			GetMaterial(AbsX(iX-iSize),AbsY(iY+iSize))==Material(sMat))
		{
			iToX = iX; iToY = iY;
			return true; // Location found.
		}
	}
	return false; // No location found.
}

// Gamecall from Race-goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(LOAM);
	clonk->CreateContents(MJOW);
	return;
}

// Gamecall from Race-goal, on reaching a bonus cp.
protected func GivePlrBonus(int iPlr, object cp)
{
	// No bonus.
	return;
}
