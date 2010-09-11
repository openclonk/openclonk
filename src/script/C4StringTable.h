/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002  Peter Wortmann
 * Copyright (c) 2009  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
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
/* string table: holds all strings used by script engine */

#ifndef C4STRINGTABLE_H

#define C4STRINGTABLE_H

#include <vector>

class C4StringTable;
class C4Group;

class C4String
{
	explicit C4String(StdStrBuf strString);
	explicit C4String(const char *strString);

	StdCopyStrBuf Data; // string data
	int iRefCnt; // reference count on string (by C4Value)

	friend class C4StringTable;
public:
	~C4String();

	// increment/decrement reference count on this string
	void IncRef();
	void DecRef();

	const char * GetCStr() const { return Data.getData(); }
	StdStrBuf GetData() const { return Data.getRef(); }

	unsigned int Hash;
};

template<typename T> class C4Set
{
	unsigned int Capacity;
	unsigned int Size;
	T * Table;
	T * AddInternal(T e)
	{
		unsigned int h = Hash(e);
		T * p = &Table[h % Capacity];
		while (*p && *p != e)
		{
			p = &Table[++h % Capacity];
		}
		*p = e;
		return p;
	}
public:
	template<typename H> static unsigned int Hash(H);
	template<typename H> static bool Equals(T, H);
	static bool Equals(T a, T b) { return a == b; }
	// FIXME: Profile for initial size
	C4Set(): Capacity(16), Size(0), Table(new T[Capacity])
	{
		Clear();
	}
	~C4Set()
	{
		delete[] Table;
	}
	void CompileFunc(StdCompiler *pComp);
	void Clear()
	{
		for (unsigned int i = 0; i < Capacity; ++i)
			Table[i] = 0;
	}
	template<typename H> T & Get(H e) const
	{
		unsigned int h = Hash(e);
		T * r = &Table[h % Capacity];
		while (*r && !Equals(*r, e))
		{
			r = &Table[++h % Capacity];
		}
		return *r;
	}
	template<typename H> bool Has(H e) const
	{
		unsigned int h = Hash(e);
		T * r = &Table[h % Capacity];
		while (*r && !Equals(*r, e))
		{
			r = &Table[++h % Capacity];
		}
		return !!*r;
	}
	unsigned int GetSize() const { return Size; }
	T * Add(T e)
	{
		// FIXME: Profile for load factor
		if (Size > Capacity / 2)
		{
			unsigned int OCapacity = Capacity;
			Capacity *= 2;
			T * OTable = Table;
			Table = new T[Capacity];
			Clear();
			for (unsigned int i = 0; i < OCapacity; ++i)
			{
				if (OTable[i])
					AddInternal(OTable[i]);
			}
			delete [] OTable;
		}
		T * r = AddInternal(e);
		++Size;
		return r;
	}
	template<typename H> void Remove(H e)
	{
		unsigned int h = Hash(e);
		T * r = &Table[h % Capacity];
		while (*r && !Equals(*r, e))
		{
			r = &Table[++h % Capacity];
		}
		assert(*r);
		*r = 0;
		--Size;
		// Move entries which might have collided with e
		while (*(r = &Table[++h % Capacity]))
		{
			T m = *r;
			*r = 0;
			AddInternal(m);
		}
	}
	T const * First() const { return Next(Table - 1); }
	T const * Next(T const * p) const
	{
		while (++p != &Table[Capacity])
		{
			if (*p) return p;
		}
		return 0;
	}
};

template<> template<>
inline unsigned int C4Set<C4String *>::Hash<const C4String *>(const C4String * e)
{
	return e->Hash;
}
template<> template<>
inline unsigned int C4Set<C4String *>::Hash<C4String *>(C4String * e)
{
	return e->Hash;
}

enum C4PropertyName
{
	P_Prototype,
	P_Name,
	P_Collectible,
	P_ActMap,
	P_Attach,
	P_Visibility,
	P_Parallaxity,
	P_LineColors,
	P_LineAttach,
	P_PictureTransformation,
	P_MeshTransformation,
	P_Procedure,
	P_Directions,
	P_FlipDir,
	P_Length,
	P_Delay,
	P_X,
	P_Y,
	P_Wdt,
	P_Hgt,
	P_OffX,
	P_OffY,
	P_FacetBase,
	P_FacetTopFace,
	P_FacetTargetStretch,
	P_NextAction,
	P_Hold,
	P_Idle,
	P_NoOtherAction,
	P_StartCall,
	P_EndCall,
	P_AbortCall,
	P_PhaseCall,
	P_Sound,
	P_ObjectDisabled,
	P_DigFree,
	P_EnergyUsage,
	P_InLiquidAction,
	P_TurnAction,
	P_Reverse,
	P_Step,
	P_MouseDragImage,
	P_Animation,
	P_Action,
	P_LAST
};

// There is only one Stringtable
class C4StringTable
{
public:
	C4StringTable();
	virtual ~C4StringTable();

	C4String *RegString(StdStrBuf String);
	C4String *RegString(const char * s) { return RegString(StdStrBuf(s)); }
	// Find existing C4String
	C4String *FindString(const char *strString);
	// Check wether the pointer is a C4String
	C4String *FindString(C4String *pString);

	C4Set<C4String *> Set;
	C4String * P[P_LAST];
};

extern C4StringTable Strings;

#endif
