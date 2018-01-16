/**
	Gravestone Rule
	Dead clonks are replaced by gravestones.
	
	@author Ringwaul
*/


local fade_out;


protected func Initialize()
{	
	// Under no circumstance there may by multiple copies of this rule.
	if (ObjectCount(Find_ID(Rule_Gravestones)) > 1)
		return RemoveObject();
	fade_out = nil;
	return;
}

public func SetFadeOut(int time)
{
	fade_out = time;
	return;
}

public func OnClonkDeath(object clonk)
{
	// Add a gravestone if the clonk died.
	if (!clonk->GetAlive())
		clonk->CreateEffect(FxAddGravestone, 100, 1, fade_out);
	return;
}

local FxAddGravestone = new Effect
{
	Construction = func(int fade_out)
	{
		this.fade_out = fade_out;	
	},
	Timer = func(int time)
	{
		// Wait for the death animation to be over.
		if (time >= 20)
			return FX_Execute_Kill;
		return FX_OK;
	},
	Destruction = func(int reason)
	{
		// Don't do anything if the clonk has been removed.
		if (reason == FX_Call_RemoveClear)
			return;
		// Create the grave and remove the clonk.
		this.grave = Target->CreateObjectAbove(Clonk_Grave, 0, 0, Target->GetController());
		this.grave->SetInscription(Target);
		if (this.fade_out > 0)
			this.grave->FadeOut(this.fade_out, true);
		Target->RemoveObject();
		// Smoke effect.
		var particles =
		{
			Prototype = Particles_Dust(),
			R = 200,
			G = 100,
			B = 50,
			Size = PV_KeyFrames(0, 0, 0, 300, 40, 1000, 15)
		};
		this.grave->CreateParticle("Dust", 0, 0, PV_Random(-3, 3), PV_Random(-3, 3), PV_Random(18, 1 * 36), particles, 6);
	}
};

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}


/*-- Scenario Saving -- */

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (fade_out != nil)
		props->AddCall("FadeOut", this, "SetFadeOut", fade_out);
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
