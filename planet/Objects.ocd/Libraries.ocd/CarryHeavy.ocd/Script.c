/*--
	Carry-Heavy Library
	Author: Ringwaul, Clonkonaut

	To be included by all objects that are heavy and need carrying by two hands.
--*/

/* Status */
public func IsCarryHeavy() { return true; }
public func GetDropDescription() { return Format("$TxtPutDown$", GetName()); }

/* Visibility */
public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryBone() { return "main"; }
public func GetCarrySpecial(clonk)
{
	var action = clonk->~GetAction();
	if (action == "Scale" || action == "Hangle" || action == "Push" || action == "Swim")
		return "skeleton_body";
}

/* Pickup */

// Draws a hand symbol in addition to the regular selection highlight
public func PickupHighlight(object dummy, int width, int height, int player)
{
	var selector =
	{
		Size = PV_Step(3, 2, 1, Max(width, height)),
		Attach = ATTACH_Front,
		Rotation = PV_Step(1, PV_Random(0, 360), 1),
		Alpha = 200
	};

	dummy->CreateParticle("Selector", 0, 0, 0, 0, 0, Particles_Colored(selector, GetPlayerColor(player)), 1);

	selector.Rotation = 0;
	selector.Alpha = 230;
	// 25% bigger
	selector.Size = PV_Step(3, 2, 1, Max(width, height) * 5 / 4);

	dummy->CreateParticle("SelectorHeavy", 0, 0, 0, 0, 0, selector, 1);

	return true;
}