/**
	Airplane
	Acrobatic air-vehicle. Capable of firing lead shot.
	
	@author: Ringwaul, Clonkonaut, Maikel
*/

// Airplane is destructible.
#include Library_Destructible

local dir = DIR_Left;

local prop_speed, prop_speed_target, prop_speed_timer; // current and target propeller speed [0, 100]

local pilot;
local clonkmesh;

// What happens on mouse left
local main_action = "Cannon"; // default: fire bullets

// All available click actions
local action_list = ["Cannon", "Rocket", "Bomb", "Spray", "Balloon", "Drop", "Afterburner"];


/*-- Engine Callbacks --*/

public func Construction()
{
	SetR(-90);
}

public func Initialize()
{
	CreateEffect(FxControlPlaneFlight, 1, 1);
	SetAction("Land");
}

public func RejectCollect(id def, object obj)
{
	var contents_count = ObjectCount(Find_Container(this), Find_Not(Find_OCF(OCF_CrewMember)));
	if (contents_count >= MaxContentsCount)
		return true;
	return false;
}

public func ActivateEntrance(object clonk)
{
	if (clonk->Contained() == this)
		return clonk->Exit();

	// Clonks cannot get into the plane if it is underwater
	if (GBackLiquid()) return false;

	var passengers = ObjectCount(Find_Container(this), Find_OCF(OCF_CrewMember));
	if (passengers >= MaxPassengerCount) return false;

	clonk->Enter(this);
	clonk->SetAction("Walk");
	clonk->PlayAnimation("Drive", CLONK_ANIM_SLOT_Movement, Anim_Const(10), Anim_Const(1000));
	ShowMainActionTo(clonk);
}

public func Ejection(object obj)
{
	if (pilot && obj == pilot)
		PlaneDismount(obj);
	if (obj->Contained())
		Exit();
	obj->SetSpeed(this->GetXDir(), this->GetYDir());
}


/*-- Callbacks --*/

public func IsContainer() { return true; }

public func IsProjectileTarget(object projectile, object shooter) { return true; }


/*-- Interface --*/

//Quick command for scenario designers. The plane starts facing right instead of left.
public func FaceRight()
{
	SetR(90);
	RollPlane(1, true);
}

public func FaceLeft()
{
	SetR(-90);
	RollPlane(0, true);
}


/*-- Interaction --*/

// Provides an own interaction menu.
public func HasInteractionMenu() { return true; }

// Show settings in interaction menu
public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];

	// Do not show the menu if flying and if the clonk is not the pilot.
	if (GetAction() == "Fly" && clonk != pilot)
		return menus;

	var plane_menu =
	{
		title = "$AirplaneSettings$",
		entries_callback = this.GetAirplaneMenuEntries,
		callback = nil,
		callback_hover = "OnSettingsHover",
		callback_target = this,
		BackgroundColor = RGB(0, 0, 50),
		Priority = 20
	};
	PushBack(menus, plane_menu);

	return menus;
}

public func GetAirplaneMenuEntries(object clonk)
{
	var control_prototype =
	{
		Bottom = "2em",
		BackgroundColor = { Std = 0, Hover = RGB(100, 30, 30) },
		OnMouseOut = GuiAction_SetTag("Std"),
		OnMouseIn = GuiAction_SetTag("Hover"),
		text = { Left = "2em" },
		icon = { Right = "2em" }
	};

	var menu_entries = [];

	// main action (= left click)
	var main_action_settings = {
		Bottom = "2em",
		settings = {
			Prototype = control_prototype,
			Priority = 1000,
			Tooltip = GetActionTypeTooltip(main_action),
			OnClick = GuiAction_Call(this, "ToggleMainAction", clonk),
			text =
				{ Prototype = control_prototype.text,
				  Style = GUI_TextVCenter,
				  Text = Format("$SwitchMainAction$\n%s", GetPlayerControlAssignment(clonk->GetOwner(), CON_Use, true, true), GetActionTypeName(main_action))
				},
			icon =
				{ Prototype = control_prototype.icon,
				  Symbol = GetActionTypeInfo(main_action)
				}
		}
	};

	PushBack(menu_entries, { symbol = this, extra_data = "MainAction", custom = main_action_settings });

	return menu_entries;
}

