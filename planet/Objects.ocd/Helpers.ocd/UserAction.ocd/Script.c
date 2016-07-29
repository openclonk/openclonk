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
	Evaluator.Definition = { Name="$UserDefinition$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Player = { Name="$UserPlayer$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.PlayerList = { Name="$UserPlayerList$", Type="enum", OptionKey="Function", Options = [ { Name="$Noone$" } ] };
	Evaluator.Boolean = { Name="$UserBoolean$", Type="enum", OptionKey="Function", Options = [] };
	Evaluator.Integer = { Name="$UserInteger$", Type="enum", OptionKey="Function", Options = [] };
	Evaluator.Condition = { Name="$UserCondition$", Type="enum", OptionKey="Function", Options = [ { Name="$None$" } ] };
	Evaluator.Position = { Name="$UserPosition$", Type="enum", OptionKey="Function", Options = [ { Name="$Here$" } ] };
	// Action evaluators
	EvaluatorCallbacks = {};
	EvaluatorDefs = {};
	AddEvaluator("Action", "$Sequence$", "$Sequence$", "$SequenceHelp$", "sequence", [def, def.EvalAct_Sequence], { Actions=[] }, { Type="proplist", DescendPath="Actions", Display="{{Actions}}", EditorProps = {
		Actions = { Name="$Actions$", Type="array", Elements=Evaluator.Action },
		} } );
	AddEvaluator("Action", "$Sequence$", "$Goto$", "$GotoHelp$", "goto", [def, def.EvalAct_Goto], { Index=0 }, { Type="proplist", Display="{{Index}}", EditorProps = {
		Index = { Name="$Index$", Type="int", Min=0 }
		} } );
	AddEvaluator("Action", "$Sequence$", "$StopSequence$", "$StopSequenceHelp$", "stop_sequence", [def, def.EvalAct_StopSequence]);
	AddEvaluator("Action", "$Sequence$", "$SuspendSequence$", "$SuspendSequenceHelp$", "suspend_sequence", [def, def.EvalAct_SuspendSequence]);
	AddEvaluator("Action", "$Sequence$", "$Wait$", "$WaitHelp$", "wait", [def, def.EvalAct_Wait], { Time=60 }, { Type="proplist", Display="{{Time}}", EditorProps = {
		Time = { Name="$Time$", Type="int", Min=1 }
		} } );
	AddEvaluator("Action", "$Ambience$", "$Sound$", "$SoundHelp$", "sound", [def, def.EvalAct_Sound], { Pitch={Function="int_constant", Value=0}, Volume={Function="int_constant", Value=100}, TargetPlayers={Function="all_players"} }, { Type="proplist", Display="{{Sound}}", EditorProps = {
		Sound = { Name="$SoundName$", EditorHelp="$SoundNameHelp$", Type="sound", AllowEditing=true },
		Pitch = new Evaluator.Integer { Name="$SoundPitch$", EditorHelp="$SoundPitchHelp$" },
		Volume = new Evaluator.Integer { Name="$SoundVolume$", EditorHelp="$SoundVolumeHelp$" },
		Loop = { Name="$SoundLoop$", EditorHelp="$SoundLoopHelp$", Type="enum", Options=[
			{ Name="$SoundLoopNone$" },
			{ Name="$SoundLoopOn$", Value=+1 },
			{ Name="$SoundLoopOff$", Value=-1 }
			] },
		TargetPlayers = new Evaluator.PlayerList { EditorHelp="$SoundTargetPlayersHelp$" },
		SourceObject = new Evaluator.Object { Name="$SoundSourceObject$", EditorHelp="$SoundSourceObjectHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$CreateObject$", "$CreateObjectHelp$", "create_object", [def, def.EvalAct_CreateObject], { SpeedX={Function="int_constant", Value=0},SpeedY={Function="int_constant", Value=0} }, { Type="proplist", Display="{{ID}}", EditorProps = {
		ID = new Evaluator.Definition { EditorHelp="$CreateObjectDefinitionHelp$" },
		Position = new Evaluator.Position { EditorHelp="$CreateObjectPositionHelp$" },
		CreateAbove = { Name="$CreateObjectCreationOffset$", EditorHelp="$CreateObjectCreationOffsetHelp$", Type="enum", Options=[
			{ Name="$Center$" },
			{ Name="$Bottom$", Value=true }
			]},
		Owner = new Evaluator.Player { Name="$Owner$", EditorHelp="$CreateObjectOwnerHelp$" },
		Container = new Evaluator.Object { Name="$Container$", EditorHelp="$CreateObjectContainerHelp$" },
		SpeedX = new Evaluator.Integer { Name="$SpeedX$", EditorHelp="$CreateObjectSpeedXHelp$" },
		SpeedY = new Evaluator.Integer { Name="$SpeedY$", EditorHelp="$CreateObjectSpeedYHelp$" }
		} } );
	AddEvaluator("Action", "$Object$", "$RemoveObject$", "$RemoveObjectHelp$", "remove_object", [def, def.EvalAct_RemoveObject], { }, { Type="proplist", Display="{{Object}}", EditorProps = {
		Object = new Evaluator.Object { EditorHelp="$RemoveObjectObject$" },
		EjectContents = { Name="$EjectContents$", EditorHelp="$EjectContentsHelp$", Type="enum", Options=[
			{ Name="$EjectContentsNo$" },
			{ Name="$EjectContentsYes$", Value=true }
			] },
		} } );
	// Object evaluators
	AddEvaluator("Object", nil, "$ActionObject$", "$ActionObjectHelp$", "action_object", [def, def.EvalObj_ActionObject]);
	AddEvaluator("Object", nil, "$TriggerClonk$", "$TriggerClonkHelp$", "triggering_clonk", [def, def.EvalObj_TriggeringClonk]);
	AddEvaluator("Object", nil, "$TriggerObject$", "$TriggerObjectHelp$", "triggering_object", [def, def.EvalObj_TriggeringObject]);
	AddEvaluator("Object", nil, "$ConstantObject$", "$ConstantObjectHelp$", "object_constant", [def, def.EvalConstant], { Value=nil }, { Type="object", Name="$Value$" });
	AddEvaluator("Object", nil, "$LastCreatedObject$", "$LastCreatedObjectHelp$", "last_created_object", [def, def.EvalObj_LastCreatedObject]);
	// Definition evaluators
	AddEvaluator("Definition", nil, "$Constant$", "$ConstantHelp$", "def_constant", [def, def.EvalConstant], { Value=nil }, { Type="def", Name="$Value$" });
	// Player evaluators
	AddEvaluator("Player", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player", [def, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$TriggeringPlayer$", "$TriggeringPlayerHelp$", "triggering_player_list", [def, def.EvalPlrList_Single, def.EvalPlr_Trigger]);
	AddEvaluator("PlayerList", nil, "$AllPlayers$", "$AllPlayersHelp$", "all_players", [def, def.EvalPlrList_All]);
	// Boolean (condition) evaluators
	AddEvaluator("Boolean", nil, "$Constant$", "$ConstantHelp$", "bool_constant", [def, def.EvalConstant], { Value=true }, { Type="bool", Name="$Value$" });
	// Integer evaluators
	AddEvaluator("Integer", nil, "$Constant$", "$ConstantHelp$", "int_constant", [def, def.EvalConstant], { Value=0 }, { Type="int", Name="$Value$" });
	// User action editor props
	Prop = Evaluator.Action;
	PropProgressMode = { Name="$UserActionProgressMode$", EditorHelp="$UserActionProgressModeHelp$", Type="enum", Options = [ { Name="$Session$", Value="session" }, { Name="$Player$", Value="player" }, { Name="$Global$" } ] };
	PropParallel = { Name="$ParallelAction$", EditorHelp="$ParallelActionHelp$", Type="bool" };
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

public func AddEvaluator(string eval_type, string group, string name, string help, string identifier, callback_data, default_val, proplist delegate)
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
	var action_def = { Name=name, EditorHelp=help, Group=group, Value=default_val, OptionKey="Function", Delegate=delegate, Get=default_get }, n;
	if (delegate)
	{
		if (delegate.EditorProps || delegate.Elements)
		{
			// Proplist of array parameter for this evaluator: Descend path title should be name
			delegate.Name = name;
			var child_delegate = delegate;
			if (delegate.DescendPath) child_delegate = delegate.EditorProps[delegate.DescendPath];
			if (!child_delegate.EditorHelp) child_delegate.EditorHelp = help;
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
		AddEvaluator("Condition", group, name, help, identifier, callback_data, default_val, delegate);
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
	// No action
	if (!props) if (finish_callback) return action_object->Call(finish_callback); else return;
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
	if (!allow_parallel && (context.hold && !context.suspended)) return false;
	// Init context settings
	context->InitContext(action_object, triggering_player, triggering_object, props);
	// Execute action
	EvaluateValue("Action", props, context);
	FinishAction(context);
	return true;
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

private func EvaluatePosition(proplist props, object context)
{
	// Execute position evaluator; fall back to position of action object
	var position = EvaluateValue("Position", props, context);
	if (!position)
	{
		if (context.action_object) position = [context.action_object->GetX(), context.action_object->GetY()];
		else position = [0,0];
	}
	return position;
}

private func ResumeAction(proplist context, proplist resume_props)
{
	//Log("ResumeAction %v %v", context, resume_props);
	// Resume only if on hold for the same entry
	if (context.hold != resume_props) return;
	// Not if owning object is dead
	if (!context.action_object) return;
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
private func EvalObj_TriggeringClonk(proplist props, proplist context) { return context.triggering_clonk; }
private func EvalObj_LastCreatedObject(proplist props, proplist context) { return context.last_created_object; }
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
		context.sequence_progress[context.last_sequence._sequence_id] = props.Index;
		context.sequence_had_goto[context.last_sequence._sequence_id] = true;
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

private func EvalAct_Sound(proplist props, proplist context)
{
	if (!props.Sound) return;
	var sound_context = props.SourceObject ?? Global;
	var volume = EvaluateValue("Integer", props.Volume, context);
	var pitch = EvaluateValue("Integer", props.Pitch, context);
	if (props.TargetPlayers == "all_players")
	{
		sound_context->Sound(props.Sound, true, volume, nil, props.Loop, nil, pitch);
	}
	else
	{
		for (var plr in EvaluateValue("PlayerList", props.TargetPlayers, context))
		{
			sound_context->Sound(props.Sound, false, volume, plr, props.Loop, nil, pitch);
		}
	}
}

private func EvalAct_CreateObject(proplist props, proplist context)
{
	// Create a new object
	var create_id = EvaluateValue("Definition", props.ID, context);
	if (!create_id) return;
	var owner = EvaluateValue("Player", props.Owner, context);
	var container = EvaluateValue("Object", props.Container, context);
	var obj;
	if (container)
	{
		// Contained object
		obj = container->CreateContents(create_id);
		if (obj) obj->SetOwner(owner);
	}
	else
	{
		// Uncontained object
		var position = EvaluatePosition(props.Position, context);
		var speed_x = EvaluateValue("Integer", props.SpeedX, context);
		var speed_y = EvaluateValue("Integer", props.SpeedY, context);
		if (props.CreateAbove)
			obj = Global->CreateObjectAbove(create_id, position[0], position[1], owner);
		else
			obj = Global->CreateObject(create_id, position[0], position[1], owner);
		// Default speed
		if (obj) obj->SetXDir(speed_x, 100);
		if (obj) obj->SetYDir(speed_y, 100);
	}
	// Remember object for later access
	context.last_created_object = obj;
}

private func EvalAct_RemoveObject(proplist props, proplist context)
{
	var obj = EvaluateValue("Object", props.Object, context);
	if (!obj) return;
	obj->RemoveObject(props.EjectContents);
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
	// Determine triggering player+objects
	var triggering_clonk;
	// Triggering player unknown? Try fallback to the controller of the triggering object
	if (!GetType(triggering_player) && triggering_object)
	{
		triggering_player = triggering_object->GetController();
	}
	// Triggering clonk is the selected clonk of the triggering player
	if (GetType(triggering_player))
	{
		triggering_clonk = GetCursor(triggering_player);;
		if (!triggering_clonk) triggering_clonk = GetCrew(triggering_player);
	}
	// Triggering object: Fallback to triggering player clonk
	if (!triggering_object) triggering_object = triggering_clonk;
	// Init context settings
	this.action_object = action_object;
	this.triggering_object = triggering_object;
	this.triggering_clonk = triggering_clonk;
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
		sequence_progress[last_sequence._sequence_id] = opt.Goto;
		hold = nil;
	}
	UserAction->ResumeAction(this, hold);
}

public func SaveScenarioObject(props) { return false; } // temp. don't save.
