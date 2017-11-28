/**
	Objects.c
	Functions generally applicable to objects; not enough to be worth distinct scripts though.
		
	@author Maikel, boni, Ringwaul, Sven2, flgr, Clonkonaut, Günther, Randrian
*/

// Does not set the speed of an object. But you can set two components of the velocity vector with this function.
// documented in /docs/sdk/script/fn
global func SetSpeed(int x_dir, int y_dir, int prec)
{
	SetXDir(x_dir, prec);
	SetYDir(y_dir, prec);
	return;
}

// Returns the speed of an object.
global func GetSpeed(int prec)
{
	return Sqrt(GetXDir(prec)**2 + GetYDir(prec)**2);
}

// You can add to the two components of the velocity vector individually with this function.
global func AddSpeed(int x_dir, int y_dir, int prec)
{
	SetXDir(GetXDir(prec) + x_dir, prec);
	SetYDir(GetYDir(prec) + y_dir, prec);
}

// Sets an objects's speed and its direction, doesn't it?
// Can set either speed or angle of velocity, or both
global func SetVelocity(int angle, int speed, int precAng, int precSpd)
{
	if (!precSpd) precSpd = 10;
	if (!precAng) precAng = 1;
	speed = speed ?? Distance(0, 0, GetXDir(precSpd), GetYDir(precSpd));
	angle = angle ?? Angle(0, 0, GetXDir(precSpd), GetYDir(precSpd), precAng);
		
	var x_dir = Sin(angle, speed, precAng);
	var y_dir = -Cos(angle, speed, precAng);

	SetXDir(x_dir, precSpd);
	SetYDir(y_dir, precSpd);
	return;
}

// Adds to an objects's speed and its direction:
// Can set either speed or angle of velocity, or both
global func AddVelocity(int angle, int speed, int precision_angle, int precision_speed)
{
	precision_speed = precision_speed ?? 10;
	precision_angle = precision_angle ?? 1;
	speed = speed ?? 0;
	angle = angle ?? 0;

	var current_x_dir = GetXDir(precision_speed);
	var current_y_dir = GetYDir(precision_speed);
		
	var x_dir = +Sin(angle, speed, precision_angle);
	var y_dir = -Cos(angle, speed, precision_angle);

	SetXDir(current_x_dir + x_dir, precision_speed);
	SetYDir(current_y_dir + y_dir, precision_speed);
	return;
}


// Sets the completion of this to new_con.
// documented in /docs/sdk/script/fn
global func SetCon(int new_con, int precision, bool grow_from_center)
{
	return DoCon(new_con - GetCon(), precision, grow_from_center);
}

global func GetObjAlpha()
{
	return (GetClrModulation() >> 24) & 0xFF;
}

// Sets the object's transparency.
global func SetObjAlpha(int by_alpha)
{
	var clr_mod = GetClrModulation();
	
	if (!clr_mod)
		clr_mod = by_alpha << 24;
	else
		clr_mod = clr_mod & 16777215 | by_alpha << 24;
	return SetClrModulation(clr_mod);
}

global func MakeInvincible(bool allow_fire)
{
	if (!this) return false;
	var fx = GetEffect("IntInvincible", this);
	if (!fx) fx = AddEffect("IntInvincible", this, 300, 0);
	if (!fx) return false;
	fx.allow_fire = allow_fire;
	if (!allow_fire && this->OnFire()) this->Extinguish();
	fx.OnShockwaveHit = this.OnShockwaveHit;
	fx.RejectWindbagForce = this.RejectWindbagForce;
	fx.QueryCatchBlow = this.QueryCatchBlow;
	this.OnShockwaveHit = Global.Invincibility_OnShockwaveHit;
	this.RejectWindbagForce = Global.Invincibility_RejectWindbagForce;
	this.QueryCatchBlow = Global.Invincibility_QueryCatchBlow;
	return true;
}

global func IsInvincible()
{
	return !!GetEffect("IntInvincible", this);
}

global func SetInvincibility(bool to_val)
{
	// Turn invincibility on or off
	if (to_val)
		return MakeInvincible(false);
	else
		return ClearInvincible();
}

