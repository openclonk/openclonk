/**
	Dialogue
	Attach to a non player charachter to provide a message interface.
	
	@author Sven
*/


local dlg_target;
local dlg_name;
local dlg_info;
local dlg_progress;
local dlg_section;   // if set, this string is included in progress callbacks (i.e., func Dlg_[Name]_[Section][Progress]() is called)
local dlg_status;
local dlg_interact;  // default true. can be set to false to deactivate the dialogue
local dlg_attention; // if set, a red attention mark is put above the clonk
local dlg_broadcast; // if set, all non-message (i.e. menu) MessageBox calls are called as MessageBoxBroadcast.
local dlg_last_opt_sel; // may contain array with recently selected options
local user_dialogue; // Dialogue action set by user in editor
local user_dialogue_allow_parallel;
local user_dialogue_progress_mode;

static const DLG_Status_Active = 0; // next interaction calls progress function
static const DLG_Status_Stop   = 1; // dialogue is done and menu closed on next interaction
static const DLG_Status_Remove = 2; // dialogue is removed on next interaction
static const DLG_Status_Wait   = 3; // dialogue is deactivated temporarily to prevent accidental restart after dialogue end

public func IsDialogue() { return true; }

/*-- Dialogue creation --*/

// Sets a new dialogue for a npc.
global func SetDialogue(string name, bool attention)
{
	if (!this)
		return;
	var dialogue = Dialogue->FindByTarget(this);
	if (!dialogue) dialogue = CreateObject(Dialogue);
	dialogue->InitDialogue(name, this, attention);
	
	dialogue->SetObjectLayer(nil);
	dialogue.Plane = this.Plane + 1; // for proper placement of the attention symbol

	return dialogue;
}

// Removes the existing dialogue of an object.
global func RemoveDialogue()
{
	var dialogue = Dialogue->FindByTarget(this);
	if (dialogue) return dialogue->RemoveObject();
	return false;
}

// Find dialogue attached to a target (definition call, e.g. var dlg = Dialogue->FindByTarget(foo))
public func FindByTarget(object target)
{
	if (!target) return nil;
	return FindObject(Find_ID(Dialogue), Find_ActionTarget(target));
}

// Find dialogue with a given name.
public func FindByName(string name)
{
	if (!name) return nil;
	return FindObject(Find_ID(Dialogue), Find_Func("HasName", name));
}

public func HasName(string name)
{
	return name == dlg_name;
}

/*-- Dialogue properties --*/

protected func Initialize()
{
	// Dialogue progress to one.
	dlg_progress = 1;
	// Dialogue allows interaction by default.
	dlg_interact = true;
	// Dialogue is active by default.
	dlg_status = DLG_Status_Active;
	return;
}

public func InitDialogue(string name, object target, bool attention)
{
	dlg_target = target;
	dlg_name = name;

	// Attach dialogue object to target.
	if (attention)
	{
		// Attention: Show exclamation mark and glitter effect every five seconds
		AddAttention();
	}
	else
	{
		// No attention: Set invisible action
		SetAction("Dialogue", target);
		RemoveAttention();
	}
	
	// Update dialogue to target.
	UpdateDialogue();
	
	// Effect on targets to remove the dialogue when target dies or is removed
	AddEffect("IntDialogue", target, 1, 0, this);
	
	// Custom dialogue initialization
	if (!Call(Format("~Dlg_%s_Init", dlg_name), dlg_target))
		GameCall(Format("~Dlg_%s_Init", dlg_name), this, dlg_target);
	
	return true;
}

private func FxIntDialogueStop(object target, proplist fx, int reason, bool temp)
{
	// Target removed or died: Remove dialogue
	if (!temp) RemoveObject();
	return FX_OK;
}

public func AddAttention()
{
	// Attention: Show exclamation mark and glitter effect every five seconds
	if (!dlg_attention)
	{
		SetAction("DialogueAttention", dlg_target);
		RemoveTimer("AttentionEffect"); AddTimer("AttentionEffect", 36*5);
		dlg_attention = true;
	}
	return true;
}

public func RemoveAttention()
{
	// No longer show exclamation mark and glitter effects
	if (dlg_attention)
	{
		RemoveTimer("AttentionEffect");
		if (dlg_target) SetAction("Dialogue", dlg_target);
		dlg_attention = false;
	}
	return true;
}

public func SetAttention(bool to_val)
{
	if (to_val) return AddAttention(); else return RemoveAttention();
}

