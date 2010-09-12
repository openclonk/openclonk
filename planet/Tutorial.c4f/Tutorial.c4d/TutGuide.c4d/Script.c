/*-- 
	Tutorial Guide
	Author: Maikel
		
	The tutorial guide can be clicked on by the player, it supplies the player with information and hints.
	Callbacks to the scenario script:
	* OnGuideMessageShown(int plr, int index) when a message is shown
	* OnGuideMessageRemoved(int plr, int index) when a message is removed, all events
--*/


local messages; // A container to hold all messages.
local index; // Progress in reading messages.

protected func Initialize()
{
	// parallaxity
	this["Parallaxity"] = [0, 0];
	// visibility
	this["Visibility"] = VIS_Owner;
	messages = [];
	index = 0;
	return;
}

// Creates the tutorial guide in the upper hud, returns the guide as a pointer.
global func CreateTutorialGuide(int plr)
{
	var guide = CreateObject(TutorialGuide, 0, 0 , plr);
	guide->SetPosition(- 128 - 32 - TutorialGuide->GetDefWidth() / 2, 8 + TutorialGuide->GetDefHeight() / 2);
	return guide;
}

// Get-Setters for the message index.
public func GetGuideIndex() { return index; }
public func SetGuideIndex(int to_index)
{
	index = BoundBy(to_index, 0, GetLength(messages));
	return;
} 

// Add a message to the guide, the index is set to this message.
public func AddGuideMessage(string msg)
{
	// Automatically set index to current.
	index = GetLength(messages);
	// Add message to list.
	messages[index] = msg;
	// Make visible that there is a new message.
	AddEffect("NotifyPlayer", this, 100, 72, this);
	return;
}

// Shows a guide message to the player, also resets the index to that point.
public func ShowGuideMessage(int show_index)
{
	index = Max(0, show_index);
	if (!messages[index]) 
		return;
	if (GetEffect("NotifyPlayer", this))
		RemoveEffect("NotifyPlayer", this);
	GuideMessage(index);
	if (messages[index + 1]) 
		index++;
	return;
}

// Removes the current guide message.
public func ClearGuideMessage()
{
	if (GetEffect("MessageShown", this))
		RemoveEffect("MessageShown", this);
	CustomMessage("", nil, GetOwner(), nil, nil, nil, nil, nil, MSG_HCenter);
	return;
}

// Callback: the player has clicked on the guide.
public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (GetEffect("NotifyPlayer", this))
		RemoveEffect("NotifyPlayer", this);
	// Clear guide message if the latest is already shown.
	var effect = GetEffect("MessageShown", this, nil, 0);
	if (effect && EffectVar(0, this, effect) == index)
		return ClearGuideMessage();
	// Show guide message if there is a new one, and increase index if possible.
	if (!messages[index]) 
		return;
	GuideMessage(index);
	if (messages[index + 1]) 
		index++;
	return;
}

// Shows a message at the right place in message box.
private func GuideMessage(int show_index)
{
	if (GetOwner() == NO_OWNER)
		return false;
	var message = messages[show_index];
	if (!message)
		return false;
	// Guide portrait.
	var portrait_def = "Portrait:TutorialGuide::00ff00::1";
	// Message as regular one, don't stop the player.
	CustomMessage(message, nil, GetOwner(), 0, 16 + TutorialGuide->GetDefHeight(), 0xffffff, GUI_MenuDeco, portrait_def, MSG_HCenter);
	var effect = AddEffect("MessageShown", this, 100, 2 * GetLength(message), this);
	EffectVar(0, this, effect) = show_index;
	return true;
}

// Effect exists as long as the message is shown.
// Message string is stored in EffectVar 0.
// TODO account for infinite messages, those with @ in front.
protected func FxMessageShownStart(object target, int num, int temporary)
{
	if (temporary == 0)
		GameCall("OnGuideMessageShown", target->GetOwner(), index);
	return 1;
}

protected func FxMessageShownTimer(object target, int num, int time)
{
	// Delete effect if time has passed, i.e. message has disappeared.
	if (time)
		return -1;
	return 1;
}

protected func FxMessageShownStop(object target, int num, int reason, bool temporary)
{
	if (!temporary)
		GameCall("OnGuideMessageRemoved", target->GetOwner(), index);
	return 1;
}

// Effect to display the player notification for some time.
protected func FxNotifyPlayerStart(object target, int num, int temporary)
{
	// Display notifier.
	SetGraphics("0", Icon_Number, 1, GFXOV_MODE_Base); //TODO get rid of this placeholder image.
	SetObjDrawTransform(500, 0, 10000, 0, 500, -10000, 1);
	return 1;
}

protected func FxNotifyPlayerTimer(object target, int num, int time)
{
	// Delete effect if time has passed.
	if (time)
		return -1;	
	return 1;
}

protected func FxNotifyPlayerStop(object target, int num, int reason, bool temporary)
{
	// Remove notifier.
	SetGraphics(nil, nil, 1);
	return 1;
}

local Name = "$Name$";

