/**
	ConstructionPreviewer
	

	@author Clonkonaut
*/

local dimension_x, dimension_y, clonk, structure;

func Initialize()
{
	SetProperty("Visibility", VIS_Owner);
}

func Set(id to_construct, object constructing_clonk)
{
	SetGraphics(nil, to_construct, GFX_Overlay, GFXOV_MODE_Base);
	dimension_x = to_construct->GetDefWidth();
	dimension_y = to_construct->GetDefHeight();
	clonk = constructing_clonk;
	structure = to_construct;
	AdjustPreview();
}

// Positions the preview according to the landscape, coloring it green or red
func AdjustPreview(bool look_up, bool no_call)
{
	// Place on material
	var search_dir = 1;
	if (look_up) search_dir = -1;
	var x = 0, y = 0, fail = false;
	var half_y = dimension_y / 2;
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
		return SetClrModulation(RGBa(255,50,50, 100), GFX_Overlay);
	SetPosition(GetX(), GetY() + y);
	
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
		SetClrModulation(RGBa(50,255,50, 100), GFX_Overlay);
	else
		SetClrModulation(RGBa(255,50,50, 100), GFX_Overlay);
}

// Positions the preview according to the mouse cursor, calls AdjustPreview afterwards
// x and y are refined mouse coordinates so always centered at the clonk
func Reposition(int x, int y)
{
	x = BoundBy(x, -dimension_x/2, dimension_x/2);
	y = BoundBy(y, -dimension_y/2, dimension_y/2);
	SetPosition(clonk->GetX() + x, clonk->GetY() + y);
	AdjustPreview();
}