private func AttentionEffect() { return SetAction("DialogueAttentionEffect", dlg_target); }

private func UpdateDialogue()
{
	// Adapt size to target. Updateing to direction does not work well for NPCs that walk around 
	// It's also not very intuitive if the player just walks to the attention symbol anyway.
	var wdt = dlg_target->GetID()->GetDefWidth() + 10;
	var hgt = dlg_target->GetID()->GetDefHeight();
	//var dir = dlg_target->GetDir();
	SetShape(-wdt/2, -hgt/2, wdt, hgt);
	// Transfer position immediately so it's updated in paused mode
	SetPosition(dlg_target->GetX(), dlg_target->GetY());
	// Transfer target name.
	//SetName(Format("$MsgSpeak$", dlg_target->GetName()));
	return;
}

public func SetDialogueInfo()
{

	return;
}

public func SetInteraction(bool allow)
{
	dlg_interact = allow;
	return;
}

public func SetDialogueProgress(int progress, string section, bool add_attention)
{
	dlg_progress = Max(1, progress);
	dlg_section = section;
	if (add_attention) AddAttention();
	return;
}

public func SetDialogueStatus(int status)
{
	dlg_status = status;
	return;
}

public func GetDialogueTarget()
{
	return dlg_target;
}

public func SetDialogueTarget(object target)
{
	// Change dialogue target
	// Do not allow nil
	if (!target) return;
	// Update attachment and ! marker
	var had_attention = dlg_attention;
	RemoveAttention();
	dlg_target = target;
	if (had_attention) AddAttention(); else SetAction("Dialogue", dlg_target);
	// Update shape
	UpdateDialogue();
	return true;
}

// to be called from within dialogue after the last message
public func StopDialogue()
{
	// clear remembered positions
	dlg_last_opt_sel = nil;
	// put on wait for a while; then reenable
	SetDialogueStatus(DLG_Status_Stop);
	return true;
}

/*-- Interaction --*/

// Players can talk to NPC via the interaction bar.
public func IsInteractable() { return dlg_interact; }

// Adapt appearance in the interaction bar.
public func GetInteractionMetaInfo(object clonk)
{
	if (InDialogue(clonk))
		return { Description = Format("$MsgSpeak$", dlg_target->GetName()) , IconName = nil, IconID = Clonk, Selected = true };

	return { Description = Format("$MsgSpeak$", dlg_target->GetName()) , IconName = nil, IconID = Clonk, Selected = false };
}

// Advance dialogue from script
public func CallDialogue(object clonk, progress, string section)
{
	if (GetType(progress)) SetDialogueProgress(progress, section);
	return Interact(clonk);
}

// Called on player interaction.
public func Interact(object clonk)
{
	// Should not happen: not active -> stop interaction
	if (!dlg_interact)
		return true;	
	
	// Currently in a dialogue: abort that dialogue.
	if (InDialogue(clonk))
		clonk->CloseMenu();
		
	// User sequence provided in editor?
	if (user_dialogue)
	{
		UserAction->EvaluateAction(user_dialogue, this, clonk, nil, user_dialogue_progress_mode, user_dialogue_allow_parallel);
		return true;
	}
	
	// No conversation context: abort.
	if (!dlg_name)
		return true;
		
	// Dialogue still waiting? Do nothing then
	// (A sound might be nice here)
	if (dlg_status == DLG_Status_Wait)
	{
		return true;
	}
		
	// Stop dialogue?
	if (dlg_status == DLG_Status_Stop)
	{
		clonk->CloseMenu();
		dlg_status = DLG_Status_Wait;
		ScheduleCall(this, this.SetDialogueStatus, 30, 0, DLG_Status_Active);
		// Do a call on a closed dialogue as well.
		var fn_closed = Format("~Dlg_%s_Closed", dlg_name);
		if (!Call(fn_closed, clonk, dlg_target))
			GameCall(fn_closed, this, clonk, dlg_target);
		return true;
	}
	// Remove dialogue?
	if (dlg_status == DLG_Status_Remove)
	{
		clonk->CloseMenu();
		RemoveObject();
		return true;		
	}
	
	// Remove attention mark on first interaction
	RemoveAttention();
	
	// Have speakers face each other
	SetSpeakerDirs(dlg_target, clonk);

	// Start conversation context.
	// Update dialogue progress first.
	var progress = dlg_progress;
	dlg_progress++;
	// Then call relevant functions.
	// Call generic function first, then progress function
	var fn_generic = Format("~Dlg_%s", dlg_name);
	var fn_progress = Format("~Dlg_%s_%s%d", dlg_name, dlg_section ?? "", progress);
	if (!Call(fn_generic, clonk))
		if (!GameCall(fn_generic, this, clonk, dlg_target))
			if (!Call(fn_progress, clonk))
				GameCall(fn_progress, this, clonk, dlg_target);

	return true;
}

