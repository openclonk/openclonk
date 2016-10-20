/*
 * OpenClonk, http://www.openclonk.org
 *
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

#ifndef INC_C4BltTransform
#define INC_C4BltTransform

// rotation info class
class C4BltTransform
{
public:
	float mat[9]; // transformation matrix
public:
	C4BltTransform() {} // default: don't init fields
	void Set(float fA, float fB, float fC, float fD, float fE, float fF, float fG, float fH, float fI)
	{ mat[0]=fA; mat[1]=fB; mat[2]=fC; mat[3]=fD; mat[4]=fE; mat[5]=fF; mat[6]=fG; mat[7]=fH; mat[8]=fI; }
	void SetRotate(float iAngle, float fOffX, float fOffY); // set by angle and rotation offset
	bool SetAsInv(C4BltTransform &rOfTransform);
	void Rotate(float Angle, float fOffX, float fOffY) // rotate by angle around rotation offset
	{
		// multiply matrix as seen in SetRotate by own matrix
		C4BltTransform rot; rot.SetRotate(Angle, fOffX, fOffY);
		(*this) *= rot;
	}
	void SetMoveScale(float dx, float dy, float sx, float sy)
	{
		mat[0] = sx; mat[1] = 0;  mat[2] = dx;
		mat[3] = 0;  mat[4] = sy; mat[5] = dy;
		mat[6] = 0;  mat[7] = 0;  mat[8] = 1;
	}
	void MoveScale(float dx, float dy, float sx, float sy)
	{
		// multiply matrix by movescale matrix
		C4BltTransform move; move.SetMoveScale(dx,dy,sx,sy);
		(*this) *= move;
	}
	void ScaleAt(float sx, float sy, float tx, float ty)
	{
		// scale and move back so tx and ty remain fixpoints
		MoveScale(-tx*(sx-1), -ty*(sy-1), sx, sy);
	}
	C4BltTransform &operator *= (const C4BltTransform &r)
	{
		// transform transformation
		Set(mat[0]*r.mat[0] + mat[3]*r.mat[1] + mat[6]*r.mat[2],
		    mat[1]*r.mat[0] + mat[4]*r.mat[1] + mat[7]*r.mat[2],
		    mat[2]*r.mat[0] + mat[5]*r.mat[1] + mat[8]*r.mat[2],
		    mat[0]*r.mat[3] + mat[3]*r.mat[4] + mat[6]*r.mat[5],
		    mat[1]*r.mat[3] + mat[4]*r.mat[4] + mat[7]*r.mat[5],
		    mat[2]*r.mat[3] + mat[5]*r.mat[4] + mat[8]*r.mat[5],
		    mat[0]*r.mat[6] + mat[3]*r.mat[7] + mat[6]*r.mat[8],
		    mat[1]*r.mat[6] + mat[4]*r.mat[7] + mat[7]*r.mat[8],
		    mat[2]*r.mat[6] + mat[5]*r.mat[7] + mat[8]*r.mat[8]);
		return *this;
	}
	void TransformPoint(float &rX, float &rY) const; // rotate point by angle
};

#endif
