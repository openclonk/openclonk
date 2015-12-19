/*
 * mape - C4 Landscape.txt editor
 *
 * Copyright (c) 2005-2009, Armin Burgmeier
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

#include "C4Include.h"

#include "C4Aul.h"
#include "C4AulDebug.h"
#include "C4GameControl.h"
#include "C4Def.h"
#include "C4DefList.h"
#include "C4Facet.h"
#include "C4GameObjects.h"
#include "C4GameParameters.h"
#include "C4GraphicsResource.h"
#include "C4Landscape.h"
#include "C4PXS.h"
#include "C4Record.h"
#include "C4RoundResults.h"
#include "C4TextureShape.h"

/* This file implements stubs for the parts of the engine that are not used
 * by mape. */

C4Landscape Landscape;
C4PXSSystem PXS;

class C4SoundInstance *StartSoundEffectAt(const char *, int32_t, int32_t, int32_t, int32_t, int32_t, class C4SoundModifier *) { return NULL; }

C4Facet::C4Facet() {}
void C4Facet::Set(C4Surface*, float, float, float, float) {}

C4Surface::C4Surface() {}
C4Surface::~C4Surface() {}
bool C4Surface::Read(CStdStream &, const char *, int) { return false; }
bool C4Surface::Lock() { return false; }
bool C4Surface::Unlock() { return false; }
DWORD C4Surface::GetPixDw(int iX, int iY, bool fApplyModulation) { return 0; }

C4TexRef::~C4TexRef() {}

C4Pattern::C4Pattern() {}
void C4Pattern::Clear() {}
bool C4Pattern::Set(C4Surface *, int) { return false; }
DWORD C4Pattern::PatternClr(unsigned int, unsigned int) const { return 0; }
C4Pattern& C4Pattern::operator=(C4Pattern const&) { return *this; }

C4IDList::C4IDList() {}
C4IDList::~C4IDList() {}
void C4IDList::Default() {}
void C4IDList::Clear() {}
C4IDList& C4IDList::operator=(C4IDList const&) { return *this; }
bool C4IDList::operator==(const C4IDList&) const { return false; }
int32_t C4IDList::GetIDCount(C4ID, int32_t) const { return 0; }
bool C4IDList::SetIDCount(C4ID, int32_t, bool) { return false; }
void C4IDList::CompileFunc(StdCompiler *, bool) {}
C4IDListChunk::C4IDListChunk() {}
C4IDListChunk::~C4IDListChunk() {}

C4DefGraphics::C4DefGraphics(C4Def*) {}
void C4DefGraphics::Clear() {}

void C4DefList::CallEveryDefinition() {}
void C4DefList::ResetIncludeDependencies() {}
bool C4DefList::DrawFontImage(const char* szImageTag, C4Facet& rTarget, C4DrawTransform* pTransform) { return false; }
float C4DefList::GetFontImageAspect(const char* szImageTag) { return -1.0f; }

C4Landscape::C4Landscape() {}
C4Landscape::~C4Landscape() {}
bool C4Landscape::FindMatSlide(int&, int&, int, int, int) const { return false; }
int32_t C4Landscape::ExtractMaterial(int32_t, int32_t, bool) { return 0; }
bool C4Landscape::InsertMaterial(int32_t, int32_t *, int32_t *, int32_t, int32_t, bool) { return false; }
bool C4Landscape::Incinerate(int32_t, int32_t, int32_t) { return false; }
bool C4Landscape::ClearPix(int32_t, int32_t) { return false; }
void C4Landscape::CheckInstabilityRange(int32_t, int32_t) {}

void C4Sky::Default() {}
C4Sky::~C4Sky() {}

C4PXSSystem::C4PXSSystem() {}
C4PXSSystem::~C4PXSSystem() {}
bool C4PXSSystem::Create(int, C4Real, C4Real, C4Real, C4Real) { return false; }

bool C4TextureShape::Load(C4Group &group, const char *filename, int32_t base_tex_wdt, int32_t base_tex_hgt) { return true; }

C4Shader::C4Shader() {}
C4Shader::~C4Shader() {}
