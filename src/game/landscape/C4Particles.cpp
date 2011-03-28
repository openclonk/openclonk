/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004-2005  Sven Eberhardt
 * Copyright (c) 2005, 2009-2010  Tobias Zwick
 * Copyright (c) 2005-2006, 2008, 2010  Günther Brammer
 * Copyright (c) 2008  Peter Wortmann
 * Copyright (c) 2010  Benjamin Herr
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
// newgfx particle system for smoke, sparks, ...

#include <C4Include.h>
#include <C4Particles.h>

#include <C4Config.h>
#include <C4Physics.h>
#include <C4Object.h>
#include <C4Random.h>
#include <C4Game.h>
#include <C4Components.h>
#include <C4Log.h>
#include <C4Weather.h>
#include <C4GameObjects.h>

void C4ParticleDefCore::CompileFunc(StdCompiler * pComp)
{
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),       "Name",       ""));
	pComp->Value(mkNamingAdapt(MaxCount,                "MaxCount",    C4Px_MaxParticle));
	pComp->Value(mkNamingAdapt(MinLifetime,             "MinLifetime", 0));
	pComp->Value(mkNamingAdapt(MaxLifetime,             "MaxLifetime", 0));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(InitFn),     "InitFn",     ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(ExecFn),     "ExecFn",     ""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(CollisionFn),"CollisionFn",""));
	pComp->Value(mkNamingAdapt(toC4CStrBuf(DrawFn),     "DrawFn",     ""));
	pComp->Value(mkNamingAdapt(GfxFace,                 "Face"));
	pComp->Value(mkNamingAdapt(YOff,                    "YOff",        0));
	pComp->Value(mkNamingAdapt(Delay,                   "Delay",       0));
	pComp->Value(mkNamingAdapt(Repeats,                 "Repeats",     0));
	pComp->Value(mkNamingAdapt(Reverse,                 "Reverse",     0));
	pComp->Value(mkNamingAdapt(FadeOutLen,              "FadeOutLen",  0));
	pComp->Value(mkNamingAdapt(FadeOutDelay,            "FadeOutDelay",0));
	pComp->Value(mkNamingAdapt(RByV,                    "RByV",        0));
	pComp->Value(mkNamingAdapt(GravityAcc,              "GravityAcc",  0));
	pComp->Value(mkNamingAdapt(WindDrift,               "WindDrift",   0));
	pComp->Value(mkNamingAdapt(VertexCount,             "VertexCount", 0));
	pComp->Value(mkNamingAdapt(VertexY,                 "VertexY",     0));
	pComp->Value(mkNamingAdapt(Additive,                "Additive",    0));
	pComp->Value(mkNamingAdapt(AlphaFade,               "AlphaFade",   0));
	pComp->Value(mkNamingAdapt(FadeDelay,               "FadeDelay",   0));
	pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Parallaxity,100),"Parallaxity"));
	pComp->Value(mkNamingAdapt(Attach,                  "Attach",      0));
}

C4ParticleDefCore::C4ParticleDefCore():
		MaxCount(C4Px_MaxParticle),
		MinLifetime(0),MaxLifetime(0),
		YOff(0),
		Delay(0),Repeats(0),Reverse(0),
		FadeOutLen(0),FadeOutDelay(0),
		RByV(0),
		Placement(0),
		GravityAcc(0),
		VertexCount(0),VertexY(0),
		Additive(0),
		Attach(0),
		AlphaFade(0),
		FadeDelay(0)
{
	GfxFace.Default();
	Parallaxity[0] = Parallaxity[1] = 100;
}

bool C4ParticleDefCore::Compile(char *particle_source, const char *name)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "Particle"),
	       StdStrBuf(particle_source), name);
}

C4ParticleDef::C4ParticleDef():
		C4ParticleDefCore(),
		InitProc(&fxStdInit),
		ExecProc(&fxStdExec),
		CollisionProc(NULL),
		DrawProc(&fxStdDraw),
		Count(0)
{
	// zero fields
	Gfx.Default();
	// link into list
	if (!ParticleSystem.pDef0)
	{
		pPrev = NULL;
		ParticleSystem.pDef0 = this;
	}
	else
	{
		pPrev = ParticleSystem.pDefL;
		pPrev->pNext = this;
	}
	ParticleSystem.pDefL = this;
	pNext = NULL;
}

