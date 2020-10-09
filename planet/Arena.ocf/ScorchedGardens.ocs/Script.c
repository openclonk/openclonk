/** 
	Scorches Gardens
	A melee in a fiery setting.

	@author Mimmo_O
*/



protected func Initialize()
{
	// Goal.
	CreateObject(Goal_LastManStanding);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	GetRelaunchRule()
		->SetRespawnDelay(3)
		->SetLastWeaponUse(false);
	
	// Enviroment.
	CreateObject(Rule_ObjectFade)->DoFadeTime(10 * 36);
	SetSkyAdjust(RGB(255, 128, 0));
	SetSkyParallax(1, 20, 20, 0, 0, nil, nil);
	CreateObjectAbove(Column, 160, 304)->SetClrModulation(RGB(255, 100, 80));
	CreateObjectAbove(Column, 448, 272)->SetClrModulation(RGB(255, 100, 80));
	SetMatAdjust(RGB(255, 150, 128));
	
	AddEffect("RandomMeteor", nil, 100, 20);
	AddEffect("DangerousLava", nil, 100, 1);

	PlaceEdges();
	PlaceGras();
	return;
}

// Lava hurts a lot.
global func FxDangerousLavaTimer()
{
	for (var burning in FindObjects(Find_ID(Clonk),Find_OCF(OCF_OnFire)))
		burning->DoEnergy(-3); 
}

global func FxRandomMeteorTimer()
{
	if (!Random(GetPlayerCount() + 2)) 
		return FX_OK;
	LaunchMeteor(50 + Random(LandscapeWidth() - 100), -10, 40 + Random(40), RandomX(-20, 20), 0);
	return FX_OK;
}

private func PlaceEdges()
{
	var x = [69, 69, 76, 84, 484, 76, 565, 565, 532, 68, 68, 476, 468, 532];
	var y = [357, 349, 364, 364, 260, 300, 305, 313, 313, 300, 308, 260, 268, 305];
	var d = [2, 3, 2, 1, 0, 0, 3, 2, 1, 3, 2, 3, 3, 0];
	for (var i = 0; i < GetLength(x); i++)
		DrawMaterialTriangle("Brick-brick", x[i], y[i], d[i]);
	return;
}

private func PlaceGras()
{
	var x=[502, 468, 530, 525, 548, 560, 555, 551, 461, 483, 354, 425, 348, 343, 338, 420, 412, 405, 300, 315, 310, 305, 290, 193, 198, 169, 181, 176, 127, 137, 142, 133, 122, 147, 35, 45, 41, 30, 122];
	var y=[225, 221, 201, 206, 191, 178, 181, 185, 228, 220, 190, 234, 190, 188, 188, 231, 226, 221, 229, 218, 221, 228, 229, 262, 260, 261, 261, 259, 227, 227, 230, 228, 237, 240, 221, 221, 219, 222, 224];
	var r=[45,-45,-45,-45,-45,-45,-45,-45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,-45, 45, 45, 0,-45, 0,-45, 45, 0,-45, 90];
	for (var i = 0; i < GetLength(x); i++)
	{
		while (GBackSolid(x[i],y[i])) y[i]--;
		var edge = CreateObjectAbove(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetCategory(C4D_StaticBack);
		edge->SetR(r[i]); 
		edge->Initialize();
		edge->SetClrModulation(RGB(225 + Random(30), Random(30), Random(30)));
		
	}
	return;
}

public func RelaunchPosition()
{
	var x = RandomX(75, 500);
	var y = 100;
	while (!GBackSolid(x, y)) y += 1;
	y -= 30;
	
	return [x, y];
}

public func OnClonkLeftRelaunch(object clonk)
{
	clonk->CreateParticle("Fire", 0, 0, PV_Random(-20, 20), PV_Random(-40, 5), PV_Random(20, 90), Particles_Glimmer(), 30);
	clonk->SetYDir(-5);
	clonk->CreateContents(TeleGlove);
	return;
}

// Remove contents of clonks on their death.
public func OnClonkDeath(object clonk)
{
	while (clonk->Contents())
		clonk->Contents()->RemoveObject();
	return;
}

// Settings for LMS and DM.
public func WinKillCount() { return 5; }
