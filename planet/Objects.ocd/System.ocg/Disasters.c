/**
	Contains functions that require definitions from the "disasters" folder.
*/

/**
	Launches a meteor.
	The meteor can spawn objects via spawn_id.
*/
global func LaunchMeteor(int x, int y, int size, int xdir, int ydir, id spawn_id, int spawn_amount)
{
	var meteor_skin = Meteor;
	if (spawn_id) meteor_skin = spawn_id->~GetMeteorSkin() ?? meteor_skin;

	var meteor = CreateObject(meteor_skin);
	return meteor->Launch(x, y, size, xdir, ydir, spawn_id, spawn_amount);
}