C4ParticleDef::~C4ParticleDef()
{
	// clear
	Clear();
	// unlink from list
	if (pPrev) pPrev->pNext = pNext; else ParticleSystem.pDef0 = pNext;
	if (pNext) pNext->pPrev = pPrev; else ParticleSystem.pDefL = pPrev;
}

void C4ParticleDef::Clear()
{
	Name.Clear();
}

bool C4ParticleDef::Load(C4Group &group)
{
	// store file
	Filename.Copy(group.GetFullName());
	// load
	char *particle_source;
	if (group.LoadEntry(C4CFN_ParticleCore,&particle_source,NULL,1))
	{
		if (!Compile(particle_source, Filename.getData()))
		{
			DebugLogF("invalid particle def at '%s'", group.GetFullName().getData());
			delete [] particle_source; return false;
		}
		delete [] particle_source;
		// load graphics
		if (!Gfx.Load(group, C4CFN_DefGraphicsPNG))
		{
			DebugLogF("particle %s has no valid graphics defined", Name.getData());
			return false;
		}
		// set facet, if assigned - otherwise, assume full surface
		if (GfxFace.Wdt) Gfx.Set(Gfx.Surface, GfxFace.x, GfxFace.y, GfxFace.Wdt, GfxFace.Hgt);
		// set phase num
		int32_t Q; Gfx.GetPhaseNum(PhasesX, Q);
		Length = PhasesX * Q;
		if (!Length)
		{
			DebugLogF("invalid facet for particle '%s'", Name.getData());
			return false;
		}
		// case fadeout from length
		if (FadeOutLen)
		{
			Length = Max<int32_t>(Length - FadeOutLen, 1);
			if (!FadeOutDelay) FadeOutDelay=1;
		}
		// if phase num is 1, no reverse is allowed
		if (Length == 1) Reverse = 0;
		// calc aspect
		Aspect=(float) Gfx.Hgt/Gfx.Wdt;
		// get proc pointers
		if (!(InitProc = ParticleSystem.GetProc(InitFn.getData())))
		{
			DebugLogF("init proc for particle '%s' not found: '%s'", Name.getData(), InitFn.getData());
			return false;
		}
		if (!(ExecProc = ParticleSystem.GetProc(ExecFn.getData())))
		{
			DebugLogF("exec proc for particle '%s' not found: '%s'", Name.getData(), ExecFn.getData());
			return false;
		}
		if (CollisionFn && CollisionFn[0]) if (!(CollisionProc = ParticleSystem.GetProc(CollisionFn.getData())))
			{
				DebugLogF("collision proc for particle '%s' not found: '%s'", Name.getData(), CollisionFn.getData());
				return false;
			}
		if (!(DrawProc = ParticleSystem.GetDrawProc(DrawFn.getData())))
		{
			DebugLogF("draw proc for particle '%s' not found: '%s'", Name.getData(), DrawFn.getData());
			return false;
		}
		// particle overloading
		C4ParticleDef *def_overload;
		if ((def_overload = ParticleSystem.GetDef(Name.getData(), this)))
		{
			if (Config.Graphics.VerboseObjectLoading >= 1)
				{ char ostr[250]; sprintf(ostr,LoadResStr("IDS_PRC_DEFOVERLOAD"),def_overload->Name.getData(),"<particle>"); Log(ostr); }
			delete def_overload;
		}
		// success
		return true;
	}
	return false;
}

bool C4ParticleDef::Reload()
{
	// no file?
	if (!Filename[0]) return false;
	// open group
	C4Group group;
	if (!group.Open(Filename.getData())) return false;
	// reset class
	Clear();
	// load
	return Load(group);
}

