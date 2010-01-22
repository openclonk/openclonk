/*-- The Summit --*/

protected func Initialize()
{
	// Create the race goal.
	var pGoal = CreateObject(PARK, 0, 0, NO_OWNER);
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
		if (!FindPosInMat(x, y, "Tunnel", 0, LandscapeHeight()-d-80, LandscapeWidth(), 80, 20))
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

protected func RACE_GiveContents()
{
	return [LOAM,MJOW];
}