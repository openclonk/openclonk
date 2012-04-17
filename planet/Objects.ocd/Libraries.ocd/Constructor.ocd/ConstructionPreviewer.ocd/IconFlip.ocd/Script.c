/**
	ConstructionPreviewer_IconFlip


	@author Clonkonaut
*/

local previewer;

func Construction(object mom)
{
	SetProperty("Visibility", VIS_Owner);
	previewer = mom;
	SetPosition(previewer->GetX(), previewer->GetY() - (previewer.dimension_y / 2) - 8);
	this->Message("@$Space$");
}

func KeepPosition()
{
	if (!previewer) return RemoveObject();

	SetPosition(previewer->GetX(), previewer->GetY() - (previewer.dimension_y / 2) - 8);
}