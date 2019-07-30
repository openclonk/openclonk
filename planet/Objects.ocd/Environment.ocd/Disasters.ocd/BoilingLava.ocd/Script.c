/**
	BoilingMagma
	Causes Lava on the map to boil

	@author Win 
*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;

local intensity;
// Magic number by which the total map size (in pixels) is divided to get the amount of tries per frame.
local intensity_quotient = 10000;
local area, last_boilpos;

public func IsBoilingLiquid() { return true; }

public func Place(int amount)
{
	amount = amount ?? 1;
	
	// The amount directly controls the intensity. More objects would not help.
	var obj = CreateObject(this, 0, 0, NO_OWNER);
	obj.intensity *= amount;
	return [obj];
}

public func Construction()
{
	area = Shape->Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());
	last_boilpos = {};
	SetIntensity();
}

public func SetArea(to_area)
{
	var has_default_intensity = (intensity == GetDefaultIntensity());
	this.area = to_area;
	if (has_default_intensity) SetIntensity();
	return true;
}

private func GetDefaultIntensity()
{
	var barea = area->GetBoundingRectangle();
	var def_int = (barea.wdt * barea.hgt) / this.intensity_quotient;
	if (!def_int) ++def_int;
	return def_int;
}

private func Boiling()
{
	for (var i = 0; i < intensity; i++)
	{
		// Checks if there is a deep enough pool of lava at a random location of the map, then creates spawner on the surface
		area->GetRandomPoint(last_boilpos);
		var x_rand = last_boilpos.x - GetX();
		var y_rand = last_boilpos.y - GetY();
		var mat = MaterialName(GetMaterial(x_rand, y_rand)), above_mat;
		if (mat == "DuroLava" || mat == "Lava")
		{
			for (var dy = 0; dy < 20; ++dy)
			{
				--y_rand;
				if ((above_mat = MaterialName(GetMaterial(x_rand, --y_rand))) != mat)
					break;
			}
			if (!above_mat || above_mat == "Tunnel")
			{
				var depth_check_math = MaterialName(GetMaterial(x_rand, y_rand + 30));
				if (depth_check_math == "DuroLava" || depth_check_math == "Lava")
				{
					var spawner = CreateObject(Dummy, x_rand, y_rand);
					spawner.Boil = this.SpawnerBoil;
					spawner.max_time = 2;
					spawner.timer = 0;
					spawner.count = 10;
					spawner->AddTimer("Boil", 1);
				}
			}
		}
	}
}

private func SpawnerBoil()
{
	if (++this.timer > this.max_time)
	{
		this.timer = 0;
		var amount = RandomX(1, 3);
		this.count -= amount;
		
		var bubbles = this->CastLavaBubbles(amount, RandomX(10, 30), RandomX(-30, 30), 0);
		for (var bubble in bubbles)
			bubble->SetCon(RandomX(105, 115));
		if (this.count <= 0)
			this->RemoveObject();
	}
}

/*
	Sets the intensity of the lava spawner. An intensity of 3 means that a new random spawn position is tested 3 times per frame.
*/
public func SetIntensity(int intensity)
{
	this.intensity = intensity ?? GetDefaultIntensity();
	RemoveTimer("Boiling");
	if (this.intensity) AddTimer("Boiling", 20);
	return true;
}

// Save the intensity IFF it is different from the definition's.
public func SaveScenarioObject(props, ...)
{
	if (!_inherited(props, ...)) return false;
	if (this.intensity != GetDefaultIntensity())
		props->AddCall("Intensity", this, "SetIntensity", this.intensity);
	if (!this.area->~IsFullMap())
		props->AddCall("Area", this, "SetArea", this.area);
	return true;
}


/* Editor */

private func GetDefaultArea(boiler, def_val)
{
	// Default area is whole map
	return Shape->Rectangle(LandscapeWidth()/3, LandscapeHeight()/3, LandscapeWidth()/3, LandscapeHeight()/3);
}

private func SetAreaRect(to_rect)
{
	// Editor sets properties only; convert to rectangle.
	if (to_rect)
		to_rect = Shape->Rectangle(to_rect.x, to_rect.y, to_rect.wdt, to_rect.hgt);
	else
		to_rect = GetDefaultArea();
	return SetArea(to_rect);
}

private func GetAreaRect()
{
	// Editor sets properties only; convert to rectangle.
	if (!area->IsFullMap()) return area;
}

public func Definition(def, ...)
{
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.intensity = { Name="$Intensity$", EditorHelp="$IntensityHelp$", Type="int", Min = 0, Set="SetIntensity" };
	def.EditorProps.area = { Name="$Area$", EditorHelp="$AreaHelp$", Type="enum", AsyncGet="GetAreaRect", Set="SetAreaRect", Options = [
		{ Name="$FullMap$" },
		{ Name="$Rect$", OptionKey="Type", DefaultValueFunction = def.GetDefaultArea, Value={ Type="rect" }, Delegate={ Type="rect", Relative = false, Storage="proplist", Color = 0xffa020, Set="SetAreaRect" } }
		// other shapes not supported for now
		] };
	if (def == BoilingLava)
	{
		UserAction->AddEvaluator("Action", "Ambience", "$SetBoilingIntensity$", "$SetBoilingIntensityHelp$", "set_boiling_intensity", [def, def.EvalAct_SetBoilingIntensity], { Intensity={ Function="int_constant", Value = 0 } }, 
			{ Type="proplist", Display="{{Boiler}}, {{Intensity}}", EditorProps = {
				Boiler = UserAction->GetObjectEvaluator("IsBoilingLiquid", "$Boiler$", "$BoilerTargetHelp$"),
				Intensity = new UserAction.Evaluator.Integer { Name="$Intensity$", EditorHelp="$IntensityHelp$" },
			} } );
	}
	return _inherited(def, ...);
}

private func EvalAct_SetBoilingIntensity(props, context)
{
	var boiler = UserAction->EvaluateValue("Boiler", props.Boiler, context);
	var new_intensity = Max(UserAction->EvaluateValue("Integer", props.Intensity, context));
	if (boiler) boiler->~SetIntensity(new_intensity);
}
