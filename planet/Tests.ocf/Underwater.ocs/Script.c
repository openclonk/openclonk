
func Initialize()
{
	Fish->Place(20, Shape->Rectangle(0, 0, LandscapeWidth() / 2 - 50, LandscapeHeight()));
	Squid->Place(10, Shape->Rectangle(0, 0, LandscapeWidth() / 2 - 50, LandscapeHeight()));
	Piranha->Place(10, Shape->Rectangle(LandscapeWidth() / 2 + 50, 0, LandscapeWidth(), LandscapeHeight()));
	PiranhaSmall->Place(20, Shape->Rectangle(LandscapeWidth() / 2 + 50, 0, LandscapeWidth(), LandscapeHeight()));
	Squid->Place(5, Shape->Rectangle(LandscapeWidth() / 2 + 50, 0, LandscapeWidth(), LandscapeHeight()), {friendly = false});
	Seaweed->Place(20);
	Coral->Place(30);
	
	CastObjects(Bread, 10, 10, LandscapeWidth()/2, 30);
}

func InitializePlayer(plr)
{
	var c = GetCrew(plr);
	c->SetPosition(LandscapeWidth()/2, 20);
}