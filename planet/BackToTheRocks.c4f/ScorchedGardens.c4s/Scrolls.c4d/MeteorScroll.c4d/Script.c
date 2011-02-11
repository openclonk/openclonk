/*--
	Scroll: Meteor
	Author: Mimmo

	Toss a meteor onto your enemies.
--*/


public func ControlUse(object pClonk, int ix, int iy)
{
	var r=Angle(0,0,ix,iy);
	var flnt = CreateObject(Firestone,Sin(r,10),-Cos(r,10),pClonk->GetOwner());
	flnt->SetXDir( Sin(r,60) + RandomX(-3,3));
	flnt->SetYDir(-Cos(r,60) + RandomX(-3,3));
	AddEffect("Meteorsparkle",flnt,100,1,nil,nil,false);
	
	RemoveObject();
	return 1;
}


local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