void C4Particle::MoveList(C4ParticleList &from, C4ParticleList &to)
{
	// remove from current list
	if (pPrev)
		pPrev->pNext = pNext;
	else
		// is it the first item in the list? then set this to the next one
		if (from.pFirst == this) from.pFirst = pNext;
	if (pNext) pNext->pPrev = pPrev;
	// add to the other list - insert before first
	if ((pNext = to.pFirst)) pNext->pPrev = this;
	to.pFirst = this; pPrev = NULL;
}

C4ParticleChunk::C4ParticleChunk()
{
	// zero linked list
	pNext=NULL;
	// zero buffer
	Clear();
}

C4ParticleChunk::~C4ParticleChunk()
{
	// list stuff done by C4ParticleSystem
}

void C4ParticleChunk::Clear()
{
	// note that this method is called in ctor with uninitialized data!
	// simply clear mem - this won't adjust any counts!
	memset(Data, 0, sizeof(Data));
	// init list
	C4Particle *particle=Data;
	for (int32_t i=0; i < C4Px_BufSize; ++i)
	{
		particle->pPrev = particle-1;
		particle->pNext = particle+1;
		++particle;
	}
	Data[0].pPrev=Data[C4Px_BufSize-1].pNext=NULL;
	// all free
	NumFree = C4Px_BufSize;
}

void C4ParticleList::Exec(C4Object *object)
{
	// execute all particles
	C4Particle *next_particle = pFirst, *particle;
	while ((particle = next_particle))
	{
		// get next now, because destruction could corrupt the list
		next_particle = particle->pNext;
		// execute it
		if (!particle->pDef->ExecProc(particle,object))
		{
			// sorry, life is over for you :P
			--particle->pDef->Count;
			particle->MoveList(*this, ::Particles.FreeParticles);
		}
	}
	// done
}

void C4ParticleList::Draw(C4TargetFacet &cgo, C4Object *object)
{
	// draw all particles
	for (C4Particle *particle = pFirst; particle; particle = particle->pNext)
		particle->pDef->DrawProc(particle, cgo, object);
	// done
}

void C4ParticleList::Clear()
{
	// remove all particles
	C4Particle *next_particle = pFirst, *particle;
	while ((particle = next_particle))
	{
		// get next now, because destruction could corrupt the list
		next_particle = particle->pNext;
		// sorry, life is over for you :P
		--particle->pDef->Count;
		particle->MoveList(*this, ::Particles.FreeParticles);
	}
}

int32_t C4ParticleList::Remove(C4ParticleDef *of_def)
{
	int32_t num_removed = 0;
	// check all particles for def
	C4Particle *next_particle = pFirst, *particle;
	while ((particle = next_particle))
	{
		// get next now, because destruction could corrupt the list
		next_particle = particle->pNext;
		// execute it
		if (!of_def || particle->pDef == of_def)
		{
			// sorry, life is over for you :P
			--particle->pDef->Count;
			particle->MoveList(*this, ::Particles.FreeParticles);
		}
	}
	// done
	return num_removed;
}

C4ParticleSystem::C4ParticleSystem()
{
	// zero fields
	pDef0=pDefL=NULL;
	pSmoke=NULL;
	pBlast=NULL;
	pFSpark=NULL;
	pFire1=NULL;
	pFire2=NULL;
}

C4ParticleSystem::~C4ParticleSystem()
{
	// clean up
	Clear();
}

C4ParticleChunk *C4ParticleSystem::AddChunk()
{
	// add another chunk
	C4ParticleChunk *new_chunk = new C4ParticleChunk();
	new_chunk->pNext = Chunk.pNext;
	Chunk.pNext = new_chunk;
	// register into free-particle-list
	if ((new_chunk->Data[C4Px_BufSize-1].pNext = FreeParticles.pFirst))
		FreeParticles.pFirst->pPrev = &new_chunk->Data[C4Px_BufSize-1];
	FreeParticles.pFirst = &new_chunk->Data[0];
	// return it
	return new_chunk;
}

