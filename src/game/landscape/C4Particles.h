/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2004-2005  Sven Eberhardt
 * Copyright (c) 2004, 2006, 2008  Günther Brammer
 * Copyright (c) 2005  Tobias Zwick
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
// - everything, that is not sync-relevant
// function pointers for drawing and executing are used
// instead of virtual classes and a hierarchy, because
// the latter ones couldn't be optimized using static
// chunks
// thus, more complex partivle behaviour should be solved via
// objects
// note: this particle system will always assume the owning def
//  object to be a static class named ::Particles!

#ifndef INC_C4Particles
#define INC_C4Particles

#include <C4FacetEx.h>
#include <C4Group.h>
#include <C4Shape.h>

class C4Object;

// class predefs
class C4ParticleDefCore;
class C4ParticleDef;
class C4Particle;
class C4ParticleChunk;
class C4ParticleList;
class C4ParticleSystem;

typedef bool (*C4ParticleProc)(C4Particle *, C4Object *);	// generic particle proc
typedef C4ParticleProc C4ParticleInitProc; // particle init proc - init and return whether particle could be created
typedef C4ParticleProc C4ParticleExecProc; // particle execution proc - returns whether particle died
typedef C4ParticleProc C4ParticleCollisionProc; // particle collision proc - returns whether particle died
typedef void (*C4ParticleDrawProc)(C4Particle *, C4TargetFacet &, C4Object *); // particle drawing code

#define ParticleSystem ::Particles

const int 
	C4Px_MaxParticle = 256,			// maximum number of particles of one type
	C4Px_BufSize = 128,			// number of particles in one buffer
	C4Px_MaxIDLen = 30;			// maximum length of internal identifiers

// core for particle defs
class C4ParticleDefCore
	{
	public:
		StdStrBuf Name;		// name
		C4TargetRect GfxFace;					// target rect for graphics; used because stup
		int32_t MaxCount;									// maximum number of particles that may coexist of this type
		int32_t MinLifetime, MaxLifetime;	// used by exec proc; number of frames this particle can exist
		int32_t YOff;											// Y-Offset for Std-particles
		int32_t Delay;										// frame delay between animation phases
		int32_t Repeats;									// number of times the animation is repeated
		int32_t Reverse;									// reverse action after it has been played
		int32_t FadeOutLen,FadeOutDelay;	// phases used for letting the particle fade out
		int32_t RByV;											// if set, rotation will be adjusted according to the movement; if 2, the particle does not move
		int32_t Placement;								// when is the particle to be drawn?
		int32_t GravityAcc;								// acceleration done by gravity
		int32_t WindDrift;
		int32_t VertexCount;							// number of vertices - 0 or 1
		int32_t VertexY;									// y-offset of vertex; 100 is object height
		int32_t Additive;									// whether particle should be drawn additively
		int32_t Attach;								  	// whether the particle moves relatively to the target
		int32_t AlphaFade;								// fadeout in each* frame
		int32_t FadeDelay;								// *each = well, can be redefined here. Standard is 1
		int32_t Parallaxity [2];          // parallaxity

		StdStrBuf InitFn;	// proc to be used for initialization
		StdStrBuf ExecFn;	// proc to be used for frame-execution
		StdStrBuf DrawFn;	// proc to be used for drawing
		StdStrBuf CollisionFn;	// proc to be called upon collision with the landscape; may be left out

		C4ParticleDefCore();					// ctor
		void CompileFunc(StdCompiler * pComp);

		bool Compile(char *szSource, const char *szName);	// compile from def file
	};

// one particle definition
class C4ParticleDef : public C4ParticleDefCore
	{
	public:
		C4ParticleDef *pPrev, *pNext;	// linked list members

		StdStrBuf Filename;	// path to group this particle was loaded from (for EM reloading)

		C4FacetSurface Gfx;								// graphics
		int32_t Length;										// number of phases in gfx
		float Aspect;									// height:width

		C4ParticleInitProc InitProc;	// procedure called once upon creation of the particle
		C4ParticleExecProc ExecProc;	// procedure used for execution of one particle
		C4ParticleCollisionProc CollisionProc;	// procedure called upon collision with the landscape; may be NULL
		C4ParticleDrawProc DrawProc;	// procedure used for drawing of one particle

		int32_t Count;										// number of particles currently existant of this kind

		C4ParticleDef();			// ctor
		~C4ParticleDef();			// dtor

		void Clear();							// free mem associated with this class

		bool Load(C4Group &rGrp);		// load particle from group; assume file to be accessed already
		bool Reload();							// reload particle from stored position
	};

