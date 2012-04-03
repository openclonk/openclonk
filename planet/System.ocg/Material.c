/*--
		Material.c
		Authors: Ringwaul

		Functions in relation to material; any kind of operations.
--*/

global func MaterialDepthCheck(int x, int y, string mat, int depth)
{
	var travelled;
	var xval = x;
	var yval = y;

	//If depth is equal to zero, the function will always measure the depth of the material.
	//If depth is not equal to zero, the function will return true if the material is as deep or deeper than depth (in pixels).
	if (depth == nil)
		depth = LandscapeHeight();

	while (travelled != depth)
	{
		if (GetMaterial(xval, yval) == Material(mat))
		{
			travelled++;
			yval++;
		}
		if (GetMaterial(xval, yval) != Material(mat))
			return travelled; // Returns depth of material.
	}
	if (travelled == depth)
		return true;
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

// Removes a material pixel from the specified location, if the material is a liquid.
// \par x X coordinate. Offset if called in object context.
// \par y Y coordinate. Offset if called in object context.
// \returns The material index of the removed pixel, or -1 if no liquid was found.
global func ExtractLiquid(int x, int y)
{
	var mat = GetMaterial(x, y);
	var density = GetMaterialVal("Density", "Material", mat);
	if (density < C4M_Liquid || density >= C4M_Solid)
		return -1;
	ExtractMaterialAmount(x, y, mat, 1);
	return mat;
}

// Removes a material pixel from the specified location, if the material is flammable.
// \par x X coordinate. Offset if called in object context.
// \par y Y coordinate. Offset if called in object context.
// \returns \c true if material was removed, \c false otherwise.
global func FlameConsumeMaterial(int x, int y)
{
	var mat = GetMaterial(x, y);
	if (mat == -1)
		return false;
	if (!GetMaterialVal("Inflammable", "Material", mat))
		return false;
	return !!ExtractMaterialAmount(x, y, mat, 1);
}