void C4ParticleSystem::PruneChunks()
{
	// check all chunks, but not the first
	// that cannot be removed anyway
	C4ParticleChunk *chunk = Chunk.pNext, *next_chunk, **previous_chunk_p;
	previous_chunk_p = &Chunk.pNext;
	do
	{
		next_chunk = chunk->pNext;
		// chunk empty?
		if (chunk->NumFree == C4Px_BufSize)
		{
			// move out all particles
			C4ParticleList tmp;
			for (int32_t i = 0; i < C4Px_BufSize; ++i)
				chunk->Data[i].MoveList(FreeParticles, tmp);
			// and remove the chunk
			*previous_chunk_p = next_chunk;
			delete chunk;
		}
		else
		{
			// keep this chunk
			previous_chunk_p = &chunk->pNext;
		}
	}
	while ((chunk = next_chunk));
}

void C4ParticleSystem::ClearParticles()
{
	// clear particle lists
	C4ObjectLink *link;
	for (link = ::Objects.First; link; link = link->Next)
		link->Obj->FrontParticles.pFirst = link->Obj->BackParticles.pFirst = NULL;
	for (link = ::Objects.InactiveObjects.First; link; link = link->Next)
		link->Obj->FrontParticles.pFirst = link->Obj->BackParticles.pFirst = NULL;
	GlobalParticles.pFirst = NULL;
	// reset chunks
	C4ParticleChunk *next_chunk = Chunk.pNext, *chunk;
	while ((chunk = next_chunk))
	{
		next_chunk = chunk->pNext;
		delete chunk;
	}
	Chunk.pNext = NULL;
	Chunk.Clear();
	FreeParticles.pFirst = Chunk.Data;
	// adjust counts
	for (C4ParticleDef *def=pDef0; def; def=def->pNext)
		def->Count=0;
}

void C4ParticleSystem::Clear()
{
	// clear particles first
	ClearParticles();
	// clear defs
	while (pDef0) delete pDef0;
	// clear system particles
	pSmoke = pBlast = pFSpark = pFire1 = pFire2 = NULL;
	// done
}

C4Particle *C4ParticleSystem::Create(C4ParticleDef *of_def,
                                     float x, float y,
                                     float xdir, float ydir,
                                     float a, int32_t b, C4ParticleList *pxList,
                                     C4Object *object)
{
	// safety
	if (!of_def) return NULL;
	// default to global list
	if (!pxList) pxList = &GlobalParticles;
	// check count
	int32_t max_count = of_def->MaxCount * (Config.Graphics.SmokeLevel + 20) / 150;
	int32_t room = max_count - of_def->Count;
	if (room <= 0) return NULL;
	// reduce creation if limit is nearly reached
	if (room < (max_count >> 1))
		if (SafeRandom(room) < SafeRandom(max_count)) return NULL;
	// get free particle
	if (!FreeParticles.pFirst) AddChunk();
	C4Particle *particle = FreeParticles.pFirst;
	if (!particle) return NULL;
	// set values
	particle->x = x; particle->y = y;
	particle->xdir = xdir; particle->ydir = ydir;
	particle->a = a; particle->b = b;
	particle->pDef = of_def;
	if (particle->pDef->Attach && object != NULL)
	{
		particle->x -= fixtof(object->GetFixedX());
		particle->y -= fixtof(object->GetFixedY());
	}
	// call initialization
	if (!of_def->InitProc(particle,object))
		// failed :(
		return NULL;
	// count particle
	++of_def->Count;
	// more to desired list
	particle->MoveList(::Particles.FreeParticles, *pxList);
	// return newly created particle
	return particle;
}

