/*-- 
		Tutorial Guide
		Author: Maikel
		
		The tutorial guide can be clicked on by the player, it supplies the player with information and hints.
--*/


local messages; // A container to hold all messages.
local index; // Progress in reading messages.

protected func Construction()
{
	// parallaxity
	this["Parallaxity"] = [0, 0];
	// visibility
	this["Visibility"] = VIS_Owner;
	messages = [];
	index = 0;
}

global func CreateTutorialGuide(int plr)
{
	var guide = CreateObject(TutorialGuide, 0, 0 , plr);
	guide->SetPosition(- 128 - 32 - TutorialGuide->GetDefWidth() / 2, 8 + TutorialGuide->GetDefHeight() / 2);
	return guide;
}

public func GetGuideIndex() { return index; }
public func SetGuideIndex(int to_index)
{
	index = BoundBy(to_index, 0, GetLength(messages));
	return;
} 

public func AddGuideMessage(string msg)
{
	// Automatically set index to current.
	index = GetLength(messages);
	// Add message to list.
	messages[index] = msg;
	return;
}

public func ShowGuideMessage(int show_index)
{
	index = Max(0, show_index);
	if (!messages[index]) 
		return;	
	GuideMessage(messages[index]);
	AddEffect("MessageShown", this, 100, 2 * GetLength(messages[index]), this);
	if (messages[index + 1]) 
		index++;
	return;
}

public func ClearGuideMessage()
{
	if (GetEffect("MessageShown", this))
		RemoveEffect("MessageShown", this);
	CustomMessage("", nil, GetOwner(), nil, nil, nil, nil, nil, MSG_HCenter);
	return;
}

public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (GetEffect("MessageShown", this))
		return ClearGuideMessage();
	if (!messages[index]) 
		return;
	GuideMessage(messages[index]);
	AddEffect("MessageShown", this, 100, 2 * GetLength(messages[index]), this);
	if (messages[index + 1]) 
		index++;
	return;
}

public func GuideMessage(string message)
{
	if (GetOwner() == NO_OWNER)
		return false;
	if (!message)
		return false;
	// Guide portrait.
	var portrait_def = "Portrait:TutorialGuide::00ff00::1";
	// Message as regular one, don't stop the player.
	CustomMessage(message, nil, GetOwner(), 0, 16 + TutorialGuide->GetDefHeight(), 0xffffff, _DCO, portrait_def, MSG_HCenter);
	return true;
}

protected func FxMessageShownTimer(object target, int num, int time)
{
	// Delete effect if message has disappeared.
	if (time)
		return -1;
	return 1;
}

protected func Definition(def)
{
	def["Name"] = "Prof. clonkine";
}
