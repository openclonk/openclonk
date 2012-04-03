/*--
		Vertices.c
		Authors: Clonkonaut, flgr

		Vertex related functions.
--*/

// Sets both the X and Y-coordinate of one vertex.
global func SetVertexXY(int index, int x, int y)
{
	// Set vertices.
	SetVertex(index, VTX_X, x);
	SetVertex(index, VTX_Y, y);
	return;
}

// Returns the number of stuck vertices. (of this)
global func VerticesStuck()
{
	var vertices = 0;
	// Loop through vertices.
	for (var i = -1; i < GetVertexNum(); i++)
		// Solid?
		if (GBackSolid(GetVertex(i, VTX_X), GetVertex(i, VTX_Y)))
			// Count vertices.
			vertices++;
	return vertices;
}

// Returns whether the object has a vertex with the given CNAT value
global func HasCNAT(int cnat)
{
	for(var i = -1; i < GetVertexNum(); i++)
		if (GetVertex(i, VTX_CNAT) == cnat)
			return true;
	return false;
}