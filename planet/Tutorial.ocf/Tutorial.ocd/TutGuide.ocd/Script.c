/** Tutorial Guide
* The tutorial guide can be clicked by the player, it supplies the player with information and hints.
* The following callbacks are made to the scenario script:
* - \c OnGuideMessageShown(int plr, int index) when a message is shown
* - \c OnGuideMessageRemoved(int plr, int index) when a message is removed, all events
* @author Maikel
*/


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

/* Creates the tutorial guide in the upper part of the HUD, returns the guide as a pointer.
* @param plr The player for which the guide should be created.
* @return the guide object.
*/
global func CreateTutorialGuide(int plr)
{
	var guide = CreateObjectAbove(TutorialGuide, 0, 0 , plr);
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

/* Adds a message to the guide. The internal index is set to this message meaning that this message will
* be shown if the player clicks the guide.
* @param msg Message that should be added to the message stack.
*/
public func AddGuideMessage(string msg)
{
	// Automatically set index to current.
	index = GetLength(messages);
	// Add message to list.
	messages[index] = msg;
	// Make visible that there is a new message.
	AddEffect("NotifyPlayer", this, 100, 0, this);
	return;
}

/* Shows a guide message to the player, also resets the internal index to that point.
*	@param show_index The message corresponding to this index will be shown.
*/
public func ShowGuideMessage(int show_index)
{
	index = Max(0, show_index);
	if (!messages[index])
		return;
	if (GetEffect("MessageShown", this))
		RemoveEffect("MessageShown", this);
	if (GetEffect("NotifyPlayer", this))
		RemoveEffect("NotifyPlayer", this);
	GuideMessage(index);
	if (GetLength(messages) > index+1)
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
	if (effect)
	{
		if (effect.show_index == index)
			return ClearGuideMessage();
		else
			RemoveEffect("MessageShown", this);
	}
	// Show guide message if there is a new one, and increase index if possible.
	if (!messages[index])
		return;
	GuideMessage(index);
	if (GetLength(messages) > index+1)
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
	// Message as regular one, don't stop the player.
	CustomMessage(message, nil, GetOwner(), 0, 64 + TutorialGuide->GetDefHeight(), 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	var effect = AddEffect("MessageShown", this, 100, 2 * GetLength(message), this);
	effect.show_index = show_index;
	// Messages with @ in front are shown infinetely long.
	if(GetChar(message, 0) == GetChar("@", 0))
		effect.var1 = true;
	return true;
}

// Effect exists as long as the message is shown.
// Message index is stored in EffectVar 0.
// For messages with @ in front, EffectVar 1 is true.
protected func FxMessageShownStart(object target, effect, int temporary)
{
	if (temporary == 0)
		GameCall("OnGuideMessageShown", target->GetOwner(), index);
	return 1;
}

protected func FxMessageShownTimer(object target, effect, int time)
{
	// Delete effect if time has passed, i.e. message has disappeared.
	// But only if it is not a message of infinite length, with @ in front.
	if (time && !effect.var1)
		return -1;
	return 1;
}

protected func FxMessageShownStop(object target, effect, int reason, bool temporary)
{
	if (!temporary)
		GameCall("OnGuideMessageRemoved", target->GetOwner(), index);
	return 1;
}

// Effect to display the player notification for some time.
protected func FxNotifyPlayerStart(object target, effect, int temporary)
{
	// Display notifier.
	SetGraphics("SpeakBubble", GetID(), 1, GFXOV_MODE_Base);
	SetObjDrawTransform(500, 0, -24000, 0, 500, -2000, 1);
	return 1;
}

protected func FxNotifyPlayerTimer(object target, effect, int time)
{
	// Delete effect if time has passed.
	if (time)
		return -1;
	return 1;
}

protected func FxNotifyPlayerStop(object target, effect, int reason, bool temporary)
{
	// Remove notifier.
	SetGraphics(nil, nil, 1);
	return 1;
}

protected func FxNotifyPlayerEffect(string new_name)
{
	if (new_name == "NotifyPlayer") 
		return -1;
}

local Name = "$Name$";

