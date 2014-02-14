func Initialize()
{
	CreateObject(OCTier, 48, 100, -1)->TestAnimation("Run");
	CreateObject(OCTier, 88, 100, -1)->TestAnimation("Dig");
	CreateObject(OCTier, 128, 100, -1)->TestAnimation("Poke");
	CreateObject(OCTier, 168, 100, -1)->TestAnimation("Walk");
}

func InitializePlayer(plr)
{
var oct = CreateObject(OCTier, 48, 52, -1);
oct->MakeCrewMember(plr);
}
