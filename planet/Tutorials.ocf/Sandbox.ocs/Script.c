/**
	Sandbox	
	Author: K-Pone
*/

public func Initialize()
{
	if (!MapGenSizeWidth) MapGenSizeWidth = 80;
	if (!MapGenSizeHeight) MapGenSizeHeight = 50;
	if (!MapGenPreset) MapGenPreset = "FlatLand";
}

public func InitializePlayer(int plr)
{
	var crew = GetCrew(plr);
	crew->ShowSandboxUI();
	
	crew->CreateContents(GodsHand);
	crew->CreateContents(DevilsHand);
	crew->CreateContents(SprayCan);
	crew->CreateContents(Teleporter);
	
	GiveAllKnowledge();
	
	crew.MaxContentsCount = 8;
}

public func GiveAllKnowledge()
{
	var i, id;
	while (id = GetDefinition(i++))
	{
		SetPlrKnowledge(nil, id);
	}
}
