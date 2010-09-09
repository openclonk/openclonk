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
	guide->SetPosition(- 128 - 32 - TutorialGuide->GetDefHeight() / 2, 8 + TutorialGuide->GetDefHeight() / 2);
	return guide;
}

public func AddGuideMessage(string msg)
{
	messages[GetLength(messages)] = msg;
	return;
}

public func ShowGuideMessage(int show_index)
{
	index = Max(0, show_index);
	if (!messages[index]) 
		return;	
	GuideMessage(messages[index]);
	if (messages[index + 1]) 
		index++;
	return;
}

public func MouseSelection(int plr)
{
	if (plr != GetOwner())
		return;
	if (!messages[index]) 
		return;
	GuideMessage(messages[index]);
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
	// Defaults
	var portrait_def = "Portrait:TutorialGuide::00ff00::1";
	// Message as regular one, don't stop the player.
	CustomMessage(message, 0, GetOwner(), 0 /* 150*/, 45, 0xffffff, _DCO, portrait_def, MSG_HCenter);
	return true;
}

local Name = "Prof. clonkine";
