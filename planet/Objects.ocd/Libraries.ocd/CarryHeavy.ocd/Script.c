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
	if(action == "Scale" || action == "Hangle" || action == "Push")
		return "skeleton_body";
}