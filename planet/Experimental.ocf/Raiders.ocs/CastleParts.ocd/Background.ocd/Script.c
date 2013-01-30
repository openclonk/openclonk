/* Castle Background */

func Initialize()
{
	CreateObject(Raiders_FrontTower, -113, 85, NO_OWNER);
	// First floor
	CreateObject(Raiders_CastleFloor, -52, 51, NO_OWNER)->SetBreaky(1+Random(2));
	CreateObject(Raiders_CastleFloor, 35, 51, NO_OWNER)->SetBreaky(1+Random(2));
	// Second floor
	CreateObject(Raiders_CastleFloor, 88, 11, NO_OWNER)->SetBreaky(1+Random(2));
	CreateObject(Raiders_CastleFloor, 1, 11, NO_OWNER)->SetBreaky(1+Random(2));
	// Third floor
	CreateObject(Raiders_CastleFloor, -52, -29, NO_OWNER)->SetBreaky(3);
	CreateObject(Raiders_CastleFloor, 35, -29, NO_OWNER)->SetBreaky(3);
}