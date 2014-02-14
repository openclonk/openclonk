/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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
