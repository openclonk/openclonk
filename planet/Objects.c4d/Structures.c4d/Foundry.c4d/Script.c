/*-- Foundry --*/

public func Initialize()
{
	AddEffect("SmeltCheck", this, 1, 12, this);
}

local exit_x = -20;
local exit_y = 16;
local cast = 0;

public func RejectCollect(id def, object obj)
{
	if(obj->GetID() != Ore && obj->~IsFuel() != true) return true;
	else
	{
		Sound("Clonk.ogg");
		return false;
	}
}

public func FoundryEject(object target)
{
	target->Exit(-20 + exit_x,exit_y,-1,0);
	//sound;
}

public func FxSmeltCheckTimer(object target, int num, int timer)
{
	if(ContentsCount(Wood) >= 2 || FindContents(Coal))
		if(FindContents(Ore))
		{
			if(!GetEffect("Smelting",this) && cast == 0)
			{
				AddEffect("Smelting",this,1,1,this);
				Sound("FurnaceStart.ogg");
				AddEffect("IntSoundDelay",this,1,1,this);
			}
			return;
		}
}

public func FxSmeltingStart(object target, int num, int temporary)
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

public func FxSmeltingTimer(object target, int num, int timer)
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

public func FxEjectMetalTimer(object target, int num, int timer)
{
	if(timer > 24)
	{
		var metal = CreateObject(Metal, exit_x, exit_y);
		metal->SetSpeed(0,-17);
		metal->SetR(30 - Random(59));
		Sound("Pop.ogg");
		cast = 0;
		return -1;
	}
}

public func FxIntSoundDelayTimer(object target, int num, int timer)
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

local Touchable = 1;
local Name = "$Name$";
