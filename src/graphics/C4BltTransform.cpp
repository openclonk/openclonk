/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "graphics/C4BltTransform.h"

void C4BltTransform::SetRotate(float iAngle, float fOffX, float fOffY) // set by angle and rotation offset
{
	// iAngle is in degrees (cycling from 0 to 360)
	// determine sine and cos of reversed angle in radians
	// fAngle = -iAngle * pi/180 = iAngle * -pi/180
	float fAngle = iAngle * -0.0174532925f;
	float fsin = sinf(fAngle); float fcos = cosf(fAngle);
	// set matrix values
	mat[0] = +fcos; mat[1] = +fsin; mat[2] = (1-fcos)*fOffX - fsin*fOffY;
	mat[3] = -fsin; mat[4] = +fcos; mat[5] = (1-fcos)*fOffY + fsin*fOffX;
	mat[6] = 0; mat[7] = 0; mat[8] = 1;
	/*    calculation of rotation matrix:
	  x2 = fcos*(x1-fOffX) + fsin*(y1-fOffY) + fOffX
	     = fcos*x1 - fcos*fOffX + fsin*y1 - fsin*fOffY + fOffX
	     = x1*fcos + y1*fsin + (1-fcos)*fOffX - fsin*fOffY

	  y2 = -fsin*(x1-fOffX) + fcos*(y1-fOffY) + fOffY
	     = x1*-fsin + fsin*fOffX + y1*fcos - fcos*fOffY + fOffY
	     = x1*-fsin + y1*fcos + fsin*fOffX + (1-fcos)*fOffY */
}

bool C4BltTransform::SetAsInv(C4BltTransform &r)
{
	// calc inverse of matrix
	float det = r.mat[0]*r.mat[4]*r.mat[8] + r.mat[1]*r.mat[5]*r.mat[6]
	            + r.mat[2]*r.mat[3]*r.mat[7] - r.mat[2]*r.mat[4]*r.mat[6]
	            - r.mat[0]*r.mat[5]*r.mat[7] - r.mat[1]*r.mat[3]*r.mat[8];
	if (!det) { Set(1,0,0,0,1,0,0,0,1); return false; }
	mat[0] = (r.mat[4] * r.mat[8] - r.mat[5] * r.mat[7]) / det;
	mat[1] = (r.mat[2] * r.mat[7] - r.mat[1] * r.mat[8]) / det;
	mat[2] = (r.mat[1] * r.mat[5] - r.mat[2] * r.mat[4]) / det;
	mat[3] = (r.mat[5] * r.mat[6] - r.mat[3] * r.mat[8]) / det;
	mat[4] = (r.mat[0] * r.mat[8] - r.mat[2] * r.mat[6]) / det;
	mat[5] = (r.mat[2] * r.mat[3] - r.mat[0] * r.mat[5]) / det;
	mat[6] = (r.mat[3] * r.mat[7] - r.mat[4] * r.mat[6]) / det;
	mat[7] = (r.mat[1] * r.mat[6] - r.mat[0] * r.mat[7]) / det;
	mat[8] = (r.mat[0] * r.mat[4] - r.mat[1] * r.mat[3]) / det;
	return true;
}

void C4BltTransform::TransformPoint(float &rX, float &rY) const
{
	// apply matrix
	float fW = mat[6] * rX + mat[7] * rY + mat[8];
	// store in temp, so original rX is used for calculation of rY
	float fX = (mat[0] * rX + mat[1] * rY + mat[2]) / fW;
	rY = (mat[3] * rX + mat[4] * rY + mat[5]) / fW;
	rX = fX; // apply temp
}
