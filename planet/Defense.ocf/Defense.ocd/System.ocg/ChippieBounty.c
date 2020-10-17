/* Chippies need to give bounty */

#appendto Chippie

public func Death(proplist killed_by)
{
	GameCallEx("OnClonkDeath", this, killed_by); // for reward
	return _inherited(killed_by, ...);
}