private func InDialogue(object clonk)
{
	return clonk->GetMenu() == Dialogue;
}

public func MessageBoxAll(string message, object talker, bool as_message)
{
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(message, GetCursor(plr), talker, plr, as_message);
	}
}

// Message box as dialog to player with a message copy to all other players
public func MessageBoxBroadcast(string message, object clonk, object talker, array options)
{
	// message copy to other players
	for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		if (GetCursor(plr) != clonk)
			MessageBox(message, GetCursor(plr), talker, plr, true);
	}
	// main message as dialog box
	return MessageBox(message, clonk, talker, nil, false, options);
}

static MessageBox_last_talker, MessageBox_last_pos;

private func MessageBox(string message, object clonk, object talker, int for_player, bool as_message, array options, proplist menu_target)
{
	// broadcast enabled: message copy to other players
	if (dlg_broadcast && !as_message)
	{
		for (var i = 0; i < GetPlayerCount(C4PT_User); ++i)
		{
			var other_plr = GetPlayerByIndex(i, C4PT_User);
			if (GetCursor(other_plr) != clonk)
				MessageBox(message, GetCursor(other_plr), talker, other_plr, true);
		}
	}
	// Use current NPC as talker if unspecified.
	// On definition call or without talker, just show the message without a source
	if (!talker && this != Dialogue) talker = dlg_target;
	if (talker) message = Format("<c %x>%s:</c> %s", talker->GetColor(), talker->GetName(), message);
	var portrait;
	if (talker) portrait = talker->~GetPortrait();
	
	// A target Clonk is given: Use a menu for this dialogue.
	if (clonk && !as_message)
	{
		var cmd;
		if (this != Dialogue) menu_target = this;
		if (menu_target) cmd = "MenuOK";
		clonk->CreateMenu(Dialogue, menu_target, C4MN_Extra_None, nil, nil, C4MN_Style_Dialog, false, Dialogue);
		var menu_item_offset = 0;
		
		// Add NPC portrait.
		//var portrait = Format("%i", talker->GetID()); //, Dialogue, talker->GetColor(), "1");
		if (talker)
			if (portrait)
				menu_item_offset += clonk->AddMenuItem("", nil, Dialogue, nil, clonk, nil, C4MN_Add_ImgPropListSpec, portrait);
			else
				menu_item_offset += clonk->AddMenuItem("", nil, Dialogue, nil, clonk, nil, C4MN_Add_ImgObject, talker);

		// Add NPC message.
		menu_item_offset += clonk->AddMenuItem(message, nil, nil, nil, clonk, nil, C4MN_Add_ForceNoDesc);
		
		// Add answers.
		if (options) for (var option in options)
		{
			var option_text, option_command;
			if (GetType(option) == C4V_Array)
			{
				// Text + Command given
				option_text = option[0];
				option_command = option[1];
				if (GetChar(option_command) == GetChar("#"))
				{
					// Command given as section name: Remove leading # and call section change
					var ichar = 1, ocmd = "", c;
					while (c = GetChar(option_command, ichar++)) ocmd = Format("%s%c", ocmd, c);
					option_command = Format("CallDialogue(Object(%d), 1, \"%s\")", clonk->ObjectNumber(), ocmd);
				}
				else
				{
					// if only a command is given, the standard parameter is just the clonk
					if (!WildcardMatch(option_command, "*(*")) option_command = Format("%s(Object(%d))", option_command, clonk->ObjectNumber());
				}
			}
			else
			{
				// Only text given - command means regular dialogue advance
				option_text = option;
				option_command = cmd;
			}
			clonk->AddMenuItem(option_text, option_command, nil, nil, clonk, nil, C4MN_Add_ForceNoDesc);
		}
		
		// If there are no answers, add a next entry
		if (cmd && !options) clonk->AddMenuItem("$Next$", cmd, nil, nil, clonk, nil, C4MN_Add_ForceNoDesc);
		
		// When reaching the same options set while clicking through a dialogue, pre-select the next item
		if (options)
		{
			if (!menu_target.dlg_last_opt_sel) menu_target.dlg_last_opt_sel = [];
			var found_remembered_option = false, remembered_opts, i = 0;
			for (remembered_opts in menu_target.dlg_last_opt_sel)
			{
				if (DeepEqual(remembered_opts.options, options))
				{
					found_remembered_option = true;
					break;
				}
				++i;
			}
			if (found_remembered_option)
			{
				// We've seen this before. Pre-select the next option
				clonk->SelectMenuItem(++remembered_opts.sel);
			}
			else
			{
				// First encounter of this option set: Select first item.
				menu_target.dlg_last_opt_sel[i] = { options = options[:], sel = menu_item_offset };
			}
		}
		
		// Set menu decoration.
		clonk->SetMenuDecoration(GUI_MenuDeco);
		
		// Set text progress to NPC name.
		if (talker)
		{
			var name = talker->GetName();
			var n_length;
			while (GetChar(name, n_length))
				n_length++;
			clonk->SetMenuTextProgress(n_length + 1);
		}
	}
	else
	{
		// No target is given: Global (player) message
		if (!GetType(for_player)) for_player = NO_OWNER;
		// altenate left/right position as speakers change
		if (talker != MessageBox_last_talker) MessageBox_last_pos = !MessageBox_last_pos;
		MessageBox_last_talker = talker;
		var flags = 0, xoff = 150;
		if (!MessageBox_last_pos)
		{
			flags = MSG_Right;
			xoff *= -1;
			CustomMessage("", nil, for_player); // clear prev msg
		}
		else
		{
			CustomMessage("", nil, for_player, 0, 0, nil, nil, nil, MSG_Right);  // clear prev msg
		}
		CustomMessage(message, nil, for_player, xoff, 150, nil, GUI_MenuDeco, portrait ?? talker, flags);
	}

	return;
}

