/*-- 
		Script goal
		Author: Maikel
		
		The script goal can be fulfilled from other scripts, e.g. a scenario script.
--*/


#include Library_Goal

local fulfilled;

protected func Initialize()
{
	fulfilled = false;
	return inherited(...);
}

public func IsFulfilled() 
{ 
	return fulfilled; 
}

public func Fulfill()
{
	fulfilled = true;
	return;
}


/*-- Proplist --*/

func Definition(def)
{
	SetProperty("Name", "$Name$", def);
}
