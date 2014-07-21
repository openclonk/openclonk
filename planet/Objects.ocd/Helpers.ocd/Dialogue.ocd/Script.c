/**
	Dialogue
	
	Attach to a non player charachter to provide a message interface.
*/


local dlg_target;
local dlg_name;
local dlg_info;
local dlg_progress;
local dlg_status;
local dlg_interact;
local dlg_attention;

static const DLG_Status_Active = 0;
static const DLG_Status_Stop = 1;
static const DLG_Status_Remove = 2;


/*-- Dialogue creation --*/

// Sets a new dialogue for a npc.
global func SetDialogue(string name, bool attention)
{
	if (!this)
		return;
	var dialogue = CreateObject(Dialogue);
	dialogue->InitDialogue(name, this, attention);
	
	dialogue->SetObjectLayer(nil);

	return dialogue;
}

// Removes the existing dialogue of an object.
global func RemoveDialogue()
{
	if (!this)
		return;
		
	var dialogue = FindObject(Find_ID(Dialogue), Find_ActionTarget(this));
	if (dialogue)
		dialogue->RemoveObject();	

	return;
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
	// Adapt size to target and its direction.
	var wdt = dlg_target->GetID()->GetDefWidth();
	var hgt = dlg_target->GetID()->GetDefHeight();
	var dir = dlg_target->GetDir();
	SetShape(-wdt/2 + 2*(dir-1)*wdt, -hgt/2, 3*wdt, hgt);
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

public func SetDialogueProgress(int progress)
{
	dlg_progress = Max(1, progress);
	return;
}

public func SetDialogueStatus(int status)
{
	dlg_status = status;
	return;
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

	// Start conversation context.
	// Update dialogue progress first.
	var progress = dlg_progress;
	dlg_progress++;
	// Then call relevant functions.
	if (!Call(Format("Dlg_%s_%d", dlg_name, progress), clonk))
		GameCall(Format("Dlg_%s_%d", dlg_name, progress), this, clonk, dlg_target);

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

private func MessageBox(string message, object clonk, object talker, int for_player, bool as_message)
{
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
		clonk->AddMenuItem(message, cmd, nil, nil, clonk, nil, C4MN_Add_ForceNoDesc);
		
		// Add answers.
		//for (var i = 0; i < GetLength(message.Answers); i++)
		//{
		//	var ans = message.Answers[i][0];
		//	var call_back = message.Answers[i][1];
		//	target->AddMenuItem(ans, call_back, nil, nil, target, nil, C4MN_Add_ForceNoDesc);
		//}
		
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
		CustomMessage(message, nil, for_player, 150,150, nil, GUI_MenuDeco, portrait ?? talker);
	}

	return;
}

public func MenuOK(proplist menu_id, object clonk)
{
	// prevent the menu from closing when pressing MenuOK
	if (dlg_interact)
		Interact(clonk);
}

/* Scenario saving */

// Scenario saving
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (!dlg_target) return false; // don't save dead dialogue object
	// Dialog has its own creation procedure
	props->RemoveCreation();
	props->Add(SAVEOBJ_Creation, "%s->SetDialogue(%v)", dlg_target->MakeScenarioSaveName(), dlg_name);
	return true;
}


/* Player deactivation during dialogues */

public func StartCinematics(object cinematics_target)
{
	// Disable crew of all players
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var j=0, crew;
		while (crew = GetCrew(plr, j++))
		{
			if (crew == GetCursor(plr)) crew.cinematics_was_cursor = true; else crew.cinematics_was_cursor = nil;
			crew->SetCrewEnabled(false);
			if (crew->~GetMenu()) crew->~GetMenu()->Close();
			crew->MakeInvincible();
			crew->SetCommand("None");
			crew->SetComDir(COMD_Stop);
		}
	}
	// Fix view on target
	if (cinematics_target) SetCinematicsTarget(cinematics_target);
	return true;
}

public func StopCinematics()
{
	SetCinematicsTarget(nil);
	// Reenable crew and reset cursor
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		var j=0, crew;
		while (crew = GetCrew(plr, j++))
		{
			crew->SetCrewEnabled(true);
			crew->ClearInvincible();
			if (crew.cinematics_was_cursor) SetCursor(plr, crew);
		}
	}
	return true;
}

// Force all player views on given target
public func SetCinematicsTarget(object cinematics_target)
{
	ClearScheduleCall(nil, Dialogue.UpdateCinematicsTarget);
	if (cinematics_target)
	{
		UpdateCinematicsTarget(cinematics_target);
		ScheduleCall(nil, Dialogue.UpdateCinematicsTarget, 30, 999999999, cinematics_target);
	}
	return true;
}

private func UpdateCinematicsTarget(object cinematics_target)
{
	// Force view of all players on target
	if (!cinematics_target) return;
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		SetPlrView(plr, cinematics_target);
	}
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
