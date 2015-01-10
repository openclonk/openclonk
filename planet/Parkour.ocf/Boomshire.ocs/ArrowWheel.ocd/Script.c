/*-- Spin Wheel --*/

local x,y;

public func Initialize()
{
	SetAction("Still");
	x=-370;
	y=50;
}



public func ControlUp(object clonk)
{
	if (GetAction() == "Still")
	{	
		SetAction("SpinLeft");
		Sound("Chain");
		var arrw= CreateObjectAbove(Arrow,x,y,-1);
		arrw->Launch(30,80, clonk);
		arrw->SetGraphics("1");
		CreateParticle("Straw", 0, 0, PV_Random(-20, 20), PV_Random(-20,20), PV_Random(30, 120), Particles_Straw(), 20);
	}
	if(GetEffect("SparklingAttention",this)) RemoveEffect("SparklingAttention",this);
}


func Definition(def) {
	SetProperty("ActMap", {
		Still = {
			Prototype = Action,
			Name = "Still",
			Procedure = DFA_NONE,
			Length = 1,
			Delay = 1,
			NextAction = "Still",
			Animation = "SpinLeft",
		},
		SpinLeft = {
			Prototype = Action,
			Name = "SpinLeft",
			Procedure = DFA_NONE,
			Length = 36,
			Delay = 1,
			NextAction = "Still",
			Animation = "SpinLeft",
		},
		SpinRight = {
			Prototype = Action,
			Name = "SpinRight",
			Procedure = DFA_NONE,
			Length = 36,
			Delay = 1,
			NextAction = "Still",
			Animation = "SpinRight",
		},
	}, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("PictureTransformation", Trans_Mul(Trans_Scale(800), Trans_Translate(0,0,0),Trans_Rotate(-20,1,0,0),Trans_Rotate(-30,0,1,0)), def);
	SetProperty("MeshTransformation", Trans_Rotate(-13,0,1,0));
}
local Touchable = 2;
