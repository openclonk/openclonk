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
/* Update package support */

#ifndef INC_C4GroupEx
#define INC_C4GroupEx

#include "lib/C4InputValidation.h"

const int C4UP_MaxUpGrpCnt = 50;

class C4UpdatePackageCore
{
public:
	int32_t RequireVersion[4] = { 0, 0, 0, 0 };
	char Name[C4MaxName + 1] = { 0 };
	char DestPath[_MAX_PATH_LEN] = { 0 };
	int32_t GrpUpdate = 0;
	int32_t UpGrpCnt = 0; // number of file versions that can be updated by this package
	uint32_t GrpChks1[C4UP_MaxUpGrpCnt] = { 0 }, GrpChks2 = 0;
public:
	void CompileFunc(StdCompiler *pComp);
	bool Load(C4Group &hGroup);
	bool Save(C4Group &hGroup);
};

#define C4UPD_CHK_OK                0
#define C4UPD_CHK_NO_SOURCE         1
#define C4UPD_CHK_BAD_SOURCE        2
#define C4UPD_CHK_ALREADY_UPDATED   3
#define C4UPD_CHK_BAD_VERSION       4

class C4UpdatePackage : public C4UpdatePackageCore
{

public:
	bool Load(C4Group *pGroup);
	bool Execute(C4Group *pGroup);
	static bool Optimize(C4Group *pGrpFrom, const char *strTarget);
	int  Check(C4Group *pGroup);
	bool MakeUpdate(const char *strFile1, const char *strFile2, const char *strUpdateFile, const char *strName = nullptr);

protected:
	bool DoUpdate(C4Group *pGrpFrom, class C4GroupEx *pGrpTo, const char *strFileName);
	bool DoGrpUpdate(C4Group *pUpdateData, class C4GroupEx *pGrpTo);
	static bool Optimize(C4Group *pGrpFrom, class C4GroupEx *pGrpTo, const char *strFileName);

	bool MkUp(C4Group *pGrp1, C4Group *pGrp2, C4GroupEx *pUpGr, bool *fModified);

	bool OpenUnpackParents(C4GroupEx &rGroup, const char *strGroup, const char *strEnsureMaker);

	CStdFile Log;
	void WriteLog(const char *strMsg, ...) GNUC_FORMAT_ATTRIBUTE_O;
};

bool C4Group_ApplyUpdate(C4Group &hGroup, unsigned long ParentProcessID);

#endif
