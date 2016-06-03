/**
	Engine callback handling for landscape inflammation
*/

// Callback from engine when landsape is incinerated at an incindiary material.
global func InflameLandscape(int x, int y, int cause_player)
{
	// Not too many flames.
	if (!FindObject(Find_InRect(x - 5, y - 4, 10, 24), Find_ID(Flame)))
		CreateObjectAbove(Flame, x, y, cause_player);
	return;
}
