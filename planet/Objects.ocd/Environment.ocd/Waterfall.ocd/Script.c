/*--
	Waterfall
	Author: Maikel
	
	Waterfall object, use to place waterfalls in the landscape.	
--*/


protected func Initialize()
{
	
	return;
}

/*-- Waterfall --*/

global func CreateWaterfall(int x, int y, int strength, string mat)
{
	var fall = CreateObjectAbove(Waterfall, x, y, NO_OWNER);
	if (!mat) mat = "Water";
	AddEffect("IntWaterfall", fall, 100, 1, fall, nil, x, y, strength, mat);
	return fall;
}

protected func FxIntWaterfallStart(object target, proplist effect, int temporary, int x, int y, int strength, string mat)
{
	if (temporary)
		return 1;
	effect.X = x;
	effect.Y = y;	
	effect.Strength = strength;
	effect.Material = mat;
	// Start sound.
	target->Sound("Environment::Waterfall", false, 5 * effect.Strength, nil, 1);
	return 1;
}

protected func FxIntWaterfallTimer(object target, proplist effect)
{
	// Insert liquid at location every frame.
	for (var i = 0; i < effect.Strength / 2; i++)
		InsertMaterial(Material(effect.Material), AbsX(effect.X), AbsY(effect.Y), effect.XDir + Random(effect.XVar), effect.YDir + Random(effect.YVar));
	return 1;
}

protected func FxIntWaterfallStop(object target, proplist effect, bool temporary)
{
	if (temporary)
		return 1;
	// Stop sound.
	target->Sound("Environment::Waterfall", false, 5 * effect.Strength, nil, -1);
	return 1;
}

public func SetStrength(int strength)
{
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
		effect.Strength = BoundBy(strength, 0, 100);
	return;
}

public func SetMaterial(int material)
{
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
		effect.Material = material;
	return;
}

public func SetDirection(int xdir, int ydir, int xvar, int yvar)
{
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
	{
		effect.XDir = xdir; 
		effect.YDir = ydir; 
		effect.XVar = xvar; 
		effect.YVar = yvar; 
	}	
	return;
}

public func SetSoundLocation(int x, int y)
{
	SetPosition(x, y);
	// Update sound.
	var effect = GetEffect("IntWaterfall", this);
	if (effect)
		Sound("Environment::Waterfall", false, 5 * effect.Strength, nil, 1);
	return;
}

// Scenario saving
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var fx_waterfall = GetEffect("IntWaterfall", this), fx_drain = GetEffect("IntLiquidDrain", this);
	if (!fx_waterfall && !fx_drain) return false; // effects lost? don't save dead object then
	// Waterfall has its own creation procedure
	props->RemoveCreation();
	if (fx_waterfall)
	{
		props->Add(SAVEOBJ_Creation, "CreateWaterfall(%d, %d, %d, %v)",fx_waterfall.X, fx_waterfall.Y, fx_waterfall.Strength, fx_waterfall.Material);
		if (fx_waterfall.X != GetX() || fx_waterfall.Y != GetY()) props->AddCall("Position", this, "SetSoundLocation", GetX(), GetY());
		if (fx_waterfall.XDir || fx_waterfall.YDir || fx_waterfall.XVar || fx_waterfall.YVar)
			props->AddCall("Direction", this, "SetDirection", fx_waterfall.XDir, fx_waterfall.YDir, fx_waterfall.XVar, fx_waterfall.YVar);
	}
	if (fx_drain) props->Add(SAVEOBJ_Creation, "CreateLiquidDrain(%d, %d, %d)",fx_drain.X, fx_drain.Y, fx_drain.Strength);
	return true;
}



/*-- Liquid Drain --*/

global func CreateLiquidDrain(int x, int y, int strength)
{
	var drain = CreateObjectAbove(Waterfall, x, y, NO_OWNER);
	AddEffect("IntLiquidDrain", drain, 100, 1, drain, nil, x, y, strength);
	return drain;
}

protected func FxIntLiquidDrainStart(object target, proplist effect, int temporary, int x, int y, int strength)
{
	if (temporary)
		return 1;
	effect.X = x;
	effect.Y = y;	
	effect.Strength = strength;
	return 1;
}

protected func FxIntLiquidDrainTimer(object target, proplist effect)
{
	ExtractLiquidAmount(AbsX(effect.X), AbsY(effect.Y),effect.Strength / 2);
	return 1;
}



local Name = "$Name$";
