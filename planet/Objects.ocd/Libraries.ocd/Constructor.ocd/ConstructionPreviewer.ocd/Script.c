/**
	ConstructionPreviewer
	

	@author Clonkonaut
*/

local dimension_x, dimension_y, clonk, structure, direction, stick_to;
local GFX_StructureOverlay = 1;
local GFX_CombineIconOverlay = 2;

public func GetFlipDescription() { return "$TxtFlipDesc$"; }

func Initialize()
{
	SetProperty("Visibility", VIS_Owner);
}

func Set(id to_construct, object constructing_clonk)
{
	SetGraphics(nil, to_construct, GFX_StructureOverlay, GFXOV_MODE_Base);
	SetGraphics(nil, to_construct, 3, GFXOV_MODE_Picture, nil, GFX_BLIT_Wireframe);
	dimension_x = to_construct->GetDefWidth();
	dimension_y = to_construct->GetDefHeight();

	clonk = constructing_clonk;
	structure = to_construct;
	direction = DIR_Left;
	AdjustPreview();
}

// Positions the preview according to the landscape, coloring it green, yellow or red
func AdjustPreview(bool look_up, bool no_call)
{
	var half_y = dimension_y / 2;
	// Do only if not sticking to another object
	if (!stick_to)
	{
		// Place on material
		var search_dir = 1;
		if (look_up) search_dir = -1;
		var x = 0, y = 0, fail = false;
		while(!(!GBackSolid(x,y + half_y) && GBackSolid(x,y + half_y + 1)))
		{
			y += search_dir;
			if (Abs(y) > dimension_y/2)
			{
				fail = true;
				break;
			}
		}
		if (fail && !no_call)
			return AdjustPreview(!look_up, true);
		if (fail)
			return SetClrModulation(RGBa(255,50,50, 100), GFX_StructureOverlay);
		SetPosition(GetX(), GetY() + y);
	}
	if (!CheckConstructionSite(structure, 0, half_y))
		fail = true;
	else
	// intersection-check with all other construction sites... bah
	for(var other_site in FindObjects(Find_ID(ConstructionSite)))
	{
		if(!(other_site->GetLeftEdge()   > GetX()+dimension_x/2  ||
		     other_site->GetRightEdge()  < GetX()-dimension_x/2  ||
		     other_site->GetTopEdge()    > GetY()+half_y  ||
		     other_site->GetBottomEdge() < GetY()-half_y))
			{
				fail = true;
			} 
	}
	
	
	if(!fail)
	{
		if (!stick_to)
			SetClrModulation(RGBa(50,255,50, 100), GFX_StructureOverlay);
		else
			SetClrModulation(RGBa(255,255,50, 200), GFX_StructureOverlay);
	}
	else
		SetClrModulation(RGBa(255,50,50, 100), GFX_StructureOverlay);
}

// Positions the preview according to the mouse cursor, calls AdjustPreview afterwards
// x and y are refined mouse coordinates so always centered at the clonk
func Reposition(int x, int y)
{
	x = BoundBy(x, -dimension_x/2, dimension_x/2);
	y = BoundBy(y, -dimension_y/2, dimension_y/2);
	var found = false;
	if (structure->~ConstructionCombineWith())
	{
		var other = FindObject(Find_Func(structure->ConstructionCombineWith(), this),
		                              Find_InRect(AbsX(clonk->GetX() + x - dimension_x/2 - 10), AbsY(clonk->GetY() + y - dimension_y/2 - 10), dimension_x + 20, dimension_y + 20),
		                              Find_OCF(OCF_Fullcon),
		                              Find_Layer(clonk->GetObjectLayer()),
		                              Find_Allied(clonk->GetOwner()),
		                              Find_NoContainer());
		if (other)
		{
			if (other->GetX() < GetX())
				x = other->GetX() + other->GetObjWidth()/2 + dimension_x / 2;
			else
				x = other->GetX() - other->GetObjWidth()/2 - dimension_x / 2;
			y = other->GetY();
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
	} else if (stick_to) {
		SetGraphics(nil, ConstructionPreviewer_IconCombine, GFX_CombineIconOverlay, GFXOV_MODE_Base);
		var dir = 1;
		if (stick_to->GetX() < GetX()) dir = -1;
		SetObjDrawTransform(1000, 0, dimension_x/2 * 1000 * dir, 0, 1000, 0, GFX_CombineIconOverlay);
	}

	SetPosition(x, y);
	AdjustPreview();
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
}