bool C4ParticleSystem::Cast(C4ParticleDef *of_def, int32_t amount,
                            float x, float y, int32_t level,
                            float a0, DWORD b0, float a1, DWORD b1, C4ParticleList *pxList, C4Object *object)
{
	// safety
	if (!of_def) return false;
	// get range for a and b
	int32_t iA0=(int32_t)(a0*100),iA1=(int32_t)(a1*100);
	if (iA1<iA0) Swap(iA0, iA1);
	int32_t iAd=iA1-iA0+1;
	if (b1<b0) { DWORD dwX=b0; b0=b1; b1=dwX; }
	DWORD db=b1-b0;
	BYTE db1=BYTE(db>>24), db2=BYTE(db>>16), db3=BYTE(db>>8), db4=BYTE(db);
	// create them
	for (int32_t i=amount; i > 0; --i)
		Create(of_def, x, y,
		       (float)(SafeRandom(level+1)-level/2)/10.0f,
		       (float)(SafeRandom(level+1)-level/2)/10.0f,
		       (float)(iA0+SafeRandom(iAd))/100.0f,
		       b0+(SafeRandom(db1)<<24)+(SafeRandom(db2)<<16)+(SafeRandom(db3)<<8)+SafeRandom(db4), pxList, object);
	// success
	return true;
}

C4ParticleProc C4ParticleSystem::GetProc(const char *name)
{
	// seek in map
	for (int32_t i = 0; C4ParticleProcMap[i].Name[0]; ++i)
		if (SEqual(C4ParticleProcMap[i].Name, name))
			return C4ParticleProcMap[i].Proc;
	// nothing found...
	return NULL;
}

C4ParticleDrawProc C4ParticleSystem::GetDrawProc(const char *name)
{
	// seek in map
	for (int32_t i = 0; C4ParticleDrawProcMap[i].Name[0]; ++i)
		if (SEqual(C4ParticleDrawProcMap[i].Name, name))
			return C4ParticleDrawProcMap[i].Proc;
	// nothing found...
	return NULL;
}

C4ParticleDef *C4ParticleSystem::GetDef(const char *name, C4ParticleDef *exclude)
{
	// seek list
	for (C4ParticleDef *def = pDef0; def; def=def->pNext)
		if (def != exclude && def->Name == name)
			return def;
	// nothing found
	return NULL;
}

void C4ParticleSystem::SetDefParticles()
{
	// get smoke
	pSmoke = GetDef("Smoke");
	// get blast
	pBlast = GetDef("Blast");
	pFSpark = GetDef("FSpark");
	// get fire, if fire particles are desired
	if (Config.Graphics.FireParticles)
	{
		pFire1 = GetDef("Fire");
		pFire2 = GetDef("Fire2");
	}
	else
		pFire1 = pFire2 = NULL;
	// if fire is drawn w/o background fct: unload fire face if both fire particles are assigned
	// but this is not done here
	//if (IsFireParticleLoaded())
	//  ::GraphicsResource.fctFire.Clear();
}

int32_t C4ParticleSystem::Push(C4ParticleDef *of_def, float dxdir, float dydir)
{
	int32_t num_pushed = 0;
	// go through all particle chunks
	for (C4ParticleChunk *a_chunk = &Chunk; a_chunk; a_chunk = a_chunk->pNext)
	{
		// go through all particles
		C4Particle *particle = a_chunk->Data; int32_t i=C4Px_BufSize;
		while (i--)
		{
			// def fits?
			if (!of_def || particle->pDef == of_def)
			{
				// push it!
				particle->xdir += dxdir;
				particle->ydir += dydir;
				// count pushed
				++num_pushed;
			}
			// next particle
			++particle;
		}
	}
	// done
	return num_pushed;
}

bool fxSmokeInit(C4Particle *particle, C4Object *target)
{
	// init lifetime
	particle->life = particle->pDef->MinLifetime;
	int32_t lifetime = particle->pDef->MaxLifetime - particle->pDef->MinLifetime;
	if (lifetime)
		particle->life += SafeRandom(lifetime);
	// use high-word of life to store init-status
	particle->life |= (particle->life/17)<<16;
	// set kind - ydir is unused anyway; set last kind reeeaaally seldom
	particle->ydir = (float) SafeRandom(15) + SafeRandom(300)/299;
	// set color
	if (!particle->b)
		particle->b = 0x004b4b4b;
	else
		particle->b &= ~0xff000000;
	// always OK
	return true;
}

