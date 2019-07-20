/*
	Crosshair
	Author: Newton
	
	Virtual cursor for gamepad controls
*/

local crew, angle, xpos, ypos, aiming, menu;

static const CURSOR_Radius = 100;
// This is supposed to be a constant, but C4Script doesn't allow constant expressions there.
private func CURSOR_Deadzone() { return PLRCON_MaxStrength / 5; }

protected func Initialize()
{
	SetVisibility(false);
	xpos = ypos = 0;
	aiming = false;
}

public func FxMoveTimer()
{
	if (!crew)
	{
		RemoveObject();
		return FX_Execute_Kill;
	}

	var target_angle = Angle(0, 0, xpos, ypos)*10;

	if (!Visible() && !InDeadzone())
	{
		// The player moved the aiming stick while the crosshair wasn't visible: Use angle directly.
		angle = target_angle;
		SetVisibility(true);
	}
	else if (!InDeadzone())
	{
		// Smooth small movements of the stick while the crosshair is visible.
		var angle_diff = Normalize(target_angle - angle, -1800, 10);
		if (Abs(angle_diff) < 450)
			angle = angle + angle_diff / 8;
		else
			angle = target_angle;
	}
	else if (!aiming)
	{
		// The player doesn't touch the stick and no item is using the crosshair right now.
		SetVisibility(false);
		// Aim somewhere useful. Note that this can be overwritten by objects and isn't used for throwing.
		angle = 800*(crew->GetDir()*2-1);
	}
	
	UpdatePosition();
	crew->TriggerHoldingControl();
}

private func AnalogStrength() { return BoundBy(Sqrt(xpos*xpos + ypos*ypos), 0, PLRCON_MaxStrength); }
private func InDeadzone() { return AnalogStrength() < CURSOR_Deadzone(); }
private func Visible() { return this.Visibility != VIS_None; }

// Updates the visibility, returing true if it was changed.
private func SetVisibility(bool visible)
{
	var newvis, oldvis;
	if (visible)
		newvis = VIS_Owner;
	else
		newvis = VIS_None;
	oldvis = this.Visibility;
	this.Visibility = newvis;
	return newvis != oldvis;
}

private func CreateMoveEffect(object clonk)
{
	crew = clonk;
	UpdatePosition();
	RemoveEffect("Move",this);
	AddEffect("Move",this, 1, 1, this);
}

public func StartAim(object clonk, int default_angle, object GUImenu)
{
	aiming = true;

	// gui or landscape mode:
	if (GUImenu)
	{
		SetCategory(C4D_StaticBack | C4D_IgnoreFoW | C4D_Foreground | C4D_Parallax);
		menu = GUImenu;
		this["Parallaxity"] = [0, 0];
	}
	else
	{
		SetCategory(C4D_StaticBack | C4D_IgnoreFoW);
		menu = nil;
	}

	// Use the given angle if the player wasn't aiming before.
	if (SetVisibility(true) && default_angle)
		angle = default_angle;

	CreateMoveEffect(clonk);
}

private func UpdatePosition()
{
	var x = +Sin(angle, CURSOR_Radius, 10);
	var y = -Cos(angle, CURSOR_Radius, 10);
	
	if (menu)
		SetPosition(menu->GetX()+x, menu->GetY()+y);
	else
		SetPosition(crew->GetX()+x, crew->GetY()+y);
		
	crew->UpdateVirtualCursorPos();
}

private func MirrorCursor()
{
	return;
	angle = -Normalize(angle,-1800, 10);
}

public func StopAim()
{
	aiming = false;
}

// Aiming means that some object is currently actively using the crosshair.
public func IsAiming()
{
	return aiming;
}

// The crosshair is also active when the player is holding the aiming stick.
public func IsActive()
{
	return aiming || Visible();
}

public func Aim(int ctrl, object clonk, int strength, int repeat, int status)
{
	// start (stealth) aiming
	if (!GetEffect("Move",this))
		CreateMoveEffect(clonk);

	// aiming with analog pad
	if (status == CONS_Moved &&
		(ctrl == CON_AimAxisUp || ctrl == CON_AimAxisDown || ctrl == CON_AimAxisLeft || ctrl == CON_AimAxisRight))
	{
		if (ctrl == CON_AimAxisUp) ypos = -strength;
		if (ctrl == CON_AimAxisDown) ypos = strength;
		if (ctrl == CON_AimAxisLeft) xpos = -strength;
		if (ctrl == CON_AimAxisRight) xpos = strength;
		return true;
	}
	return false;
}

public func Direction(int ctrl)
{
	if (!crew) return;
	
	angle = Normalize(angle,-1800, 10);
	//Message("%d, %d",this, angle, ctrl);
	if (ctrl == CON_Left)
		if (angle > 0)
			MirrorCursor();
		
	if (ctrl == CON_Right)
		if (angle < 0)
			MirrorCursor();		
	
	return;
}
