/* Returns a random still alive windmill */

global func GetRandomWindmill()
{
	if (g_lost) return nil;

	var mills = [];
	if (g_windgen1) PushBack(mills, g_windgen1);
	if (g_windgen2) PushBack(mills, g_windgen2);
	if (g_windgen3) PushBack(mills, g_windgen3);
	if (g_windmill) PushBack(mills, g_windmill);

	return RandomElement(mills);
}

/* Returns the nearest still alive windmill */

global func GetNearestWindmill()
{
	if (g_lost) return nil;
	if (!this) return nil;

	var mills = [];
	if (g_windgen1) PushBack(mills, g_windgen1);
	if (g_windgen2) PushBack(mills, g_windgen2);
	if (g_windgen3) PushBack(mills, g_windgen3);
	if (g_windmill) PushBack(mills, g_windmill);

	var nearest = 0, dist = LandscapeWidth();
	for (var i = 0; i < GetLength(mills); i++)
	{
		if (ObjectDistance(mills[i]) < dist)
		{
			nearest = i;
			dist = ObjectDistance(mills[i]);
		}
	}

	return mills[nearest];
}