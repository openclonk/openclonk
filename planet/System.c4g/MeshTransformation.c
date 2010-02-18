
// Some common transformation matrices, and a function to multiple them
// together to produce more complex transformations.

global func Trans_Identity()
{
	return [1000, 0,    0,    0,
	        0,    1000, 0,    0,
		0,    0,    1000, 0];
}

global func Trans_Translate(int dx, int dy, int dz)
{
	return [1000, 0,    0,    dx,
	        0,    1000, 0,    dy,
		0,    0,    1000, dz];
}

global func Trans_Scale(int sx, int sy, int sz)
{
	if(sy==nil && sz==nil)
		sz = sy = sx;

	return [sx, 0,  0,  0,
	        0,  sy, 0,  0,
		0,  0,  sz, 0];
}

global func Trans_Rotate(int angle, int rx, int ry, int rz)
{
	var c = Cos(angle, 1000);
	var s = Sin(angle, 1000);
	var n = Sqrt(rx*rx + ry*ry + rz*rz);
	rx = 1000 * rx / n;
	ry = 1000 * ry / n;
	rz = 1000 * rz / n;

	// Note that 0 <= rx,ry,rz,c,s <= 1000, so we don't overflow here
	return [
		rx*rx*(1000-c)/1000000+c,         rx*ry*(1000-c)/1000000-rz*s/1000, rx*rz*(1000-c)/1000000+ry*s/1000, 0,
		ry*rx*(1000-c)/1000000+rz*s/1000, ry*ry*(1000-c)/1000000+c,         ry*rz*(1000-c)/1000000-rx*s/1000, 0,
		rz*rx*(1000-c)/1000000-ry*s/1000, ry*rz*(1000-c)/1000000+rx*s/1000, rz*rz*(1000-c)/1000000+c,         0];
}

global func Trans_Mul(array lhs, array rhs)
{
	return [
		lhs[0]*rhs[0]/1000 + lhs[1]*rhs[4]/1000 + lhs[ 2]*rhs[ 8]/1000,
		lhs[0]*rhs[1]/1000 + lhs[1]*rhs[5]/1000 + lhs[ 2]*rhs[ 9]/1000,
		lhs[0]*rhs[2]/1000 + lhs[1]*rhs[6]/1000 + lhs[ 2]*rhs[10]/1000,
		lhs[0]*rhs[3]/1000 + lhs[1]*rhs[7]/1000 + lhs[ 2]*rhs[11]/1000 + lhs[3],

		lhs[4]*rhs[0]/1000 + lhs[5]*rhs[4]/1000 + lhs[ 6]*rhs[ 8]/1000,
		lhs[4]*rhs[1]/1000 + lhs[5]*rhs[5]/1000 + lhs[ 6]*rhs[ 9]/1000,
		lhs[4]*rhs[2]/1000 + lhs[5]*rhs[6]/1000 + lhs[ 6]*rhs[10]/1000,
		lhs[4]*rhs[3]/1000 + lhs[5]*rhs[7]/1000 + lhs[ 6]*rhs[11]/1000 + lhs[7],

		lhs[8]*rhs[0]/1000 + lhs[9]*rhs[4]/1000 + lhs[10]*rhs[ 8]/1000,
		lhs[8]*rhs[1]/1000 + lhs[9]*rhs[5]/1000 + lhs[10]*rhs[ 9]/1000,
		lhs[8]*rhs[2]/1000 + lhs[9]*rhs[6]/1000 + lhs[10]*rhs[10]/1000,
		lhs[8]*rhs[3]/1000 + lhs[9]*rhs[7]/1000 + lhs[10]*rhs[11]/1000 + lhs[11] ];
}
