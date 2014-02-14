/*--
	Global functions used in correlation to Objects.ocd\Libraries.ocd\Flag.ocd
	
	Authors: Zapper
--*/

// returns the flagpole that is currently holding ownership of a specific point in the landscape
global func GetFlagpoleForPosition(
	int x /* x position in local coordinates */
	, int y /* y position in local coordinates */)
{
	if(GetType(LibraryFlag_flag_list) != C4V_Array) return nil;
	
	var oldest = nil, oldest_time = 0;
	
	for(var flag in LibraryFlag_flag_list)
	{
		if (!flag) continue; // safety in case this gets called during destruction of a flag
		var d = Distance(GetX() + x, GetY() + y, flag->GetX(), flag->GetY());
		if(d > flag->GetFlagRadius()) continue; 
		
		if(oldest == nil || flag->GetFlagConstructionTime() < oldest_time)
		{
			oldest = flag;
			oldest_time = flag->GetFlagConstructionTime();
		}
	}
	return oldest;
}

// returns the current owner that controls a certain point with a flagpole or NO_OWNER
global func GetOwnerOfPosition(
	int x /* x position in local coordinates */
	, int y /* y position in local coordinates */)
{
	var flag = GetFlagpoleForPosition(x, y);
	if(!flag) return NO_OWNER;
	return flag->GetOwner();
}

// redraws all flag radiuses
global func RedrawAllFlagRadiuses()
{
	for(var f in LibraryFlag_flag_list)
		if (f) f->RedrawFlagRadius();
}
