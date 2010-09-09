/*-- 
	Flag
	
--*/

protected func Initialize()
{
	SetAction("Wave");
	PlayAnimation("Wave", 5, Anim_Linear(0, 0, GetAnimationLength("Wave"), 78, ANIM_Loop), Anim_Const(1000)); //Stupid ActMap!!!
	//The animation seems to not want to play, no matter the method used. >:(
}

/*-- Proplist --*/

local Name = "$Name$";

func Definition(def)
{
	SetProperty("MeshTransformation", Trans_Rotate(90,0,1,0),def);
}	
local ActMap = {
Wave = {
	Prototype = Action,
	Name = "Wave",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 0,
	Length = 78,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 30,
	Hgt = 40,
	NextAction = "Wave",
	Animation = "Wave",
},
};
