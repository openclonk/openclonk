/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003, 2007  Matthes Bender
 * Copyright (c) 2005  Günther Brammer
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

#ifndef C4CONFIGSHAREWARE_H_INC
#define C4CONFIGSHAREWARE_H_INC

#include <C4Config.h>
#include <C4Group.h>

const int MaxRegDataLen = 4096;
const char szInvalidKeyData[] = "r>iU218f3_030\r\n0ipX>ZeuX\r\nZichtVLpI=WeUt\r\nYNtzNcQy=EZs1\r\nAhVsloP=PYptk TE0e\r\n5Vtms5_0:-0^-0>\r\nDfhCqHy=27CIxFpB\r\n\r\nnxSR+?bD50+H[:fEnyW^UcASVTSR9n>Oez`2qHN3YWbz8P;SSqkvXtXMM6Z1UQNT\r\nrpFHRy/6pZ2T6E1iGF1Dt\370Ofw7f\370bUvuM3_jl8TsxWN8;d0kCj3v/JRWBO/Gvxpx\r\nTrBomp81>gkZoddjFcyTwx[J/dNIKEzt]Tj5em=]60w@\r\n";

void UnscrambleString(char *szString);

class C4ConfigShareware: public C4Config
	{
	public:
		C4ConfigShareware();
		~C4ConfigShareware();
	protected:
		bool RegistrationValid;
		char RegData[MaxRegDataLen + 1];
		char KeyFile[CFG_MaxString + 1];
		char InvalidKeyFile[CFG_MaxString + 1];
	public:
		void Default();
		bool Save();
		bool Load(bool forceWorkingDirectory=true, const char *szCustomFile=NULL);
	public:
		void ClearRegistrationError();
		bool Registered();
		bool LoadRegistration();
		bool LoadRegistration(const char *szFrom);
		const char* GetRegistrationData(const char* strField);
		const char* GetRegistrationError();
		const char* GetKeyFilename();
		const char* GetInvalidKeyFilename();
		const char* GetKeyPath();
		StdStrBuf GetKeyMD5();
		// checks for phising attacks: Return true if input contains user's webcode
		bool IsConfidentialData(const char *szInput);
	protected:
		StdStrBuf RegistrationError;
		bool HandleError(const char *strMessage);
	};

extern C4ConfigShareware Config;
#endif // C4CONFIGSHAREWARE_H_INC
