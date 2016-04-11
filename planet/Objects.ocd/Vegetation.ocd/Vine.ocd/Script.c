/** 
	Vine 
	A single vine which can hang down from ceilings.
	
	@author Maikel, Randrian
*/

local segments;

protected func Initialize()
{
	// Create vine segments to climb on.
	CreateSegments();
	return;
}

protected func Damage()
{	

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
	return;
}

// Callback by the ladder climb library when the vine is climbed.
public func OnLadderClimb(object clonk, object segment, int segment_index)
{
	if (clonk->GetComDir() == COMD_Up || clonk->GetComDir() == COMD_Down)
		if (!Random(20))
			segment->Sound("Environment::Vine::Grab?", {volume = 35});
	return;
}

// Callback by the ladder climb library when the vine is released.
public func OnLadderReleased(object clonk, object segment, int segment_index)
{
	segment->Sound("Environment::Vine::Grab?", {volume = 50});
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
	var loc_area = nil;
	if (area) 
		loc_area = Loc_InArea(area);
	var vines = [];
	var max_tries = Max(200, amount * 20);
	var nr_created = 0;
	for (var i = 0; i < max_tries && nr_created < amount; i++)
	{
		var loc = FindLocation(Loc_Sky(), Loc_Not(Loc_Liquid()), Loc_Wall(CNAT_Top, Loc_Or(Loc_Material("Granite"), Loc_Material("Rock"), Loc_MaterialVal("Soil", "Material", nil, 1))), loc_area);
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


/*-- Saving --*/

// Save placed ladder segments in scenarios.
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) 
		return false;
	props->AddCall("CreateSegments", this, "CreateSegments");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local BlastIncinerate = 1;
local ContactIncinerate = 3;
local Placement = 4;