public func MenuOK(proplist menu_id, object clonk)
{
	// prevent the menu from closing when pressing MenuOK
	if (dlg_interact)
		Interact(clonk);
}

// Enable or disable message broadcasting to all players for important dialogues
public func SetBroadcast(bool to_val)
{
	dlg_broadcast = to_val;
	return true;
}

public func SetSpeakerDirs(object speaker1, object speaker2)
{
	// Force direction of two clonks to ace each other for dialogue
	if (!speaker1 || !speaker2) return false;
	speaker1->SetDir(speaker1->GetX() < speaker2->GetX());
	speaker2->SetDir(speaker1->GetX() > speaker2->GetX());
	return true;
}

public func SetUserDialogue(new_user_dialogue, new_user_dialogue_progress_mode, new_user_dialogue_allow_parallel)
{
	user_dialogue = new_user_dialogue;
	user_dialogue_progress_mode = new_user_dialogue_progress_mode;
	user_dialogue_allow_parallel = new_user_dialogue_allow_parallel;
	return true;
}

public func SetEnabled(bool to_val)
{
	dlg_interact = to_val;
	return true;
}

/* Scenario saving */

// Scenario saving
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (!dlg_target) return false; // don't save dead dialogue object
	// Dialog has its own creation procedure
	props->RemoveCreation();
	props->Remove("Plane"); // updated when setting dialogue
	props->Add(SAVEOBJ_Creation, "%s->SetDialogue(%v,%v)", dlg_target->MakeScenarioSaveName(), dlg_name, !!dlg_attention);
	// Set properties
	if (user_dialogue || user_dialogue_progress_mode || user_dialogue_allow_parallel) props->AddCall("UserDialogue", this, "SetUserDialogue", user_dialogue, Format("%v", user_dialogue_progress_mode), user_dialogue_allow_parallel);
	if (!dlg_interact) props->AddCall("Enabled", this, "SetEnabled", dlg_interact);
	// Force dependency on all contained objects, so dialogue initialization procedure can access them
	var i = 0, obj;
	while (obj = dlg_target->Contents(i++)) obj->MakeScenarioSaveName();
	return true;
}




/* Properties */

