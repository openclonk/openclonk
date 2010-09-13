/*-- Plant Library --*/

private func SeedChance() {	return 500;	}
private func SeedAreaSize() { return 300; } //The distance seeds may travel
private func SeedAmount() { return 10; } //The amount of plants possible within a diameter of 500px.

protected func Construction()
{
	AddEffect("RootSurface",this,RandomX(100,120),1,this);
}

global func FxRootSurfaceTimer(object target, int num, int timer)
{
	target->RootSurface();
	if(timer > 36) return -1;
}

//Pulls the plant above ground, so it isn't buried from PlaceVegetation(). A plant must have 'Bottom' and 'Middle' CNAT vertices to make use of this.
global func RootSurface()
{
	while(GetContact(-1) & CNAT_Center != 0) SetPosition(GetX(),GetY()-1); //Move up if too far underground
	while(GetContact(-1) & CNAT_Bottom == 0) SetPosition(GetX(),GetY()+1); //Move down if in midair
}

private func Seed()
{
	if(!Random(SeedChance()))
	{
		var iSize = SeedAreaSize();
		var iOffset = iSize / -2;
		if(ObjectCount(Find_ID(this->GetID()), Find_Distance(500)) < SeedAmount())
		{
			PlaceVegetation(GetID(), iOffset, iOffset, iSize, iSize, 3);
		}
	}
}

public func IsPlant() { return true; }