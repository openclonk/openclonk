/**
	Waterfall
	Waterfall object, use to place waterfalls in the landscape.	

	@author Maikel
*/


/*-- Waterfall --*/

global func CreateWaterfall(int x, int y, int strength, string mat)
{
	var waterfall = CreateObjectAbove(Waterfall, x, y, NO_OWNER);
	if (!mat)
		mat = "Water";
	waterfall->CreateEffect(waterfall.FxWaterfallLiquidSource, 100, 1, x, y, strength, mat);
	return waterfall;
}

local FxWaterfallLiquidSource = new Effect
{
	Construction = func(int x, int y, int strength, string mat)
	{
		this.source_position = [x, y];
		this.source_direction = [x, y];
		this.source_x = x;
		this.source_y = y;
		this.source_strength = strength;
		this.source_mat = mat;
		// Start sound.
		Target->Sound("Environment::Waterfall", false, 5 * this.source_strength, nil, 1);
		return FX_OK;
	},
	Timer = func(int time)
	{
		// Insert liquid at location every frame.
		for (var i = 0; i < this.source_strength / 2; i++)
			Global->InsertMaterial(Material(this.source_mat), this.source_x, this.source_y, this.xdir + RandomX(-this.xvar, this.xvar), this.ydir + RandomX(-this.yvar, this.yvar));
		return FX_OK;
	},
	Destruction = func()
	{
		// Stop sound.
		Target->Sound("Environment::Waterfall", false, 5 * this.source_strength, nil, -1);
		return FX_OK;
	},	
	EditorProps =
	{
		source_strength = { Name = "$EditorSourceStrength$", EditorHelp = "$EditorSourceStrengthHelp$", Type = "int", Min = 2, Max = 100 },
		source_mat = { Name = "$EditorSourceMaterial$", EditorHelp = "$EditorSourceMaterialHelp$", Type = "enum", Options = [
			{ Name = "$EditorSourceMaterialWater$", Value = "Water" },
			{ Name = "$EditorSourceMaterialAcid$", Value = "Acid" },
			{ Name = "$EditorSourceMaterialLava$", Value = "Lava" },
			{ Name = "$EditorSourceMaterialDuroLava$", Value = "DuroLava" },
			{ Name = "$EditorSourceMaterialOil$", Value = "Oil" }
		] },
		// TODO: Replace these two properties with an arrow in editor once available.
		source_position = { Name = "$EditorSourcePosition$", EditorHelp = "$EditorSourcePositionHelp$", Type = "point", Relative = false, Color = 0x008fff, Set = "SetSourcePosition" },
		source_direction = { Name = "$EditorSourceDirection$", EditorHelp = "$EditorSourceDirectionHelp$", Type = "point", Relative = false, Color = 0x006bbb, Set = "SetSourceDirection" }
	},
	SetSourcePosition = func(array pos)
	{
		this.source_position = pos;
		this.source_x = pos[0];
		this.source_y = pos[1];
	},
	SetSourceDirection = func(array dir)
	{
		this.source_direction = dir;
		this.xdir = dir[0] - this.source_x;
		this.ydir = dir[1] - this.source_y;
		this.xvar = Abs(this.xdir / 2);
		this.yvar = Abs(this.ydir / 2);
	}
};

public func SetStrength(int strength)
{
	var fx_waterfall = GetEffect("FxWaterfallLiquidSource", this);
	if (fx_waterfall)
		fx_waterfall.source_strength = BoundBy(strength, 0, 100);
	return;
}

public func SetMaterial(int material)
{
	var fx_waterfall = GetEffect("FxWaterfallLiquidSource", this);
	if (fx_waterfall)
		fx_waterfall.source_mat = material;
	return;
}

