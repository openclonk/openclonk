/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005, 2008  Günther Brammer
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

/* 32-bit value to identify object definitions */

#ifndef INC_C4Id
#define INC_C4Id

#include <StdAdaptors.h>

class C4ID
	{
	uint32_t v;
	public:
	C4ID() {}
	C4ID(unsigned int i): v(i) {}
	operator unsigned int () const { return v; }
	friend bool operator ==(C4ID a, C4ID b);
	};

inline bool operator ==(C4ID a, C4ID b) { return a.v == b.v; }

const C4ID C4ID_None       = 0x0,
					 C4ID_Clonk      = 0x4B4E4C43, // CLNK
					 C4ID_Flag       = 0x47414C46, // FLAG
					 C4ID_Conkit     = 0x544B4E43, // CNKT
					 C4ID_Gold       = 0x444C4F47, // GOLD
					 C4ID_Lorry      = 0x59524F4C, // LORY
					 C4ID_Meteor     = 0x4F54454D, // METO
					 C4ID_Linekit    = 0x544B4E4C, // LNKT
					 C4ID_PowerLine  = 0x4C525750, // PWRL
					 C4ID_SourcePipe = 0x50495053, // SPIP
					 C4ID_DrainPipe  = 0x50495044, // DPIP
					 C4ID_Energy     = 0x47524E45, // ENRG
					 C4ID_CnMaterial = 0x544D4E43, // CNMT
					 C4ID_FlagRemvbl = 0x56524746, // FGRV
					 C4ID_Contents   = 0x00002710; // 10001


C4ID C4Id(const char *szId);
void GetC4IdText(C4ID id, char *sBuf);
const char *C4IdText(C4ID id);
bool LooksLikeID(const char *szText);
bool LooksLikeID(C4ID id);

// * C4ID Adaptor
struct C4IDAdapt
{
	C4ID &rValue;
	explicit C4IDAdapt(C4ID &rValue) : rValue(rValue) { }
	inline void CompileFunc(StdCompiler *pComp) const
	{
    char cC4ID[5];
    if(pComp->isDecompiler())
      GetC4IdText(rValue, cC4ID);

    pComp->Value(mkStringAdapt(cC4ID, 4, StdCompiler::RCT_ID));

    if(pComp->isCompiler())
		{
			if (strlen(cC4ID) != 4)
				rValue = 0;
			else
				rValue = C4Id(cC4ID);
		}
	}
	// Operators for default checking/setting
	inline bool operator == (const C4ID &nValue) const { return rValue == nValue; }
	inline C4IDAdapt &operator = (const C4ID &nValue) { rValue = nValue; return *this; }
	// trick g++
	ALLOW_TEMP_TO_REF(C4IDAdapt)
};
inline C4IDAdapt mkC4IDAdapt(C4ID &rValue) { return C4IDAdapt(rValue); }

#endif