global func FxIntInvincibleDamage(target)
{
	// avert all damage
	return 0;
}

global func FxIntInvincibleEffect(string new_name, object target, proplist fx)
{
	// Block fire effects.
	if (WildcardMatch(new_name, "*Fire*") && !fx.allow_fire)
		return FX_Effect_Deny;
	// All other effects are okay.
	return FX_OK;
}

global func FxIntInvincibleSaveScen(object obj, proplist fx, proplist props)
{
	// this is invincible. Save to scenario.
	props->AddCall("Invincible", obj, "MakeInvincible", fx.allow_fire);
	return true;
}

// Removes invincibility from object
global func ClearInvincible()
{
	if (!this) return nil;
	var fx = GetEffect("IntInvincible", this);
	if (fx)
	{
		this.OnShockwaveHit = fx.OnShockwaveHit;
		this.RejectWindbagForce = fx.RejectWindbagForce;
		this.QueryCatchBlow = fx.QueryCatchBlow;
	} else { // just to be sure
		this.OnShockwaveHit = this->GetID().OnShockwaveHit;
		this.RejectWindbagForce = this->GetID().RejectWindbagForce;
		this.QueryCatchBlow = this->GetID().QueryCatchBlow;
	}
	return RemoveEffect("IntInvincible", this);
}

global func Invincibility_OnShockwaveHit()
{
	return true;
}

global func Invincibility_RejectWindbagForce()
{
	return true;
}

global func Invincibility_QueryCatchBlow()
{
	return true;
}

// Move an object by the given parameters relative to its position.
global func MovePosition(int x, int y, int prec)
{
	SetPosition(GetX(prec) + x, GetY(prec) + y, nil, prec);
}

// Returns the position as an array
global func GetPosition(int prec)
{
	return [GetX(prec), GetY(prec)];
}

// Speed the calling object into the given direction (angle)
global func LaunchProjectile(int angle, int dist, int speed, int x, int y, int precAng, int precSpd, bool rel_x)
{
	// dist: Distance object travels on angle. Offset from calling object.
	// x: X offset from container's center
	// y: Y offset from container's center
	// rel_x: if true, makes the X offset relative to container direction. (x=+30 will become x=-30 when Clonk turns left. This way offset always stays in front of a Clonk.)

	var x_offset = x ?? Sin(angle, dist, precAng);
	var y_offset = y ?? -Cos(angle, dist, precAng);
	
	if(!precAng) precAng = 1;
	if(!precSpd) precSpd = 10;

	if (Contained() != nil && rel_x == true)
		if (Contained()->GetDir() == 0)
			x = -x;

	if (Contained() != nil)
	{
		Exit(x_offset, y_offset, angle / precAng);
		SetVelocity(angle, speed, precAng, precSpd);
		return true;
	}

	if (Contained() == nil)
	{
		SetPosition(GetX() + x_offset, GetY() + y_offset);
		SetR(angle/precAng);
		SetVelocity(angle, speed, precAng, precSpd);
		return true;
	}
	return false;
}

// Sets the MaxEnergy value of an object and does the necessary callbacks.
global func SetMaxEnergy(int value)
{
	if (!this)
		return;
	value *= 1000;
	var old_maxenergy = this.MaxEnergy;
	this.MaxEnergy = value;
	// Change current energy percentage wise and implicit callback.
	DoEnergy(GetEnergy() * (value - old_maxenergy) / old_maxenergy);
	return;
}

// Returns the MaxEnergy value of an object.
global func GetMaxEnergy()
{
	if (!this)
		return;
	return this.MaxEnergy / 1000;
}

// Returns whether an object is at full energy, that is if its energy is >= its MaxEnergy
global func HasMaxEnergy()
{
	if (!this)
		return false;
	return GetEnergy() >= GetMaxEnergy();
}

// Sets the MaxBreath value of an object and does the necessary callbacks.
global func SetMaxBreath(int value)
{
	if (!this)
		return;
	var old_maxbreath = this.MaxBreath;
	this.MaxBreath = value;
	// Change current breath percentage wise and implicit callback.
	DoBreath(GetBreath() * (value - old_maxbreath) / old_maxbreath);
	return;
}

