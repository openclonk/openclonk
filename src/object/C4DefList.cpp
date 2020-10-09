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

/* Object definition */

#include "C4Include.h"
#include "object/C4DefList.h"

#include "c4group/C4Components.h"
#include "control/C4Record.h"
#include "game/C4GameScript.h"
#include "game/C4GameVersion.h"
#include "lib/StdMeshLoader.h"
#include "object/C4Def.h"
#include "platform/C4FileMonitor.h"

namespace
{
	class C4SkeletonManager : public StdMeshSkeletonLoader
	{
		StdMeshSkeleton* GetSkeletonByDefinition(const char* definition) const override
		{
			// find the definition
			C4Def* def = ::Definitions.ID2Def(C4ID(definition));
			if (!def)
			{
				DebugLogF("WARNING: Looking up skeleton from definition '%s' failed, because there is no such definition with that ID", definition);
				return nullptr;
			}

			// append animations, if the definition has a mesh
			if (!def->Graphics.IsMesh())
			{
				DebugLogF("WARNING: Looking up skeleton from definition '%s' failed, because the definition has no mesh", definition);
				return nullptr;
			}
			else
			{
				StdMesh* mesh = def->Graphics.Mesh;

				return &(mesh->GetSkeleton());
			}
		}
	};
}

C4DefList::C4DefList() : SkeletonLoader(new C4SkeletonManager)
{
	Default();
}

C4DefList::~C4DefList()
{
	Clear();
}

int32_t C4DefList::Load(C4Group &hGroup, DWORD dwLoadWhat,
                        const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload,
                        bool fSearchMessage, int32_t iMinProgress, int32_t iMaxProgress, bool fLoadSysGroups)
{
	int32_t iResult=0;
	C4Def *nDef = nullptr;
	char szEntryname[_MAX_FNAME_LEN];
	C4Group hChild;
	bool fPrimaryDef=false;
	bool fThisSearchMessage=false;
	bool can_be_primary_def = SEqualNoCase(GetExtension(hGroup.GetName()), "ocd");

	// This search message
	if (fSearchMessage)
		if (can_be_primary_def
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"ocs")
		    || SEqualNoCase(GetExtension(hGroup.GetName()),"ocf"))
		{
			fThisSearchMessage=true;
			fSearchMessage=false;
		}

	if (fThisSearchMessage) { LogF("%s...",GetFilename(hGroup.GetName())); }

	// Load primary definition
	if (can_be_primary_def)
	{
		if ((nDef = new C4Def))
		{
			if (nDef->Load(hGroup, *SkeletonLoader, dwLoadWhat, szLanguage, pSoundSystem) && Add(nDef, fOverload))
			{
				iResult++; fPrimaryDef = true;
			}
			else
			{
				delete nDef;
				nDef = nullptr;
			}
		}
	}

	// Remember localized name for pure definition groups
	if (!nDef)
	{
		C4ComponentHost title_file;
		StdCopyStrBuf title;
		C4Language::LoadComponentHost(&title_file, hGroup, C4CFN_Title, Config.General.LanguageEx);
		if (title_file.GetLanguageString(Config.General.LanguageEx, title))
		{
			StdCopyStrBuf group_path(hGroup.GetFullName());
			group_path.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
			localized_group_folder_names[group_path] = title;
		}
	}

	// Load sub definitions
	int i = 0;
	hGroup.ResetSearch();
	while (hGroup.FindNextEntry(C4CFN_DefFiles,szEntryname))
		if (hChild.OpenAsChild(&hGroup,szEntryname))
		{
			// Hack: Assume that there are sixteen sub definitions to avoid unnecessary I/O
			int iSubMinProgress = std::min(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * i) / 16);
			int iSubMaxProgress = std::min(iMaxProgress, iMinProgress + ((iMaxProgress - iMinProgress) * (i + 1)) / 16);
			++i;
			iResult += Load(hChild,dwLoadWhat,szLanguage,pSoundSystem,fOverload,fSearchMessage,iSubMinProgress,iSubMaxProgress);
			hChild.Close();
		}

	// Load additional system scripts: Def groups (primary def), as well as real definitions
	if (fLoadSysGroups) Game.LoadAdditionalSystemGroup(hGroup);

	if (fThisSearchMessage) { LogF(LoadResStr("IDS_PRC_DEFSLOADED"),iResult); }

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return iResult;
}

