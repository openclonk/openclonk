
func InitializePlayer(proplist plr)
{
	GetCrew(plr)->CreateContents(Shovel);
	for (var elev in FindObjects(Find_ID(Elevator)))
		elev->CreateShaft(400);
}