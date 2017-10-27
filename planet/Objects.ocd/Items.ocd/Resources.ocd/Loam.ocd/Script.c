/* Loam */

local loamused;       // amount of loam already used

protected func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d",graphic));
}

// Impact sound
func Hit()
{
	Sound("Hits::GeneralHit?");
}

public func RejectUse(object clonk)
{
	// Duplicate check from Clonk::Bridge. This was done instead of an additional CanBridge callback in the Clonk because that would most likely stay the only usecase of that callback.
	var proc = clonk->GetProcedure();
	return (proc != "WALK") && (proc != "SCALE");
}

// Item activation
func ControlUseStart(object clonk, int x, int y)
{
	if (!(clonk->~Bridge()))
	{
		clonk->CancelUse();
		return true;
	}
	// Add bridge effect and pass target coordinates.
	AddEffect("IntBridge", clonk, 1, 1, this, nil, x, y);
	// Make sure you can't turn the Clonk with left/right during construction.
	var dir = DIR_Left;
	if (x > 0) dir = DIR_Right;
	clonk->SetTurnForced(dir);
	return true;
}

func HoldingEnabled() { return true; }

func FxIntBridgeStart(object clonk, effect fx, int temp, int x, int y)
{
	if (temp)
		return FX_OK;
	// Drawing times.
	fx.Begin = 0;
	fx.Last = 0;
	// Last bridge coordinates.
	fx.LastX = GetX();
	fx.LastY = clonk->GetDefBottom() + 4;
	// Last points of the material.
	fx.last_rectangle_points = nil;
	// Target coordinates.
	fx.TargetX = x;
	fx.TargetY = y;
	// Dust particles that are used to hide that the material pops up out of nowhere.
	fx.particles = 
	{
		Prototype = Particles_Dust(),
		R = 200, G = 150, B = 50,
		Size = PV_KeyFrames(0, 0, 1, 100, 3, 1000, 2),
	};
	// Let the player know the clonk is about to do stuff
	clonk->PlayAnimation("Dig", CLONK_ANIM_SLOT_Movement, Anim_Linear(0,0, clonk->GetAnimationLength("Dig"), 35, ANIM_Loop), Anim_Const(1000));
	return FX_OK;
}