int32_t C4DefList::Load(const char *szFilename,
                        DWORD dwLoadWhat, const char *szLanguage,
                        C4SoundSystem *pSoundSystem,
                        bool fOverload, int32_t iMinProgress, int32_t iMaxProgress)
{
	// Load from specified file
	C4Group hGroup;
	if (!Reloc.Open(hGroup, szFilename))
	{
		// Specified file not found (failure)
		LogFatal(FormatString(LoadResStr("IDS_PRC_DEFNOTFOUND"), szFilename).getData());
		LoadFailure=true;
		return 0; // 0 definitions loaded
	}
	int32_t nDefs = Load(hGroup,dwLoadWhat,szLanguage,pSoundSystem,fOverload,true,iMinProgress,iMaxProgress);
	hGroup.Close();

	// progress (could go down one level of recursion...)
	if (iMinProgress != iMaxProgress) Game.SetInitProgress(float(iMaxProgress));

	return nDefs;
}

bool C4DefList::Add(C4Def *pDef, bool fOverload)
{
	if (!pDef) return false;

	// Check old def to overload
	C4Def *pLastDef = ID2Def(pDef->id);
	if (pLastDef && !fOverload) return false;

	// Log overloaded def
	if (Config.Graphics.VerboseObjectLoading>=1)
		if (pLastDef)
		{
			LogF(LoadResStr("IDS_PRC_DEFOVERLOAD"),pDef->GetName(),pLastDef->id.ToString());
			if (Config.Graphics.VerboseObjectLoading >= 2)
			{
				LogF("      Old def at %s",pLastDef->Filename);
				LogF("     Overload by %s",pDef->Filename);
			}
		}

	// Remove old def
	Remove(pDef->id);

	// Add new def
	pDef->Next=FirstDef;
	FirstDef=pDef;

	return true;
}

bool C4DefList::Remove(C4ID id)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=nullptr; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef->id==id)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return true;
		}
	return false;
}

void C4DefList::Remove(C4Def *def)
{
	C4Def *cdef,*prev;
	for (cdef=FirstDef,prev=nullptr; cdef; prev=cdef,cdef=cdef->Next)
		if (cdef==def)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			return;
		}
}

void C4DefList::Clear()
{
	C4Def *cdef,*next;
	for (cdef=FirstDef; cdef; cdef=next)
	{
		next=cdef->Next;
		delete cdef;
	}
	FirstDef=nullptr;
	// clear quick access table
	table.clear();
	// clear localized group names table
	localized_group_folder_names.clear();
	// clear loaded skeletons
	SkeletonLoader->Clear();
}

C4Def* C4DefList::ID2Def(C4ID id)
{
	if (id==C4ID::None) return nullptr;
	if (table.empty())
	{
		// table not yet built: search list
		C4Def *cdef;
		for (cdef=FirstDef; cdef; cdef=cdef->Next)
			if (cdef->id==id) return cdef;
	}
	else
	{
		Table::const_iterator it = table.find(id);
		if (it != table.end())
			return it->second;
	}
	// none found
	return nullptr;
}

C4Def * C4DefList::GetByName(const StdStrBuf & name)
{
	return ID2Def(C4ID(name));
}

int32_t C4DefList::GetIndex(C4ID id)
{
	C4Def *cdef;
	int32_t cindex;
	for (cdef=FirstDef,cindex=0; cdef; cdef=cdef->Next,cindex++)
		if (cdef->id==id) return cindex;
	return -1;
}