public func OnSettingsHover(symbol, extra_data, desc_menu_target, menu_id)
{
	if (symbol == nil) return;

	var text = "";
	if (extra_data == "MainAction")
	{
		if (main_action == "Cannon")
			text = Format("$BulletInfo$", GetBulletAmount());
		if (main_action == "Rocket")
			text = Format("$RocketInfo$", GetRocketAmount());
		if (main_action == "Bomb")
			text = Format("$BombInfo$", GetBombAmount());
		if (main_action == "Spray")
			text = Format("$SprayInfo$", GetBarrelAmount());
		if (main_action == "Balloon")
			text = Format("$BalloonInfo$", GetBalloonAmount());
		if (main_action == "Drop")
			text = Format("$DropInfo$");
		if (main_action == "Afterburner")
			text = Format("$AfterburnerInfo$");
		if (text == "")
			text = "$Description$";
	}

	GuiUpdate({ Text = text }, menu_id, 1, desc_menu_target);
}

private func GetActionTypeTooltip(string action)
{
	// Show next firing mode
	var position = GetIndexOf(action_list, action);
	if (position == -1) return ""; // ??

	if (position == GetLength(action_list)-1)
		position = 0;
	else
		position++;

	return Format("$SwitchTo$", GetActionTypeName(action_list[position]));
}

private func GetActionTypeName(string action)
{
	if (action == "Cannon")
		return "$Bullet$";
	if (action == "Rocket")
		return "$Rocket$";
	if (action == "Bomb")
		return "$Bomb$";
	if (action == "Spray")
		return "$Spray$";
	if (action == "Balloon")
		return "$Balloon$";
	if (action == "Drop")
		return "$Drop$";
	if (action == "Afterburner")
		return "$Afterburner$";
}

private func GetActionTypeInfo(string action)
{
	if (action == "Cannon")
		return LeadBullet;
	if (action == "Rocket")
		return Boompack;
	if (action == "Bomb")
		return IronBomb;
	if (action == "Spray")
		return Barrel;
	if (action == "Balloon")
		return Balloon;
	if (action == "Drop")
		return Icon_LetGo;
	if (action == "Afterburner")
		return Icon_Flame;
}

public func ToggleMainAction(object clonk)
{
	var current_action = GetIndexOf(action_list, main_action);
	if (current_action == -1)
		return; // ??

	if (current_action == GetLength(action_list)-1)
		current_action = 0;
	else
		current_action++;

	main_action = action_list[current_action];

	UpdateInteractionMenus(this.GetAirplaneMenuEntries);

	if (clonk)
	{
		Sound("UI::Click2", false, nil, clonk->GetOwner());
		ShowMainActionTo(clonk);
	}
}


/*-- Usage --*/

// Main action
public func ContainedUseStart(object clonk, int x, int y)
{
	var call = Format("Use%s", main_action);
	return this->Call(call, clonk, x, y);
}

public func ContainedUseStop(object clonk, int x, int y)
{
	var call = Format("Cancel%s", main_action);
	return this->Call(call, clonk, x, y);
}

public func ContainedUseCancel(object clonk, int x, int y)
{
	var call = Format("Cancel%s", main_action);
	return this->Call(call, clonk, x, y);
}

// Alt Use: toggle firing mode
public func ContainedUseAltStart(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	ToggleMainAction(clonk);
	return true;
}

public func ContainedUseAltStop(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	return true;
}

public func ContainedUseAltCancel(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	return true;
}

// Starting the plane.
public func ContainedUp(object clonk)
{
	if (pilot)
	{
		// For safety, check if the pilot is dead (which is never particularly good).
		// During flight, pilot's health is constantly checked by the flying effect.
		if (!pilot->GetAlive())
			PlaneDismount(pilot);
		else if (clonk != pilot)
			return false;
	}
	// Start engine.
	if (clonk && GetAction() == "Land")
	{
		if (!pilot)
			PlaneMount(clonk);
		StartFlight(15);
		return true;
	}
	return false;
}

