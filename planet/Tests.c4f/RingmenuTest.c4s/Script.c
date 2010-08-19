/* Sky race */

func Initialize()
{
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(LandscapeWidth()/2,240);

	//pGoal->SetFinishpoint(LandscapeWidth()/2,LandscapeHeight()-30);
	CreateObject(WindGenerator,68,188)->SetR(95);
	CreateObject(WindGenerator,LandscapeWidth()-68,188)->SetR(-95);
	SetSkyAdjust (RGB(230,210,150), RGB(150,100,0));
	for(var i=0; i<30; i++) CreateObject(Rule_ObjectFade); 
	var b=LandscapeWidth();

	DrawMaterialQuad("Vehicle",0,110,22,120,27,255,0,255);
	DrawMaterialQuad("Vehicle",0,255,25,255,55,285,55,310);
	DrawMaterialQuad("Vehicle",55,285,185,285,185,330,55,310);
	DrawMaterialQuad("Vehicle",185,285,275,195,340,195,185,310);
	
	DrawMaterialQuad("Vehicle",b-0,110,b-22,120,b-27,255,b-0,255);
	DrawMaterialQuad("Vehicle",b-0,255,b-25,255,b-55,285,b-55,310);
	DrawMaterialQuad("Vehicle",b-55,285,b-185,285,b-185,330,b-55,310);
	DrawMaterialQuad("Vehicle",b-185,285,b-275,195,b-340,195,b-185,310);
}



// Gamecall from Race-goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	var relaunch = CreateObject(RelaunchRoom,LandscapeWidth()/2,240,clonk->GetOwner());
	clonk->Enter(relaunch);
	relaunch->ChooseMenu(clonk);
	//clonk->CreateContents(SpellChooser,2);
	return;
}
