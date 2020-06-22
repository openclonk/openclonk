/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// a set of group files
// manages system file overwriting by scearios or folders

#ifndef INC_C4GroupSet
#define INC_C4GroupSet

// group set priorities
#define C4GSPrio_Base         0 // lowest priority for global system files
#define C4GSPrio_Pack         1 // overloads by object packs
#define C4GSPrio_ExtraRoot    2 // overloads by Extra.ocg root folder
#define C4GSPrio_Extra        3 // overloads by Extra.ocg
#define C4GSPrio_Definition   4 // overloads by definition file - latter defined definition files have higher priority
#define C4GSPrio_Definition2 99 // highest priority a given definition may have
#define C4GSPrio_Folder     100 // overloads by local scenario folder - each child folder has higher priority
#define C4GSPrio_Folder2    199 // highest priority a folder may have
#define C4GSPrio_Scenario   200 // overloads by scenario: highest priority

// group node contents
#define C4GSCnt_Graphics    1 // contains Graphics.ocg
#define C4GSCnt_Loaders     2 // contains loader files
#define C4GSCnt_Material    4 // contains Material.ocg
#define C4GSCnt_Music       8 // contains music
#define C4GSCnt_Definitions 16 // contains definition files
#define C4GSCnt_FontDefs    32 // contains font definitions
#define C4GSCnt_Language    64 // contains language files
#define C4GSCnt_Component   128 // other components

#define C4GSCnt_Folder    (C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)
#define C4GSCnt_OriginFolder    (C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)
#define C4GSCnt_Directory (C4GSCnt_Loaders | C4GSCnt_Music)
#define C4GSCnt_Scenario  C4GSCnt_Folder
#define C4GSCnt_Root      (C4GSCnt_Graphics | C4GSCnt_Material)
#define C4GSCnt_Extra     (C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)
#define C4GSCnt_ExtraRoot (C4GSCnt_Graphics | C4GSCnt_Loaders | C4GSCnt_Material | C4GSCnt_Music | C4GSCnt_FontDefs)

#define C4GSCnt_All        ~0

// one node in the group set holds one group
class C4GroupSetNode
{
protected:
	class C4GroupSet *pParent;  // owning set
	C4GroupSetNode *pPrev, *pNext; // linked list - always valid

	C4Group *pGroup;      // ptr to group owned by this node
	bool fGrpOwned;       // flag if group ptr is owned

	int32_t id;               // group node ID

public:
	C4GroupSetNode(class C4GroupSet &rParent, C4GroupSetNode *pPrev, C4Group &rGroup, bool fGrpOwned, int32_t id);  // ctor
	~C4GroupSetNode();                                          // dtor

	int32_t Priority;         // group priority
	int32_t Contents;         // content held by this group

	friend class C4GroupSet;
};

// a group set manages file overloading within several groups
class C4GroupSet
{
protected:
	C4GroupSetNode *pFirst, *pLast; // linked list
	int32_t iIndex; // index to keep track of group node IDs

public:
	bool UnregisterGroup(int32_t iIndex);
	void Clear();
	void Default();

	C4GroupSet();   // ctor
	C4GroupSet(const C4GroupSet &rCopy); // copy-constructor that registers all groups with all contents	
	~C4GroupSet();  // dtor

	bool RegisterGroup(C4Group &rGroup, bool fOwnGrp, int32_t Priority, int32_t Contents, bool fCheckContent=true); // add group to list
	bool RegisterGroups(const C4GroupSet &rCopy, int32_t Contents, const char *szFilename=nullptr, int32_t iMaxSkipID=0);  // add all matching (child-)groups of the set
	C4Group *FindGroup(int32_t Contents, C4Group *pAfter=nullptr, bool fSamePrio=false);       // search for suitable group in list
	C4Group *FindEntry(const char *szWildcard, int32_t *pPriority=nullptr, int32_t *pID=nullptr);                   // find entry in groups; store priority of group if ptr is given
	C4Group *FindSuitableFile(const char *szName, const char * const extensions[], char *szFileName, int32_t *pID=nullptr);
	int32_t GetGroupCount();
	C4Group *GetGroup(int32_t iIndex);
	bool LoadEntry(const char *szEntryName, char **lpbpBuf, size_t *ipSize=nullptr, int32_t iAppendZeros=0);
	bool LoadEntryString(const char *szEntryName, StdStrBuf * rBuf);
	bool LoadEntryString(const StdStrBuf & name, StdStrBuf * Buf) { return LoadEntryString(name.getData(), Buf); }
	C4Group *RegisterParentFolders(const char *szScenFilename); // register all parent .ocf groups to the given scenario filename and return an open group file of the innermost parent ocf
	int32_t PreCacheEntries(const char *szEntryMask); // pre-cache all entries matching mask for packed files in all groups.

	static int32_t CheckGroupContents(C4Group &rGroup, int32_t Contents);
	int32_t GetLastID() { return iIndex; } // return ID assigned to the last added group

	bool CloseFolders();      // remove all groups associated with scenario folders

	friend class C4GroupSetNode;
};

#endif
