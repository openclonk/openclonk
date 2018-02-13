/**
	Volcano
	Destructive eruptions of lava originating from the centre of the earth.
	
	@author Maikel
*/


/*-- Disaster Control --*/

public func SetChance(int chance)
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (!effect)
		effect = AddEffect("IntVolcanoControl", nil, 100, 20, nil, Volcano);
	effect.chance = chance;
	return;
}

public func GetChance()
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		return effect.chance;
	return;
}

// Sets the material, defaults to "Lava";
public func SetMaterial(string material)
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		effect.material = material ?? "Lava";
	return;
}

// Sets the minimum strength in 1/4 pixels, defaults to 4.
public func SetMinStrength(int strength)
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		effect.min_strength = strength ?? 4;
	return;
}

public func GetMinStrength()
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		return effect.min_strength;
	return;
}

// Sets the minimum strength in 1/4 pixels, defaults to 4.
public func SetMaxStrength(int strength)
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		effect.max_strength = strength ?? 100;
	return;
}

public func GetMaxStrength()
{
	if (this != Volcano)
		return;
	var effect = GetEffect("IntVolcanoControl");
	if (effect)
		return effect.max_strength;
	return;
}

protected func FxIntVolcanoControlStart(object target, proplist effect, int temp)
{
	// Default to lava as material, 1 to 25 pixels wide streams
	if (!temp)
	{
		effect.material = "Lava";
		effect.min_strength = 4;
		effect.max_strength = 100;
	}
	return FX_OK;
}

protected func FxIntVolcanoControlTimer(object target, proplist effect, int time)
{
	if (Random(100) < effect.chance && !Random(10))
		LaunchVolcano(Random(LandscapeWidth()), LandscapeHeight(), Random(40) + 35, effect.material, RandomX(-10, 10));
	return FX_OK;
}

// Scenario saving
func FxIntVolcanoControlSaveScen(obj, fx, props)
{
	props->Add("Volcano", "Volcano->SetChance(%d)", fx.chance);
	if (fx.material && fx.material != "Lava") props->Add("Volcano", "Volcano->SetMaterial(%v)", fx.material);
	if (fx.min_strength && fx.min_strength != 4) props->Add("Volcano", "Volcano->SetMinStrength(%d)", fx.min_strength);
	if (fx.max_strength && fx.max_strength != 100) props->Add("Volcano", "Volcano->SetMaxStrength(%d)", fx.max_strength);
	return true;
}

// Launches a volcano at a given position, strength is measured in 1/4 pixels.
global func LaunchVolcano(int x, int y, int strength, string material, int angle)
{
	var volcano = CreateObjectAbove(Volcano);
	return volcano->Launch(x, y, strength, material, angle);
}

/*-- Volcano --*/

local str; // Volcano strength, max 100.
local mat; // Volcano material, string.
local angle; // Direction of the volcano.
local oldx, oldy; // Old coordinates.

// returns true on a succesful volcano launch, false otherwise.
public func Launch(int x, int y, int strength, string material, int angle)
{
	// Initial coordinates of the volcano.
	SetPosition(x, y);
	// Strength of the volcano.
	str = BoundBy(strength, Volcano->GetMinStrength(), Volcano->GetMaxStrength());
	// Volcano material.
	mat = Material(material);
	// Direction of the volcano.
	this.angle = angle;
	// Safety check.
	if (!InGround())
	{
		RemoveObject();
		return false;
	}
	// Launch volcano.
	SetAction("Advance");
	return true;
}