// Returns the MaxBreath value of an object.
global func GetMaxBreath()
{
	if (!this)
		return;
	return this.MaxBreath;
}

// Makes an object gain Con until it is FullCon.
// value: the object grows approx. every second, in tenths of percent.
// max_size = the maximum object size in tenths of percent.
global func StartGrowth(int value, int max_size)
{
	if (value <= 0) return nil;
	// Ensure max size is set and does not conflict with Oversize.
	max_size = max_size ?? 1000;
	if (!GetDefCoreVal("Oversize", "DefCore"))
		max_size = Min(max_size, 1000);
	var fx = AddEffect("IntGrowth", this, 1, 35, this, nil, value, max_size);
	fx.Time = Random(35);
	return fx;
}

global func StopGrowth()
{
	return RemoveEffect("IntGrowth", this);
}

global func GetGrowthValue()
{
	var fx = GetEffect("IntGrowth", this);
	if (!fx) 
		return 0;
	return fx.growth;
}

global func GetGrowthMaxSize()
{
	var fx = GetEffect("IntGrowth", this);
	if (!fx) 
		return 0;
	return fx.max_size;
}

global func FxIntGrowthStart(object obj, effect fx, int temporary, int value, int max_size)
{
	if (!temporary)
	{
		fx.growth = value;
		fx.max_size = max_size;	
	}
	return FX_OK;
}

global func FxIntGrowthTimer(object obj, effect fx)
{
	if (obj->OnFire())
		return FX_OK;
	obj->DoCon(fx.growth, 1000);
	// Negative growth might have removed the object.
	if (!obj) 
		return FX_Execute_Kill; 
	var done = obj->GetCon(1000) >= fx.max_size;
	return -done;
}

// Plays hit sounds for an average object made of stone or stone-like material.
// x and y need to be the parameters passed to Hit() from the engine.
global func StonyObjectHit(int x, int y)
{
	// Failsafe
	if (!this) return false;
	var xdir = GetXDir(), ydir = GetYDir();
	if (x) x = x / Abs(x);
	if (y) y = y / Abs(y);
	// Check for solid in hit direction
	var i = 0;
	var average_obj_size = Distance(0,0, GetObjWidth(), GetObjHeight()) / 2 + 2;
	while (!GBackSolid(x * i, y * i) && i < average_obj_size)
		i++;
	// To catch some high speed cases: if no solid found, check directly beneath
	if (!GBackSolid(x * i, y * i))
		while (!GBackSolid(x * i, y * i) && i < average_obj_size)
			i++;
	// Check if digfree
	if (!GetMaterialVal("DigFree", "Material", GetMaterial(x*i, y*i)) && GBackSolid(x*i, y*i))
		return Sound("Hits::Materials::Rock::RockHit?");
	// Else play standard sound
	if (Distance(0,0,xdir,ydir) > 10)
			return Sound("Hits::SoftTouch?");
		else
			return Sound("Hits::SoftHit?");
}

// Removes all objects of the given type.
// documented in /docs/sdk/script/fn
global func RemoveAll(p, ...)
{
	var cnt;
	if (GetType(p) == C4V_PropList) p = Find_ID(p); // RemoveAll(ID) shortcut
	for (var obj in FindObjects(p, ...))
	{
		if (obj)
		{
			obj->RemoveObject();
			cnt++;
		}
	}
	return cnt;
}

// Pulls an object above ground if it was buried (e.g. by PlaceVegetation), mainly used by plants.
// The object must have 'Bottom' and 'Center' CNAT to use this.
// (bottom is the point which should be buried, center the lowest point that must not be buried)
global func RootSurface(int max_movement)
{
	max_movement = max_movement ?? GetObjHeight() / 2;

	if (HasCNAT(CNAT_Center))
	{
		var i = 0;
		// Move up if too far underground.
		while (GetContact(-1) & CNAT_Center && i < max_movement)
		{
			SetPosition(GetX(),	GetY()	-	1);
			i++;
		}
	}
	if (HasCNAT(CNAT_Bottom))
	{
		var i = 0;
		 // Move down if in midair.
		while (!(GetContact(-1) & CNAT_Bottom) && i < max_movement)
		{
			SetPosition(GetX(), GetY() + 1);
			i++;
		}
		// Try to make the plant stuck.
		if (!Stuck())
			SetPosition(GetX(), GetY() + 1); 
	}
}

