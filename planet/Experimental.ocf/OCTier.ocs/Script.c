func Initialize()
{
	CreateObjectAbove(OCTier, 48, 100, -1)->TestAnimation("Run");
	CreateObjectAbove(OCTier, 88, 100, -1)->TestAnimation("Dig");
	CreateObjectAbove(OCTier, 128, 100, -1)->TestAnimation("Poke");
	CreateObjectAbove(OCTier, 168, 100, -1)->TestAnimation("Walk");
}

func InitializePlayer(plr)
{
var oct = CreateObjectAbove(OCTier, 48, 52, -1);
oct->MakeCrewMember(plr);
}
