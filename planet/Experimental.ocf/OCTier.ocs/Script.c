func Initialize()
{
	CreateObjectAbove(OCTier, 48, 100, NO_OWNER)->TestAnimation("Run");
	CreateObjectAbove(OCTier, 88, 100, NO_OWNER)->TestAnimation("Dig");
	CreateObjectAbove(OCTier, 128, 100, NO_OWNER)->TestAnimation("Poke");
	CreateObjectAbove(OCTier, 168, 100, NO_OWNER)->TestAnimation("Walk");
}

func InitializePlayer(proplist plr)
{
var oct = CreateObjectAbove(OCTier, 48, 52, NO_OWNER);
oct->MakeCrewMember(plr);
}
