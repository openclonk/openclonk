/*--
	Global functions used in correlation to Objects.ocd\Libraries.ocd\Flag.ocd
	
	Authors: Zapper
--*/

global func GetFlagpoleForPosition(float x, float y)
{
	if(GetType(LibraryFlag_flag_list) != C4V_Array) return nil;
	
	var oldest = nil, oldest_time = 0;
	
	for(var flag in LibraryFlag_flag_list)
	{
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

global func GetOwnerOfPosition(float x, float y)
{
	var flag = GetFlagpoleForPosition(x, y);
	if(!flag) return NO_OWNER;
	return flag->GetOwner();
}

global func RedrawAllFlagRadiuses()
{
	for(var f in LibraryFlag_flag_list)
		f->RedrawFlagRadius();
}