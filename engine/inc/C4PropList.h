/* Property lists */

#ifndef C4PROPLIST_H
#define C4PROPLIST_H

#include "C4Value.h"
#include "C4StringTable.h"

class C4Property {
	public:
	C4String * Key;
	C4Value Value;
	operator void * () { return Key; }
	C4Property & operator = (void * p) { assert(!p); Key = 0; Value.Set0(); return *this; }
};

class C4PropList {
	public:
	int32_t Number;
	int32_t Status; // NoSave //
	void AddRef(C4Value *pRef);
	void DelRef(const C4Value *pRef, C4Value * pNextRef);
	void AssignRemoval();
	const char *GetName() { return "FIXME"; }

	bool GetProperty(C4String * k, C4Value & to);
	void SetProperty(C4String * k, C4Value & to);
	C4PropList();
	virtual ~C4PropList();
	private:
	C4Value *FirstRef; // No-Save

	C4Set<C4Property> Properties;
	C4PropList * prototype;
};

#endif // C4PROPLIST_H
