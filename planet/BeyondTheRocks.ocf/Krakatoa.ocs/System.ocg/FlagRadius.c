// Reduced flag radius to make expansion goal and settling a little harder.

#appendto Library_Flag

public func Construction()
{
	_inherited(...);
	lflag.radius = 140;
	return;
}