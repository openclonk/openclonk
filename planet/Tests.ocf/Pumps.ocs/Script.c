
func InitializePlayer(int plr)
{
	CreateObject(Flagpole, 100,200, plr);
	CreateObject(SteamEngine, 50,200, plr);
	CreateObject(Pump, 130,200, plr);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Shovel);
	GetCrew(plr)->CreateContents(Coal, 10);
	GetCrew(plr)->SetPosition(130,190);
	return true;
}
