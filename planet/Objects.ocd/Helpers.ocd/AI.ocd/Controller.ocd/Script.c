/**
	AI
	Controls bots.

	@author Marky
	@credits Original AI structure/design by Sven2, Maikel
*/

static const SAVESCEN_ID_AI = "AI";

/*-- Engine callbacks --*/

public func Definition(proplist type)
{
	this->OnDefineAI(type);
}

/*-- Public interface --*/

// Change whether target Clonk has an AI (used by editor).
public func SetAI(object clonk, id type)
{
	if (type)
	{
		return type->AddAI(clonk, type); // call from the definition there
	}
	else
	{
		return RemoveAI(clonk);
	}
}


// Add AI execution timer to target Clonk.
public func AddAI(object clonk, id type)
{
	AssertDefinitionContext(Format("AddAI(%v, %v)", clonk, type));
	AssertNotNil(clonk);

	var fx_ai = GetAI(clonk) ?? clonk->CreateEffect(FxAI, 1, 1, type ?? this);

	return fx_ai;
}


// Remove the AI execution timer
public func RemoveAI(object clonk)
{
	AssertDefinitionContext(Format("RemoveAI(%v)", clonk));

	var fx_ai = GetAI(clonk);
	if (fx_ai)
	{
		fx_ai->Remove();
	}
}


public func GetAI(object clonk)
{
	AssertDefinitionContext(Format("GetAI(%v)", clonk));

	if (clonk)
	{
		return clonk->~GetAI();
	}
	else
	{
		return nil;
	}
}


public func GetControlEffect()
{
	return this.FxAI;
}


// Set active state: Enables/Disables timer
public func SetActive(object clonk, bool active)
{
	AssertDefinitionContext(Format("SetActive(%v, %v)", clonk, active));

	var fx_ai = GetAI(clonk);
	if (fx_ai)
	{
		if (!active)
		{
			// Inactive: Stop any activity.
			clonk->SetCommand("None");
			clonk->SetComDir(COMD_Stop);
		}
		return fx_ai->SetActive(active);
	}
	else
	{
		return false;
	}
}


/*-- AI Effect --*/

// The AI effect stores a lot of information about the AI clonk. This includes its state, enemy target, alertness etc.
// Each of the functions which are called in the AI definition pass the effect and can then access these variables.
// The most important variables are:
// fx.Target     - The AI clonk.
// fx.target     - The current target the AI clonk will attack.
// fx.alert      - Whether the AI clonk is alert and aware of enemies around it.
// fx.weapon     - Currently selected weapon by the AI clonk.
// fx.ammo_check - Function that is called to check ammunition for fx.weapon.
// fx.commander  - Is commanded by another AI clonk.
// fx.control    - Definition controlling this AI, all alternative AIs should include the basic AI.

local FxAI = new Effect
{
	Construction = func(id control_def)
	{
		// Execute AI every 3 frames.
		this.Interval = control_def->~GetTimerInterval() ?? 1;
		// Store the definition that controls this AI.
		this.control = control_def;
		// Give the AI a helper function to get the AI control effect.
		this.Target.ai = this;
		this.Target.GetAI = this.GetAI;
		// Callback to the controller
		this.control->~OnAddAI(this);
		return FX_OK;
	},
	
	GetAI = func()
	{
		return this.ai;
	},

	GetControl = func()
	{
		return this.control;
	},

	Timer = func(int time)
	{
		// Execute the AI in the clonk.
		this.control->Execute(this, time);
		return FX_OK;
	},

	Destruction = func(int reason)
	{
		// Callback to the controller
		this.control->~OnRemoveAI(this, reason);
		// Remove AI reference.
		if (Target && Target.ai == this)
			Target.ai = nil;
		return FX_OK;	
	},
	
	Damage = func(int dmg, int cause)
	{
		// AI takes damage: Make sure we're alert so evasion and healing scripts are executed!
		// It might also be feasible to execute encounter callbacks here (in case an AI is shot from a position it cannot see).
		// However, the attacking clonk is not known and the callback might be triggered e.g. by an unfortunate meteorite or lightning blast.
		// So let's just keep it at alert state for now.
		if (dmg < 0) 
			this.alert = this.time;
		
		this.control->~OnDamageAI(this, dmg, cause);	
		
		return dmg;
	},
	SetActive = func(bool active)
	{
		this.Interval = (this.control->~GetTimerInterval() ?? 1) * active;
		
		if (active)
		{
			this.control->~OnActivateAI(this);
		}
		else
		{
			this.control->~OnDeactivateAI(this);
		}
	},
	GetActive = func()
	{
		return this.Interval != 0;	
	},
	EditorProps = {
		active = { Name = "$Active$", EditorHelp = "$ActiveHelp$", Type = "bool", Priority = 50, AsyncGet = "GetActive", Set = "SetActive" },
	},
	// Save this effect and the AI for scenarios.
	SaveScen = func(proplist props)
	{
		if (this.Target)
		{
			props->AddCall(SAVESCEN_ID_AI, this.control, "AddAI", this.Target);
			if (!this.Interval)
			{
				props->AddCall(SAVESCEN_ID_AI, this.control, "SetActive", this.Target, false);
			}
			this.control->~OnSaveScenarioAI(this, props);
			return true;
		}
		else
		{
			return false;
		}
	}	
};