local ActMap = {
	Dialogue = {
		Prototype = Action,
		Name = "Dialogue",
		Procedure = DFA_ATTACH,
		Delay = 0,
		NextAction = "Dialogue",
	},
	DialogueAttention = {
		Prototype = Action,
		Name = "DialogueAttention",
		Procedure = DFA_ATTACH,
		X = 0, Y = 0, Wdt = 8, Hgt = 24, OffX = 0, OffY = -30,
		Delay = 0,
		NextAction = "DialogueAttention",
	},
	DialogueAttentionEffect = {
		Prototype = Action,
		Name = "DialogueAttentionEffect",
		Procedure = DFA_ATTACH,
		X = 0, Y = 0, Wdt = 8, Hgt = 24, OffX = 0, OffY = -30,
		Delay = 2,
		Length = 4,
		NextAction = "DialogueAttentionREffect",
	},
	DialogueAttentionREffect = {
		Prototype = Action,
		Name = "DialogueAttentionREffect",
		Procedure = DFA_ATTACH,
		X = 0, Y = 0, Wdt = 8, Hgt = 24, OffX = 0, OffY = -30,
		Delay = 2,
		Length = 4,
		Reverse = 1,
		NextAction = "DialogueAttention",
	}
};
local Name = "$Name$";
local Description = "$Description$";


/* EditorProps */

private func EvalObj_NPC(proplist props, proplist context) { if (context.action_object) return context.action_object.dlg_target; }

private func EvalAct_Message(proplist props, proplist context)
{
	//Log("EvalAct_Message %v %v", props, context);
	// Message parameters
	var after_message = props.AfterMessage;
	var speaker = UserAction->EvaluateValue("Object", props.Speaker, context);
	var n_options = 0, any_message = false;
	if (props.Options) n_options = GetLength(props.Options);
	if (n_options)
	{
		var options_msg = CreateArray(n_options), i = 0;
		for (var opt in props.Options)
		{
			opt._goto = UserAction->EvaluateValue("Integer", opt.Goto, context); // evaluated in UserAction menu callback
			options_msg[i] = [UserAction->EvaluateString(opt.Text, context), Format("MenuSelectOption(%d)", i)];
			++i;
		}
		props._options_msg = options_msg;
	}
	var text = UserAction->EvaluateString(props.Text, context);
	// Show message to desired players
	for (var plr in UserAction->EvaluateValue("PlayerList", props.TargetPlayers, context))
	{
		Dialogue->MessageBox(text, context.triggering_object, speaker, plr, after_message != "next" && !n_options, props._options_msg, context);
		any_message = true;
	}
	// After-message-option
	if (GetType(after_message) == C4V_Int)
	{
		// Wait for some time
		context.hold = props;
		context->ScheduleCall(context, UserAction.ResumeAction, after_message, 1, context, props);
	}
	else if (after_message == "next" || n_options)
	{
		// Wait for user to press "Next" on dialogue or select an option
		// (If there are options, suspend or stop setting does not make sense)
		if (any_message) context.hold = props;
	}
	else if (after_message == "suspend")
	{
		// Wait for re-initiation of action
		context.suspended = true;
		context.hold = props;
	}
	else if (after_message == "stop")
	{
		// Reset action
		context.suspended = true;
		context.hold = nil;
	}
}

private func EvalAct_SetAttention(proplist props, proplist context)
{
	// User action: Set dialogue attention marker
	var target = UserAction->EvaluateValue("Object", props.Target, context);
	var status = UserAction->EvaluateValue("Boolean", props.Status, context);
	if (!target) return;
	if (target->GetID() != Dialogue)
		if (!(target = Dialogue->FindByTarget(target)))
			return;
	target->~SetAttention(status);
}

private func EvalAct_SetEnabled(proplist props, proplist context)
{
	// User action: Enable/disable dialogue
	var target = UserAction->EvaluateValue("Object", props.Target, context);
	var status = UserAction->EvaluateValue("Boolean", props.Status, context);
	if (!target) return;
	if (target->GetID() != Dialogue)
		if (!(target = Dialogue->FindByTarget(target)))
			return;
	target->~SetEnabled(status);
}

public func IsDialogue() { return true; }