// Stopping the plane.
public func ContainedDown(object clonk)
{
	// Shut down engine.
	if (GetAction() == "Fly" && clonk == pilot)
	{
		CancelFlight();
		return true;
	}
	if (pilot)
	{
		if (!pilot->GetAlive())
			PlaneDismount(pilot);
		else if (clonk != pilot)
			return false;
	}
	// Allow reverse pilot.
	if (clonk && GetAction() == "Land")
	{
		if (!pilot)
			PlaneMount(clonk);
		StartFlight(-5);
		return true;
	}
	return false;
}

// Steering.
public func ContainedLeft(object clonk)
{
	if (clonk != pilot)
		return false;
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
		ctrl_fx->SetRotationDir(-1);
	return true;
}

public func ContainedRight(object clonk)
{
	if (clonk != pilot)
		return false;
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
		ctrl_fx->SetRotationDir(1);
	return true;
}

public func ContainedStop(object clonk)
{
	if (clonk != pilot)
		return false;
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
		ctrl_fx->SetRotationDir(0);
	return true;
}


/*-- Control --*/

public func GetFacingDir()
{
	return dir;
}

public func ControlLeft(object clonk)
{
	if (Inside(GetR(), 80, 100) && dir == DIR_Right)
		FaceLeft();
	return false;
}

public func ControlRight(object clonk)
{
	if (Inside(GetR(), -100, -80) && dir == DIR_Left)
		FaceRight();
	return false;
}


/*-- Bullet firing --*/

public func UseCannon(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetBulletAmount())
	{
		CustomMessage("$NoShots$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
		return true;
	}
	AddEffect("FireBullets", this, 100, 12, this);
	return true;
}

public func CancelCannon(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (GetEffect("FireBullets", this))
		RemoveEffect("FireBullets", this);

	return true;
}

public func GetBulletAmount()
{
	var shots = 0;
	var bullets = FindObjects(Find_Container(this), Find_Func("IsBullet"));
	for (var bullet in bullets)
	{
		if (bullet->~GetStackCount())
			shots += bullet->GetStackCount();
		else
			shots++;
	}
	return shots;
}

public func FxFireBulletsStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	effect.reticle = CreateObject(GUI_Reticle);
	effect.reticle->SetOwner(this->GetController());
	effect.reticle->SetAction("Show", this);
	var ammo = FindObject(Find_Container(this), Find_Func("IsBullet"));
	if (!ammo)
		return FX_Execute_Kill;
	FireBullet(ammo);
	return FX_OK;
}

public func FxFireBulletsTimer(object target, proplist effect, int time)
{
	var ammo = FindObject(Find_Container(this), Find_Func("IsBullet"));
	if (!ammo)
		return FX_Execute_Kill;
	FireBullet(ammo);
	return FX_OK;
}

public func FxFireBulletsStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	if (effect.reticle)
		effect.reticle->RemoveObject();
	return FX_OK;
}

public func FireBullet(object ammo)
{
	var shot = ammo->TakeObject();
	var angle = this->GetR();
	shot->Launch(this, angle, 35, 200);
	Sound("Objects::Weapons::Blunderbuss::GunShoot?");

	// Muzzle Flash & gun smoke
	var IX = Sin(GetR(), 30);
	var IY = -Cos(GetR(), 30);

	var x = Sin(angle, 20);
	var y = -Cos(angle, 20);
	CreateParticle("Smoke", IX, IY, PV_Random(x - 20, x + 20), PV_Random(y - 20, y + 20), PV_Random(40, 60), Particles_Smoke(), 20);

	CreateMuzzleFlash(IX, IY, angle, 20);
	CreateParticle("Flash", 0, 0, GetXDir(), GetYDir(), 8, Particles_Flash());
}


/*-- Rocket firing --*/

public func UseRocket(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetRocketAmount())
	{
		CustomMessage("$NoRockets$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
	}

	return true;
}

public func CancelRocket(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	var rocket = FindObject(Find_Container(this), Find_ID(Boompack));
	if (!rocket)
		return true;
	FireRocket(rocket, x, y);

	return true;
}

public func GetRocketAmount()
{
	return ContentsCount(Boompack);
}

public func FireRocket(object rocket, int x, int y)
{
	var launch_x = Cos(GetR() - 180 * (1 - dir), 10);
	var launch_y = Sin(GetR() - 180 * (1 - dir), 10);
	rocket->Exit(launch_x, launch_y, GetR(), GetXDir(), GetYDir());
	rocket->Launch(GetR(), nil, this);
	var fx = AddEffect("IntControlRocket", rocket, 100, 1, this);
	fx.x = GetX() + x;
	fx.y = GetY() + y;
	rocket->SetDirectionDeviation(0);
}

