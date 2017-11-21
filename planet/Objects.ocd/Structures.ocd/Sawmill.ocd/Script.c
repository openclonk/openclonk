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
	// Only take one tree at a time
	if (!ContentsCount(Wood))
		FindTrees();
}

// Automatically search for trees in front of sawmill. Temporary solution?
private func FindTrees()
{
	var tree = FindObject(Find_AtPoint(), Find_Func("IsTree"), Find_Not(Find_Func("IsStanding")), Find_Func("GetComponent", Wood));
	if (!tree)
		return;
	
	return Saw(tree);
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