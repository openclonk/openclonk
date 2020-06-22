/**
	Mesh.c
	Functions regarding meshes.

	@author Randrian, Marky
*/

/* Mesh transformations */


global func SetMeshTransformation(array transformation, int layer)
{
	AssertObjectContext();

	var mesh_transformation_list = GetEffect(FxLayeredMeshTransformation.Name, this) ?? CreateEffect(FxLayeredMeshTransformation, 1);

	if (!mesh_transformation_list.Layers)
	{
		mesh_transformation_list.Layers = [];
	}
	if (GetLength(mesh_transformation_list.Layers) < layer)
	{
		SetLength(mesh_transformation_list.Layers, layer + 1);
	}
	mesh_transformation_list.Layers[layer] = transformation;
	var all_transformations = nil;
	for (var trans in mesh_transformation_list.Layers)
	{
		if (!trans)
		{
			continue;
		}
		if (all_transformations)
		{
			all_transformations = Trans_Mul(trans, all_transformations);
		}
		else
		{
			all_transformations = trans;
		}
	}
	SetProperty("MeshTransformation", all_transformations);
}

static const FxLayeredMeshTransformation = new Effect
{
	Name = "LayeredMeshTransformation",
};