// one tiny little particle
// note: list management done by the chunk, which spares one ptr here
// the chunk initializes the values to zero here, too; so no ctors here...
class C4Particle
	{
	protected:
		C4Particle *pPrev,*pNext;	// previous/next particle of the same list in the buffer

		void MoveList(C4ParticleList &rFrom, C4ParticleList &rTo); // move from one list to another

	public:
		C4ParticleDef *pDef;		// kind of particle
		float x,y,xdir,ydir;		// position and movement
		int32_t life;								// lifetime remaining for this particle
		float a; int32_t b;					// all-purpose values

		friend class C4ParticleChunk;
		friend class C4ParticleList;
		friend class C4ParticleSystem;
	};

// one chunk of particles
// linked list is managed by owner
class C4ParticleChunk
	{
	protected:
		C4ParticleChunk *pNext;						// single linked list
		C4Particle Data[C4Px_BufSize];		// the particles

		int32_t iNumFree;     // number of free particles

	public:
		C4ParticleChunk();				// ctor
		~C4ParticleChunk();				// dtor

		C4Particle *Create(C4ParticleDef *pOfDef); // get a particle from this chunk
		void Destroy(C4Particle *pPrt);		// remove particle from this chunk

		void Clear();							// clear all particles

		friend class C4ParticleSystem;
	};

// a subset of particles
class C4ParticleList
	{
	public:
		C4Particle *pFirst; // first particle in list - others follow in linked list

		C4ParticleList() { pFirst=NULL; } // ctor

		void Exec(C4Object *pObj=NULL);                  // execute all particles
		void Draw(C4TargetFacet &cgo, C4Object *pObj=NULL);  // draw all particles
		void Clear();                                    // remove all particles
		int32_t Remove(C4ParticleDef *pOfDef);               // remove all particles of def

		operator bool() { return !!pFirst; } // checks whether list contains particles
	};

// the main particle system
class C4ParticleSystem
	{
	protected:
		C4ParticleChunk Chunk;	      // linked list for particle chunks
		C4ParticleDef *pDef0, *pDefL;	// linked list for particle defs

		C4ParticleChunk *AddChunk();	// add a new chunk to the list
		void PruneChunks();						// check if all chunks are needed; remove unused

		C4ParticleProc GetProc(const char *szName);					// get init/exec proc for a particle type
		C4ParticleDrawProc GetDrawProc(const char *szName);	// get draw proc for a particle type

	public:
		C4ParticleList FreeParticles; // list of free particles
		C4ParticleList GlobalParticles; // list of free particles

		C4ParticleDef *pSmoke;			// default particle: smoke
		C4ParticleDef *pBlast;			// default particle: blast
		C4ParticleDef *pFSpark;			// default particle: firy spark
		C4ParticleDef *pFire1;			// default particle: fire base
		C4ParticleDef *pFire2;			// default particle: fire additive

		C4ParticleSystem();			// ctor
		~C4ParticleSystem();		// dtor

		void ClearParticles();		// remove all particles
		void Clear();							// remove all particle definitions and particles

		C4Particle *Create(C4ParticleDef *pOfDef, // create one particle of given type
			float x, float y, float xdir=0.0f, float ydir=0.0f,
			float a=0.0f, int32_t b=0, C4ParticleList *pPxList=NULL, C4Object *pObj=NULL);
		bool Cast(C4ParticleDef *pOfDef, // create several particles with different speeds and params
			int32_t iAmount,
			float x, float y, int32_t level,
			float a0=0.0f, DWORD b0=0, float a1=0.0f, DWORD b1=0,
			C4ParticleList *pPxList=NULL, C4Object *pObj=NULL);

		C4ParticleDef *GetDef(const char *szName, C4ParticleDef *pExclude=NULL);	// get particle def by name
		void SetDefParticles();		// seek and assign default particels (smoke, etc.)

		int32_t Push(C4ParticleDef *pOfDef, float dxdir, float dydir); // add movement to all particles of type

		bool IsFireParticleLoaded() { return pFire1 && pFire2; }

		friend class C4ParticleDef;
		friend class C4Particle;
		friend class C4ParticleChunk;
	};

extern C4ParticleSystem Particles;

// default particle execution/drawing functions
bool fxStdInit(C4Particle *pPrt, C4Object *pTarget);
bool fxStdExec(C4Particle *pPrt, C4Object *pTarget);
void fxStdDraw(C4Particle *pPrt, C4TargetFacet &cgo, C4Object *pTarget);

// structures used for static function maps
struct C4ParticleProcRec
	{
	char Name[C4Px_MaxIDLen+1]; // name of procedure
	C4ParticleProc Proc;				// procedure
	};

struct C4ParticleDrawProcRec
	{
	char Name[C4Px_MaxIDLen+1]; // name of procedure
	C4ParticleDrawProc Proc;		// procedure
	};

extern C4ParticleProcRec C4ParticleProcMap[]; // particle init/execution function map
extern C4ParticleDrawProcRec C4ParticleDrawProcMap[]; // particle drawing function map


#endif
