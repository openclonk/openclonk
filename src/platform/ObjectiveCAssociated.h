/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010  Martin Plicht
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
 
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

typedef
#ifdef __OBJC__
	id
#else
	void*
#endif
	ObjCPtr;

class ObjectiveCAssociated {
public:
	ObjCPtr _objectiveCObject;
public:
	ObjectiveCAssociated(): _objectiveCObject(NULL) {}
#ifdef __OBJC__
	void setObjectiveCObject(id obj) { _objectiveCObject = obj; }
	template<class T> inline T* objectiveCObject() { return (T*)_objectiveCObject; }
#endif
};