bool fxSmokeExec(C4Particle *particle, C4Object *target)
{
	// lifetime
	if (!--particle->life) return false;
	bool is_building = !!(particle->life&0x7fff0000);
	// still building?
	if (is_building)
	{
		// decrease init-time
		particle->life -= 0x010000;
		// increase color value
		particle->b += 0x10000000;
		// if full-grown, adjust to lifetime
		if (!(particle->life&0x7fff0000))
			particle->b = (particle->b&0xffffff)|((particle->life)<<24);
	}
	// color change
	DWORD color = particle->b;
	particle->b = (LightenClrBy(color, 1)&0xffffff) | Min<int32_t>((color>>24)-1, 255)<<24;
	// wind to float
	if (!(particle->b % 12) || is_building)
	{
		particle->xdir = 0.025f*::Weather.GetWind(int32_t(particle->x),int32_t(particle->y));
		if (particle->xdir < -2.0f)
			particle->xdir = -2.0f;
		else if (particle->xdir > 2.0f)
			particle->xdir = 2.0f;
		particle->xdir += 0.1f * SafeRandom(41) - 2.0f;
	}
	// float
	if (GBackSolid(int32_t(particle->x), int32_t(particle->y-particle->a)))
	{
		// if stuck, decay; otherwise, move down
		if (!GBackSolid(int32_t(particle->x), int32_t(particle->y)))
			particle->y+=0.4f;
		else
			particle->a-=2;
	}
	else
		--particle->y;
	particle->x += particle->xdir;
	// increase in size
	particle->a *= 1.01f;
	// done, keep
	return true;
}

void fxSmokeDraw(C4Particle *particle, C4TargetFacet &cgo, C4Object *target)
{
	C4ParticleDef *def = particle->pDef;
	// apply parallaxity to target pos
	int32_t tx = cgo.TargetX * def->Parallaxity[0]/100;
	int32_t ty = cgo.TargetY * def->Parallaxity[1]/100;
	// check if it's in screen range
	if (!Inside(particle->x, tx-particle->a, tx+cgo.Wdt+particle->a)) return;
	if (!Inside(particle->y, ty-particle->a, ty+cgo.Hgt+particle->a)) return;
	// get pos
	float cx = particle->x + cgo.X - tx;
	float cy = particle->y + cgo.Y - ty;
	// get phase by particle index
	int32_t i = (int32_t) particle->ydir;
	int32_t px = i/4;
	int32_t py = i%4;
	// draw at pos
	lpDDraw->ActivateBlitModulation(particle->b);

	float fx = float(def->Gfx.X + def->Gfx.Wdt * px);
	float fy = float(def->Gfx.Y + def->Gfx.Hgt * py);
	float fwdt = float(def->Gfx.Wdt);
	float fhgt = float(def->Gfx.Hgt);

	lpDDraw->Blit(def->Gfx.Surface,fx,fy,fwdt,fhgt,
	              cgo.Surface, cx - particle->a, cy - particle->a, particle->a * 2, particle->a * 2,
	              true);

	lpDDraw->DeactivateBlitModulation();
}

bool fxStdInit(C4Particle *particle, C4Object *target)
{
	if (particle->pDef->Delay)
		// delay given: lifetime starts at zero
		particle->life=0;
	else
		// init lifetime as phase
		particle->life=SafeRandom(particle->pDef->Length);
	// default color
	if (!particle->b) particle->b=0xffffffff;
	// always OK
	return true;
}

