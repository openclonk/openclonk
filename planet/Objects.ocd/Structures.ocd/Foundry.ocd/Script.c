/**
	Foundry
	Melts iron ore to metal, using some sort of fuel.

	@authors Ringwaul, Maikel
*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer
#include Library_LampPost
#include Library_LiquidContainer
#include Library_PipeControl


// Foundry does not need power.
public func PowerNeed() { return 0; }

public func IsPowerConsumer() { return false; }

public func LampPosition(id def) { return [-11 * GetCalcDir(), 2]; }

public func Construction(object creator)
{
	AddTimer("CollectionZone", 1);
	SetAction("Default");
	return _inherited(creator, ...);
}

public func IsHammerBuildable() { return true; }

public func Initialize()
{
	// Update vertices to fit shape of flipped building after construction is finished.
	// Vertices are being reset when the construction is finished (i.e. on shape updates).
	if (GetDir() == DIR_Right)
		FlipVertices();
	return _inherited(...);
}

public func SetDir(int dir)
{
	// Update vertices to fit shape of flipped building when dir is changed.
	if (GetDir() != dir)
		FlipVertices();
	return _inherited(dir, ...);
}


/*-- Production --*/

private func IgnoreKnowledge() { return true; }

private func IsProduct(id product_id)
{
	return product_id->~IsFoundryProduct();
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 290; }

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
public func CollectionZone()
{
	if (GetCon() < 100)
		return;
	for (var obj in FindObjects(Find_InRect(16 - 45 * GetDir(), 3, 13, 13), Find_OCF(OCF_Collectible), Find_NoContainer(), Find_Layer(GetObjectLayer())))
		Collect(obj, true);
}

public func Collection()
{
	Sound("Objects::Clonk");
	return _inherited(...);
}

public func FxSmeltingTimer(object target, proplist effect, int time)
{
	// Fire in the furnace.
	CreateParticle("Fire", -10 * GetCalcDir() + RandomX(-1, 1), 20 + RandomX(-1, 1), PV_Random(-1, 1), PV_Random(-1, 1), PV_Random(3, 10), Particles_Fire(), 2);

	// Smoke from the pipes.
	Smoke( -10*GetCalcDir(), -26, 6);
	Smoke(-16*GetCalcDir(), -22, 3);
	
	// Furnace sound after some time.
	if (time == 30)
		Sound("Structures::Furnace::Loop", false, 100, nil, +1);

	// Pour after some time.
	if (time == 244)
		SetMeshMaterial("MetalFlow", 1);

	// Molten metal hits cast... Sizzling sound.
	if (time == 256)
		Sound("Liquids::Sizzle");

	// Fire from the pouring exit.
	if (Inside(time, 244, 290))
		CreateParticle("SphereSpark", 16 * GetCalcDir(), 20, PV_Random(2 * GetCalcDir(), 0), PV_Random(-2, 3), PV_Random(18, 36), Particles_Material(RGB(255, 200, 0)), 2);

	if (time == 290)
	{
		SetMeshMaterial("Metal", 1);
		Sound("Structures::Furnace::Loop", false, 100, nil, -1);
		Sound("Structures::Furnace::Stop");
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func OnProductEjection(object product)
{
	// Enter produced liquids.
	if (product->~IsLiquid())
	{
		product->Enter(this);
		return;
	}
	// Other products exit the foundry.	
	product->SetPosition(GetX() + 18 * GetCalcDir(), GetY() + 16);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Structures::EjectionPop");
	return;
}

/*-- Pipeline --*/

public func IsLiquidContainerForMaterial(string liquid)
{
	return WildcardMatch("Oil", liquid) || WildcardMatch("Water", liquid) || WildcardMatch("Concrete", liquid);
}

public func GetLiquidContainerMaxFillLevel(liquid_name)
{
	if (GetLiquidDef(liquid_name) == Water)
		return 600;	
	return 300;
}

// Set to source or drain pipe.
public func OnPipeConnect(object pipe, string specific_pipe_state)
{
	if (PIPE_STATE_Source == specific_pipe_state)
	{
		SetSourcePipe(pipe);
		pipe->SetSourcePipe();
	}
	else if (PIPE_STATE_Drain == specific_pipe_state)
	{
		SetDrainPipe(pipe);
		pipe->SetDrainPipe();
	}
	else
	{
		if (!GetDrainPipe())
			OnPipeConnect(pipe, PIPE_STATE_Drain);
		else if (!GetSourcePipe())
			OnPipeConnect(pipe, PIPE_STATE_Source);
	}
	pipe->Report("$MsgConnectedPipe$");
}


/*-- Properties --*/

public func Definition(proplist def)
{
	def.PictureTransformation = Trans_Mul(Trans_Translate(2000, 0, 7000), Trans_Rotate(-20, 1, 0, 0), Trans_Rotate(30, 0, 1, 0));
	return _inherited(def, ...);
}

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

local Name = "$Name$";
local Description = "$Description$";
local ContainBlast = true;
local BlastIncinerate = 100;
local HitPoints = 100;
local FireproofContainer = true;
local Components = {Rock = 4, Wood = 2};

// The foundry may have one drain and one source.
local PipeLimit_Air = 0;
local PipeLimit_Drain = 1;
local PipeLimit_Source = 1;
