//No rain for clouds.

#appendto Cloud

public func TimedEvents()
{
	var iRight = LandscapeWidth() - 10;

	if(iWaitTime != 0) (iWaitTime = --iWaitTime);
	//if(iWaitTime == 0) Precipitation();	//no rain!
	WindDirection();
	CloudShade();
	//Makes clouds loop around map;
	if(GetX() >= iRight) SetPosition(12, GetY());
	if(GetX() <= 10) SetPosition(LandscapeWidth()-12, GetY());
	if(GetY() <= 5) SetPosition(0,6);
	if(GetYDir()!=0) SetYDir(0);

	while(Stuck()) SetPosition(GetX(),GetY()-5);
}