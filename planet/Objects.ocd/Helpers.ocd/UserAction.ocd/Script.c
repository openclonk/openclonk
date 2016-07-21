/* User action execution handler */
// Handles actions set in editor e.g. for dialogues, switches, etc.
// An object is sometimes needed to show a menu or start a timer, so this definition is created whenever a user action is run

local Name = "UserAction";
local Plane=0;

/* UserAction definition */

// Base classes for EditorProps using actions 
local Evaluator;

// EditorProps for generic user action callbacks
local Prop, PropProgressMode, PropParallel;

// Base props for action execution conditions
local ActionEvaluation ;

// Proplist containing callback function. Indexed by option names.
local EvaluatorCallbacks;

// Proplist containing option definitions. Indexed by option names.
local EvaluatorDefs;

// Call this definition early (but after EditorBase) to allow EditorProp initialization
local DefinitionPriority=99;

func Definition(def)
{
	// Typed evaluator base definitions
	Evaluator = {};
	Evaluator.Action = { Name="$UserAction$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Object = { Name="$UserObject$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Player = { Name="$UserPlayer$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.PlayerList = { Name="$UserPlayerList$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.Boolean = { Name="$UserBoolean$", Type="enum", OptionKey="Function", Options = [] };
	Evaluator.Condition = { Name="$UserCondition$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	// Action evaluators
	EvaluatorCallbacks = {};
	EvaluatorDefs = {};
	AddEvaluator("Action", "$Sequence$", "$Sequence$", "sequence", [def, def.EvalAct_Sequence], { Actions=[] }, { Type="proplist", DescendPath="Actions", Display="{{Actions}}", EditorProps = {
		Actions = { Name="$Actions$", Type="array", Elements=Evaluator.Action },
		} } );
	AddEvaluator("Action", "$Sequence$", "$Goto$", "goto", [def, def.EvalAct_Goto], { Index=0 }, { Type="proplist", Display="{{Index}}", EditorProps = {
		Index = { Name="$Index$", Type="int", Min=0 }
		} } );
	AddEvaluator("Action", "$Sequence$", "$StopSequence$", "stop_sequence", [def, def.EvalAct_StopSequence]);
	AddEvaluator("Action", "$Sequence$", "$SuspendSequence$", "suspend_sequence", [def, def.EvalAct_SuspendSequence]);
	AddEvaluator("Action", "$Sequence$", "$Wait$", "wait", [def, def.EvalAct_Wait], { Time=60 }, { Type="proplist", Display="{{Time}}", EditorProps = {
		Time = { Name="$Time$", Type="int", Min=1 }
		} } );
	// Object evaluators
	AddEvaluator("Object", nil, "$ActionObject$", "action_object", [def, def.EvalObj_ActionObject]);
	AddEvaluator("Object", nil, "$TriggerObject$", "triggering_object", [def, def.EvalObj_TriggeringObject]);
	AddEvaluator("Object", nil, "$ConstantObject$", "object_constant", [def, def.EvalConstant], { Value=nil }, { Type="object", Name="$Value$" });
	// Player evaluators
	AddEvaluator("Player", nil, "$TriggeringPlayer$", "triggering_player", [def, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$TriggeringPlayer$", "triggering_player_list", [def, def.EvalPlrList_Single, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$AllPlayers$", "all_players", [def, def.EvalPlrList_All]);
	// Boolean (condition) evaluators
	AddEvaluator("Boolean", nil, "$Constant$", "bool_constant", [def, def.EvalConstant], { Value=true }, { Type="bool", Name="$Value$" });
	// User action editor props
	Prop = Evaluator.Action;
	PropProgressMode = { Name="$UserActionProgressMode$", Type="enum", Options = [ { Name="$Session$", Value="session" }, { Name="$Player$", Value="player" }, { Name="$Global$" } ] };
	PropParallel = { Name="$ParallelAction$", Type="bool" };
	return true;
}

public func GetObjectEvaluator(filter_def, name)
{
	// Create copy of the Evaluator.Object delegate, but with the object_constant proplist replaced by a version with filter_def
	var object_options = Evaluator.Object.Options[:];
	var const_option = new EvaluatorDefs["object_constant"] {};
	const_option.Delegate = new const_option.Delegate { Filter=filter_def };
	object_options[const_option.OptionIndex] = const_option;
	return new Evaluator.Object { Name=name, Options=object_options };
}

public func AddEvaluator(string eval_type, string group, string name, string identifier, callback_data, default_val, proplist delegate)
{
	// Add an evaluator for one of the data types. Evaluators allow users to write small action sequences and scripts in the editor using dropdown lists.
	// eval_type: Return type of the evaluator (Action, Object, Boolean, Player, etc. as defined in UserAction.Evaluator)
	// group [optional] Localized name of sub-group for larger enums (i.e. actions)
	// name: Localized name as it appears in the dropdown list of evaluators
	// identifier: Unique identifier that is used to store this action in savegames and look up the action def. Identifiers must be unique among all data types.
	// callback_data: Array of [definition, definition.Function, parameter (optional)]. Function to be called when this evaluator is called
	// default_val [optional]: Default value to be set when this evaluator is selected. Must be a proplist. Should contain values for all properties in the delegate
	// delegate: Parameters for this evaluator
	if (!default_val) default_val = {};
	var default_get;
	if (GetType(default_val) == C4V_Function)
	{
		default_get = default_val;
		default_val = Call(default_get);
	}
	default_val.Function = identifier;
	var action_def = { Name=name, Group=group, Value=default_val, OptionKey="Function", Delegate=delegate, Get=default_get }, n;
	if (delegate)
	{
		if (delegate.EditorProps)
		{
			// Proplist of array parameter for this evaluator: Descend path title should be name
			delegate.Name = name;
		}
		else
		{
			// Any other parameter type: Store in value
			action_def.ValueKey = "Value";
		}
	}
	Evaluator[eval_type].Options[n = GetLength(Evaluator[eval_type].Options)] = action_def;
	action_def.OptionIndex = n;
	EvaluatorCallbacks[identifier] = callback_data;
	EvaluatorDefs[identifier] = action_def;
	// Copy most boolean props to condition prop
	if (eval_type == "Boolean" && identifier != "bool_constant")
		AddEvaluator("Condition", group, name, identifier, callback_data, default_val, delegate);
	return action_def;
}

public func EvaluateValue(string eval_type, proplist props, proplist context)
{
	//Log("EvaluateValue %v %v %v", eval_type, props, context);
	if (!props) return nil;
	// Finish any hold-action
	if (context.hold == props)
	{
		context.hold = nil;
		return context.hold_result;
	}
	// Not on hold: Perform evaluation
	var cb = EvaluatorCallbacks[props.Function];
	return cb[0]->Call(cb[1], props, context, cb[2]);
}

public func EvaluateAction(proplist props, object action_object, object triggering_object, int triggering_player, string progress_mode, bool allow_parallel, finish_callback)
{
	// Determine context
	var context;
	if (!progress_mode)
	{
		if (!(context = props._context))
			props._context = context = CreateObject(UserAction);
	}
	else if (progress_mode == "player")
	{
		if (!props._contexts) props._contexts = [];
		var plr_id;
		if (action_object) plr_id = GetPlayerID(action_object->GetOwner());
		if (!(context = props._contexts[plr_id]))
			props._contexts[plr_id] = context = CreateObject(UserAction);
	}
	else // if (progress_mode == "session")
	{
		// Temporary context
		context = CreateObject(UserAction);
		context.temp = true;
	}
	// Prevent duplicate parallel execution
	if (!allow_parallel && (context.hold && !context.suspended)) return nil;
	// Init context settings
	context->InitContext(action_object, triggering_player, triggering_object, props);
	// Execute action
	EvaluateValue("Action", props, context);
	FinishAction(context);
}

public func EvaluateCondition(proplist props, object action_object, object triggering_object, int triggering_player)
{
	// Build temp context
	var context = CreateObject(UserAction);
	context.temp = true;
	// Init context settings
	context->InitContext(action_object, triggering_player, triggering_object, props);
	// Execute condition evaluator
	var result = EvaluateValue("Condition", props, context);
	// Cleanup
	if (context) context->RemoveObject();
	// Done
	return result;
}

private func ResumeAction(proplist context, proplist resume_props)
{
	//Log("ResumeAction %v %v", context, resume_props);
	// Resume only if on hold for the same entry
	if (context.hold != resume_props) return;
	// Resume action
	EvaluateValue("Action", context.root_action, context);
	// Cleanup action object (unless it ran into another hold)
	FinishAction(context);
}

private func FinishAction(proplist context)
{
	// Cleanup action object (unless it's kept around for callbacks or to store sequence progress)
	// Note that context.root_action.contexts is checked to kill session-contexts that try to suspend
	// There would be no way to resume so just kill the context
	if (!context.hold || context.temp)
	{
		if (context.action_object && context.finish_callback) context.action_object->Call(context.finish_callback, context);
		context->RemoveObject();
	}
}

private func EvalConstant(proplist props, proplist context) { return props.Value; }
private func EvalObj_ActionObject(proplist props, proplist context) { return context.action_object; }
private func EvalObj_TriggeringObject(proplist props, proplist context) { return context.triggering_object; }
private func EvalPlr_Trigger(proplist props, proplist context) { return context.triggering_player; }
private func EvalPlrList_Single(proplist props, proplist context, fn) { return [Call(fn, props, context)]; }

private func EvalPlrList_All(proplist props, proplist context, fn)
{
	var n = GetPlayerCount(C4PT_User);
	var result = CreateArray(n);
	for (var i=0; i<n; ++i) result[i] = GetPlayerByIndex(i);
	return result;
}

private func EvalAct_Sequence(proplist props, proplist context)
{
	// Sequence execution: Iterate over actions until one action puts the context on hold
	var n = GetLength(props.Actions), sid = props._sequence_id;
	if (!sid) sid = props._sequence_id = Format("%d", ++UserAction_SequenceIDs);
	for (var progress = context.sequence_progress[sid] ?? 0; progress < n; ++progress)
	{
		//Log("Sequence progress exec %v %v", progress, context.hold);
		// goto preparations
		context.sequence_had_goto[sid] = false;
		context.last_sequence = props;
		// Evaluate next sequence step
		EvaluateValue("Action", props.Actions[progress], context);
		if (context.hold || context.suspended)
		{
			// Execution on hold (i.e. wait action). Stop execution for now
			if (!context.hold) progress = 0; // No hold specified: Stop with sequence reset
			context.sequence_progress[sid] = progress;
			return;
		}
		// Apply jump in the sequence
		if (context.sequence_had_goto[sid]) progress = context.sequence_progress[sid] - 1;
	}
	// Sequence finished
	context.last_sequence = nil;
	// Reset for next execution.
	context.sequence_progress[sid] = 0;
}

private func EvalAct_Goto(proplist props, proplist context)
{
	// Apply goto by jumping in most recently executed sequence
	if (context.last_sequence)
	{
		context.sequence_progress[context.last_sequence.sequence_id] = props.Index;
		context.sequence_had_goto[context.last_sequence.sequence_id] = true;
	}
}

private func EvalAct_StopSequence(proplist props, proplist context)
{
	// Stop: Suspend without hold props, which causes all sequences to reset
	context.hold = nil;
	context.suspended = true;
}

private func EvalAct_SuspendSequence(proplist props, proplist context)
{
	// Suspend: Remember hold position and stop action execution
	context.hold = props;
	context.suspended = true;
}

private func EvalAct_Wait(proplist props, proplist context)
{
	// Wait for specified number of frames
	context.hold = props;
	ScheduleCall(context, UserAction.ResumeAction, props.Time, 1, context, props);
}


/* Context instance */

// Proplist holding progress in each sequence
local sequence_progress, sequence_had_goto;
static UserAction_SequenceIDs;

// Set to innermost sequence (for goto)
local last_sequence;

// If this action is paused and will be resumed by a callback or by re-execution of the action, this property is set to the props of the holding action
local hold;

// Set to true if action is on hold but waiting for re-execution
local suspended;

// Return value if a value-providing evaluator is held
local hold_result;

public func Initialize()
{
	sequence_progress = {};
	sequence_had_goto = {};
	return true;
}

public func InitContext(object action_object, int triggering_player, object triggering_object, proplist props)
{
	// Determine triggering player+object
	if (!GetType(triggering_player))
	{
		if (triggering_object) triggering_player = triggering_object->GetController();
	}
	else if (!triggering_object)
	{
		triggering_object = GetCursor(triggering_player);
		if (!triggering_object) triggering_object = GetCrew(triggering_player);
	}
	// Init context settings
	this.action_object = action_object;
	this.triggering_object = triggering_object;
	this.triggering_player = triggering_player;
	this.root_action = props;
	this.suspended = false;
	return true;
}

public func MenuOK(proplist menu_id, object clonk)
{
	// Pressed 'Next' in dialogue: Resume in user action
	UserAction->ResumeAction(this, this.hold);
}

public func MenuSelectOption(int index)
{
	// Selected an option in dialogue: Resume at specified position in innermost sequence
	if (!hold || !hold.Options) return;
	var opt = this.hold.Options[index];
	if (opt && last_sequence)
	{
		sequence_progress[last_sequence.sequence_id] = opt.Goto;
		hold = nil;
	}
	UserAction->ResumeAction(this, hold);
}

public func SaveScenarioObject(props) { return false; } // temp. don't save.
