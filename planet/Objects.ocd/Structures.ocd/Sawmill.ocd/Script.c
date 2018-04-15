/**
	Sawmill
	Cuts trees or other objects into wood. Accepts only objects purely made from wood (like trees).
	
	@authors Ringwaul, Clonkonaut
*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer

local running = false;
local power = false;

public func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(-20, 0, 1, 0));
	SetAction("Default");
	return _inherited(...);
}

public func IsHammerBuildable() { return true; }

public func Initialize()
{
	this.SpinAnimation = PlayAnimation("work", 10, Anim_Const(0));
	AddTimer("CollectTrees", 4);
	return _inherited(...);
}

/*-- Interaction --*/

// Sawmill acts as a container to be able to collect wooden objects.
public func IsContainer() { return true; }

// Do not show normal inventory menu. Instead we show the remaining wood in an extra menu.
public func RejectContentsMenu() { return true; }

// Sawmill can't be interacted with.
public func IsInteractable() { return false; }

/*-- Production --*/

private func Collection(object obj)
{
	Sound("Objects::Clonk");
	Saw(obj);
}

private func RejectCollect(id id_def, object collect)
{
	// Don't collect wood
	if (id_def == Wood)
		return true;
	if (CheckWoodObject(collect))
		return false;
	return true;
}

// Timer, check for objects to collect in the designated collection zone
public func CollectTrees()
{
	if (GetCon() < 100)
		return;
	var tree = FindObject(Find_AtPoint(), Find_Func("IsTree"), Find_Not(Find_Func("IsStanding")), Find_Func("GetComponent", Wood));
	// If there is no tree in front of the sawmill try to get one from the cable car network.
	if (!tree)
		RequestTree();
	// Only take one tree at a time.
	if (!ContentsCount(Wood) && tree)
		Saw(tree);
	return;
}

// Returns whether the object is made purely out of wood.
private func CheckWoodObject(object target)
{
	if (target->GetComponent(nil, 0) != Wood) 
		return false;
	if (target->GetComponent(nil, 1)) 
		return false;
	return true;
}

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show a helpful hint to the player. The hint is colored and titled the same as the production menu for more visual coherence.
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	var prod_menu =
	{
		title = "$Production$",
		entries_callback = this.GetInfoMenuEntries,
		callback = nil,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, prod_menu);
	
	return menus;
}

public func GetInfoMenuEntries()
{
	var wood_count = ContentsCount(Wood);
	var info_text =
	{
		Right = "100%", Bottom = "6em",
		text = {Left = "2em", Text = "$AutoProduction$", Style = GUI_TextVCenter | GUI_TextHCenter},
		image = {Right = "2em", Bottom = "2em", Symbol = Wood},
		queue =
		{
			Top = "100% - 1.5em",
			Style = GUI_TextRight,
			Text = Format("$WoodInQueue$: %2d {{Wood}}", wood_count)
		}
	};
	return [{symbol = Wood, custom = info_text}];
}

public func Saw(object target)
{
	if (target->Contained() != this)
		target->Enter(this);
	target->Split2Components();
	AddEffect("WoodProduction", this, 100, 3, this);
	// Refresh interaction menus to show the wood count.
	UpdateInteractionMenus(this.GetInfoMenuEntries);
	return true;
}

private func ProductionTime(id product) { return _inherited(product, ...) ?? 100; }
private func PowerNeed() { return 20; }

private func FxWoodProductionStart(object t, proplist effect, int temp)
{
	if (temp) return;

	effect.runtime = 0;
	effect.starttime = 0;

	RegisterPowerRequest(PowerNeed());
}

private func FxWoodProductionTimer(object t, proplist effect, int time)
{
	if (!power)
	{
		if (effect.starttime)
			effect.starttime = 0;
		return FX_OK;
	}

	if (!(time%20))
		Smoke(10 * GetCalcDir(),10,10);

	if (!running)
	{
		SpinOn(effect.starttime);
		effect.starttime += 3;
		return FX_OK;
	}

	effect.runtime += 3;
	var dir = GetCalcDir();
	CreateParticle("WoodChip", PV_Random(-7 * dir, -3 * dir), PV_Random(3, 6), PV_Random(-5 * dir, -11 * dir), PV_Random(-4, -2), PV_Random(36 * 3, 36 * 10), Particles_WoodChip(), 3);

	if (effect.runtime >= ProductionTime())
	{
		EjectWood();
		effect.runtime = 0;
	}
	else
		return FX_OK;

	if (!ContentsCount(Wood))
		return FX_Execute_Kill;
}

private func FxWoodProductionStop(object t, proplist effect, int r, bool temp)
{
	if (temp) return;

	UnregisterPowerRequest();
	SpinOff();
	power = false;
}

public func OnEnoughPower()
{
	if (power) return _inherited(...);

	power = true;
	return _inherited(...);
}

public func OnNotEnoughPower()
{
	if (!power) return _inherited(...);

	power = false;
	SpinOff();
	return _inherited(...);
}

public func EjectWood()
{
	var wood = FindContents(Wood);
	if (!wood) return;

	wood->Exit(-25 * GetCalcDir(), -8, 30 - Random(59), -2 * GetCalcDir(), 1);
	Sound("Structures::EjectionPop");
	
	// Refresh interaction menus to show the wood count.
	UpdateInteractionMenus(this.GetInfoMenuEntries);
}

/*-- Animation --*/

