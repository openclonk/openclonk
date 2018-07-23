/**
	ConstructionPreviewer
	

	@author Clonkonaut
*/

static const CONSTRUCTION_STICK_Left = 1;
static const CONSTRUCTION_STICK_Right = 2;
static const CONSTRUCTION_STICK_Bottom = 4;
static const CONSTRUCTION_STICK_Top = 8;

local extra_overlay, dimension_x, dimension_y, clonk, structure, direction, stick_to, blocked;
local GFX_StructureOverlay = 1;
local GFX_CombineIconOverlay = 2;
local GFX_PreviewerPictureOverlay = 2;

public func GetFlipDescription() { return "$TxtFlipDesc$"; }

func Initialize()
{
	SetProperty("Visibility", VIS_Owner);
}

func Set(id to_construct, object constructing_clonk)
{
	SetGraphics(nil, to_construct, GFX_StructureOverlay, GFXOV_MODE_Base);
	SetGraphics(nil, to_construct, GFX_PreviewerPictureOverlay, GFXOV_MODE_Picture, nil, GFX_BLIT_Wireframe);
	// Buildings may add something to the preview and can do so on GFX_PreviewerPictureOverlay
	// This is used by the elevator to preview the case
	// The definition should return true
	extra_overlay = to_construct->~ConstructionPreview(this, GFX_PreviewerPictureOverlay, direction);
	dimension_x = to_construct->GetDefWidth();
	dimension_y = to_construct->GetDefHeight();
	clonk = constructing_clonk;
	structure = to_construct;
	direction = DIR_Left;
	blocked = true;
	// Allow the definition to draw an alternative preview.
	to_construct->~AlternativeConstructionPreview(this, direction);
	AdjustPreview(structure->~IsBelowSurfaceConstruction());
	return;
}

// Positions the preview according to the landscape, coloring it green, yellow or red
public func AdjustPreview(bool below_surface, bool look_up, bool no_call)
{
	var half_y = dimension_y / 2;
	blocked = false;
	// Do only if not sticking to another object
	if (!stick_to)
	{
		// Place on material
		var search_dir = 1;
		if (look_up) search_dir = -1;
		var x = 0, y = 0;
		while (!(!GBackSolid(x,y + half_y) && GBackSolid(x,y + half_y + 1)))
		{
			y += search_dir;
			if (Abs(y) > half_y)
			{
				blocked = true;
				break;
			}
		}

		if (blocked && !no_call)
			return AdjustPreview(below_surface, !look_up, true);
		if (blocked)
		{
			if (extra_overlay) SetClrModulation(RGBa(255,50,50, 100), GFX_PreviewerPictureOverlay);
			return SetClrModulation(RGBa(255,50,50, 100), GFX_StructureOverlay);
		}
		// Position depends on whether the object should below surface.
		if (!below_surface)
			SetPosition(GetX(), GetY() + y);
		else
			SetPosition(GetX(), GetY() + y + dimension_y + 1);
	}
	// Check for construction site.
	if (!below_surface && !CheckConstructionSite(structure, 0, half_y))
		blocked = true;
	// intersection-check with all other construction sites... bah
	for (var other_site in FindObjects(Find_ID(ConstructionSite)))
	{
		if (!(other_site->GetLeftEdge() > GetX()+dimension_x/2 || other_site->GetRightEdge() < GetX()-dimension_x/2 || other_site->GetTopEdge() > GetY()+half_y || other_site->GetBottomEdge() < GetY()-half_y))
			blocked = true;
	}
	
	if(!blocked)
	{
		if (!stick_to)
		{
			if (extra_overlay) SetClrModulation(RGBa(50,255,50, 100), GFX_PreviewerPictureOverlay);
			SetClrModulation(RGBa(50,255,50, 100), GFX_StructureOverlay);
		}
		else
		{
			if (extra_overlay) SetClrModulation(RGBa(255,255,50, 200), GFX_PreviewerPictureOverlay);
			SetClrModulation(RGBa(255,255,50, 200), GFX_StructureOverlay);
		}
	}
	else
	{
		if (extra_overlay) SetClrModulation(RGBa(255,50,50, 100), GFX_PreviewerPictureOverlay);
		SetClrModulation(RGBa(255,50,50, 100), GFX_StructureOverlay);
	}
}

