/*-- Star --*/

protected func Initialize()
{
	DoCon(-30+Random(50));
	SetR(Random(359));
	var alpha=0;
	if(GetTime()<300 || GetTime()>1140) alpha=255;
	SetClrModulation(RGBa(255,255,255,alpha));
	this["Parallaxity"] = [10+Random(30),100];

	SetCategory(GetCategory() | C4D_Parallax | C4D_Background);
}

public func IsCelestial() { return true; }

func Definition(def) {
	SetProperty("Name","$Name$",def);
}