public func FxIntControlRocketTimer(object target, effect fx, int time)
{
	// Remove gravity on rocket.
	target->SetYDir(target->GetYDir(100) - GetGravity(), 100);
	// Adjust angle to target.
	var angle_to_target = Angle(target->GetX(), target->GetY(), fx.x, fx.y);
	var angle_rocket = target->GetR();
	if (angle_rocket < 0)
		angle_rocket += 360;
	var angle_delta = angle_rocket - angle_to_target;
	if (Inside(angle_delta, -3, 3))
		return FX_OK;
	if (Inside(angle_delta, 0, 180) || Inside(angle_delta, -360, -180))
		target->SetR(target->GetR() - 5);
	else if (Inside(angle_delta, -180, 0) || Inside(angle_delta, 180, 360))
		target->SetR(target->GetR() + 5);
	return FX_OK;
}


/*-- Bombing --*/

public func UseBomb(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetBombAmount())
	{
		CustomMessage("$NoBombs$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
	}

	return true;
}

public func CancelBomb(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	var bomb = FindObject(Find_Container(this), Find_ID(IronBomb));
	if (!bomb)
		bomb = FindObject(Find_Container(this), Find_ID(Dynamite));
	if (!bomb)
	{
		bomb = FindObject(Find_Container(this), Find_ID(DynamiteBox), Find_Func("GetDynamiteCount"));
		if (bomb)
			bomb = bomb->Contents();
	}
	if (!bomb)
		return true;

	DropBomb(bomb);

	return true;
}

public func GetBombAmount()
{
	var bombs = ObjectCount(Find_Container(this), Find_Or(Find_ID(IronBomb), Find_ID(Dynamite)));
	var boxes = FindObjects(Find_Container(this), Find_ID(DynamiteBox));
	for (var box in boxes)
		bombs += box->GetDynamiteCount();
	return bombs;
}

private func DropBomb(object bomb)
{
	if (!bomb) return;

	bomb->Exit();
	bomb->SetPosition(GetX(), GetY() + 12);
	bomb->SetXDir(GetXDir());
	bomb->SetYDir(GetYDir());
	bomb->Fuse(true); // fuse and explode on hit
}


/*-- Liquid spray --*/

public func UseSpray(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetBarrelAmount())
	{
		CustomMessage("$NoBarrels$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
	}

	return true;
}

public func CancelSpray(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	var barrels = FindObjects(Find_Container(this), Find_Or(Find_ID(Barrel), Find_ID(MetalBarrel)));
	var barrel;
	for (var b in barrels)
		if (GetLength(b->GetLiquidContents()))
		{
			barrel = b;
			break;
		}

	if (!barrel) return true;

	SprayLiquid(barrel, clonk);

	return true;
}

public func GetBarrelAmount()
{
	var barrels = FindObjects(Find_Container(this), Find_Or(Find_ID(Barrel), Find_ID(MetalBarrel)));
	var count = 0;
	for (var barrel in barrels)
		if (GetLength(barrel->GetLiquidContents()))
			count++;
	return count;
}

private func SprayLiquid(object barrel, object clonk)
{
	if (!barrel) return;

	var added_r = -60;
	if (dir == DIR_Right)
		added_r = 60;
	barrel->EmptyBarrel(GetR() + added_r, 50, clonk);
	Sound("Liquids::Splash1");
}


/*-- Parachuting --*/

