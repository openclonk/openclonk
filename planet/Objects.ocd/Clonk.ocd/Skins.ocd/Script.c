/*--
	Logic for different skins.
--*/

local gender, skin, skin_name;

func Construction()
{
	_inherited(...);

	SetSkin(0);
}

/* When adding to the crew of a player */

protected func Recruitment(int player)
{
	//The clonk's appearance
	//Player settings can be overwritten for individual Clonks. In your clonk file: "ExtraData = 1;Skin = iX" (X = chosen skin)
	var skin_setting = GetCrewExtraData("Skin");
	if (skin_setting == nil) skin_setting = GetPlrClonkSkin(player);
	if (skin_setting != nil) SetSkin(skin_setting);
	else SetSkin(Random(GetSkinCount()));

	return _inherited(player, ...);
}


func SetSkin(int new_skin)
{
	// Remember skin
	skin = new_skin;
	
	//Adventurer
	if (skin == 0)
	{	SetGraphics(skin_name = nil);
		gender = 0;	}

	//Steampunk
	if (skin == 1)
	{	SetGraphics(skin_name = "Steampunk");
		gender = 1; }

	//Alchemist
	if (skin == 2)
	{	SetGraphics(skin_name = "Alchemist");
		gender = 0;	}
	
	//Farmer
	if (skin == 3)
	{	SetGraphics(skin_name = "Farmer");
		gender = 1;	}

	//refreshes animation (whatever that means?)
	// Go back to original action afterwards and hope
	// that noone calls SetSkin during more compex activities
	var prev_action = GetAction();
	SetAction("Idle");
	SetAction(prev_action);

	return skin;
}
func GetSkinCount() { return 4; }

func GetSkin() { return skin; }
func GetSkinName() { return skin_name; }


// Returns the skin name as used to select the right sound subfolder.
public func GetSoundSkinName()
{
	if (skin_name == nil) return "Adventurer";
	return skin_name;
}

public func PlaySkinSound(string sound, ...)
{
	Sound(Format("Clonk::Skin::%s::%s", GetSoundSkinName(), sound), ...);
}


//Portrait definition of this Clonk for messages
func GetPortrait()
{
	return this.portrait ?? { Source = GetID(), Name = Format("Portrait%s", skin_name ?? ""), Color = GetColor() };
}

/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	// Skins override mesh material
	if (skin)
	{
		props->Remove("MeshMaterial");
		props->AddCall("Skin", this, "SetSkin", skin);
	}
	return true;
}

func Definition(def) {

	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.skin = { Name="$Skin$", EditorHelp="$SkinHelp$", Type="enum", Set="SetSkin", Options = [
	{ Value = 0, Name="Adventurer"},
	{ Value = 1, Name="Steampunk"},
	{ Value = 2, Name="Alchemist"},
	{ Value = 3, Name="Farmer"}
	]};
	
	_inherited(def);
}
