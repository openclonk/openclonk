/**
	Rope Test
	Test scenario for the rope and all related objects.
	
	@author Maikel
*/


public func Initialize()
{
	// Rope ladders at the start.
	CreateObjectAbove(Ropeladder, 104, 40)->Unroll(1, 0, 12);
	CreateObjectAbove(Ropeladder, 222, 40)->Unroll(1, -1, 12);
	
	// Ropebridge.
	var post1 = CreateObjectAbove(Ropebridge_Post, 80, 152);
	var post2 = CreateObjectAbove(Ropebridge_Post, 176, 152);
	post2->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
	post2.Double->SetObjDrawTransform(-1000, 0, 0, 0, 1000);
	CreateObjectAbove(Ropebridge, 140, 152)->MakeBridge(post1, post2);
	
	// Series of rope ladders.
	CreateObjectAbove(Ropeladder, 292, 60)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 332, 60)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 372, 60)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 412, 60)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 452, 60)->Unroll(-1, COMD_Up);
	CreateObjectAbove(Ropeladder, 492, 60)->Unroll(-1, COMD_Up);
	
	DrawMaterialQuad("Earth", 550, 224, 600, 156, 640, 156, 640, 224);
	CreateObjectAbove(Ropeladder, 602, 158)->Unroll(1, -1);
	
	// Profile.
	//ProfileObject(Ropebridge);
	ProfileObject(Ropeladder);
	//StartScriptProfiler();
	//ArrayAccess();
	//ProplistAccess();
	//StopScriptProfiler();
	return;
}

public func InitializePlayer(int plr)
{
	GetCrew(plr)->CreateContents(GrappleBow, 2);
	GetCrew(plr)->CreateContents(Ropeladder);
	GetCrew(plr)->CreateContents(Shovel);
	return;
}

global func ProfileObject(id def, int duration)
{
	if (duration == nil)
		duration = 10;
	StartScriptProfiler(def);
	ScheduleCall(nil, "StopScriptProfiler", duration * 36, 0);
}

global func ArrayAccess()
{
	var a = [[[0,0]]], c;
	for (var i = 0; i < 10**6; i++)
		c = a[0][0][0];
}

global func ProplistAccess()
{
	var a = [{x = 1}], c;
	for (var i = 0; i < 10**6; i++)
		c = a[0].x;
}