// Positions the preview according to the mouse cursor, calls AdjustPreview afterwards
// x and y are refined mouse coordinates so always centered at the clonk
func Reposition(int x, int y)
{
	var clonk_width = clonk->GetObjWidth();
	var clonk_height = clonk->GetObjHeight();
	x = BoundBy(x, -dimension_x - clonk_width/2, dimension_x + clonk_width/2);
	y = BoundBy(y, -dimension_y - clonk_height/2, dimension_y + clonk_height/2);
	// Try to combine the structure with other structures.
	var found = false;
	// Hopefully, in the end this contains a single sticking direction.
	var single_stick_to = 0;

	if (structure->~ConstructionCombineWith())
	{
		// There is no use in doing all the other checks if no sticking direction is defined at all
		// That's just wrong use of ConstructionCombineWith
		if (structure->~ConstructionCombineDirection())
		{
			//var find_rect = Find_InRect(AbsX(clonk->GetX() + x - dimension_x/2 - 10), AbsY(clonk->GetY() + y - dimension_y/2 - 10), dimension_x + 20, dimension_y + 20);
			//if ((stick_dir & CONSTRUCTION_STICK_Bottom))

			var find_rect = Find_AtPoint(clonk->GetX() - GetX() + x, clonk->GetY() - GetY() + y);

			var other = FindObject(Find_Func(structure->ConstructionCombineWith(), this),
										  find_rect,
										  Find_OCF(OCF_Fullcon),
										  Find_Layer(clonk->GetObjectLayer()),
										  Find_Allied(clonk->GetOwner()),
										  Find_NoContainer());

			if (other)
			{
				var stick_dir = structure->ConstructionCombineDirection(other);
				var other_width = other->GetObjWidth();
				var other_height = other->GetObjHeight();
				// Determine the position from the other object's center currently hovered
				var other_offset_x = clonk->GetX() + x - other->GetX();
				var other_offset_y = clonk->GetY() + y - other->GetY();

				// The tricky part is now to determine which of the four possible directions should be used
				// The shape of the 'other' is divided like this (* is center):
				//  __________________________
				// |       |   Top   |       |
				// | Left  |----*----| Right |
				// |_______|_Bottom__|_______|
				//
				// Whichever part is howered on is checked first for stick direction

				// Left
				if (other_offset_x < other_width / -6)
				{
					single_stick_to = GetStickingDirection(stick_dir, CONSTRUCTION_STICK_Left, CONSTRUCTION_STICK_Bottom, CONSTRUCTION_STICK_Top, CONSTRUCTION_STICK_Right, other_offset_y, 0);
				}
				// Right
				else if (other_offset_x > other_width / 6)
				{
					single_stick_to = GetStickingDirection(stick_dir, CONSTRUCTION_STICK_Right, CONSTRUCTION_STICK_Bottom, CONSTRUCTION_STICK_Top, CONSTRUCTION_STICK_Left, other_offset_y, 0);
				}
				// Bottom
				else if (other_offset_y >= 0)
				{
					single_stick_to = GetStickingDirection(stick_dir, CONSTRUCTION_STICK_Bottom, CONSTRUCTION_STICK_Right, CONSTRUCTION_STICK_Left, CONSTRUCTION_STICK_Top, other_offset_x, 0);
				}
				// Top
				else if (other_offset_y < 0)
				{
					single_stick_to = GetStickingDirection(stick_dir, CONSTRUCTION_STICK_Top, CONSTRUCTION_STICK_Right, CONSTRUCTION_STICK_Left, CONSTRUCTION_STICK_Bottom, other_offset_x, 0);
				}

				// If no direction is found, something went wrong.
				// Probably ConstructionCombineDirection() returned garbage.
				if (single_stick_to)
				{
					if (single_stick_to == CONSTRUCTION_STICK_Left)
					{
						x = other->GetX() - other_width/2 - dimension_x/2;
						y = other->GetY();
					}
					if (single_stick_to == CONSTRUCTION_STICK_Right)
					{
						x = other->GetX() + other_width/2 + dimension_x/2;
						y = other->GetY();
					}
					if (single_stick_to == CONSTRUCTION_STICK_Bottom)
					{
						x = other->GetX();
						y = other->GetY() + other_height/2 + dimension_y/2;
					}
					if (single_stick_to == CONSTRUCTION_STICK_Top)
					{
						x = other->GetX();
						y = other->GetY() - other_height/2 - dimension_y/2;
					}
					// Add an additional offset if needed, for example a basement can be place
					// only under a part of the structure.
					var stick_offset = structure->~ConstructionCombineOffset(other, single_stick_to);
					if (stick_offset)
					{
						x += stick_offset[0];
						y += stick_offset[1];
					}
					// Save the other building for use in AdjustPreview and for color changing
					stick_to = other;
					// Found another building and a way to stick to it!
					found = true;
				}
			}
		}
	}

	if (!found)
	{
		// Narrow the distance a construction site can be built around the clonk
		x = BoundBy(x, -dimension_x/2 - clonk_width/2, dimension_x/2 + clonk_width/2);
		y = BoundBy(y, -dimension_y/2 - clonk_height/2, dimension_y/2 + clonk_height/2);
		x = clonk->GetX() + x;
		y = clonk->GetY() + y;
	}

	if (!found && stick_to)
	{
		stick_to = nil;
		SetGraphics(nil, nil, GFX_CombineIconOverlay);
	} 
	else if (stick_to)
	{
		SetGraphics(nil, ConstructionPreviewer_IconCombine, GFX_CombineIconOverlay, GFXOV_MODE_Base);
		var x_dir = 1, y_dir = 1;
		if (stick_to->GetX() < GetX()) x_dir = -1;
		if (stick_to->GetY() < GetY()) y_dir = -1;
		if (single_stick_to == CONSTRUCTION_STICK_Bottom || single_stick_to == CONSTRUCTION_STICK_Top)
			x_dir = 0;
		if (single_stick_to == CONSTRUCTION_STICK_Left || single_stick_to == CONSTRUCTION_STICK_Right)
			y_dir = 0;
		SetObjDrawTransform(1000, 0, dimension_x/2 * 1000 * x_dir, 0, 1000, dimension_y/2 * 1000 * y_dir, GFX_CombineIconOverlay);
	}
	// Update the extra overlay possibly added to the preview.
	extra_overlay = structure->~ConstructionPreview(this, GFX_PreviewerPictureOverlay, direction);
	// Update alternative preview in the definition to be placed.
	structure->~AlternativeConstructionPreview(this, direction, stick_to);
	SetPosition(x, y);
	AdjustPreview(structure->~IsBelowSurfaceConstruction());
}

