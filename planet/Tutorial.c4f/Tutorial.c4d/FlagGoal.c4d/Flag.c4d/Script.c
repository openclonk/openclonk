/*--
	Flag
	
--*/

protected func Initialize()
{
	PlayAnimation("Wave", 1, Anim_Linear(0, 0, GetAnimationLength("Wave"), 78, ANIM_Loop), Anim_Const(1000));
}

/*-- Proplist --*/

local Name = "$Name$";

func Definition(def)
{
	SetProperty("MeshTransformation", Trans_Rotate(60,0,1,0),def);
}
