/**
	Global functions used in correlation to Objects.ocd\Libraries.ocd\Flag.ocd
	
	@author Zapper
*/

// Returns the flagpole that is currently holding ownership of a specific point in the landscape.
global func GetFlagpoleForPosition(int x, int y)
{
	if (GetType(LIB_FLAG_FlagList) != C4V_Array)
		return nil;
	
	var oldest = nil;
	var oldest_time = 0;
	
	for (var flag in LIB_FLAG_FlagList)
	{
		// Safety in case this gets called during destruction of a flag.
		if (!flag) 
			continue;
		var d = Distance(GetX() + x, GetY() + y, flag->GetX(), flag->GetY());
		if (d > flag->GetFlagRadius()) 
			continue; 
		
		if (oldest == nil || flag->GetFlagConstructionTime() < oldest_time)
		{
			oldest = flag;
			oldest_time = flag->GetFlagConstructionTime();
		}
	}
	return oldest;
}

// Returns the current owner that controls a certain point with a flagpole or NO_OWNER.
global func GetOwnerOfPosition(int x, int y)
{
	var flag = GetFlagpoleForPosition(x, y);
	if (!flag) 
		return NO_OWNER;
	return flag->GetOwner();
}

// Redraws all flag radiuses.
global func RedrawAllFlagRadiuses()
{
	for (var flag in LIB_FLAG_FlagList)
		if (flag) 
			flag->RedrawFlagRadius();
	return;
}
