/*
	Crosshair
	Author: Newton
	
	Virtual cursor for gamepad controls
*/

local crew, angle, dirx, diry, xpos,ypos, analogaim, aiming;

static const CURSOR_Radius = 100;

protected func Initialize()
{
	this["Visibility"] = VIS_None;
	dirx = diry = xpos = ypos = 0;
	aiming = false;
}

public func FxMoveTimer()
{
	var speed = 0;
	var dpad_rotatespeed = 35;

	// dpad mode
	if(diry)
	{
		if (diry < 0) speed = -Sin(angle,100,10);
		else if (diry > 0) speed = +Sin(angle,100,10);
		angle += dpad_rotatespeed*speed/100;
		UpdateAnalogpadPos();
	}
	if(dirx)
	{
		if (dirx < 0) speed = -Cos(angle,100,10);
		else if (dirx > 0) speed = +Cos(angle,100,10);
		angle += dpad_rotatespeed*speed/100;
		UpdateAnalogpadPos();
	}
	// analog pad mode
	if(!dirx && !diry)
	{
		var target_angle = Angle(0,0,xpos,ypos)*10;
		var analog_strength = BoundBy(Sqrt(xpos*xpos+ypos*ypos),0,100);

		var angle_diff = Normalize(target_angle - angle, -1800, 10);
		if (angle_diff == 0) angle_diff = 1;
		var dir = angle_diff / Abs(angle_diff);
		
		angle = angle + angle_diff * analog_strength / 100 / 8;
	}
	
	UpdatePosition();
	if(aiming) crew->TriggerHoldingControl();
}

private func UpdateAnalogpadPos()
{
	xpos = Sin(angle/10,100);
	ypos = Cos(angle/10,-100);
}

public func StartAim(object clonk, bool stealth)
{
	// only reinitialize angle if the crosshair hasn't been there before
	if(!GetEffect("Move",this))
	{
		// which should basically be only the case on the first time aiming
		angle = 800*(clonk->GetDir()*2-1);
	}
	
	// set starting position for analog pad
	UpdateAnalogpadPos();
	
	crew = clonk;
	UpdatePosition();
	RemoveEffect("Move",this);
	AddEffect("Move",this,1,1,this);

	if(!stealth)
	{
		this["Visibility"] = VIS_Owner;
		crew->SetComDir(COMD_Stop);
		aiming = true;
		EnableKeyAimControls(true);
	}
}

private func UpdatePosition()
{
	var x = +Sin(angle,CURSOR_Radius,10);
	var y = -Cos(angle,CURSOR_Radius,10);
	
	SetPosition(crew->GetX()+x,crew->GetY()+y);
	crew->UpdateVirtualCursorPos();
}

private func MirrorCursor()
{
	angle = -Normalize(angle,-1800,10);
	UpdateAnalogpadPos();
}

public func StopAim()
{
	RemoveEffect("Move",this);
	this["Visibility"] = VIS_None;
	dirx = 0;
	diry = 0;
	EnableKeyAimControls(false);
	analogaim = false;
	aiming = false;
}

private func EnableKeyAimControls(bool enable)
{
	SetPlayerControlEnabled(GetOwner(), CON_AimUp, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimDown, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimLeft, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimRight, enable);
	
	// never disabled
	/*SetPlayerControlEnabled(GetOwner(), CON_AimAxisUp, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimAxisDown, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimAxisLeft, enable);
	SetPlayerControlEnabled(GetOwner(), CON_AimAxisRight, enable);*/
}

public func IsAiming()
{
	return aiming;
}

public func Aim(int ctrl, object clonk, int strength, int repeat, int release)
{
	// start (stealth) aiming
	if(!GetEffect("Move",this))
		StartAim(clonk,true);

	// aiming with analog pad
	if (ctrl == CON_AimAxisUp || ctrl == CON_AimAxisDown || ctrl == CON_AimAxisLeft || ctrl == CON_AimAxisRight)
	{	
		dirx = diry = 0;
		
		if(ctrl == CON_AimAxisUp) ypos = -strength;
		if(ctrl == CON_AimAxisDown) ypos = strength;
		if(ctrl == CON_AimAxisLeft) xpos = -strength;
		if(ctrl == CON_AimAxisRight) xpos = strength;
		analogaim = true;
		return true;
	}
	// stop
	else if (release && !analogaim)
	{
		if(ctrl == CON_AimUp || ctrl == CON_AimDown) diry = 0;
		else if(ctrl == CON_AimLeft || ctrl == CON_AimRight) dirx = 0;
		return true;
	}
	else if(!release /*&& !repeat */ && !analogaim)
	{
		if(ctrl == CON_AimUp) diry = -1;
		else if(ctrl == CON_AimDown) diry = 1;
		else if(ctrl == CON_AimLeft) dirx = -1;
		else if(ctrl == CON_AimRight) dirx = 1;
		return true;
	}
	return false;
}

public func Direction(int ctrl)
{
	if(!crew) return;
	
	angle = Normalize(angle,-1800,10);
	//Message("%d, %d",this,angle,ctrl);
	if(ctrl == CON_Left)
		if(angle > 0)
			MirrorCursor();
		
	if(ctrl == CON_Right)
		if(angle < 0)
			MirrorCursor();		
	
	return;
}