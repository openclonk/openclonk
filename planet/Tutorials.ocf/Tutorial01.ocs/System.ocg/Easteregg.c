// Triggers colored wipf rainbow on a succesful jump roll and wall jump performed by the player.

#appendto Clonk

public func DoRoll()
{
	// Only perform events on first roll.
	if (GetEffect("IntDoneRoll", this))
		return _inherited(...);
	else
		AddEffect("IntDoneRoll", this, 100);
		
	// Add some stars effect to the clonk indicating the easteregg.
	CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 25, Particles_Magic(), 20);
	
	if (GetEffect("IntDoneWallJump", this))
		DoEasterEgg();

	return _inherited(...);
}

protected func FxIntDoneRollStart(object target, proplist effect, int temporary)
{
	// Just a an effect which should always stay and hence accept always.
	return FX_OK;
}

public func StartJump()
{
	// Only do something if it is a wal jump, not an ordinary one.
	if (!GetEffect("WallKick",this))
		return _inherited(...);
		
	// Only perform events on first wall jump.
	if (GetEffect("IntDoneWallJump", this))
		return _inherited(...);
	else
		AddEffect("IntDoneWallJump", this, 100);

	// Add some stars effect to the clonk indicating the easteregg.
	CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 25, Particles_Magic(), 20);
	
	if (GetEffect("IntDoneRoll", this))
		DoEasterEgg();

	return _inherited(...);
}

protected func FxIntDoneWallJumpStart(object target, proplist effect, int temporary)
{
	// Just a an effect which should always stay and hence accept always.
	return FX_OK;
}

// Award achievement if a gold bar has been collected.
public func Collection(object collected)
{
	if (collected->GetID() == GoldBar)
	{
		// Only perform events on first wall jump.
		if (GetEffect("IntAwardedAchievement", this))
			return _inherited(...);
		AddEffect("IntAwardedAchievement", this, 100);
		// Add some stars effect to the clonk indicating the easteregg.
		CreateParticle("StarSpark", PV_Random(-3, 3), PV_Random(-14, -10), PV_Random(-5, 5), PV_Random(-8, 0), 25, Particles_Magic(), 20);		
		// Achievement: easter egg found.
		GainScenarioAchievement("TutorialEasterEgg");
	}
	return _inherited(collected, ...);
}

protected func FxIntAwardedAchievementStart(object target, proplist effect, int temporary)
{
	// Just a an effect which should always stay and hence accept always.
	return FX_OK;
}


// Creates the easteregg.
public func DoEasterEgg()
{
	// Create the wipf rainbow at the cave exit.
	CreateRainbow();
	
	// Create a chest with gold at the other end of the rainbow.
	var chest = CreateObjectAbove(Chest, AbsX(576), AbsY(260));
	chest->CreateContents(GoldBar, 10);
	return;
}

// Creates a rainbow consisting of small colored wipfs.
public func CreateRainbow()
{
	// Rainbow properties
	var cx = 772, cy = 400; // rainbow center
	var r = 244; // rainbow radius
	var colors = [[255, 0, 0], [255, 127, 0], [255, 255, 0], [0, 255, 0], [0, 0, 255], [75, 0, 130], [143, 0, 255]];

	// Loop over the number for particles in the rainbow and create it. 
	var arc_start = 120;
	var arc_length = 1360;
	var nparticle = 150;
	
	// Dummy object for attaching the particles and make them background.
	var dummy = CreateObject(Dummy);
	dummy.Visibility = VIS_Owner;
	dummy.Plane = -600;
	dummy->SetCategory(C4D_StaticBack | C4D_Background | C4D_Parallax);
	dummy.Parallaxity = [20, 20];
		
	// Loop over the arc and the colors and create the particles.	
	for (var angle = arc_start; angle < arc_start + arc_length; angle += (arc_length / nparticle))
	{
		for (var i = 0; i < GetLength(colors); i++)
		{
			var x = dummy->AbsX(cx + Cos(angle, r - 2 * i, 10));
			var y = dummy->AbsY(cy - Sin(angle, r - 2 * i, 10));
			var wipf =
			{
				Size = 7,
				Rotation = 90 - angle / 10,
				R = colors[i][0],
				G = colors[i][1],
				B = colors[i][2],
				Alpha = 128, //PV_Step(1, 0, 1, 255),	
				Attach = ATTACH_Back,
			};
			dummy->CreateParticle("Sphere", x, y, 0, 0, 0, wipf, 1);
		}
	}
	return;
}