int32_t C4DefList::GetDefCount()
{
	C4Def *cdef; int32_t ccount=0;
	for (cdef=FirstDef; cdef; cdef=cdef->Next)
		ccount++;
	return ccount;
}

C4Def* C4DefList::GetDef(int32_t iIndex)
{
	C4Def *pDef; int32_t iCurrentIndex;
	if (iIndex<0) return nullptr;
	for (pDef=FirstDef,iCurrentIndex=-1; pDef; pDef=pDef->Next)
	{
		iCurrentIndex++;
		if (iCurrentIndex==iIndex) return pDef;
	}
	return nullptr;
}

std::vector<C4Def*> C4DefList::GetAllDefs(C4String *filter_property) const
{
	// Collect vector of all definitions
	// Filter for those where property evaluates to true if filter_property!=nullptr
	std::vector<C4Def*> result;
	result.reserve(filter_property ? 32 : table.size());
	C4Value prop_val;
	for (C4Def *def = FirstDef; def; def = def->Next)
	{
		if (filter_property)
		{
			if (!def->GetPropertyByS(filter_property, &prop_val)) continue;
			if (!prop_val) continue;
		}
		result.push_back(def);
	}
	return result;
}

C4Def *C4DefList::GetByPath(const char *szPath)
{
	// search defs
	const char *szDefPath;
	for (C4Def *pDef = FirstDef; pDef; pDef = pDef->Next)
		if ((szDefPath = Config.AtRelativePath(pDef->Filename)))
			if (SEqual2NoCase(szPath, szDefPath))
			{
				// the definition itself?
				if (!szPath[SLen(szDefPath)])
					return pDef;
				// or a component?
				else if (szPath[SLen(szDefPath)] == '\\')
					if (!strchr(szPath + SLen(szDefPath) + 1, '\\'))
						return pDef;
			}
	// not found
	return nullptr;
}

int32_t C4DefList::RemoveTemporary()
{
	C4Def *cdef,*prev,*next;
	int32_t removed=0;
	for (cdef=FirstDef,prev=nullptr; cdef; cdef=next)
	{
		next=cdef->Next;
		if (cdef->Temporary)
		{
			if (prev) prev->Next=next;
			else FirstDef=next;
			delete cdef;
			removed++;
		}
		else
			prev=cdef;
	}
	// rebuild quick access table
	BuildTable();
	return removed;
}

int32_t C4DefList::CheckEngineVersion(int32_t ver1, int32_t ver2)
{
	int32_t rcount=0;
	C4Def *cdef,*prev,*next;
	for (cdef=FirstDef,prev=nullptr; cdef; cdef=next)
	{
		next=cdef->Next;
		if (CompareVersion(cdef->rC4XVer[0],cdef->rC4XVer[1],ver1,ver2) > 0)
		{
			if (prev) prev->Next=cdef->Next;
			else FirstDef=cdef->Next;
			delete cdef;
			rcount++;
		}
		else prev=cdef;
	}
	return rcount;
}

int32_t C4DefList::CheckRequireDef()
{
	int32_t rcount=0, rcount2;
	C4Def *cdef,*prev,*next;
	do
	{
		rcount2 = rcount;
		for (cdef=FirstDef,prev=nullptr; cdef; cdef=next)
		{
			next=cdef->Next;
			for (int32_t i = 0; i < cdef->RequireDef.GetNumberOfIDs(); i++)
				if (GetIndex(cdef->RequireDef.GetID(i)) < 0)
				{
					(prev ? prev->Next : FirstDef) = cdef->Next;
					delete cdef;
					rcount++;
				}
		}
	}
	while (rcount != rcount2);
	return rcount;
}

void C4DefList::Draw(C4ID id, C4Facet &cgo, bool fSelected, int32_t iColor)
{
	C4Def *cdef = ID2Def(id);
	if (cdef) cdef->Draw(cgo,fSelected,iColor);
}

void C4DefList::Default()
{
	FirstDef=nullptr;
	LoadFailure=false;
	table.clear();
}

