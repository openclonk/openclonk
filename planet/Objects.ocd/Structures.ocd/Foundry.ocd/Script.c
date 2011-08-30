/*--
	Foundry
	Authors: Ringwaul, Maikel
	
	Melts iron ore to metal, using some sort of fuel.
--*/


#include Library_Producer

public func Construction()
{
	SetProperty("MeshTransformation",Trans_Rotate(RandomX(-40,20),0,1,0));
	//queue = [[Metal, nil]];
	return _inherited(...);;
}

/*-- Production --*/

public func CanProduceItem(id item_id)
{
	if (item_id == Metal)
		return true;	
	return false;
}

public func NeedsRawMaterial(id rawmat_id)
{
	if (rawmat_id == Coal || rawmat_id == Wood || rawmat_id == Ore)
		return true;
	return false;
}

public func IsProducing()
{
	if (GetEffect("Smelting", this))
		return true;
	return false;
}

private func Produce(id item_id)
{
	// Check if material is available.
	if (item_id == Metal)
		if (!FindContents(Ore))
			return false;
	// Check if fuel is available, TODO: oil
	if (ContentsCount(Wood) < 2 &&  !FindContents(Coal))
		return false;
	// If already busy, wait a little.
	if (IsProducing())
		return false;
	// Start production.	
	AddEffect("Smelting",this,1,1,this);
	Sound("FurnaceStart.ogg");
	AddEffect("IntSoundDelay",this,1,1,this);
	return true;
}

private func ProductionCosts(id item_id)
{
	if (item_id == Metal)
		return [[Ore, 1],[Coal, 1]];

	return _inherited(item_id, ...);
}

local cast = 0;

protected func Collection()
{
	Sound("Clonk.ogg");
	return;
}

public func FxSmeltingStart(object target, num, int temporary)
{
	FindContents(Ore)->RemoveObject();

	//Use coal as firing material
	var coal = FindContents(Coal);
	if(coal)
	{
		coal->RemoveObject();
		return;
	}

	//Use wood as firing material
	if(ContentsCount(Wood) >= 2)
	{
		FindContents(Wood)->RemoveObject();
		FindContents(Wood)->RemoveObject();
		return;
	}
}

public func FxSmeltingTimer(object target, num, int timer)
{
	Message(Format("Smelting %d",timer));
	//Visuals
	//Fire
	CreateParticle("Fire",10,14,RandomX(-1,1),RandomX(-1,1),RandomX(25,50),RGB(255,255,255), this);

	//Smoke
	CreateParticle("ExploSmoke",9,-35,RandomX(-1,1),-7 + RandomX(-2,2),RandomX(30,125),RGBa(255,255,255,50));
	CreateParticle("ExploSmoke",16,-33,RandomX(-1,1),-7 + RandomX(-2,2),RandomX(30,90),RGBa(255,255,255,50));

	
	if(timer == 244)
	{
		//Pour
		SetMeshMaterial("MetalFlow",1);
	}

	//Molten metal hits cast... Sizzling sound
	if(timer == 256) Sound("Sizzle.ogg");

	if(timer > 244 && timer < 290)
	{
		CreateParticle("Fire",-17,14,-1 + RandomX(-1,1), 2+ RandomX(-1,1),RandomX(5,15),RGB(255,255,255));
	}

	if(timer == 290)
	{
		SetMeshMaterial("Metal",1);
		cast = 1;
		AddEffect("EjectMetal",this, 1, 1, this);
		Sound("FurnaceLoop.ogg",false,100,nil,-1);
		Sound("FurnaceStop.ogg");
		return -1;
	}
}

public func FxEjectMetalTimer(object target, num, int timer)
{
	if(timer > 24)
	{
		var metal = CreateObject(Metal, -20, 16);
		metal->SetSpeed(0,-17);
		metal->SetR(30 - Random(59));
		metal->Enter(this);
		Sound("Pop.ogg");
		cast = 0;
		return -1;
	}
}

public func FxIntSoundDelayTimer(object target, num, int timer)
{
	if(timer >= 100)
	{
		Sound("FurnaceLoop.ogg",false,100,nil,+1);
		return -1;
	}
}

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000,0,7000),Trans_Rotate(-20,1,0,0),Trans_Rotate(30,0,1,0)), def);
}

local Touchable = 2;
local Name = "$Name$";
