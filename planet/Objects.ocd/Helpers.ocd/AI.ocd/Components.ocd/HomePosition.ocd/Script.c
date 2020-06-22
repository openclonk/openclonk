/**
	Set the home position the Clonk returns to if he has no target.

	@author Sven2, Maikel
*/

/*-- Public interface --*/

// Set the home position the Clonk returns to if he has no target.
public func SetHome(object clonk, int x, int y, int dir)
{
	AssertDefinitionContext(Format("SetHome(%v, %d, %d, %d)", clonk, x, y, dir));
	var fx_ai = this->GetAI(clonk);
	if (!fx_ai)
		return false;
	// nil/nil defaults to current position.
	x = x ?? clonk->GetX();
	y = y ?? clonk->GetY();
	dir = dir ?? clonk->GetDir();
	fx_ai.home_x = x;
	fx_ai.home_y = y;
	fx_ai.home_dir = dir;
	return true;
}

/*-- Callbacks --*/

// Callback from the effect Construction()-call
public func OnAddAI(proplist fx_ai)
{
	_inherited(fx_ai);

	// Add AI default settings.	
	SetHome(fx_ai.Target);
}


// Callback from the effect SaveScen()-call
public func OnSaveScenarioAI(proplist fx_ai, proplist props)
{
	_inherited(fx_ai, props);

	if (fx_ai.home_x != fx_ai.Target->GetX() || fx_ai.home_y != fx_ai.Target->GetY() || fx_ai.home_dir != fx_ai.Target->GetDir())
		props->AddCall(SAVESCEN_ID_AI, fx_ai->GetControl(), "SetHome", fx_ai.Target, fx_ai.home_x, fx_ai.home_y, GetConstantNameByValueSafe(fx_ai.home_dir, "DIR_"));
}


/*-- Editor Properties --*/

// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
	
	// Add AI user actions.

	UserAction->AddEvaluator("Action", "Clonk", "$SetAINewHome$", "$SetAINewHomeHelp$", "ai_set_new_home", [def, def.EvalAct_SetNewHome],
		{
			Enemy = nil,
			HomePosition = nil,
			Status = {
				Function = "bool_constant",
				Value = true
			}
		},
		{
			Type = "proplist",
			Display = "{{Enemy}} -> {{NewHome}}",
			EditorProps = {
				Enemy = this->~UserAction_EnemyEvaluator(),
				NewHome = new UserAction.Evaluator.Position {
					Name = "$NewHome$",
					EditorHelp = "$NewHomeHelp$"
				},
				NewHomeDir = {
					Type = "enum",
					Name = "$NewHomeDir$",
					EditorHelp = "$NewHomeDirHelp$",
					Options = [
						{ Name = "$Unchanged$" },
						{ Name = "$Left$", Value = DIR_Left },
						{ Name = "$Right$", Value = DIR_Right }
					]
				},
			}
		}
	);
}


public func EvalAct_SetNewHome(proplist props, proplist context)
{
	// User action: Set new home.
	var enemy = UserAction->EvaluateValue("Object", props.Enemy, context);
	var new_home = UserAction->EvaluatePosition(props.NewHome, context);
	var new_home_dir = props.NewHomeDir;
	if (!enemy)
		return;
	// Ensure enemy AI exists.
	var fx = this->GetAI(enemy); // TODO: Streamline this
	if (!fx)
	{
		fx = this->AddAI(enemy, this);
		if (!fx || !enemy)
			return;
		// Create without attack command.
		this->~SetAutoSearchTarget(enemy, false);
	}
	fx.command = this.ExecuteIdle;
	fx.home_x = new_home[0];
	fx.home_y = new_home[1];
	if (GetType(new_home_dir))
		fx.home_dir = new_home_dir;
}
