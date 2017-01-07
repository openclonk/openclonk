/* Automatically created objects file */

static g_goal, g_object_fade, g_flag, g_windgen1, g_windgen2, g_windgen3, g_windmill, g_chest, g_windbag, g_lorry;

func InitializeObjects()
{
	g_goal = CreateObject(Goal_SaveTheWindmills, 10, 10);
	g_goal.StaticSaveVar = "g_goal";

	g_object_fade = CreateObject(Rule_ObjectFade, 0, 0);
	g_object_fade.StaticSaveVar = "g_object_fade";
	
	CreateObject(Rule_NoFriendlyFire, 0, 0);

	g_flag = CreateObject(Flagpole, 1033, 937);
	g_flag.StaticSaveVar = "g_flag";
	g_flag->SetNeutral(true);

	g_windgen1 = CreateObject(WindGenerator, 1041, 799);
	g_windgen1.StaticSaveVar = "g_windgen1";
	g_windgen2 = CreateObject(WindGenerator, 1105, 907);
	g_windgen2.StaticSaveVar = "g_windgen2";
	g_windgen3 = CreateObject(WindGenerator, 898, 908);
	g_windgen3.StaticSaveVar = "g_windgen3";
	g_windmill = CreateObjectAbove(Windmill, 991, 970);
	g_windmill.StaticSaveVar = "g_windmill";

	g_chest = CreateObjectAbove(Chest, 997, 1023);
	g_chest.StaticSaveVar = "g_chest";
//	g_windbag = g_chest->CreateContents(WindBag);
//	g_windbag.StaticSaveVar = "g_windbag";

	g_lorry = CreateObjectAbove(Lorry, 924, 938);
	g_lorry.StaticSaveVar = "g_lorry";

	CreateObjectAbove(Grass, 1006, 826);
	CreateObjectAbove(Grass, 990, 826);
	CreateObjectAbove(Grass, 955, 835);
	CreateObjectAbove(Grass, 975, 827);

	CreateObjectAbove(Flower, 965, 840);
	CreateObjectAbove(Flower, 1016, 835);

	var Trunk001 = CreateObject(Trunk, 827, 937);
	Trunk001->SetR(-30);

	CreateObjectAbove(Tree_Coniferous3, 1167, 945);

	var Branch001 = CreateObject(Branch, 938, 839);
	Branch001->SetR(-30);

	var Ropeladder001 = CreateObject(Ropeladder, 1057, 826);
	Ropeladder001->Unroll(DIR_Right);

	CreateObject(Basement, 957, 859);

	return true;
}