// Buys an object. Returns the object if it could be bought.
// documented in /docs/sdk/script/fn
global func Buy(id buy_def, int for_plr, int pay_plr, object from_vendor, bool show_errors)
{
	// if no vendor is given try this
	if (!from_vendor) 
		from_vendor = this;
	// not a vendor?
	if (!from_vendor->~IsVendor())
		return nil;
	return from_vendor->DoBuy(buy_def, for_plr, pay_plr, nil, 0, show_errors);
}

// Sells an object. Returns true if it could be sold.
// documented in /docs/sdk/script/fn
global func Sell(int plr, object obj, object to_vendor)
{
	// if no vendor is given try this
	if (!to_vendor)
		to_vendor = this;
	// not a vendor?
	if (!to_vendor->~IsVendor())
		return false;
	return to_vendor->DoSell(obj, plr);
}

// GetXEdge returns the position of the objects top/bottom/left/right edge.
global func GetLeftEdge()
{
	return GetX() - GetObjWidth() / 2;
}

global func GetRightEdge()
{
	return GetX() + GetObjWidth() / 2;
}

global func GetTopEdge()
{
	return GetY() - GetObjHeight() / 2;
}

global func GetBottomEdge()
{
	return GetY() + GetObjHeight() / 2;
}

// Returns if the object is standing in front of the back-object
global func InFrontOf(object back)
{
	var front = this;
	if (!front)
		return;
	return front->FindObject(front->Find_AtPoint(), Find_Not(Find_Exclude(back))) != nil;
}

// Returns the current left of the object relative to its current position.
global func GetLeft()
{
	var offset_x = GetObjectVal("Offset", nil, 0);
	if (offset_x == nil)
		offset_x = GetDefCoreVal("Offset", nil, 0);
	return offset_x;
}

// Returns the current right of the object relative to its current position.
global func GetRight()
{
	var offset_x = GetObjectVal("Offset", nil, 0);
	if (offset_x == nil)
		offset_x = GetDefCoreVal("Offset", nil, 0);
	var width = GetObjectVal("Width");
	return offset_x + width;
}

// Returns the current top of the object relative to its current position.
global func GetTop()
{
	var offset_y = GetObjectVal("Offset", nil, 1);
	if (offset_y == nil)
		offset_y = GetDefCoreVal("Offset", nil, 1);
	return offset_y;
}

// Returns the current bottom of the object relative to its current position.
global func GetBottom()
{
	var offset_y = GetObjectVal("Offset", nil, 1);
	if (offset_y == nil)
		offset_y = GetDefCoreVal("Offset", nil, 1);
	var height = GetObjectVal("Height");
	return offset_y + height;
}

// Returns the current shape as an array [offset_x, offset_y, width, height].
global func GetShape()
{
	var offset_x = GetObjectVal("Offset", nil, 0);
	if (offset_x == nil)
		offset_x= GetDefCoreVal("Offset", nil, 0);
	var offset_y = GetObjectVal("Offset", nil, 1);
	if (offset_y == nil)
		offset_y = GetDefCoreVal("Offset", nil, 1);
	var width = GetObjectVal("Width");
	var height = GetObjectVal("Height");
	return [offset_x, offset_y, width, height];
}

global func GetEntranceRectangle()
{
	var entrance_x = GetDefCoreVal("Entrance", "DefCore", 0);
	var entrance_y = GetDefCoreVal("Entrance", "DefCore", 1);
	var entrance_wdt = GetDefCoreVal("Entrance", "DefCore", 2);
	var entrance_hgt = GetDefCoreVal("Entrance", "DefCore", 3);
	return [entrance_x, entrance_y, entrance_wdt, entrance_hgt];
}
