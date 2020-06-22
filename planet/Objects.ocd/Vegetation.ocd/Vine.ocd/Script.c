/** 
	Vine 
	A single vine which can hang down from ceilings.
	
	@author Maikel, Randrian
*/

local segments;
local leaf_particle;

protected func Initialize()
{
	// Create vine segments to climb on.
	CreateSegments();
	// Initialize the leaf particle.
	leaf_particle = Particles_Leaf(RGB(0, 255, 0));
	leaf_particle.Phase = 2;
	leaf_particle.Size = PV_Random(3, 5);
	return;
}


/*-- Ladder Control --*/

// Creates the segments which control the climbing.
private func CreateSegments()
{
	segments = [];
	var nr_segments = (GetBottom() - GetTop()) / 8;
	for (var index = 0; index < nr_segments; index++)
	{
		var y = GetTop() + index * 8;
		var segment = CreateObject(VineSegment, 0, y + 4);
		segment->SetMaster(this, index);
		// Store the segments.
		PushBack(segments, segment);
		// Set next and previous segment for climbing control.
		if (index > 0)
		{
			segments[index - 1]->SetPreviousLadder(segment);
			segment->SetNextLadder(segments[index - 1]);		
		}
	}
	return;
}

// Callback by the ladder climb library when the vine is grabbed.
public func OnLadderGrab(object clonk, object segment, int segment_index)
{
	segment->Sound("Environment::Vine::Grab?");
	segment->CreateParticle("Leaf", PV_Random(-2, 2), PV_Random(-3, 3), PV_Random(-4, 4), PV_Random(-4, 4), PV_Random(210, 240), leaf_particle, 6);
	return;
}

// Callback by the ladder climb library when the vine is climbed.
public func OnLadderClimb(object clonk, object segment, int segment_index)
{
	if (clonk->GetComDir() == COMD_Up || clonk->GetComDir() == COMD_Down)
	{
		if (!Random(20))
			segment->Sound("Environment::Vine::Grab?", {volume = 35});
		if (!Random(8))
			segment->CreateParticle("Leaf", PV_Random(-2, 2), PV_Random(-3, 3), PV_Random(-4, 4), PV_Random(-4, 4), PV_Random(210, 240), leaf_particle, 1);
	}
	return;
}

// Callback by the ladder climb library when the vine is released.
public func OnLadderReleased(object clonk, object segment, int segment_index)
{
	segment->Sound("Environment::Vine::Grab?", {volume = 50});
	segment->CreateParticle("Leaf", PV_Random(-2, 2), PV_Random(-3, 3), PV_Random(-4, 4), PV_Random(-4, 4), PV_Random(210, 240), leaf_particle, 3);
	return;
}


/*-- Placement --*/

// Place an amount of branches in the specified area. Settings:
// min_dist: the minimal distance between vines (default 32 pixels).
public func Place(int amount, proplist area, proplist settings)
{
	// Only allow definition call.
	if (this != Vine) 
		return;
	// Default parameters.
	if (!settings) 
		settings = {};
	if (!settings.min_dist)
		settings.min_dist = 32;
	if (!settings.attach_material)
		settings.attach_material = Loc_Or(Loc_Material("Granite"), Loc_Material("Rock"), Loc_MaterialVal("Soil", "Material", nil, 1));
	var loc_area = nil;
	if (area) 
		loc_area = Loc_InArea(area);
	var vines = [];
	var max_tries = Max(200, amount * 20);
	var nr_created = 0;
	for (var i = 0; i < max_tries && nr_created < amount; i++)
	{
		var loc = FindLocation(Loc_Sky(), Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Top, settings.attach_material), loc_area);
		if (!loc)
			continue;
		var vine = CreateObject(Vine);
		vine->SetPosition(loc.x, loc.y);
		if (!Random(3))
			vine.Plane = 510; 
		// Adjust position with respect to landscape.
		vine->AdjustPosition();
		// Retry if the center is at a solid location or if another vine is too close.
		if (vine->GBackSolid() || vine->FindObject(Find_ID(Vine), Find_Distance(settings.min_dist + Random(8)), Find_Exclude(vine)))
		{
			vine->RemoveObject();
			continue;		
		}		
		PushBack(vines, vine);
		nr_created++;
	}
	return vines;
}

// Adjust position with respect to material.
public func AdjustPosition()
{
	// Find distance to material.
	var d = 0;
	while (!GBackSolid(0, d) && d < 36 * GetCon() / 100)
		d++;
	// Adjust position.
	var size = 12 * GetCon() / 100;
	SetPosition(GetX(), GetY() + d - size);
	return;
}


/*-- Damage --*/

public func Incineration()
{
	// Color burned vine dark.
	SetClrModulation(RGB(60, 50, 30));
	return;
}

public func Damage()
{
	// Remove vine if above certain damage.
	if (GetDamage() > 30)
	{
		var particle = new leaf_particle {};
		particle.R = 51;
		particle.G = 25;
		particle.B = 5;
		// Create some burned leafs at each of the segments.
		for (var segment in segments)
		{
			var x = segment->GetX();
			var y = segment->GetY();
			Global->CreateParticle("Leaf", PV_Random(x - 2, x + 2), PV_Random(y - 3, y + 3), PV_Random(-4, 4), PV_Random(-4, 4), PV_Random(210, 240), particle, 3);
		}
		RemoveObject();
	}
	return;
}

public func Destruction()
{
	// Remove all segments.
	for (var segment in segments)
		if (segment)
			segment->RemoveObject();
	return;
}


/*-- Saving --*/

// Save placed ladder segments in scenarios.
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local NoBurnDecay = true;
local Placement = 4;