// Helper function to return a definite sticking direction.
// Used whenever the cursor hovers the 'left' or 'right' part of the other building.
// See Reposition() to see an example of the four parts.
func GetStickingDirection(int stick_dir, int primary_dir, int secondary_dir, int tertiary_dir, int fourth_dir, int cursor_coord, int other_coord)
{
	// If the primary direction is in stick_dir, we're done
	if (stick_dir & primary_dir)
	{
		return primary_dir;
	}
	// Afterwards check second / third directions
	else if (stick_dir & secondary_dir || stick_dir & tertiary_dir)
	{
		// If one of those isn't in stick_dir, no coordinate checking is necessary
		if (!(stick_dir & tertiary_dir))
		{
			return secondary_dir;
		}
		else if (!(stick_dir & secondary_dir))
		{
			return tertiary_dir;
		}
		else
		{
			// Coordinates have to be checked
			// secondary always is one pixel better than tertiary
			if (cursor_coord >= other_coord)
				return secondary_dir;
			else
				return tertiary_dir;
		}
	}
	// Fourth direction is returned last but no other directions takes the point
	else if (stick_dir & fourth_dir)
	{
		return fourth_dir;
	}
	// If this happens, something is wrong
	return 0;
}

// Flips the preview horizontally
func Flip()
{
	// Flip not allowed?
	if (structure->~NoConstructionFlip()) return;

	if (direction == DIR_Left)
	{
		direction = DIR_Right;
		SetObjDrawTransform(-1000,0,0, 0,1000,0, GFX_StructureOverlay);
	} else {
		direction = DIR_Left;
		SetObjDrawTransform(1000,0,0, 0,1000,0, GFX_StructureOverlay);
	}
	if (extra_overlay)
			structure->~ConstructionPreview(this, GFX_PreviewerPictureOverlay, direction);
}

// UI not saved.
func SaveScenarioObject() { return false; }

local Plane = 210;
