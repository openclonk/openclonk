/*--
	Foundry
	Authors: Ringwaul, Maikel
	
	Melts iron ore to metal, using some sort of fuel.
--*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost
#include Library_Tank

// does not need power
public func PowerNeed() { return 0; }
public func IsPowerConsumer() { return false; }

public func LampPosition(id def) { return [GetCalcDir()*-11,2]; }

public func Construction(object creator)
{
	
	//SetProperty("MeshTransformation",Trans_Rotate(RandomX(-40,20),0,1,0));
	SetAction("Default");
	AddTimer("CollectionZone", 1);
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

/*-- Production --*/

private func IgnoreKnowledge() { return true; }

private func IsProduct(id product_id)
{
	return product_id->~IsFoundryProduct();
}
private func ProductionTime(id toProduce) { return 290; }

public func OnProductionStart(id product)
{
	AddEffect("Smelting", this, 100, 1, this);
	Sound("Structures::Furnace::Start");
	return;
}

public func OnProductionHold(id product)
{
	return;
}

public func OnProductionFinish(id product)
{
	RemoveEffect("Smelting", this);
	return;
}

// Timer, check for objects to collect in the designated collection zone
func CollectionZone()
{
	if (GetCon() < 100) return;

	for (var obj in FindObjects(Find_InRect(16 - 45 * GetDir(),3,13,13), Find_OCF(OCF_Collectible), Find_NoContainer(), Find_Layer(GetObjectLayer())))
		Collect(obj, true);
}

func Collection()
{
	Sound("Objects::Clonk");
	return;
}

public func FxSmeltingTimer(object target, proplist effect, int time)
{
	//Message(Format("Smelting %d",timer));
	// Fire in the furnace.
	CreateParticle("Fire", -10 * GetCalcDir() + RandomX(-1, 1), 20 + RandomX(-1, 1), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(3, 10), Particles_Fire(), 2);

	// Smoke from the pipes.
	Smoke( -10*GetCalcDir(), -26, 6);
	Smoke(-16*GetCalcDir(), -22, 3);
	
	// Furnace sound after some time.
	if (time == 30)
		Sound("Structures::Furnace::Loop", false, 100, nil, +1);

	// Pour after some time.
	if(time == 244)
		SetMeshMaterial("MetalFlow", 1);

	//Molten metal hits cast... Sizzling sound
	if (time == 256)
		Sound("Liquids::Sizzle");

	// Fire from the pouring exit.
	if (Inside(time, 244, 290))
		CreateParticle("SphereSpark", 16 * GetCalcDir(), 20, PV_Random(2 * GetCalcDir(), 0), PV_Random(-2, 3), PV_Random(18, 36), Particles_Material(RGB(255, 200, 0)), 2);

	if (time == 290)
	{
		SetMeshMaterial("Metal", 1);
		Sound("Structures::Furnace::Loop", false ,100, nil, -1);
		Sound("Structures::Furnace::Stop");
		return -1;
	}
	return 1;
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() + 18 * GetCalcDir(), GetY() + 16);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Structures::EjectionPop");
	return;
}

/*-- Pipeline --*/

func IsLiquidContainerForMaterial(string liquid)
{
	return WildcardMatch("Oil", liquid) || WildcardMatch("Water", liquid);
}

func QueryConnectPipe(object pipe)
{
	if (GetNeutralPipe())
	{
		pipe->Report("$MsgHasPipes$");
		return true;
	}

	if (pipe->IsDrainPipe() || pipe->IsNeutralPipe())
	{
		return false;
	}
	else
	{
		pipe->Report("$MsgPipeProhibited$");
		return true;
	}
}

func OnPipeConnect(object pipe, string specific_pipe_state)
{
	SetNeutralPipe(pipe);
	pipe->Report("$MsgConnectedPipe$");
}

func GetLiquidContainerMaxFillLevel()
{
	return 300;
}


/*-- Properties --*/


local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 1,
			NextAction = "Default",
		},
};

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description = "$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local HitPoints = 100;
local Components = {Rock = 4, Wood = 2};