private func SpinOn(int diff)
{
	var spin;
	var rotate = 0;
	if (diff == 0)
	{
		ClearScheduleCall(this, "SpinOff");
		Sound("Sawmill::EngineStart");
		SetMeshMaterial("Beltspin", 1);
	}
	if (diff > 20) rotate = Sin(70 * diff, 1) - 1;
	if (Inside(diff, 0, 20)) spin = 100;
	if (Inside(diff, 21, 40)) spin = 75;
	if (Inside(diff, 41, 60)) spin = 50;
	if (Inside(diff, 61, 80)) spin = 30;
	if (diff > 80)
	{
		spin = 30;
		rotate = 0;
		SetMeshMaterial("SawmillBlade.Spin", 2);
		running = true;
		Sound("Structures::SawmillRipcut", {loop_count = +1});
		Sound("Sawmill::EngineLoop", {loop_count = +1});
	}

	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(-20, 0, 1, 0), Trans_Rotate(rotate, 1, 0, 0)));
	SetAnimationPosition(this.SpinAnimation, Anim_Linear(GetAnimationPosition(this.SpinAnimation), 0, GetAnimationLength("work"), spin, ANIM_Loop));
}

private func SpinOff(int call)
{
	var spin;
	if (!call)
	{
		running = false;
		spin = 50;
		SetMeshMaterial("SawmillBlade", 2);
		Sound("Structures::SawmillRipcut", {loop_count = -1});
		SetProperty("MeshTransformation", Trans_Rotate(-20, 0, 1, 0));
	}
	if (call == 1) spin = 75;
	if (call == 2)
	{
		spin = 100;
		Sound("Sawmill::EngineLoop", {loop_count = -1});
		Sound("Sawmill::EngineStop");
	}
	if (call == 3) spin = 150;
	if (call == 4)
	{
		SetMeshMaterial("SawmillBelt", 1);
		SetAnimationPosition(this.SpinAnimation, Anim_Const(GetAnimationPosition(this.SpinAnimation)));
		return;
	}

	SetAnimationPosition(this.SpinAnimation, Anim_Linear(GetAnimationPosition(this.SpinAnimation), 0, GetAnimationLength("work"), spin, ANIM_Loop));

	ScheduleCall(this, "SpinOff", this.SpinStep * 2, nil, call+1);
}


/*-- Cable Network --*/

local cable_station;

public func AcceptsCableStationConnection() { return true; }

public func IsNoCableStationConnected() { return !cable_station; }

public func ConnectCableStation(object station)
{
	cable_station = station;
}

private func RequestTree()
{
	if (!cable_station)
		return;
	// Find a collectible tree on the network.
	var collectible_tree = cable_station->FindObjectOnNetworkCables(Find_And(Find_Func("IsTree"), Find_Not(Find_Property("is_being_cable_car_collected"))));
	if (!collectible_tree)
		return;
	// Find an available hoist in the network.
	var hoist = cable_station->FindCableCar(Find_And(Find_Not(Find_Func("GetAttachedVehicle")), Find_Not(Find_Property("is_busy_collecting_tree"))));
	if (!hoist)
		return;
	hoist->CreateEffect(Sawmill.FxCableCarCollectTree, 100, 1, collectible_tree, cable_station);
	return;
}

local FxCableCarCollectTree = new Effect
{
	Construction = func(array collectible_tree, object sawmill_station)
	{
		this.tree = collectible_tree[0];
		this.station_1 = collectible_tree[1];
		this.station_2 = collectible_tree[2];
		this.station_sawmill = sawmill_station;
		this.tree.is_being_cable_car_collected = true;
		Target.is_busy_collecting_tree = true;
		Target->SetDestination(this.station_1);
		return FX_OK;
	},
	Timer = func()
	{
		if (!this.station_1 || !this.station_2 || !this.station_sawmill)
			return FX_Execute_Kill;
		if (IsValueInArray(this.station_1->GetIdleCars(), Target))
			Target->SetDestination(this.station_2);
		if (IsValueInArray(this.station_2->GetIdleCars(), Target))
			Target->SetDestination(this.station_sawmill);	
		var tree = Target->FindObject(Target->Find_AtPoint(), Find_InArray([this.tree]));
		if (tree && !tree.has_been_picked_up)
		{
			this.tree.has_been_picked_up = true;
			this.tree->CreateEffect(Sawmill.FxCableCarRotateTree, 100, 1);
			Target->PickupVehicle(this.tree);
		}
		if (IsValueInArray(this.station_sawmill->GetIdleCars(), Target))
			return FX_Execute_Kill;
		return FX_OK;
	},
	Destruction = func()
	{
		//this.tree.is_being_cable_car_collected = false;
		Target.is_busy_collecting_tree = false;
		Target->DropVehicle();
	}
};

local FxCableCarRotateTree = new Effect
{
	Construction = func(int rotate_goal)
	{
		this.rotate_goal = 90;
		if (Target->GetR() < 0)
			this.rotate_goal = -90;
	},
	Timer = func()
	{
		var step = 3;
		var dr = this.rotate_goal - Target->GetR();
		if (Inside(dr, -step, step))
			return FX_Execute_Kill;
		Target->SetR(Target->GetR() + BoundBy(dr, -step, step));	
		return FX_OK;	
	}
};


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

func Definition(def) 
{
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2000, 0, 7000), Trans_Rotate(-20, 1, 0, 0),Trans_Rotate(30, 0, 1, 0)), def);
	return _inherited(def, ...);
}
local Name = "$Name$";
local Description = "$Description$";
local SpinStep = 30;
local ContainBlast = true;
local BlastIncinerate = 100;
local FireproofContainer = true;
local HitPoints = 70;
local Components = {Rock = 4, Wood = 1};