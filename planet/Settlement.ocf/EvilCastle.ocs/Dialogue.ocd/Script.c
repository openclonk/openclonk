/* Nachrichten fuers Intro */

func MessageBoxAll(string message, object talker, bool permanent)
{
	if (permanent) permanent = "@"; else permanent = "";
	message = Format("%s<c %x>%s:</c> %s", permanent, talker->GetColor(), talker->GetName(), message);
	CustomMessage(message, nil, NO_OWNER, 150,150, nil, GUI_MenuDeco, GetPortraitDef(talker));
}

func GetPortraitDef(object talker)
{
	var portrait = talker.portrait;
	// Default definition has Clonk portrait
	// (Can't get default from skin, because there's no function GetSkin D:)
	if (!portrait || portrait == "" || portrait == "Clonk") return Dialogue;
	// Otherwise, bind portrait to an invisible object
	// (note: invisible object is leaked. can't really know when the message will be gone.)
	if (!talker.portrait_obj)
	{
		talker.portrait_obj = CreateObject(Dialogue);
		talker.portrait_obj->SetAction("Attach", talker);
		talker.portrait_obj->SetGraphics(portrait);
		talker.portrait_obj.Visibility = VIS_None;
	}
	return talker.portrait_obj;
}

func AttachTargetLost() { return RemoveObject(); }

local ActMap=
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure=DFA_ATTACH,
		NextAction="Attach",
		Length=1,
		FacetBase=1,
		AbortCall = "AttachTargetLost"
	}
};