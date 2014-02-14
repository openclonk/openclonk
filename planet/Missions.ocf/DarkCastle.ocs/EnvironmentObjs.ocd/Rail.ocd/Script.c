/**
	@author Dustin Neﬂ (dness.de)
*/

protected func Construction()
{
	//random Side
	if(Random(2))
	{
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(15,0,10), Trans_Scale(120)));
	} else {
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(195,0,180), Trans_Scale(120)));
	}
}

//bring to front
public func SetToFront(bool fBool) {
	if(fBool) {
		return this.Plane = 510;
	} else {
		return this.Plane = 110;	
	}
}
