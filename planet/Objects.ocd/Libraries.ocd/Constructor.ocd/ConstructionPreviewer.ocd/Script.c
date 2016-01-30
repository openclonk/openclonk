/**
	ConstructionPreviewer
	

	@author Clonkonaut
*/

static const CONSTRUCTION_STICK_Left = 1;
static const CONSTRUCTION_STICK_Right = 2;
static const CONSTRUCTION_STICK_Bottom = 4;

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
	x = BoundBy(x, -dimension_x/2, dimension_x/2);
	y = BoundBy(y, -dimension_y/2, dimension_y/2);
	// Try to combine the structure with other structures.
	var found = false;
	if (structure->~ConstructionCombineWith())
	{
		var stick_dir = structure->~ConstructionCombineDirection();
		var find_rect = Find_InRect(AbsX(clonk->GetX() + x - dimension_x/2 - 10), AbsY(clonk->GetY() + y - dimension_y/2 - 10), dimension_x + 20, dimension_y + 20);
		if ((stick_dir & CONSTRUCTION_STICK_Bottom))
			find_rect = Find_AtPoint(AbsX(clonk->GetX() + x), AbsY(clonk->GetY() + y));
		var other = FindObject(Find_Func(structure->ConstructionCombineWith(), this),
		                              find_rect,
		                              Find_OCF(OCF_Fullcon),
		                              Find_Layer(clonk->GetObjectLayer()),
		                              Find_Allied(clonk->GetOwner()),
		                              Find_NoContainer());
		if (other)
		{
			x = other->GetX();
			y = other->GetY();
			// Combine to different directions.
			if ((stick_dir & CONSTRUCTION_STICK_Left) && other->GetX() >= GetX())
				x = other->GetX() - other->GetObjWidth()/2 - dimension_x / 2;
			if ((stick_dir & CONSTRUCTION_STICK_Right) && other->GetX() < GetX())
				x = other->GetX() + other->GetObjWidth()/2 + dimension_x / 2;
			if ((stick_dir & CONSTRUCTION_STICK_Bottom))
				y = other->GetY() + other->GetObjHeight()/2 + dimension_y / 2;
			// Add an additional offset if needed, for example a basement can be place
			// only under a part of the structure.
			var stick_offset = structure->~ConstructionCombineOffset(other);
			if (stick_offset)
			{
				x += stick_offset[0];
				y += stick_offset[1];
			}
			stick_to = other;
			found = true;
		}
	}
	if (!found)
	{
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
		var dir = 1;
		if (stick_to->GetX() < GetX()) dir = -1;
		if (structure->~CombineToBottom())
			dir = 0;
		SetObjDrawTransform(1000, 0, dimension_x/2 * 1000 * dir, 0, 1000, 0, GFX_CombineIconOverlay);
	}
	// Update the extra overlay possibly added to the preview.
	extra_overlay = structure->~ConstructionPreview(this, GFX_PreviewerPictureOverlay, direction);
	// Update alternative preview in the definition to be placed.
	structure->~AlternativeConstructionPreview(this, direction, stick_to);
	SetPosition(x, y);
	AdjustPreview(structure->~IsBelowSurfaceConstruction());
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