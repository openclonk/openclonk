/* Flagpole */
// Changes default flag radius

#appendto Flagpole

func Construction()
{
	var retval = _inherited(...);
	lflag.radius = 130;
	return retval;
}