/*-- AI Execution --*/

public func Execute(effect fx, int time) // TODO: Adjust
{
	return this->Call(fx.strategy, fx);
}


/*-- Editor Properties --*/

// Adds an AI to the selection list in the editor
public func AddEditorProp_AISelection(proplist type, id ai_type)
{
	InitEditorProp_AISelection(type);
	PushBack(type.EditorProps.AI_Controller.Options, EditorProp_AIType(ai_type));
}


// Initializes the selection property
private func InitEditorProp_AISelection(proplist type)
{
	// ensure that the poperties exist
	if (!type.EditorProps)
	{
		type.EditorProps = {};
	}
	
	// ensure that the list exists
	if (!type.EditorProps.AI_Controller)
	{
		type.EditorProps.AI_Controller =
		{
			Type = "enum",
			Name = "$ChooseAI$",
			Options = [],
			Set = Format("%i->SetAI", AI_Controller), // this is the AI_Controller on purpose
			SetGlobal = true
		};
	}
	
	// add default options
	if (GetLength(type.EditorProps.AI_Controller.Options) == 0)
	{
		PushBack(type.EditorProps.AI_Controller.Options, EditorProp_AIType(nil));
	}
}


// Gets an AI type entry for the selection list
private func EditorProp_AIType(id type)
{
	if (type)
	{
		var option_ai = {
			Name = type.Name ?? type->GetName(),
			EditorHelp = type.EditorHelp,
			Value = type
		};
	
		if (!option_ai.EditorHelp && type.GetEditorHelp) option_ai.EditorHelp = type->GetEditorHelp();
	
		return option_ai;
	}
	else
	{
		return {
			Name = "$NoAI$",
			EditorHelp = "$NoAIEditorHelp$",
			Value = nil,
		};
	}
}


/*-- Properties --*/

local Plane = 300;


/*-- Callbacks --*/

// Callback from the effect Construction()-call
public func OnAddAI(proplist fx_ai)
{
	// called by the effect Construction()
	_inherited(fx_ai);
}


// Callback from the effect Destruction()-call
public func OnRemoveAI(proplist fx_ai, int reason)
{
	// called by the effect Destruction()
	_inherited(fx_ai, reason);
}


// Callback when the AI is activated by a trigger
public func OnActivateAI(proplist fx_ai)
{
	_inherited(fx_ai);
}

// Callback when the AI is deactivated by a trigger
public func OnDeactivateAI(proplist fx_ai)
{
	_inherited(fx_ai);
}


// Callback when the AI is damaged
public func OnDamageAI(proplist fx_ai, int damage, int cause)
{
	_inherited(fx_ai, damage, cause);
}


// Callback from the effect SaveScen()-call
public func OnSaveScenarioAI(proplist fx_ai, proplist props)
{
	// called by the effect SaveScen()
	_inherited(fx_ai, props);
}


// Callback from the Definition()-call
public func OnDefineAI(proplist def)
{
	_inherited(def);
		
	// Add AI user actions.
	UserAction->AddEvaluator("Action", "Clonk", "$SetAIActivated$", "$SetAIActivatedHelp$", "ai_set_activated", [def, def.EvalAct_SetActive], 
		{
			Enemy = nil,
			AttackTarget = {
				Function= "triggering_clonk"
			},
			Status = {
				Function = "bool_constant",
				Value = true
			}
		},
		{
			Type = "proplist",
			Display = "{{Enemy}}: {{Status}} ({{AttackTarget}})",
			EditorProps = {
				Enemy = this->~UserAction_EnemyEvaluator(),
				AttackTarget = this->~UserAction_AttackTargetEvaluator(),
				Status = new UserAction.Evaluator.Boolean { Name = "$Status$" }
			}
		}
	);
}


/*-- Editor Properties --*/

public func EvalAct_SetActive(proplist props, proplist context)
{
	// User action: Activate enemy AI.
	var enemy = UserAction->EvaluateValue("Object", props.Enemy, context);
	var attack_target = UserAction->EvaluateValue("Object", props.AttackTarget, context);
	var status = UserAction->EvaluateValue("Boolean", props.Status, context);
	if (!enemy)
		return;
	// Ensure enemy AI exists
	var fx = GetAI(enemy);
	if (!fx)
	{
		// Deactivated? Then we don't need an AI effect.
		if (!status)
			return;
		fx = AddAI(enemy);
		if (!fx || !enemy)
			return;
	}
	// Set activation.
	fx->SetActive(status);
	// Set specific target if desired.
	if (attack_target)
		fx.target = attack_target;
}
