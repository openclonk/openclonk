/*
	Signpost
	Author: Sven2

	Storage for text.
*/

local text;

func SetText(string t) { text = t; }

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgRead$", IconName = nil, IconID = nil };
}

// Read on interaction
public func Interact(object clonk)
{
	var message = Format("%s              ", text ?? "$MsgUnreadable$");
	CustomMessage(message, nil, clonk->GetController(), 150, 150, nil, GUI_MenuDeco, Signpost);
	return true;
}

local Name = "$Name$";
