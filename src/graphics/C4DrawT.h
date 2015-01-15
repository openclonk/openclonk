/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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
/* Implemention of NewGfx - without gfx */

#ifndef INC_StdNoGfx
#define INC_StdNoGfx

#include <C4Draw.h>

class CStdNoGfx : public C4Draw
{
public:
	CStdNoGfx();
	virtual bool BeginScene() { return true; }
	virtual void EndScene() { }
	virtual void TaskOut() { }
	virtual void TaskIn() { }
	virtual bool UpdateClipper() { return true; }
	virtual bool OnResolutionChanged(unsigned int, unsigned int) { return true; }
	virtual bool PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat);
	virtual bool PrepareRendering(C4Surface *) { return true; }
	virtual void FillBG(DWORD dwClr=0) { }
	virtual void PerformMesh(StdMeshInstance &, float, float, float, float, DWORD, C4BltTransform* pTransform) { }
	virtual void PerformLine(C4Surface *, float, float, float, float, DWORD, float) { }
	virtual void PerformPix(C4Surface *, float, float, DWORD) { }
	virtual bool InitDeviceObjects() { return true; }
	virtual bool RestoreDeviceObjects() { return true; }
	virtual bool InvalidateDeviceObjects() { return true; }
	virtual bool DeleteDeviceObjects() { return true; }
	virtual bool DeviceReady() { return true; }
	virtual bool CreatePrimarySurfaces(bool, unsigned int, unsigned int, int, unsigned int);
	virtual bool SetOutputAdapter(unsigned int) { return true; }
};

#endif
