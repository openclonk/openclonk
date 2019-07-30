/*
	Alternative Materials
	Authors: Sven2
	
	A few alternative textures that can be put on various objects.
*/

local DefinitionPriority = -10; // load after the regular objects

public func AddMaterial(proplist target_def, string new_material_name, string editor_name, int index, string editor_help)
{
	if (!Inside(index, 0, 2)) FatalError(Format("Deco_AltMaterials::AltAddMaterial: Material index our of range (%d)", index));
	// Definition call to be run during Definition() callbacks
	// Just add the editor prop. Saving is done regularly through the MeshMaterial property.
	// (works only for material 0)
	// Prepare editor prop in target definition
	var prop_name = Format("alternative_material%d", index);
	if (!target_def.EditorProps) target_def.EditorProps = {};
	if (!target_def.EditorProps.alternative_material)
	{
		var get_fn = Format("GetMeshMaterial%d", index);
		var set_fn = Format("SetMeshMaterial%d", index);
		target_def[get_fn] = this[get_fn];
		target_def[set_fn] = this[set_fn];
		target_def.EditorProps.alternative_material = {
			Name = "$AlternativeMaterial$",
			EditorHelp = "$AlternativeMaterialHelp$",
			Type = "enum",
			AsyncGet = get_fn,
			Set = set_fn,
			Options = [ { Name="$Default$", Value = target_def->GetMeshMaterial() } ]
		};
	}
	var opts = target_def.EditorProps.alternative_material.Options;
	var new_material_option = {
		Name = editor_name,
		EditorHelp = editor_help,
		Value = new_material_name
	};
	opts[GetLength(opts)] = new_material_option;
	return new_material_option; // Return new option in case the user wants to do something with it
}

// We don't have lambdas yet
private func GetMeshMaterial0() { return GetMeshMaterial(0); }
private func GetMeshMaterial1() { return GetMeshMaterial(1); }
private func GetMeshMaterial2() { return GetMeshMaterial(2); }
private func SetMeshMaterial0(string to_mat) { return SetMeshMaterial(to_mat, 0); }
private func SetMeshMaterial1(string to_mat) { return SetMeshMaterial(to_mat, 1); }
private func SetMeshMaterial2(string to_mat) { return SetMeshMaterial(to_mat, 2); }

public func Definition(proplist def)
{
	// Add materials
	AddMaterial(Chest, "MetalChest", "$MetalChest$");
	AddMaterial(Chest, "GoldenChest", "$GoldenChest$");
	AddMaterial(Column, "AncientColumn", "$AncientColumn$");
	AddMaterial(LargeCaveMushroom, "FlyAmanitaMushroom", "$FlyAmanitaMushroom$");
	AddMaterial(LargeCaveMushroom, "FrozenCaveMushroom", "$FrozenCaveMushroom$");
	AddMaterial(Cannon, "GoldenCannon", "$GoldenCannon$");
	AddMaterial(Lorry, "RuinedLorry", "$RuinedLorry$");
	AddMaterial(SpinWheel, "SpinWheelBaseAlt", "$SpinWheelBaseAlt$", 1);
	AddMaterial(SpinWheel, "SpinWheelGearRed", "$SpinWheelGearRed$", 0);
	AddMaterial(SpinWheel, "SpinWheelGearBlue", "$SpinWheelGearBlue$", 0);
	AddMaterial(Idol, "IdolGrayColor", "$IdolGrayColor$");
	AddMaterial(Airplane, "CrashedAirplane", "$CrashedAirplane$");
	AddMaterial(Sword, "LaserSword", "$LaserSword$");
	AddMaterial(Sword, "OgreSword", "$OgreSword$");
	AddMaterial(PowderKeg, "NukePowderKeg", "$NukePowderKeg$");
	// Add custom attack modes for swords and keg (currently only affecting visuals, but should really add some extra differences)
	AI->RegisterAttackMode("LaserSword", new AI.SingleWeaponAttackMode { Weapon = Sword, Strategy = AI.ExecuteMelee, Skin="LaserSword", SkinName="$LaserSword$" });
	AI->RegisterAttackMode("OgreSword", new AI.SingleWeaponAttackMode { Weapon = Sword, Strategy = AI.ExecuteMelee, Skin="OgreSword", SkinName="$OgreSword$" });
	AI->RegisterAttackMode("NukePowderKeg", new AI.SingleWeaponAttackMode { Weapon = PowderKeg, Strategy = AI.ExecuteBomber, Skin="NukePowderKeg", SkinName="$NukePowderKeg$" });
}
