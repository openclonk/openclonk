/*--
	Foundry
	Authors: Ringwaul, Maikel
	
	Melts iron ore to metal, using some sort of fuel.
--*/


#include Library_Producer

public func Construction()
{
	
	//SetProperty("MeshTransformation",Trans_Rotate(RandomX(-40,20),0,1,0));
	return _inherited(...);
}

/*-- Production --*/

private func IsProduct(id product_id)
{
	return product_id->~IsFoundryProduct();
}
private func ProductionTime() { return 290; }
private func FuelNeed(id product) { return 100; }

public func NeedsRawMaterial(id rawmat_id)
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

protected func Collection()
{
	Sound("Clonk");
	return;
}

public func FxSmeltingTimer(object target, proplist effect, int time)
{
	//Message(Format("Smelting %d",timer));
	// Fire in the furnace.
	CreateParticle("Fire",10,20,RandomX(-1,1),RandomX(-1,1),RandomX(25,50),RGB(255,255,255), this);

	// Smoke from the pipes.
	CreateParticle("ExploSmoke", 9, -31, RandomX(-2,1), -7 + RandomX(-2,2), RandomX(60,125), RGBa(255,255,255,50));
	CreateParticle("ExploSmoke", 16, -27, RandomX(-1,2), -7 + RandomX(-2,2), RandomX(30,90), RGBa(255,255,255,50));
	
	// Furnace sound after some time.
	if (time == 100)
		Sound("FurnaceLoop", false, 100, nil, +1);

	// Pour after some time.
	if(time == 244)
		SetMeshMaterial("MetalFlow", 1);

	//Molten metal hits cast... Sizzling sound
	if (time == 256)
		Sound("Sizzle");

	// Fire from the pouring exit.
	if (Inside(time, 244, 290))
		CreateParticle("Fire",-17,19,-1 + RandomX(-1,1), 2+ RandomX(-1,1),RandomX(5,15),RGB(255,255,255));

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
	product->SetPosition(GetX() - 18, GetY() + 16);
	product->SetSpeed(0, -17);
	product->SetR(30 - Random(59));
	Sound("Pop");
	return;
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}
local Name = "$Name$";
