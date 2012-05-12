/*--
	Foundry
	Authors: Ringwaul, Maikel
	
	Melts iron ore to metal, using some sort of fuel.
--*/

#include Library_Structure
#include Library_Ownable
#include Library_Producer

// does not need power
func PowerNeed() { return 0; }

public func Construction(object creator)
{
	
	//SetProperty("MeshTransformation",Trans_Rotate(RandomX(-40,20),0,1,0));
	SetAction("Default");
	AddTimer("CollectionZone", 1);	
	return _inherited(creator, ...);
}

/*-- Production --*/

private func IgnoreKnowledge() { return true; }

private func IsProduct(id product_id)
{
	return product_id->~IsFoundryProduct();
}
private func ProductionTime(id toProduce) { return 290; }

public func NeedRawMaterial(id rawmat_id)
{
	if (rawmat_id->~IsFuel() || rawmat_id == Ore || rawmat_id == Nugget)
		return true;
	return false;
}


public func OnProductionStart(id product)
{
	AddEffect("Smelting", this, 100, 1, this);
	Sound("FurnaceStart");
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

	for (var object in FindObjects(Find_InRect(16 - 45 * GetDir(),3,13,13), Find_OCF(OCF_Collectible), Find_NoContainer(), Find_Layer(GetObjectLayer())))
		Collect(object);
}

func Collection()
{
	Sound("Clonk");
	return;
}

public func FxSmeltingTimer(object target, proplist effect, int time)
{
	//Message(Format("Smelting %d",timer));
	// Fire in the furnace.
	CreateParticle("Fire",-10*GetCalcDir(),20,RandomX(-1,1),RandomX(-1,1),RandomX(25,50),RGB(255,255,255), this);

	// Smoke from the pipes.
	CreateParticle("ExploSmoke", -9*GetCalcDir(), -31, RandomX(-2,1), -7 + RandomX(-2,2), RandomX(60,125), RGBa(255,255,255,50));
	CreateParticle("ExploSmoke", -16*GetCalcDir(), -27, RandomX(-1,2), -7 + RandomX(-2,2), RandomX(30,90), RGBa(255,255,255,50));
	
	// Furnace sound after some time.
	if (time == 30)
		Sound("FurnaceLoop", false, 100, nil, +1);

	// Pour after some time.
	if(time == 244)
		SetMeshMaterial("MetalFlow", 1);

	//Molten metal hits cast... Sizzling sound
	if (time == 256)
		Sound("Sizzle");

	// Fire from the pouring exit.
	if (Inside(time, 244, 290))
		CreateParticle("Fire",17*GetCalcDir(),19,-1 + RandomX(-1,1), 2+ RandomX(-1,1),RandomX(5,15),RGB(255,255,255));

	if (time == 290)
	{
		SetMeshMaterial("Metal", 1);
		Sound("FurnaceLoop", false ,100, nil, -1);
		Sound("FurnaceStop");
		return -1;
	}
	return 1;
}

public func OnProductEjection(object product)
{
	product->SetPosition(GetX() + 18 * GetCalcDir(), GetY() + 16);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Pop");
	return;
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

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 100;