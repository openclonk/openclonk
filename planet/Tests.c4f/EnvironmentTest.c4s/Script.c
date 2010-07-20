/* Environment Testing Ground */

protected func Initialize()
{
	CreateObject(Environment_Grass);
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Celestial);
	CreateObject(Environment_Time);
	Sound("BirdsLoop.ogg",true,100,nil,+1);
}
