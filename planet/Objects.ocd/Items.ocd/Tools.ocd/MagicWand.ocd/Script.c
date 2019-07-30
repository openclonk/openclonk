/**
	Magic wand
	Item to be made magical with editor actions/
	
	@author Sven2
 --*/

public func Initialize()
{
	// We don't have nice graphics yet :(
	SetGraphics(nil, Torch);
}

public func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
}

public func ControlUse(object clonk, int x, int y)
{
	// No clonk?
	if (!clonk) return false;
	// Cooldown?
	if (last_use_frame != nil)
		if (FrameCounter() - last_use_frame < cooldown)
		{
			PlayerMessage(clonk->GetController(), "$NoUseCooldown$");
			Sound("UI::Error", true, nil, clonk->GetController());
			return true;
		}
	// Action go!
	UserAction->EvaluateAction(use_action, this, clonk, nil, nil, action_allow_parallel, nil, [x + GetX(), y + GetY()]);
	last_use_frame = FrameCounter();
	return true;
}

public func SetUseAction(new_action)
{
	use_action = new_action;
	return true;
}

public func SetCooldown(int new_cooldown)
{
	cooldown = new_cooldown;
	return true;
}


/*-- Display --*/

public func GetCarryMode(object clonk, bool idle, bool nohand)
{
	if (idle || nohand)
		return CARRY_Back;

	return CARRY_Spear;
}

func Definition(def)
{
	// Display
	SetProperty("PictureTransformation", Trans_Mul(Trans_Translate(2500, -1500, 0), Trans_Rotate(-30, 0, 0, 1)), def);
	// Actions
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.use_action = new UserAction.Prop { Name="$MagicAction$", Priority = 100, Set="SetUseAction", Save="Action", Priority = 100 };
	def.EditorProps.cooldown = { Name="$Cooldown$", Type="int", EditorHelp="$CooldownHelp$", Set="SetCooldown", Save="Cooldown", Min = 0 };
	def.EditorProps.action_allow_parallel = UserAction.PropParallel;
}

/*-- Properties --*/

local last_use_frame;
local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Diamond = 1};
local use_action = nil, action_allow_parallel = false;
local cooldown = 40;