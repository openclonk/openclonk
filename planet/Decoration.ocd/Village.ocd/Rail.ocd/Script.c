/**
	@author Dustin Neß (dness.de)
*/

protected func Construction()
{
	// random direction
	if (Random(2))
	{
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(15,0,10), Trans_Scale(120)));
	}
	else
	{
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(195,0,180), Trans_Scale(120)));
	}
}

// bring to front
public func SetToFront(bool is_in_front)
{
	if (is_in_front)
	{
		this.Plane = 510;
	}
	else
	{
		this.Plane = 110;	
	}
	return this.Plane;
}
