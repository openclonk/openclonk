
func InitializePlayer(proplist plr)
{
	plr->GetCrew()->CreateContents(Shovel);
	for (var elev in FindObjects(Find_ID(Elevator)))
		elev->CreateShaft(400);
}