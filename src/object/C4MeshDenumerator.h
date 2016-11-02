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

#include "lib/StdMesh.h"
#include "object/C4ObjectPtr.h"

// Helper struct to serialize an object's mesh instance with other object's mesh instances attached
class C4MeshDenumerator : public StdMeshInstance::AttachedMesh::Denumerator
{
private:
	C4Def* Def; // Set if a definition mesh was attached
	C4ObjectPtr Object; // Set if an instance mesh was attached

public:
	C4MeshDenumerator(): Def(nullptr), Object(nullptr) {}
	C4MeshDenumerator(C4Def* def): Def(def), Object(nullptr) {}
	C4MeshDenumerator(C4Object* object): Def(nullptr), Object(object) {}

	C4Object* GetObject() { return Object; }

	virtual void CompileFunc(StdCompiler* pComp, StdMeshInstance::AttachedMesh* attach);
	virtual void DenumeratePointers(StdMeshInstance::AttachedMesh* attach);
	virtual bool ClearPointers(C4Object* pObj);
};

extern const StdMeshInstance::AttachedMesh::DenumeratorFactoryFunc C4MeshDenumeratorFactory;