bool fxStdExec(C4Particle *particle, C4Object *target)
{

	float dx = particle->x, dy = particle->y;
	float dxdir = particle->xdir, dydir = particle->ydir;
	// rel. position & movement
	if (particle->pDef->Attach && target != NULL)
	{
		dx += fixtof(target->GetFixedX());
		dy += fixtof(target->GetFixedY());
		dxdir += fixtof(target->xdir);
		dydir += fixtof(target->ydir);
	}

	// move
	if (particle->xdir || particle->ydir)
	{
		if (particle->pDef->VertexCount && GBackSolid(int32_t( dx + particle->xdir),int32_t( dy + particle->ydir + particle->pDef->VertexY* particle->a/100.0f) ))
		{
			// collision
			if (particle->pDef->CollisionProc)
				if (!particle->pDef->CollisionProc(particle,target)) return false;
		}
		else if (particle->pDef->RByV != 2)
		{
			particle->x += particle->xdir;
			particle->y += particle->ydir;
		}
		else
		{
			// With RByV=2, the V is only used for rotation, not for movement
		}
	}
	// apply gravity
	if (particle->pDef->GravityAcc) particle->ydir+=fixtof(GravAccel * particle->pDef->GravityAcc)/100.0f;
	// apply WindDrift
	if (particle->pDef->WindDrift && !GBackSolid(int32_t(dx), int32_t(dy)))
	{
		// Air speed: Wind plus some random
		int32_t wind_speed = GBackWind(int32_t(dx), int32_t(dy));
		//C4Real txdir = itofix(wind_speed, 15) + C4REAL256(Random(1200) - 600);
		float txdir = wind_speed / 15.0f;
		//C4Real tydir = C4REAL256(Random(1200) - 600);
		float tydir = 0;

		// Air friction, based on WindDrift.
		int32_t wind_drift = Max(particle->pDef->WindDrift - 20, 0);
		particle->xdir += ((txdir - dxdir) * wind_drift) / 800;
		particle->ydir += ((tydir - dydir) * wind_drift) / 800;
	}
	// fade out
	int32_t fade = particle->pDef->AlphaFade;
	if (fade < 0)
	{
		if (Game.FrameCounter % -fade == 0) fade = 1;
		else fade = 0;
	}
	if (fade)
	{
		if (particle->pDef->FadeDelay == 0 || Game.FrameCounter % particle->pDef->FadeDelay == 0)
		{
			DWORD color = particle->b;
			int32_t alpha = color>>24;
			alpha -= particle->pDef->AlphaFade;
			if (alpha <= 0x00) return false;
			particle->b = (color&0xffffff) | (alpha<<24);
		}
	}
	// if delay is given, advance lifetime
	if (particle->pDef->Delay)
	{
		if (particle->life < 0)
		{
			// decay
			return particle->life-- >= -particle->pDef->FadeOutLen * particle->pDef->FadeOutDelay;
		}
		++particle->life;
		// check if still alive
		int32_t phase = particle->life / particle->pDef->Delay;
		int32_t length = particle->pDef->Length - particle->pDef->Reverse;
		if (phase >= length * particle->pDef->Repeats + particle->pDef->Reverse)
		{
			// do fadeout, if assigned
			if (!particle->pDef->FadeOutLen) return false;
			particle->life = -1;
		}
		return true;
	}
	// outside landscape range?
	bool kp;
	if (dxdir > 0)
		kp = (dx - particle->a < GBackWdt);
	else
		kp = (dx + particle->a > 0);

	if (dydir > 0)
		kp = kp && (dy - particle->a < GBackHgt);
	else
		kp = kp && (dy + particle->a > particle->pDef->YOff);

	return kp;
}

bool fxBounce(C4Particle *particle, C4Object *target)
{
	// reverse xdir/ydir
	particle->xdir=-particle->xdir;
	particle->ydir=-particle->ydir;
	return true;
}

bool fxBounceY(C4Particle *particle, C4Object *target)
{
	// reverse ydir only
	particle->ydir = -particle->ydir;
	return true;
}

bool fxStop(C4Particle *particle, C4Object *target)
{
	// zero xdir/ydir
	particle->xdir = particle->ydir = 0;
	return true;
}

bool fxDie(C4Particle *particle, C4Object *target)
{
	// DIEEEEEE
	return false;
}

