/**
	Rockfall
	Big pieces of rock that fall down cliffs.
	
	@author Maikel	
*/


/*-- Disaster Control --*/

public func SetChance(int chance)
{
	if (this != Rockfall)
		return;
	var effect = GetEffect("IntRockfallControl");
	if (!effect)
		effect = AddEffect("IntRockfallControl", nil, 100, 20, nil, Rockfall);
	effect.chance = chance;
	return;
}

public func GetChance()
{
	if (this != Rockfall)
		return;
	var effect = GetEffect("IntRockfallControl");
	if (effect)
		return effect.chance;
	return;
}

public func SetArea(proplist rect)
{
	if (this != Rockfall)
		return;
	var effect = GetEffect("IntRockfallControl");
	if (effect)
		effect.area = rect;
	return;
}

protected func FxIntRockfallControlTimer(object target, proplist effect, int time)
{
	if (Random(100) < effect.chance && !Random(6))
	{
		var x = Random(LandscapeWidth());
		var y = 0;
		if (effect.area)
		{
			x = effect.area.x + Random(effect.area.w);
			y = effect.area.y + Random(effect.area.h);		
		}
		// Launch rockfall of varying sizes between 40 and 120.
		LaunchRockfall(x, y, 80 + Random(41), RandomX(-2, 2), RandomX(4, 8));		
	}
	return FX_OK;
}

// Scenario saving through an effect call.
public func FxIntRockfallControlSaveScen(obj, fx, props)
{
	props->Add("Rockfall", "Rockfall->SetChance(%d)", fx.chance);
	return true;
}

// Launches an earthquake with epicenter (x,y).
global func LaunchRockfall(int x, int y, int size, int xdir, int ydir)
{
	// The rockfall size is constrained between 40 and 120%.
	size = BoundBy(size, 40, 120);
	
	// Rockfall should be launched in the free.
	if (GBackSemiSolid(x, y))
		return false;	
	
	// Create rock and adjust its size.
	var rock = CreateObject(Rockfall, x, y);
	rock->SetCon(size);
	
	// Remove rock if stuck.
	if (rock->Stuck())
		return rock->RemoveObject();
	
	// Set speed and rotation.
	rock->SetXDir(xdir);
	rock->SetYDir(ydir);
	rock->SetR(Random(360));
	rock->SetRDir(RandomX(-6, 6));
	return true;
}


/*-- Rockfall --*/

local damage;

protected func Construction()
{
	damage = 0;
	this.MeshTransformation = Trans_Scale(2000, 2000, 2000);
	// Add an effect for rolling then the rock is just lying around.
	AddEffect("IntRockMovement", this, 100, 4, this);
	return;
}

protected func FxIntRockRollStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return 1;
	effect.damtime = 0;
	return 1;
}

protected func FxIntRockMovementTimer(object target, proplist effect, int time)
{
	// If rock is not moving give it a kick.
	if (GetXDir() == 0 || GetYDir() == 0)
	{
		SetXDir(RandomX(-160, 160), 100);
		SetYDir(-60 - Random(60), 100);
		SetRDir(GetRDir() + RandomX(-2, 2));
	}
	
	// Damage rock every 120 frames to make sure it breaks at some point.
	effect.damtime += 4;
	if (effect.damtime > 120)
	{
		effect.damtime = 0;
		damage++;
	}
	return 1;
}

protected func Hit(int dx, int dy)
{
	// Determine caused damage to this rock by impact.
	damage += Distance(dx, dy, 0, 0) / 100;
	if (damage > 12)
		return SplitRock();
	
	// Rebounce or crack on downward hit.
	if (dy > 0)
	{
		SetXDir(dx + RandomX(-160, 160), 100);
		SetYDir(Min(-60, -7 * dy / 10) - Random(60), 100);
		SetRDir(GetRDir() + RandomX(-2, 2));
	}
	
	// Some particles and smoke.
	if (GetMaterial(0, 9 * GetCon() / 100))
	{
		var clr = GetAverageTextureColor(GetTexture(0, 9 * GetCon() / 100));
		var particles =
		{
			Prototype = Particles_Dust(),
			R = (clr >> 16) & 0xff,
			G = (clr >> 8) & 0xff,
			B = clr & 0xff,
			Size = 4,
		};
		CreateParticle("Dust", PV_Random(-2, 2), 8 * GetCon() / 100, PV_Random(-3, 3), PV_Random(-2, -3), PV_Random(36, 2 * 36), particles, 2);
	}
	
	// Fling living beings near impact point for a big hit.
	if (GetCon() > 80 && dy > 60)
	{
		for (var obj in FindObjects(Find_NoContainer(), Find_OCF(OCF_Alive), Find_InRect(-16, -8, 32, 24)))
		{
			if (!Random(3) || !obj->GetAction()) 
				continue;
			var act = obj.ActMap[obj->GetAction()];
			if (act.Attach || (act.Procedure && act.Procedure != DFA_FLIGHT && act.Procedure != DFA_LIFT && act.Procedure != DFA_FLOAT && act.Procedure != DFA_ATTACH && act.Procedure != DFA_CONNECT))
				obj->Fling(Random(3) - 1);
		}
	}

	// Sound.
	Sound("EarthquakeEnd", nil, 3 * GetCon() / 2);
	StonyObjectHit(dx, dy);
	return;
}

private func SplitRock()
{
	var con = GetCon();
	// Split the rock into smaller ones if it is big enough.
	if (con > 40)
	{
		while (con > 0)
		{
			var rock = CreateObject(Rockfall);
			var rock_con = Max(30, GetCon() / 2 + RandomX(-20, 20));
			rock->SetCon(rock_con);
			con -= 2 * rock_con / 3;
		
			rock->SetXDir(RandomX(-100, 100), 100);
			rock->SetYDir(RandomX(-200, -100), 100);
			rock->SetRDir(RandomX(-6, 6));		
		}		
	}
	// Some particles.
	var rock_explode =
	{	    
		Size = PV_KeyFrames(0, 180, 25, 1000, 50),
	    DampingY = PV_Random(890, 920, 5),
		DampingX = PV_Random(900, 930, 5),
		ForceY=-1,
		ForceX = PV_Wind(20, PV_Random(-2, 2)),
		Rotation=PV_Random(0,360,0),
		R=PV_KeyFrames(0, 0, 255, 260, 64, 1000, 64),
		G=PV_KeyFrames(0, 0, 128,  260, 64, 1000, 64),
		B=PV_KeyFrames(0, 0, 0, 260, 108, 1000, 108),
	    Alpha = PV_KeyFrames(0, 0, 0, 100, 20, 500, 20, 1000, 0)
	};
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5), 0, PV_Random(-2, 0), PV_Random(50, 100), rock_explode, 8);
	
	// Some sound effects.
	Sound("EarthquakeEnd", nil, 100);

	RemoveObject();
	return;
}


/*-- Proplist --*/

local Name = "$Name$";