public func UseBalloon(object clonk, int x, int y)
{
	if (!GetBalloonAmount())
	{
		CustomMessage("$NoBalloons$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
	}

	return true;
}

public func CancelBalloon(object clonk, int x, int y)
{
	if (!clonk->Contained()) return false;

	var balloon = FindObject(Find_Container(this), Find_ID(Balloon));
	if (!balloon)
		return true;

	Parachute(balloon, x, y, clonk);

	return true;
}

public func GetBalloonAmount()
{
	return ContentsCount(Balloon);
}

private func Parachute(object balloon, int x, int y, object clonk)
{
	if (!balloon || !clonk) return;

	// The balloon has to enter the clonk
	if (!balloon->Enter(clonk))
	{
		// Maybe the clonk should just drop an object?
		return CustomMessage("$NoSpaceInInventory$", this, clonk->GetOwner());
	}
	clonk->Exit();
	balloon->ControlUseStart(clonk);
}


/*-- Object dropping --*/

public func UseDrop(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetLength(GetNonClonkContents()))
	{
		CustomMessage("$NoObjects$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
	}

	return true;
}

public func CancelDrop(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	var object = GetNonClonkContents()[0];
	if (!object)
		return true;

	DropObject(object, x, y, clonk);

	return true;
}

public func GetNonClonkContents()
{
	return FindObjects(Find_Container(this), Find_Not(Find_OCF(OCF_CrewMember)));
}

private func DropObject(object to_drop, int x, int y, object clonk)
{
	if (!to_drop) return;

	to_drop->Exit();
	to_drop->SetXDir(GetXDir());
	to_drop->SetYDir(GetYDir());

	if (clonk)
		to_drop->SetController(clonk->GetOwner());
}


/*-- Afterburner --*/

public func UseAfterburner(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	if (!GetOilBarrel())
	{
		CustomMessage("$NoOil$", this, clonk->GetOwner());
		Sound("Objects::Weapons::Blunderbuss::Click?");
		return true;
	}
	CreateEffect(FxAfterburner, 100, 2);
	return true;
}

public func CancelAfterburner(object clonk, int x, int y)
{
	if (clonk != pilot)
		return false;

	var fx = GetEffect("FxAfterburner", this);
	if (fx)
		fx->Remove();
	return true;
}

public func GetOilBarrel()
{
	var barrels = FindObjects(Find_Container(this), Find_Or(Find_ID(Barrel), Find_ID(MetalBarrel)));
	for (var barrel in barrels)
		if (barrel->GetLiquidAmount("Oil") > 0)
			return barrel;
	return;
}

public func IsUsingAfterburner()
{
	return !!GetEffect("FxAfterburner", this);
}

local FxAfterburner = new Effect
{
	Construction = func()
	{
		Target->Sound("Objects::Boompack::Fly", 0, 40, nil, 1);
	},	
	Timer = func(int time)
	{
		var barrel = Target->GetOilBarrel();
		if (!barrel)
			return FX_Execute_Kill;
		if (barrel->GetLiquidAmount("Oil") <= 0)
			return FX_Execute_Kill;
		barrel->RemoveLiquid("Oil", 1);
		return FX_OK;
	},	
	Destruction = func()
	{
		Target->Sound("Objects::Boompack::Fly", 0, 40, nil, -1);
	}
};


/*-- Movement --*/

public func StartFlight(int new_throttle)
{
	SetPropellerSpeedTarget(100);
	SetAction("Fly");
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
		ctrl_fx.throttle = new_throttle;
	return;
}

public func StartInstantFlight(int angle, int new_throttle)
{
	if (angle < 0)
		angle += 360;
	if (angle < 180)
		angle -= 10;
	else
		angle += 10;
	SetPropellerSpeed(100);
	SetAction("Fly");
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
	{
		ctrl_fx.throttle = new_throttle;
		ctrl_fx.thrust = new_throttle;
	}
	SetR(angle);
	SetXDir(Sin(angle, new_throttle));
	SetYDir(-Cos(angle, new_throttle));
	return;
}

public func CancelFlight()
{
	SetPropellerSpeedTarget(0);
	SetAction("Land");
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
	{
		ctrl_fx.rdir = 0;
		ctrl_fx.throttle = 0;	
	}
	return;
}

public func GetPlaneControl()
{
	return GetEffect("FxControlPlaneFlight", this);
}

public func RemovePlaneControl()
{
	var ctrl_fx = GetPlaneControl();
	if (ctrl_fx)
		ctrl_fx->Remove();
	return;
}

// Effect that controls the plane movement and related stuff.
local FxControlPlaneFlight = new Effect
{
	Construction = func()
	{
		this.thrust = 0;
		this.throttle = 0;
		this.lift = 0;
		this.rdir = 0;
		this.propanim = Target->PlayAnimation("Propellor", 15, Anim_Const(0));
	},	
	Destruction = func()
	{
		Target->StopAnimation(this.propanim);
	},	
	SetRotationDir = func(int new_dir)
	{
		this.rdir = new_dir;
	},	
	Timer = func(int time)
	{
		// Perform flying actions.	
		if (Target->GetAction() == "Fly")
		{
			this->DoPlaneRotation();		
			this->DoEngineSmoke();
			this->StirUpLiquids();
		}
		// Perform landing actions.
		else
		{
			this->LandPlane();		
		}
		// Perform plane movements.
		this->ApplyThrust(time);
		this->ApplyDrag();
		this->PropellorAnim();
		this->PerformSafetyChecks();
		return FX_OK;
	},
	DoPlaneRotation = func()
	{
		// Perform plane rotation.
		var max_rdir = 7;
		if (Target->IsUsingAfterburner())
			max_rdir = 9;
		var current_rdir = Target->GetRDir();
		var new_rdir = current_rdir + 3 * this.rdir;
		if (this.rdir == 0)
			new_rdir = 0;
		new_rdir = BoundBy(new_rdir, -max_rdir, max_rdir);
		Target->SetRDir(new_rdir);
		// Roll plane to movement direction.
		if (this.throttle > 0)
		{
			if (Target->GetXDir() > 10 && Target->GetFacingDir() != DIR_Right)
				Target->RollPlane(1);
			if (Target->GetXDir() < -10 && Target->GetFacingDir() != DIR_Left)
				Target->RollPlane(0);
		}	
	},
	// Perform engine smoke.
	DoEngineSmoke = func()
	{
		var colour = 255 - (Target->GetDamage() * 3);
		var min = PV_Random(7, 20);
		var max = PV_Random(40, 70);
		var rot = Target->GetR();
		if (Target->GetDamage() >= Target.HitPoints / 2)
		{
			min = PV_Random(20, 30);
			max = PV_Random(70, 100);
		}
		var particles = 
		{
			Prototype = Particles_Smoke(),
			R = colour, G = colour, B = colour,
			Size = PV_Linear(min, max)
		};
		Target->CreateParticle("Smoke", -Sin(rot, 10), Cos(rot, 10), 0, 0, PV_Random(36, 2 * 36), particles, 1);
		// Add fire if after burner is turned on.
		if (Target->IsUsingAfterburner())
		{
			var particle_fire = Particles_Fire();
			particle_fire.Size = PV_KeyFrames(0, 0, PV_Random(6, 8), 500, 4, 1000, 0);
			Target->CreateParticle("Fire", -Sin(rot, 10), Cos(rot, 10), PV_Random(-1, 1), PV_Random(-1, 1), 20 + Random(10), particle_fire, 1);
		}	
	},
	// Stirs up liquids if flying above a liquid.
	StirUpLiquids = func()
	{
		if (Target->GBackLiquid(0, 40))
		{
			var surface = 40;
			while (surface && Target->GBackSemiSolid(0, surface))
				surface--;
			if (surface > 0 && Target->GBackLiquid(0, surface + 1))
			{
				var phases = PV_Linear(0, 7);
				if (Target->GetFacingDir() == DIR_Left)
					phases = PV_Linear(8, 15);
				var color = GetAverageTextureColor(Target->GetTexture(0, surface + 1));
				var particles =
				{
					Size = 16,
					Phase = phases,
					CollisionVertex = 750,
					OnCollision = PC_Die(),
					R = (color >> 16) & 0xff,
					G = (color >> 8) & 0xff,
					B = (color >> 0) & 0xff,
					Attach = ATTACH_Front,
				};
				Target->CreateParticle("Wave", 0, surface, (RandomX(-5, 5) - (-1 + 2 * Target->GetFacingDir()) * 6) / 4, 0, 16, particles);
			}
		}
	},	
	// Lands the plane.
	LandPlane = func()
	{
		// If under or on water, turn upright.
		if (Target->GBackLiquid() || Target->GBackLiquid(0, 21))
		{
			if (Target->GetR() < 0)
			{
				if (!Inside(Target->GetR(), -95, -85))
				{
					Target->SetRDir((90 + Target->GetR()) / Abs(90 + Target->GetR()) * -3);
					Target->RollPlane(0);
				}
			}
			else if (!Inside(Target->GetR(), 85, 95))
			{
				Target->SetRDir((90 - Target->GetR()) / Abs(90 - Target->GetR()) * 3);
				Target->RollPlane(1);
			}				
			// Also: slow down because the plane tends to slide across water.
			if (Target->GetXDir() > 0)
				Target->SetXDir(Target->GetXDir() - 1);
			else if (Target->GetXDir() < 0)
				Target->SetXDir(Target->GetXDir() + 1);
		}
		// Decrease thrust rapidly.
		if (this.thrust > 0)
			this.thrust--;
		if (this.thrust < 0)
			this.thrust++;	
	},	
	ApplyThrust = func(int time)
	{
		// Determine airplane lift.
		this.lift = Distance(0, 0, Target->GetXDir(), Target->GetYDir()) / 2;
		if (this.lift > 20)
			this.lift = 20;
		if (this.throttle < 1)
			this.lift = 0;
		// Modify throttle if afterburner is being used.
		var mod_throttle = this.throttle;
		if (Target->IsUsingAfterburner())
			mod_throttle = 3 * mod_throttle / 2;
		// Throttle-to-thrust lag
		if (time % 10 == 0)
		{
			if (mod_throttle > this.thrust)
				++this.thrust;
			if (mod_throttle < this.thrust)
				--this.thrust;
		}
		// Set airplane velocity.
		Target->SetXDir(Sin(Target->GetR(), this.thrust) + Target->GetXDir(100), 100);
		Target->SetYDir(-Cos(Target->GetR(), this.thrust) + Target->GetYDir(100) - this.lift, 100);
	},
	// Applies drag to the airplane to ensure a maximum speed.
	ApplyDrag = func()
	{
		var maxspeed = 40;
		if (Target->IsUsingAfterburner())
			maxspeed += 18;
		var speed = Distance(0, 0, Target->GetXDir(), Target->GetYDir());
		if (speed > maxspeed)
		{
			Target->SetXDir(Target->GetXDir(100) * maxspeed / speed, 100);
			Target->SetYDir(Target->GetYDir(100) * maxspeed / speed, 100);
		}
	},
	PropellorAnim = func()
	{
		var change = Target->GetAnimationPosition(this.propanim) + this.thrust * 3;
		if (change > Target->GetAnimationLength("Propellor"))
			change = (Target->GetAnimationPosition(this.propanim) + this.thrust * 3) - Target->GetAnimationLength("Propellor");
		if (change < 0)
			change = (Target->GetAnimationLength("Propellor") - this.thrust * 3);
		Target->SetAnimationPosition(this.propanim, Anim_Const(change));
	},
	PerformSafetyChecks = func()
	{
		// Check for pilot.
		if (!Target->GetPilot() && this.throttle != 0)
			Target->CancelFlight();
		if (Target->GetPilot() && !Target->GetPilot()->GetAlive())
			Target->PlaneDismount(Target->GetPilot());
	
		// Planes cannot fly underwater!
		if (Target->GBackLiquid())
		{
			if (Target->GetPilot())
				Target->Ejection(Target->GetPilot());
			if (this.throttle != 0)
				Target->CancelFlight();
		}
	}
};

public func RollPlane(int rolldir, bool instant)
{
	if (dir != rolldir)
	{
		var i = 36;
		if (instant)
			i = 1;
		PlayAnimation(Format("Roll%d",rolldir), 10, Anim_Linear(0, 0, GetAnimationLength(Format("Roll%d", rolldir)), i, ANIM_Hold));
		dir = rolldir;
	}
}


/*-- Piloting --*/

public func PlaneMount(object clonk)
{
	pilot = clonk;
	SetOwner(clonk->GetController());
	clonk->PlayAnimation("Stand", 15, nil, Anim_Const(1000));
	clonkmesh = AttachMesh(clonk, "pilot", "skeleton_body", Trans_Mul(Trans_Rotate(180, 1, 0, 0), Trans_Translate(-3000, 1000, 0)), AM_DrawBefore);
	return true;
}

public func PlaneDismount(object clonk)
{
	clonk->StopAnimation(clonk->GetRootAnimation(15));
	DetachMesh(clonkmesh);
	clonkmesh = nil;
	// Ensure the current use is cancelled.
	if (clonk == pilot)
		ContainedUseCancel(pilot, 0, 0);
	pilot = nil;
	CancelFlight();
	return true;
}

public func GetPilot()
{
	return pilot;
}


/*-- Effects --*/

// Instantly set new propeller speed
public func SetPropellerSpeed(int new_speed)
{
	if (prop_speed_timer)
	{
		RemoveTimer(this.PropellerSpeedTimer);
		prop_speed_timer = false;
	}
	return SetPropellerSound(prop_speed = prop_speed_target = new_speed);
}

// Schedule fading to new propeller speed
public func SetPropellerSpeedTarget(int new_speed_target)
{
	prop_speed_target = new_speed_target;
	if (!prop_speed_timer)
		prop_speed_timer = AddTimer(this.PropellerSpeedTimer, 10);
	return true;
}

// Execute fading to new propeller speed
public func PropellerSpeedTimer()
{
	prop_speed = BoundBy(prop_speed_target, prop_speed - 10, prop_speed + 4);
	if (prop_speed == prop_speed_target)
	{
		RemoveTimer(this.PropellerSpeedTimer);
		prop_speed_timer = false;
	}
	return SetPropellerSound(prop_speed);
}

// Set propeller speed sound. 0 = off, 100 = max speed.
private func SetPropellerSound(int speed)
{
	if (speed <= 0)
		return Sound("Objects::Plane::PropellerLoop", 0, 100, nil, -1);
	else
		return Sound("Objects::Plane::PropellerLoop", 0, 100, nil, 1, 0, (speed - 100) * 2 / 3);
}


/*-- Destruction --*/

// Destroyed by any type of damage.
public func IsDestroyedByExplosions() { return false; }

// Custom explosion on callback from destructible library.
public func OnDestruction(int change, int cause, int by_player)
{
	if (pilot)
		PlaneDismount(pilot);
	SetController(by_player);
	PlaneDeath();
	return true;
}

private func PlaneDeath()
{
	while (Contents(0))
		Contents(0)->Exit();
	Explode(36);
}

// Inflict damage when hitting something with the plane and already damaged.
public func Hit(int xdir, int ydir)
{
	var remaining_hp = this.HitPoints - GetDamage();
	if (remaining_hp < 10)
	{
		var speed = Distance(0, 0, xdir, ydir) / 10;
		if (speed > 4 * remaining_hp)
			DoDamage(speed / 6, FX_Call_DmgScript, GetController());
	}
}


/*-- Production --*/

public func IsVehicle() { return true; }
public func IsShipyardProduct() { return true; }


/*-- Display --*/

public func ShowMainActionTo(object clonk)
{
	CustomMessage(Format("$SelectedAction$", GetActionTypeName(main_action), GetActionTypeInfo(main_action)), this, clonk->GetOwner());
}

public func Definition(proplist def)
{
	def.MeshTransformation = Trans_Mul(Trans_Rotate(90, 0, 0, 1), Trans_Translate(-1000, -3375, 0), Trans_Rotate(25, 0, 1, 0));
	def.PictureTransformation = Trans_Mul(Trans_Rotate(-5, 1, 0, 0), Trans_Rotate(40, 0, 1, 0), Trans_Translate(-20000, -4000, 20000));
	return _inherited(def, ...);
}


/*-- Properties --*/

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 0,
		Length = 10,
		Delay = 1,
		X = 0,
		Y = 0,
		Wdt = 40,
		Hgt = 56,
		NextAction = "Fly",
	},
	Land = {
		Prototype = Action,
		Name = "Land",
		Procedure = DFA_NONE,
		Directions = 2,
		FlipDir = 0,
		Length = 1,
		Delay = 2,
		X = 0,
		Y = 0,
		Wdt = 40,
		Hgt = 56,
		NextAction = "Land",
	},
};

local Name="$Name$";
local Description="$Description$";
local BorderBound = C4D_Border_Sides | C4D_Border_Top | C4D_Border_Bottom;
local HitPoints = 50;
local MaxContentsCount = 20;
local MaxPassengerCount = 3;
local Components = {Metal = 6, Wood = 4};
local Touchable = 1;