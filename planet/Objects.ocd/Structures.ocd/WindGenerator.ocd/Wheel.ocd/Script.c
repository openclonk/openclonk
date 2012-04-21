/**
	Wheel
	Invisible helper object. Takes care of collisions.

*/

local ActMap = {
	Turn = {
		Prototype = Action,
		Name = "Turn",
		Procedure = DFA_ATTACH,
		Length = 1,
		Delay = 0,
		FacetBase=1,
		NextAction = "Hold",
	}
};

func Initialize()
{
	
}

func AttachTargetLost()
{
	return RemoveObject();
}

func SetRotation(
	int current_rotation /* in percent of 360° */,
	int speed /* number of frames for one revoluation */,
	int wind_direction /* -1 to 1 */)
{
	var direction = wind_direction * (2 * 36000) / speed;
	SetR((360 * current_rotation) / 100);
	SetRDir(direction, 1000);
}

func HasStopped()
{
	return !GetRDir(1000);
}

func Set(to, con)
{
	con = con ?? 100;
	SetCon(con);
	SetAction("Turn", to);
	SetRDir(20);
}