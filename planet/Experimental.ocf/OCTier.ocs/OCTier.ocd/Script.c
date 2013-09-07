/* OCTier */

func Initialize()
{
	SetObjDrawTransform(5000,0,0,0,5000); // scale separately by DrawTransform, because scaling in mesh transform screws up Z clipping
	SetAction("Walk");
	//SetDir(Random(2));
}

func StartWalk()
{
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func Footstep()
{
	if (GetMaterialVal("DigFree", "Material", GetMaterial(0,10)) == 0)
		Sound("StepHard?");
	else
	{
		var dir = Sign(GetXDir());
		var clr = GetAverageTextureColor(GetTexture(0,10));
		CreateParticle("Dust2", dir*-4, 8, dir*-2, -2, 25+Random(5), DoRGBaValue(clr,-150,0));
		CreateParticle("Dust2", dir*-4, 8, dir*-3, -3, 25+Random(5), DoRGBaValue(clr,-150,0));
		Sound("StepSoft?");
	}
}

func TestAnimation(anim) // run dig poke walk
{
	if (this.test_anim) StopAnimation(this.test_anim);
	this.test_anim = PlayAnimation(anim, 1, Anim_Linear(0, 0, GetAnimationLength(anim), 40, ANIM_Loop), Anim_Const(1000));
	Message("@%s", anim);
	return true;
}




local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 200,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
//	InLiquidAction = "Swim",
},
};

func Definition(def) {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(90,0,1,0), Trans_Translate(0,-2500)), def);
	
}