void fxStdDraw(C4Particle *particle, C4TargetFacet &cgo, C4Object *target)
{
	// get def
	C4ParticleDef *def = particle->pDef;

	// apply parallaxity to target pos
	int32_t tax = cgo.TargetX * def->Parallaxity[0] / 100;
	int32_t tay = cgo.TargetY * def->Parallaxity[1] / 100;

	// get the phases per row
	int32_t phases = def->PhasesX;

	float dx = particle->x, dy = particle->y;
	float dxdir = particle->xdir, dydir = particle->ydir;

	// relative position & movement
	if (def->Attach && target != NULL)
	{
		dx += fixtof(target->GetFixedX());
		dy += fixtof(target->GetFixedY());
		dxdir += fixtof(target->xdir);
		dydir += fixtof(target->ydir);
	}

	// check if it's in screen range
	if (!Inside(dx, tax-particle->a, tax + cgo.Wdt + particle->a)) return;
	if (!Inside(dy, tay-particle->a, tay + cgo.Hgt + particle->a)) return;

	// get pos
	int32_t cgox = cgo.X - tax, cgoy = cgo.Y - tay;
	float cx = dx + cgox;
	float cy = dy + cgoy;

	// get phase
	int32_t phase = particle->life;
	if (def->Delay)
	{
		if (phase >= 0)
		{
			phase /= def->Delay;
			int32_t length = def->Length;
			if (def->Reverse)
			{
				--length;
				phase %= length*2;
				if (phase > length)
					phase = length*2 + 1 - phase;
			}
			else phase %= length;
		}
		else phase = (phase+1) / -def->FadeOutDelay + def->Length;
	}
	// get rotation
	int32_t r=0;
	if ((def->RByV == 1) || (def->RByV == 2)) // rotation by direction
		r = Angle(0,0, (int32_t) (dxdir*10.0f), (int32_t) (dydir*10.0f))*100;
	if (def->RByV == 3) // random rotation - currently a pseudo random rotation by x/y position
		r = (((int32_t)(particle->x * 23 + particle->y * 12)) % 360) * 100;
	// draw at pos
	lpDDraw->ActivateBlitModulation(particle->b);
	lpDDraw->StorePrimaryClipper();
	lpDDraw->SubPrimaryClipper(cgox, cgoy+def->YOff, 100000, 100000);
	if (def->Additive)
		lpDDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);

	// draw
	float draw_width = particle->a;
	float draw_height = def->Aspect * draw_width;

	int32_t phaseX = phase%phases;
	int32_t phaseY = phase/phases;

	float fx = float(def->Gfx.X + def->Gfx.Wdt * phaseX);
	float fy = float(def->Gfx.Y + def->Gfx.Hgt * phaseY);
	float fwdt = float(def->Gfx.Wdt);
	float fhgt = float(def->Gfx.Hgt);
	float tx = cx-draw_width;
	float ty = cy-draw_height;
	float twdt = draw_width*2;
	float thgt = draw_height*2;

	if (r)
	{
		CBltTransform rot;
		rot.SetRotate(r, (float) (tx+tx+twdt)/2, (float) (ty+ty+thgt)/2);
		lpDDraw->Blit(def->Gfx.Surface,fx,fy,fwdt,fhgt,
		                    cgo.Surface,tx,ty,twdt,thgt,
		                    true,&rot);
	}
	else
	{
		lpDDraw->Blit(def->Gfx.Surface,fx,fy,fwdt,fhgt,
		              cgo.Surface,tx,ty,twdt,thgt,
		              true);
	}

	lpDDraw->ResetBlitMode();
	lpDDraw->RestorePrimaryClipper();
	lpDDraw->DeactivateBlitModulation();
}

C4ParticleProcRec C4ParticleProcMap[] =
{
	{ "SmokeInit",  fxSmokeInit },
	{ "SmokeExec",  fxSmokeExec },
	{ "StdInit",    fxStdInit },
	{ "StdExec",    fxStdExec },
	{ "Bounce",     fxBounce  },
	{ "BounceY",    fxBounceY },
	{ "Stop",       fxStop  },
	{ "Die",        fxDie },
	{ "",           0 }
};

C4ParticleDrawProcRec C4ParticleDrawProcMap[] =
{
	{ "Smoke",  fxSmokeDraw },
	{ "Std",    fxStdDraw },
	{ "",       0 }
};

C4ParticleSystem Particles;