bool C4DefList::Reload(C4Def *pDef, DWORD dwLoadWhat, const char *szLanguage, C4SoundSystem *pSoundSystem)
{
	// Safety
	if (!pDef) return false;
	// backup graphics names and pointers
	// GfxBackup-dtor will ensure that upon loading-failure all graphics are reset to default
	C4DefGraphicsPtrBackup GfxBackup;
	GfxBackup.Add(&pDef->Graphics);
	// Clear def
	pDef->Clear(); // Assume filename is being kept
	// Reload def
	C4Group hGroup;
	if (!hGroup.Open(pDef->Filename)) return false;
	// clear all skeletons in that group, so that deleted skeletons are also deleted in the engine
	SkeletonLoader->RemoveSkeletonsInGroup(hGroup.GetName());
	// load the definition
	if (!pDef->Load(hGroup, *SkeletonLoader, dwLoadWhat, szLanguage, pSoundSystem, &GfxBackup)) return false;
	hGroup.Close();
	// rebuild quick access table
	BuildTable();
	// handle skeleton appends and includes
	AppendAndIncludeSkeletons();
	// restore graphics
	GfxBackup.AssignUpdate();
	// Success
	return true;
}

bool C4DefList::DrawFontImage(const char* szImageTag, C4Facet& cgo, C4DrawTransform* pTransform)
{
	return Game.DrawTextSpecImage(cgo, szImageTag, pTransform);
}

float C4DefList::GetFontImageAspect(const char* szImageTag)
{
	return Game.GetTextSpecImageAspect(szImageTag);
}

void C4DefList::Synchronize()
{
	for (auto & it : table)
		it.second->Synchronize();
}

void C4DefList::ResetIncludeDependencies()
{
	for (auto & it : table)
		it.second->ResetIncludeDependencies();
}

void C4DefList::SortByPriority()
{
	// Sort all definitions by DefinitionPriority property (descending)
	// Build vector of definitions
	int32_t n = GetDefCount();
	if (!n) return;
	std::vector<C4Def *> def_vec;
	def_vec.reserve(n);
	for (C4Def *def = FirstDef; def; def = def->Next)
		def_vec.push_back(def);
	// Sort it
	std::stable_sort(def_vec.begin(), def_vec.end(), [](C4Def *a, C4Def *b) {
		return b->GetPropertyInt(P_DefinitionPriority) < a->GetPropertyInt(P_DefinitionPriority);
	});
	// Restore linked list in new definition order
	C4Def *prev_def = nullptr;
	for (C4Def *def : def_vec)
	{
		if (prev_def)
			prev_def->Next = def;
		else
			FirstDef = def;
		prev_def = def;
	}
	if (prev_def) prev_def->Next = nullptr;
}

void C4DefList::CallEveryDefinition()
{
	for (C4Def *def = FirstDef; def; def = def->Next)
	{
		if (Config.General.DebugRec)
		{
			// TODO: Might not be synchronous on runtime join since is run by joining
			// client but not by host. Might need to go to Synchronize().
			char sz[32+1];
			strncpy(sz, def->id.ToString(), 32+1);
			AddDbgRec(RCT_Definition, sz, 32);
		}
		C4AulParSet Pars(def);
		def->Call(PSF_Definition, &Pars);
	}
}

void C4DefList::BuildTable()
{
	table.clear();
	for (C4Def *def = FirstDef; def; def = def->Next)
		table.insert(std::make_pair(def->id, def));
}

void C4DefList::AppendAndIncludeSkeletons()
{
	SkeletonLoader->ResolveIncompleteSkeletons();
}

StdMeshSkeletonLoader& C4DefList::GetSkeletonLoader()
{
	return *SkeletonLoader;
}

const char *C4DefList::GetLocalizedGroupFolderName(const char *folder_path) const
{
	// lookup in map
	auto iter = localized_group_folder_names.find(StdCopyStrBuf(folder_path));
	if (iter == localized_group_folder_names.end()) return nullptr;
	return iter->second.getData();
}
