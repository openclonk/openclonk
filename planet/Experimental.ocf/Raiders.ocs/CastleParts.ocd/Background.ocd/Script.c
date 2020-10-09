/* Castle Background */

func Initialize()
{
	CreateObjectAbove(Raiders_FrontTower, -113, 85, NO_OWNER);
	// First floor
	CreateObjectAbove(Raiders_CastleFloor, -52, 51, NO_OWNER)->SetBreaky(1 + Random(2));
	CreateObjectAbove(Raiders_CastleFloor, 35, 51, NO_OWNER)->SetBreaky(1 + Random(2));
	// Second floor
	CreateObjectAbove(Raiders_CastleFloor, 88, 11, NO_OWNER)->SetBreaky(1 + Random(2));
	CreateObjectAbove(Raiders_CastleFloor, 1, 11, NO_OWNER)->SetBreaky(1 + Random(2));
	// Third floor
	CreateObjectAbove(Raiders_CastleFloor, -52, -29, NO_OWNER)->SetBreaky(3);
	CreateObjectAbove(Raiders_CastleFloor, 35, -29, NO_OWNER)->SetBreaky(3);
}
