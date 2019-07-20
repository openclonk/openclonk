/**
	Tunnel support
	Purely for decoration

	@authors: Clonkonaut, Ringwaul (Graphics)
*/

local extension = 0;

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Rotate(90, 0, 1, 0);
}

func Construction()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-15, 15), 0, 1, 0), GetID().MeshTransformation));
}

// Stretch the support beams
// 0 equals standard height (~30 pixels)
// 100 is about 85 pixels
public func Extend(int percentage)
{
	percentage = BoundBy(percentage, 0, 100);

	extension = percentage;

	var height = 30 + (55 * percentage / 100);
	percentage = 2500 * percentage / 100;
	PlayAnimation("extend", 1, Anim_Const(percentage));

	SetShape(-15, -15 + (30 - height), 30, height);
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;

	props->AddCall("Extension", this, "Extend", extension);
	return true;
}

private func GetExtensionPoint() { return [0, -10 - extension*60/100]; }
private func SetExtensionPoint(pt) { Extend((-10 - pt[1]) * 100/60); }

public func Definition(def)
{
	// Drag handle for extension
	if (!def.EditorProps) def.EditorProps = {};
	def.EditorProps.extension_point = {
	  Type="point",
	  Relative = true,
	  HorizontalFix = true,
	  AsyncGet="GetExtensionPoint",
	  Set="SetExtensionPoint",
	  Color = 0xffff00 };
}

/*-- Properties --*/

local Name = "$Name$";
local Plane = 701;