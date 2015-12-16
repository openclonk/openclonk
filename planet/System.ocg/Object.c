/*--
		Objects.c
		Authors: Maikel, boni, Ringwaul, Sven2, flgr, Clonkonaut, Günther, Randrian

		Functions generally applicable to objects; not enough to be worth distinct scripts though.
--*/

// Does not set the speed of an object. But you can set two components of the velocity vector with this function.
global func SetSpeed(int x_dir, int y_dir, int prec)
{
	SetXDir(x_dir, prec);
	SetYDir(y_dir, prec);
	return;
}

// Sets an objects's speed and its direction, doesn't it?
// Can set either speed or angle of velocity, or both
global func SetVelocity(int angle, int speed, int precAng, int precSpd)
{
	if(!precSpd) precSpd = 10;
	if(!precAng) precAng = 1;
	if(!speed)
		speed = Distance(0,0, GetXDir(precSpd), GetYDir(precSpd));
	if(!angle)
		angle = Angle(0,0, GetXDir(precSpd), GetYDir(precSpd), precAng);
		
	var x_dir = Sin(angle, speed, precAng);
	var y_dir = -Cos(angle, speed, precAng);

	SetXDir(x_dir, precSpd);
	SetYDir(y_dir, precSpd);
	return;
}

// Sets the completion of this to new_con.
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

// Makes an object gain Con until it is FullCon
global func StartGrowth(int value /* the value the object grows approx. every second, in tenths of percent */)
{
	var effect;
	effect = AddEffect("IntGrowth", this, 1, 35, this, nil, value);
	effect.Time = Random(35);
	return effect;
}

global func StopGrowth()
{
	return RemoveEffect("IntGrowth", this);
}

global func GetGrowthValue()
{
	var e = GetEffect("IntGrowth", this);
	if(!e) return 0;
	return e.growth;
}

global func FxIntGrowthStart(object obj, effect, int temporary, int value)
{
	if (!temporary) effect.growth = value;
}

global func FxIntGrowthTimer(object obj, effect)
{
	if (obj->OnFire()) return;
	obj->DoCon(effect.growth, 1000);
	if (!obj) return FX_Execute_Kill; // Negative growth might have removed the object
	var done = obj->GetCon(1000) >= 1000;
	return -done;
}

// Plays hit sounds for an average object made of stone or stone-like material.
// x and y need to be the parameters passed to Hit() from the engine.
global func StonyObjectHit(int x, int y)
{
	// Failsafe
	if (!this) return false;
	var xdir = GetXDir(), ydir = GetYDir();
	if(x) x = x / Abs(x);
	if(y) y = y / Abs(y);
	// Check for solid in hit direction
	var i = 0;
	var average_obj_size = Distance(0,0, GetObjWidth(), GetObjHeight()) / 2 + 2;
	while(!GBackSolid(x*i, y*i) && i < average_obj_size) i++;
	// To catch some high speed cases: if no solid found, check directly beneath
	if (!GBackSolid(x*i, y*i))
		while(!GBackSolid(x*i, y*i) && i < average_obj_size) i++;
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

// Splits the calling object into its components.
global func Split2Components()
{
	if (!this)
		return false;
	var ctr = Contained();
	// Transfer all contents to container.
	while (Contents())
		if (!ctr || !Contents()->Enter(ctr))
			Contents()->Exit();
	// Split components.
	for (var i = 0, compid; compid = GetComponent(nil, i); ++i)
		for (var j = 0; j < GetComponent(compid); ++j)
		{
			var comp = CreateObjectAbove(compid, nil, nil, GetOwner());
			if (OnFire()) comp->Incinerate();
			if (!ctr || !comp->Enter(ctr))
			{
				comp->SetR(Random(360));
				comp->SetXDir(Random(3) - 1);
				comp->SetYDir(Random(3) - 1);
				comp->SetRDir(Random(3) - 1);
			}
		}
	RemoveObject();
	return;
}

// Pulls an object above ground if it was buried (e.g. by PlaceVegetation).
// The object must have 'Bottom' and 'Center' CNAT to use this.
// (bottom is the point which should be buried, center the lowest point that must not be buried)
// Mainly used by plants.
global func RootSurface()
{
	if (HasCNAT(CNAT_Center))
	{
		var i = 0;
		while(GetContact(-1) & CNAT_Center && i < GetObjHeight()/2) { SetPosition(GetX(),GetY()-1); i++; } //Move up if too far underground
	}
	if (HasCNAT(CNAT_Bottom))
	{
		i = 0;
		while(!(GetContact(-1) & CNAT_Bottom) && i < GetObjHeight()/2) { SetPosition(GetX(),GetY()+1); i++; } //Move down if in midair

		if (!Stuck()) SetPosition(GetX(),GetY()+1); // try make the plant stuck
	}
}

// Buys an object.
global func Buy (id idBuyObj, int iForPlr, int iPayPlr, object pToBase, bool fShowErrors)
{
	// if no base is given try this
	if(!pToBase) pToBase = this;
	// not a base?
	if( !pToBase->~IsBase() )
		return 0;
	return pToBase->DoBuy(idBuyObj, iForPlr, iPayPlr, 0, 0, fShowErrors);
}

// Sells an object.
global func Sell (int iPlr, object pObj, object pToBase)
{
	// if no base is given try this
	if(!pToBase) pToBase = this;
	// not a base?
	if( !pToBase->~IsBase() )
		return 0;
	return pToBase->DoSell(pObj, iPlr);
}

// Returns the owner if this is a base.
global func GetBase ()
{
	if(!(this->~IsBase())) return NO_OWNER;
	return GetOwner();
}


/* GetXEdge returns the position of the objects top/bottom/left/right edge */
global func GetLeftEdge()
{
	return GetX()-GetObjWidth()/2;
}

global func GetRightEdge()
{
	return GetX()+GetObjWidth()/2;
}

global func GetTopEdge()
{
	return GetY()-GetObjHeight()/2;
}

global func GetBottomEdge()
{
	return GetY()+GetObjHeight()/2;
}

// Returns if the object is standing in front of the back-object
global func InFrontOf(object back)
{
	var front = this;
	if(!front)
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
