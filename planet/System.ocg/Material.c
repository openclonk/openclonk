/**
	Material.c
	Functions relating to materials; any kind of operations.
	
	@author Ringwaul
*/

// If depth is nil, the function will always measure the depth of the material.
// Otherwise, the function will return true if the material is as deep or deeper than depth (in pixels).
global func MaterialDepthCheck(int x, int y, string mat, int depth)
{
	var travelled = 0;
	var matnum = Material(mat);

	while (y+GetY() < LandscapeHeight() && GetMaterial(x, y) == matnum)
	{
		travelled++;
		y++;
		if (travelled == depth)
			return true;
	}
	if (depth == nil)
		return travelled;
	return false;
}

global func FindPosInMat(string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
{
	var iX, iY;
	var iMaterial = Material(sMat);
	for(var i = 0; i < 500; i++)
	{
		iX = AbsX(iXStart+Random(iWidth));
		iY = AbsY(iYStart+Random(iHeight));
		if(GetMaterial(iX,iY)==iMaterial &&
		   GetMaterial(iX+iSize,iY+iSize)==iMaterial &&
		   GetMaterial(iX+iSize,iY-iSize)==iMaterial &&
		   GetMaterial(iX-iSize,iY-iSize)==iMaterial &&
		   GetMaterial(iX-iSize,iY+iSize)==iMaterial
		) {
			return [iX, iY]; // Location found.
		}
	}
	return 0; // No location found.
}

/** Removes a material pixel from the specified location, if the material is a liquid.
	@param x X coordinate
	@param y Y coordinate
	@param distant_first If true, extraction position takes largest horizontal distance from given offset at same height to a maximum value of MaxSlide. Useful to ensure that no floor of 1px of liquid remains.
	@return The material index of the removed pixel, or -1 if no liquid was found. */
// documented in /docs/sdk/script/fn
global func ExtractLiquid(int x, int y, bool distant_first)
{
	var result = ExtractLiquidAmount(x, y, 1, distant_first);
	if (!result)
		return -1;
	return result[0];
}

/** Tries to remove amount material pixels from the specified location if the material is a liquid.
	@param x X coordinate
	@param y Y coordinate
	@param amount amount of liquid that should be extracted
	@param distant_first If true, extraction position takes largest horizontal distance from given offset at same height to a maximum value of MaxSlide. Useful to ensure that no floor of 1px of liquid remains.
	@return an array with the first position being the material index being extracted and the second the
			actual amount of pixels extracted OR nil if there was no liquid at all */
global func ExtractLiquidAmount(int x, int y, int amount, bool distant_first)
{
	var mat = GetMaterial(x, y);
	if(mat == -1)
		return nil;
	var density = GetMaterialVal("Density", "Material", mat);
	if (density < C4M_Liquid || density >= C4M_Solid)
		return nil;
	var extracted_amount = ExtractMaterialAmount(x, y, mat, amount, distant_first);
	if (extracted_amount <= 0)
		return nil;
	return [mat, extracted_amount];
}

/** Removes a material pixel from the specified location, if the material is flammable
	@param x X coordinate. Offset if called in object context.
	@param y Y coordinate. Offset if called in object context.
	@return true if material was removed, false otherwise. */
global func FlameConsumeMaterial(int x, int y)
{
	var mat = GetMaterial(x, y);
	if (mat == -1)
		return false;
	if (!GetMaterialVal("Inflammable", "Material", mat))
		return false;
	return !!ExtractMaterialAmount(x, y, mat, 1);
}

// Draws a rectangular triangle of the specified material.
// The center coordinates (cx, cy) represent the center of the square the triangle would be in.
// dir (0, 1, 2, 3) is the direction of the long edge (up-right, down-right, down-left, up-left).
global func DrawMaterialTriangle(string mat_tex, int cx, int cy, int dir, int edge_size, bool sub)
{
	// Default to the standard map pixel size.
	if (edge_size == nil)
		edge_size = 8;
	edge_size /= 2;
	// Determine direction.
	var sx = -1;
	if (dir == 2 || dir == 3)
		sx = 1;
	var sy = -1;
	if (dir == 1 || dir == 2)
		sy = 1;
	DrawMaterialQuad(mat_tex, 
		cx + sx * edge_size, cy - sy * edge_size, 
		cx + sx * edge_size, cy + sy * edge_size, 
		cx - sx * edge_size, cy - sy * edge_size,
		cx                 , cy - sy * edge_size,
		sub
	);
	return;
}