public func Definition(def)
{
	// Actions provided by this definition
	UserAction->AddEvaluator("Action", "$Dialogue$", "$Message$", "$MessageDesc$", "message", [def, def.EvalAct_Message], def.GetDefaultMessageProp, { Type="proplist", Display="{{Speaker}}: \"{{Text}}\" {{Options}}", EditorProps = {
		Speaker = new UserAction.Evaluator.Object { Name = "$Speaker$" },
		Text = new UserAction.Evaluator.String { Name="$Text$", EditorHelp="$TextHelp$" },
		TargetPlayers = new UserAction.Evaluator.PlayerList { Name = "$TargetPlayers$" },
		AfterMessage = { Type="enum", Options = [{ Name="$ContinueAction$" }, { Name="$WaitForNext$", Value="next" }, { Name="$SuspendAction$", Value="suspend" }, { Name="$StopAction$", Value="stop" }, { Name="$WaitTime$", Value = 60, Type = C4V_Int, Delegate={ Type="int", Min = 1 } }] },
		Options = { Name="$Options$", Type="array", Display = 3, DefaultValue = { Text = { Function="string_constant", Value="$DefaultOptionText$" }, Goto = { Function="int_constant", Value = 0 } }, Elements = { Type="proplist", Display="({{Goto}}) {{Text}}", EditorProps = {
			Text = new UserAction.Evaluator.String { Name="$Text$", EditorHelp="$OptionTextHelp$" },
			Goto = new UserAction.Evaluator.Integer { Name="$Goto$", EditorHelp="$GotoHelp$" }
			} } }
		} } );
	UserAction->AddEvaluator("Action", "$Dialogue$", "$SetAttention$", "$SetAttentionDesc$", "dialogue_set_attention", [def, def.EvalAct_SetAttention], { Target = { Function="action_object" }, Status = { Function="bool_constant", Value = true } }, { Type="proplist", Display="{{Target}}: {{Status}}", EditorProps = {
		Target = UserAction->GetObjectEvaluator("IsDialogue", "$Dialogue$"),
		Status = new UserAction.Evaluator.Boolean { Name = "$Status$" }
		} } );
	UserAction->AddEvaluator("Action", "$Dialogue$", "$SetEnabled$", "$SetEnabledDesc$", "dialogue_set_enabled", [def, def.EvalAct_SetEnabled], { Target = { Function="action_object" }, Status = { Function="bool_constant", Value = true } }, { Type="proplist", Display="{{Target}}: {{Status}}", EditorProps = {
		Target = UserAction->GetObjectEvaluator("IsDialogue", "$Dialogue$"),
		Status = new UserAction.Evaluator.Boolean { Name = "$Status$" }
		} } );
	UserAction->AddEvaluator("Object", nil, "$NPC$", "$NPCDesc$", "npc", [def, def.EvalObj_NPC]);
	// Clonks can create a dialogue
	if (!Clonk.EditorActions) Clonk.EditorActions = {};
	Clonk.EditorActions.Dialogue = { Name="$Dialogue$", EditorHelp = "$DialogueHelp$", Command="SetDialogue(\"Editor\", true)", Select = true };
	// Dialogue EditorProps
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.dlg_target = { Name="$Target$", EditorHelp="$TargetDesc$", Type="object", Filter="IsClonk", Set="SetDialogueTarget" };
	def.EditorProps.user_dialogue = { Name="$Dialogue$", EditorHelp="$DialogueDesc$", Type="enum", OptionKey="Function", Options = [ { Name="$NoDialogue$" }, new UserAction.EvaluatorDefs.sequence { Group = nil, Value = { Function="sequence", Actions=[] } } ] };
	def.EditorProps.user_dialogue_allow_parallel = UserAction.PropParallel;
	def.EditorProps.user_dialogue_progress_mode = UserAction.PropProgressMode;
	def.EditorProps.dlg_attention = { Name="$Attention$ (!)", EditorHelp="$AttentionDesc$", Type="bool", Set="SetAttention" };
	def.EditorProps.dlg_interact = { Name="$Enabled$", EditorHelp="$EnabledDesc$", Type="bool", Set="SetEnabled" };
}

private func GetDefaultMessageProp(object target_object)
{
	if (target_object && target_object->~IsDialogue())
	{
		// Message prop for dialogue: Default is a NPC message with a "wait for next" option
		return { Function="message", Speaker = { Function="npc" }, TargetPlayers = { Function="triggering_player_list" }, Text = { Function="string_constant", Value="$DefaultDialogueMessage$" }, AfterMessage="next", Options=[] };
	}
	else
	{
		// Message prop for other dialogue: Default is a triggering clonk message with a "wait for next" option
		return { Function="message", Speaker = { Function="triggering_clonk" }, TargetPlayers = { Function="triggering_player_list" }, Text = { Function="string_constant", Value="$DefaultMessage$" }, AfterMessage = 60, Options=[] };
	}
}

// Editor object drop happens easily - so move stuff directly to target
public func EditorCollection(obj)
{
	if (dlg_target && obj) obj->Enter(dlg_target);	
}
