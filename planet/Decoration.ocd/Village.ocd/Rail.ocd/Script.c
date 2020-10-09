/**
	@author Dustin Neß (dness.de)
*/

func Definition(proplist def)
{
	def.MeshTransformation = Trans_Scale(120);
}

protected func Construction()
{
	// random direction
	if (Random(2))
	{
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(15, 0, 10), GetID().MeshTransformation));
	}
	else
	{
		SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(195, 0, 180), GetID().MeshTransformation));
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
