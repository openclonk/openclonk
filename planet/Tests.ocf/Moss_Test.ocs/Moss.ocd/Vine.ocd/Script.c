/*-- Vine --*/

local size;
local parent;
local maxsize;

func Initialize()
{
	var graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	size = 1;
	SetObjDrawTransform(10,0,0,0,10);
	maxsize=75+Random(10);
	AddEffect("VineGrow", this, 100, 18, this, this.ID);
	SetR(Random(360));
}

private func FxVineGrowTimer(target, effect, time)
{
	if(Distance(GetX(),GetY(),parent->GetX(),parent->GetY())>2)
		RemoveObject();

	size++;
	if(size > maxsize)
		return -1;
	SetObjDrawTransform(10*size,0,0,0,10*size );
	return 1;

}


