/**
	Dialogue
	
	Attach to a non player charachter to provide a message interface.
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

static const DLG_Status_Active = 0; // next interaction calls progress function
static const DLG_Status_Stop   = 1; // dialogue is done and menu closed on next interaction
static const DLG_Status_Remove = 2; // dialogue is removed on next interaction
static const DLG_Status_Wait   = 3; // dialogue is deactivated temporarily to prevent accidental restart after dialogue end


/*-- Dialogue creation --*/

// Sets a new dialogue for a npc.
global func SetDialogue(string name, bool attention)
{
	if (!this)
		return;
	var dialogue = CreateObject(Dialogue);
	dialogue->InitDialogue(name, this, attention);
	
	dialogue->SetObjectLayer(nil);
	dialogue.Plane = this.Plane+1; // for proper placement of the attention symbol

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
func FindByTarget(object target)
{
	if (!target) return nil;
	return FindObject(Find_ID(Dialogue), Find_ActionTarget(target));
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
	
	// Custom dialogue initialization
	if (!Call(Format("~Dlg_%s_Init", dlg_name), dlg_target))
		GameCall(Format("~Dlg_%s_Init", dlg_name), this, dlg_target);
	
	return;
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

private func AttentionEffect() { return SetAction("DialogueAttentionEffect", dlg_target); }

private func UpdateDialogue()
{
	// Adapt size to target. Updateing to direction does not work well for NPCs that walk around 
	// It's also not very intuitive if the player just walks to the attention symbol anyway.
	var wdt = dlg_target->GetID()->GetDefWidth() + 10;
	var hgt = dlg_target->GetID()->GetDefHeight();
	//var dir = dlg_target->GetDir();
	SetShape(-wdt/2, -hgt/2, wdt, hgt);
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

// to be called from within dialogue after the last message
public func StopDialogue()
{
	// put on wait for a while; then reenable
	SetDialogueStatus(DLG_Status_Wait);
	ScheduleCall(this, this.SetDialogueStatus, 30, 1, DLG_Status_Stop);
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
		dlg_status = DLG_Status_Active;
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
	for(var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		MessageBox(message, GetCursor(plr), talker, plr, as_message);
	}
}

// Message box as dialog to player with a message copy to all other players
public func MessageBoxBroadcast(string message, object clonk, object talker, array options)
{
	// message copy to other players
	for(var i = 0; i < GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		if (GetCursor(plr) != clonk)
			MessageBox(message, GetCursor(plr), talker, plr, true);
	}
	// main message as dialog box
	return MessageBox(message, clonk, talker, nil, false, options);
}

static MessageBox_last_talker, MessageBox_last_pos;

private func MessageBox(string message, object clonk, object talker, int for_player, bool as_message, array options)
{
	// broadcast enabled: message copy to other players
	if (dlg_broadcast && !as_message)
	{
		for(var i = 0; i < GetPlayerCount(C4PT_User); ++i)
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
		var menu_target, cmd;
		if (this != Dialogue)
		{
			menu_target = this;
			cmd = "MenuOK";
		}
		clonk->CreateMenu(Dialogue, menu_target, C4MN_Extra_None, nil, nil, C4MN_Style_Dialog, false, Dialogue);
		
		// Add NPC portrait.
		//var portrait = Format("%i", talker->GetID()); //, Dialogue, talker->GetColor(), "1");
		if (talker)
			if (portrait)
				clonk->AddMenuItem("", nil, Dialogue, nil, clonk, nil, C4MN_Add_ImgPropListSpec, portrait);
			else
				clonk->AddMenuItem("", nil, Dialogue, nil, clonk, nil, C4MN_Add_ImgObject, talker);

		// Add NPC message.
		clonk->AddMenuItem(message, nil, nil, nil, clonk, nil, C4MN_Add_ForceNoDesc);
		
		// Add answers.
		if (options) for (var option in options)
		{
			var option_text, option_command;
			if (GetType(option) == C4V_Array)
			{
				// Text+Command given
				option_text = option[0];
				option_command = option[1];
				if (GetChar(option_command) == GetChar("#"))
				{
					// Command given as section name: Remove leading # and call section change
					var ichar=1, ocmd = "", c;
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
			CustomMessage("", nil, for_player, 0,0, nil, nil, nil, MSG_Right);  // clear prev msg
		}
		CustomMessage(message, nil, for_player, xoff,150, nil, GUI_MenuDeco, portrait ?? talker, flags);
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
	// Force dependency on all contained objects, so dialogue initialization procedure can access them
	var i=0, obj;
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