func FxIntBridgeTimer(object clonk, effect fx, int time)
{
	// something happened - don't try to dig anymore
	if (!(clonk->~IsBridging()))
	{
		clonk->CancelUse();
		return true;
	}

	// get global drawing coordinates
	var x = fx.TargetX + GetX();
	var y = fx.TargetY + GetY();

	// bridge speed: Build in smaller steps when briding upwards so Clonk moves up with bridge
	var min_dt = 3;
	if (fx.TargetY < -20 && !Abs(fx.TargetX*5/fx.TargetY))
		min_dt = 2;

	// bridge speed by dig physical
	var speed = clonk.ActMap.Dig.Speed * 2;

	// build bridge in chunks (for better angle precision)
	var dt = time - fx.Last;
	if (dt < min_dt) return FX_OK;
	fx.Last += dt;

	// draw loam (earth) line
	var line_wdt = 3;
	var line_len = speed * dt / 100;
	var last_x = fx.LastX;
	var last_y = fx.LastY;
	var dx = x-last_x, dy=y-last_y;
	var d = Distance(dx, dy);
	// Quantize angle as a multiple of 30 degrees.
	var quant = 30;
	var angle = Angle(0, 0, dx, dy);
	angle = angle + quant/2 - Sign(angle-quant/2)*((angle-quant/2) % quant);
	dx = Sin(angle, line_len);
	dy = -Cos(angle, line_len);

	// Don't use up loam if the mouse position is reached...
	// wait for the mouse being moved and then continue bridging
	// into that direction
	if(d <= 1) return FX_OK;

	// Calculate offset edges of rectangle (which are meant to be added to a side's midpoint).
	var off_x = Sin(angle + 90, line_wdt);
	var off_y = -Cos(angle + 90, line_wdt);
	
	// If we don't have a last position, initialize it so that it fits the angle best.
	// Otherwise, we already have an endpoint.
	if (fx.last_rectangle_points == nil)
	{
		fx.last_rectangle_points = [
			last_x + off_x,
			last_y + off_y,
			last_x - off_x,
			last_y - off_y
			];
	}
	
	// Calculate new points based on the last position.
	// Center of the new side.
	fx.LastX += dx;
	fx.LastY += dy;

	var new_rectangle_points = [
		fx.LastX + off_x,
		fx.LastY + off_y,
		fx.LastX - off_x,
		fx.LastY - off_y
		];

	DrawMaterialQuad("Earth-earth",
		fx.last_rectangle_points[0], fx.last_rectangle_points[1],
		new_rectangle_points[0], new_rectangle_points[1],
		new_rectangle_points[2], new_rectangle_points[3], 
		fx.last_rectangle_points[2], fx.last_rectangle_points[3],
		DMQ_Bridge);
	
	fx.last_rectangle_points = new_rectangle_points;
	
	// Some dust to hide the otherwise ugly construction.
	var local_x = fx.LastX - clonk->GetX();
	var local_y = fx.LastY - clonk->GetY();
	clonk->CreateParticle("SmokeDirty", PV_Random(local_x - 5, local_x + 5), PV_Random(local_y - 5, local_y + 5), PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(5, 10), fx.particles, 40); 
	
	// Clonk faces bridge direction.
	var view_direction = DIR_Left;
	if (local_x > 0)
		view_direction = DIR_Right;
	clonk->SetTurnForced(view_direction);
	
	// bridge time is up?
	loamused += Max(line_len, 1);
	if (loamused >= BridgeLength)
	{
		clonk->CancelUse();
	}
	else
	{
		// Update usage bar.
		clonk->~OnInventoryChange();
	}
	return FX_OK;
}

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	var effect = GetEffect("IntBridge", clonk);
	if (!effect)
		return true;
	// Update target coordinates in bridge effect.
	effect.TargetX = new_x;
	effect.TargetY = new_y;	
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	LoamDone(clonk);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	LoamDone(clonk);
	return true;
}

private func LoamDone(object clonk)
{
	// Get out of animation
	if (clonk->IsBridging())
	{
		clonk->SetAction("Walk");
		clonk->SetComDir(COMD_Stop);
	}
	// Remove Effect
	RemoveEffect("IntBridge", clonk);
	// Allow Clonk to turn freely again.
	clonk->SetTurnForced(-1);
	// Remove loam object if most of it has been consumed
	if (loamused > BridgeLength - 10)
		RemoveObject();
	return;
}

// Do not put used loam on top of fresh loam in an inventory menu.
public func CanBeStackedWith(object other)
{
	if (this.loamused != other.loamused) return false;
	return inherited(other, ...);
}

// Display the usage as a bar over the loam icon.
public func GetInventoryIconOverlay()
{
	if (!this.loamused) return nil;

	var percentage = 100 - 100 * this.loamused / this.BridgeLength;
	
	// Overlay a usage bar.
	var overlay = 
	{
		Bottom = "0.75em", Margin = ["0.1em", "0.25em"],
		BackgroundColor = RGB(0, 0, 0),
		margin = 
		{
			Margin = "0.05em",
			bar = 
			{
				BackgroundColor = RGB(200, 150, 0),
				Right = Format("%d%%", percentage),
			}
		}
	};
	
	return overlay;
}

public func IsFoundryProduct() { return true; }
public func GetSubstituteComponent(id component) // Can be made from earth or sand
{
	if (component == Earth)
		return Sand;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BridgeLength = 37; // bridge length in pixels
local Plane = 470;
local Components = {Earth = 2, Water = 60};
