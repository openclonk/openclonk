/** 
	Scorches Gardens
	A melee in a fiery setting.

	@author Mimmo_O (edit by DasWipf)
*/



protected func Initialize()
{
	// Goal.
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	
	if (SCENPAR_Goal == 0)
	{
		CreateObject(Goal_LastManStanding);
	}
	else 
	{	
		CreateObject(Goal_DeathMatch);
	}
	
	
	// Enviroment.
	CreateObject(Rule_ObjectFade)->DoFadeTime(10 * 36);
	SetSkyAdjust(RGB(255, 128, 0));
	SetSkyParallax(1, 20, 20, 0, 0, nil, nil);
	SetMatAdjust(RGB(255, 150, 128));
	
	AddEffect("RandomMeteor", nil, 100, 20);
	AddEffect("DangerousLava", nil, 100, 1);

	PlaceGras();
	return;
}


global func FxRandomMeteorTimer()
{
	if (!Random(GetPlayerCount() + 2)) 
		return FX_OK;
	LaunchMeteor(50 + Random(LandscapeWidth() - 100), -10, 40 + Random(40), RandomX(-20, 20), 0);
	return FX_OK;
}

private func PlaceGras()
{
	var x=[502,468,530,525,548,560,555,551,461,483,354,425,348,343,338,420,412,405,300,315,310,305,290,193,198,169,181,176,127,137,142,133,122,147,35,45,41,30,122];
	var y=[225,221,201,206,191,178,181,185,228,220,190,234,190,188,188,231,226,221,229,218,221,228,229,262,260,261,261,259,227,227,230,228,237,240,221,221,219,222,224];
	var r=[45,-45,-45,-45,-45,-45,-45,-45,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-45,45,45,0,-45,0,-45,45,0,-45,90];
	for (var i = 0; i < GetLength(x); i++)
	{
		while(GBackSolid(x[i],y[i])) y[i]--;
		var edge=CreateObjectAbove(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetCategory(C4D_StaticBack);
		edge->SetR(r[i]); 
		edge->Initialize();
		edge->SetClrModulation(RGB(225+Random(30), Random(30), Random(30)));
		
	}
	return;
}

protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x = RandomX(75,500);
	var y=100;
	while(!GBackSolid(x,y)) y+=1;
	y-=30;
	var relaunch = CreateObjectAbove(RelaunchContainer, x, y, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(3);
	clonk.MaxContentsCount = 2;
	clonk->CreateContents(TeleGlove);
	clonk->CreateContents(WindBag);

	return;
}

public func OnClonkLeftRelaunch(object clonk)
{
	clonk->CreateParticle("Fire", 0, 0, PV_Random(-20, 20), PV_Random(-40, 5), PV_Random(20, 90), Particles_Glimmer(), 30);
	clonk->SetYDir(-5);
	return;
}

// Remove contents of clonks on their death.
public func OnClonkDeath(object clonk)
{
	while (clonk->Contents())
		clonk->Contents()->RemoveObject();
	return;
}

//Settings for LMS and DM.

public func RelaunchCount()
{
	var RelaunchCount = (SCENPAR_Relaunchs);
	return RelaunchCount;
}	
public func WinKillCount() 
{ 
	var WinKillCount = (SCENPAR_Relaunchs);
	return WinKillCount; 
}