public func SetDirection(int xdir, int ydir, int xvar, int yvar)
{
	var fx_waterfall = GetEffect("FxWaterfallLiquidSource", this);
	if (fx_waterfall)
	{
		fx_waterfall.xdir = xdir; 
		fx_waterfall.ydir = ydir; 
		fx_waterfall.xvar = xvar; 
		fx_waterfall.yvar = yvar;
		fx_waterfall.source_direction = [fx_waterfall.source_x + xdir, fx_waterfall.source_y + ydir];
	}	
	return;
}

public func SetSoundLocation(int x, int y)
{
	SetPosition(x, y);
	// Update sound.
	var fx_waterfall = GetEffect("FxWaterfallLiquidSource", this);
	if (fx_waterfall)
		Sound("Environment::Waterfall", false, 5 * fx_waterfall.source_strength, nil, 1);
	return;
}


/*-- Liquid Drain --*/

global func CreateLiquidDrain(int x, int y, int strength)
{
	var drain = CreateObjectAbove(Waterfall, x, y, NO_OWNER);
	drain->CreateEffect(drain.FxWaterfallLiquidDrain, 100, 1, x, y, strength);
	return drain;
}

local FxWaterfallLiquidDrain = new Effect
{
	Construction = func(int x, int y, int strength)
	{
		this.drain_position = [x, y];
		this.drain_x = x;
		this.drain_y = y;
		this.drain_strength = strength;
		return FX_OK;
	},
	Timer = func(int time)
	{
		Global->ExtractLiquidAmount(this.drain_x, this.drain_y, this.drain_strength / 2);
		return FX_OK;
	},
	EditorProps =
	{
		drain_strength = { Name = "$EditorDrainStrength$", EditorHelp = "$EditorDrainStrengthHelp$", Type = "int", Min = 2, Max = 100},
		drain_position = { Name = "$EditorDrainPosition$", EditorHelp = "$EditorDrainPositionHelp$", Type = "point", Relative = false, Color = 0x00ff8f, Set = "SetDrainPosition" }
	},
	SetDrainPosition = func(array pos)
	{
		this.drain_position = pos;
		this.drain_x = pos[0];
		this.drain_y = pos[1];
	}
};


/*-- Editor --*/

public func EditorInitialize()
{
	// Init waterfall with the source directly above the drain.
	this->CreateEffect(this.FxWaterfallLiquidSource, 100, 1, GetX(), GetY() - 40, 10, "Water");
	this->CreateEffect(this.FxWaterfallLiquidDrain, 100, 1, GetX(), GetY() + 40, 10);
	return;
}


/*-- Scenario Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!inherited(props, ...))
		return false;
	var fx_source = GetEffect("FxWaterfallLiquidSource", this);
	var fx_drain = GetEffect("FxWaterfallLiquidDrain", this);
	if (!fx_source && !fx_drain)
		return false;
	if (fx_source)
	{
		props->AddCall("SourceEffect", this, "AddSourceEffect", fx_source.source_x, fx_source.source_y, fx_source.source_strength, Format("%v", fx_source.source_mat));
		if (fx_source.source_x != GetX() || fx_source.source_y != GetY())
			props->AddCall("SoundLocation", this, "SetSoundLocation", GetX(), GetY());
		if (fx_source.xdir || fx_source.xdir || fx_source.xvar || fx_source.yvar)
			props->AddCall("Direction", this, "SetDirection", fx_source.xdir, fx_source.ydir, fx_source.xvar, fx_source.yvar); 
	}	
	if (fx_drain)
		props->AddCall("DrainEffect", this, "AddDrainEffect", fx_drain.drain_x, fx_drain.drain_y, fx_drain.drain_strength);
	return true;		
}

public func AddSourceEffect(int x, int y, int strength, string mat)
{
	CreateEffect(this.FxWaterfallLiquidSource, 100, 1, x, y, strength, mat);
	return;
}

public func AddDrainEffect(int x, int y, int strength)
{
	CreateEffect(this.FxWaterfallLiquidDrain, 100, 1, x, y, strength);
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
