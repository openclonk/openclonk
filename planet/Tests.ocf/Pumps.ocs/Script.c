
func InitializePlayer(int plr)
{
	CreateObjectAbove(Flagpole, 100,200, plr);
	CreateObjectAbove(SteamEngine, 50,200, plr);
	CreateObjectAbove(Pump, 130,200, plr);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Shovel);
	GetCrew(plr)->CreateContents(Coal, 10);
	GetCrew(plr)->SetPosition(130,190);
	return true;
}