// Advance action call: Used for underground advancing.
private func Advance()
{
	// Branch volcano.
	if (!Random((Volcano->GetMaxStrength() + 20 - str) / 12))
		Branch();
	// Store old coordinates.
	oldx = GetX();
	oldy = GetY();
	// Advance volcano.
	angle += RandomX(-12, 12); // Slight angle distortion.
	var adv = 12 + Random(6) + Random(str / 16);
	var advx, advy;
	var above_ground = false;
	for (var x = 1; x <= adv; x++)
	{
		advx = Sin(angle, x);
		advy = -Cos(angle, x);
		SetPosition(oldx + advx, oldy + advy);
		if (!InGround())
		{
			above_ground = true;
			break;
		}
	}
	// Draw volcano.
	var strx = Abs(Cos(angle, str));
	var stry = Abs(Sin(angle, str));
	DrawMaterialQuad(MaterialName(mat),
		GetX() - strx / 4, GetY() - stry / 4,
		GetX() + strx / 4, GetY() + stry / 4,
		oldx + strx / 4, oldy + stry / 4,
		oldx - strx / 4, oldy - stry / 4, DMQ_Sub);
	// Drag Objects along.
	for (var pObj in FindObjects(Find_OCF(OCF_Collectible), Find_InRect()))
	{
		pObj->SetXDir(advx);
		pObj->SetYDir(advy);
	}
	// Shake free possible top layer.
	if (above_ground)
		ShakeFree(GetX(), GetY(), str / 4);
	// Reduce strength
	str -= Random(2);
	if (str <= 0)
		return RemoveObject();
	// Above ground -> start Erupting.
	if (above_ground)
		SetAction("Erupt");
	// Finished.
	return;
}

// Erupt action call: Used for surface eruptions.
private func Erupt()
{
	// Build up banks at the sides.
	//CastPXS("Coal", str/4, str/4);
	//CastPXS("Coal", str/4, str/4);
	
	// Cast volcano material.
	CastPXS(MaterialName(mat), 2 * str, 3 * str);

	// Cast other particles (lava chunks, ashes, ashclouds).
	if (!Random(6))
	{
		CastChunks(MaterialName(mat));
	}

	// Reduce strength.
	if(!Random(3))
		str--;
	if (str <= 0)
		return RemoveObject();
	// If in ground -> start advancing.
	if (InGround())
		SetAction("Advance");
	return;
}

// Cast other particles (lava chunks, ashes, ashclouds).
private func CastChunks(string material)
{
	if (WildcardMatch(material, "*Lava*"))
	{		
		CastObjects(LavaChunk, 1, 60, 0, 0, 0, 40);
	}
	return _inherited(material, ...);
}

// The volcano mainline branches into mainline + sideline.
private func Branch()
{
	// Branch volcano.
	var side = 2 * Random(2) - 1; // At which side the volcano branches.
	var new_str = Max(Random(str / 2), Volcano->GetMinStrength()); // Strength of the branch.
	var x = side * Cos(angle, str / 4 - new_str / 4);
	var y = side * Sin(angle, str / 4 - new_str / 4);
	var new_mat = MaterialName(mat);
	var new_angle = angle + side * RandomX(5, 15);
	var volcano = CreateObjectAbove(Volcano, x, y, NO_OWNER);
	volcano->Launch(GetX() + x, GetY() + y, new_str, new_mat, new_angle);
	// Reset volcano.
	str -= new_str;
	x = side * Cos(new_angle, new_str / 4);
	y = side * Sin(new_angle, new_str / 4);
	SetPosition(GetX() + x, GetY() + y);
	return;
}

// returns whether the volcano is in ground.
private func InGround()
{
	if (GBackSolid(0, -2))
		return true;
	if (GetMaterial(0, -2) == mat)
		return true;
	return false;
}

// Individual volcanoes not stored in scenario. It would look weird because the
// lava already drawn would not appear on the recreated created map.
// Only the controller effect is saved (see FxIntVolcanoControlSaveScen).
public func SaveScenarioObject() { return false; }


/*-- Proplist --*/

local Name = "$Name$";
local ActMap = {
	Advance = {
		Prototype = Action,
		Name = "Advance",
		Procedure = DFA_NONE,
		Delay = 2,
		NextAction = "Advance",
		StartCall = "Advance",
		// Sound = "VolcanoAdvance",
	}, 
	Erupt = {
		Prototype = Action,
		Name = "Erupt",
		Procedure = DFA_NONE,
		Delay = 2,
		NextAction = "Erupt",
		StartCall = "Erupt",
		// Sound = "VolcanoErupt",
	},
};
