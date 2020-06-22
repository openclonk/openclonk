#appendto SpinWheel

local shoot_arrow;

public func SetArrowWheel()
{
	shoot_arrow = true;
}

public func ControlUp(object clonk)
{
	if (GetEffect("SparklingAttention",this)) RemoveEffect("SparklingAttention",this);
	if (!shoot_arrow) return _inherited(clonk, ...);

	if (GetAction() == "Still")
	{	
		SetAction("SpinLeft");
		Sound("Structures::StoneGate::Chain");
		var arrw= CreateObjectAbove(Arrow, -370, 50,-1);
		arrw->Launch(40, 80, clonk);
		arrw->SetGraphics("1");
		CreateParticle("Straw", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(30, 120), Particles_Straw(), 20);
	}
}
