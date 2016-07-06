/* User action execution handler */
// Handles actions set in editor e.g. for dialogues, switches, etc.
// An object is sometimes needed to show a menu or start a timer, so this definition is created whenever a user action is run

local Name = "UserAction";
local Plane=0;

// Base classes for EditorProps using actions 
local Evaluator;

// EditorProps for generic user action callbacks
local Prop, PropProgressMode, PropParallel;

// Base props for action execution conditions
local ActionEvaluation ;

// Proplist containing callback function. Indexed by option names.
local EvaluatorCallbacks;

// If this action is paused and will be resumed by a callback or by re-execution of the action, this property is set to the props of the holding action
local hold;

// Set to true if action is on hold but waiting for re-execution
local suspended;

// Return value if a value-providing evaluator is held
local hold_result;

// Call this definition early (but after EditorBase) to allow EditorProp initialization
local DefinitionPriority=99;

// Proplist holding progress in each sequence
local sequence_progress, sequence_had_goto;
static UserAction_SequenceIDs;

// Set to innermost sequence (for goto)
local last_sequence;

public func Initialize()
{
	sequence_progress = {};
	sequence_had_goto = {};
	return true;
}

public func SaveScenarioObject(props) { return false; } // temp. don't save.

func Definition(def)
{
	// Typed evaluator base definitions
	Evaluator = {};
	Evaluator.Action = { Name="$UserAction$", Type="enum", OptionKey="Option", Options = [ { Name="$None$" } ] };
	Evaluator.Object = { Name="$UserObject$", Type="enum", OptionKey="Option", Options = [ { Name="$None$" } ] };
	Evaluator.Player = { Name="$UserPlayer$", Type="enum", OptionKey="Option", Options = [ { Name="$Noone$" } ] };
	Evaluator.PlayerList = { Name="$UserPlayerList$", Type="enum", OptionKey="Option", Options = [ { Name="$Noone$" } ] };
	// Action evaluators
	EvaluatorCallbacks = {};
	AddEvaluator("Action", "$Sequence$", "$Sequence$", "sequence", [def, def.EvalAct_Sequence], { Actions=[] }, { Type="proplist", DescendPath="Actions", Display="{{Actions}}", Elements = {
		EditorProp_Actions = { Name="$Actions$", Type="array", Elements=Evaluator.Action },
		} } );
	AddEvaluator("Action", "$Sequence$", "$Goto$", "goto", [def, def.EvalAct_Goto], { Index=0 }, { Type="proplist", Display="{{Index}}", Elements = {
		EditorProp_Index = { Name="$Index$", Type="int", Min=0 }
		} } );
	AddEvaluator("Action", "$Sequence$", "$StopSequence$", "stop_sequence", [def, def.EvalAct_StopSequence]);
	AddEvaluator("Action", "$Sequence$", "$SuspendSequence$", "suspend_sequence", [def, def.EvalAct_SuspendSequence]);
	AddEvaluator("Action", "$Sequence$", "$Wait$", "wait", [def, def.EvalAct_Wait], { Time=60 }, { Type="proplist", Display="{{Time}}", Elements = {
		EditorProp_Time = { Name="$Time$", Type="int", Min=1 }
		} } );
	// Object evaluators
	AddEvaluator("Object", nil, "$ActionObject$", "action_object", [def, def.EvalObj_ActionObject]);
	AddEvaluator("Object", nil, "$TriggerObject$", "triggering_object", [def, def.EvalObj_TriggeringObject]);
	// Player evaluators
	AddEvaluator("Player", nil, "$TriggeringPlayer$", "triggering_player", [def, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$TriggeringPlayer$", "triggering_player_list", [def, def.EvalPlrList_Single, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$AllPlayers$", "all_players", [def, def.EvalPlrList_All]);
	// User action editor props
	Prop = Evaluator.Action;
	PropProgressMode = { Name="$UserActionProgressMode$", Type="enum", Options = [ { Name="$Session$", Value="session" }, { Name="$Player$", Value="player" }, { Name="$Global$" } ] };
	PropParallel = { Name="$ParallelAction$", Type="bool" };
	return true;
}

public func AddEvaluator(string eval_type, string group, string name, string identifier, callback_data, default_val, proplist delegate)
{
	if (!default_val) default_val = {};
	var default_get;
	if (GetType(default_val) == C4V_Function)
	{
		default_get = default_val;
		default_val = Call(default_get);
	}
	if (delegate && delegate.Elements)
	{
		delegate.Elements.Name = name;
	}
	default_val.Option = identifier;
	var action_def = { Name=name, Group=group, Value=default_val, OptionKey="Option", Delegate=delegate, Get=default_get };
	Evaluator[eval_type].Options[GetLength(Evaluator[eval_type].Options)] = action_def;
	EvaluatorCallbacks[identifier] = callback_data;
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
	var cb = EvaluatorCallbacks[props.Option];
	return cb[0]->Call(cb[1], props, context, cb[2]);
}

public func EvaluateAction(proplist props, object action_object, object triggering_object, string progress_mode, bool allow_parallel)
{
	// Determine context
	var context;
	if (!progress_mode)
	{
		if (!(context = props.context))
			props.context = context = CreateObject(UserAction);
	}
	else if (progress_mode == "player")
	{
		if (!props.contexts) props.contexts = [];
		var plr_id;
		if (action_object) plr_id = GetPlayerID(action_object->GetOwner());
		if (!(context = props.contexts[plr_id]))
			props.contexts[plr_id] = context = CreateObject(UserAction);
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
	context.action_object = action_object;
	context.triggering_object = triggering_object;
	context.root_action = props;
	context.suspended = false;
	// Execute action
	EvaluateValue("Action", props, context);
	FinishAction(context);
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
	if (!context.hold || context.temp) context->RemoveObject();
}

private func EvalObj_ActionObject(proplist props, proplist context) { return context.action_object; }
private func EvalObj_TriggeringObject(proplist props, proplist context) { return context.triggering_object; }
private func EvalPlr_Trigger(proplist props, proplist context) { if (context.triggering_object) return context.triggering_object->GetOwner(); }
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
	var n = GetLength(props.Actions), sid = props.sequence_id;
	if (!sid) sid = props.sequence_id = Format("%d", ++UserAction_SequenceIDs);
	for (var progress = context.sequence_progress[props.sequence_id] ?? 0; progress < n; ++progress)
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
	context.sequence_progress[props.sequence_id] = 0;
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
