/* Automatically created objects file */

static g_goal, g_object_fade, g_statue, g_doorleft, g_doorright;

func InitializeObjects()
{
	g_goal = CreateObject(Goal_ProtectTheStatue, 0, 0);
	g_goal.StaticSaveVar = "g_goal";

	g_object_fade = CreateObject(Rule_ObjectFade, 0, 0);
	g_object_fade.StaticSaveVar = "g_object_fade";
	
	CreateObject(Rule_NoFriendlyFire, 0, 0);

	g_statue = CreateObjectAbove(Idol, 632, 440);
	g_statue.StaticSaveVar = "g_statue";
	g_statue.Touchable = 0;
	g_statue->SetAction("ItemRightHigh");
	g_statue->EditorSetItemLeft({Bone = "main", MeshTransformation = [0, 0, 1200, 0, 1200, 0, 0, 0, 0, 1200, 0, -1200], Type = Shield});
	g_statue->EditorSetItemRight({Bone = "Javelin", MeshTransformation = [-1500, 0, 0, 0, 0, -1500, 0, 0, 0, 0, 1500, 0], Type = Javelin});

	g_doorleft = CreateObjectAbove(StoneDoor, 495, 449);
	g_doorleft->SetComDir(COMD_Down);
	g_doorleft.StaticSaveVar = "g_doorleft";
	g_doorleft->SetAutoControl();
	g_doorright = CreateObjectAbove(StoneDoor, 765, 448);
	g_doorright->SetComDir(COMD_Down);
	g_doorright.StaticSaveVar = "g_doorright";
	g_doorright->SetAutoControl();

	CreateObjectAbove(Rock, 312, 713);
	CreateObjectAbove(Rock, 353, 679);
	CreateObjectAbove(Rock, 894, 707);
	CreateObjectAbove(Rock, 1084, 582);

	CreateObjectAbove(Loam, 752, 608);
	CreateObjectAbove(Loam, 130, 555);

	CreateObjectAbove(Firestone, 436, 652);
	CreateObjectAbove(Firestone, 554, 533);
	CreateObjectAbove(Firestone, 199, 583);
	CreateObjectAbove(Firestone, 883, 559);
	return true;
}
