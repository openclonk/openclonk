/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

#include <C4FacetEx.h>

#include <StdScheduler.h>
#include <boost/noncopyable.hpp>


#ifndef INC_C4Particles
#define INC_C4Particles

enum C4ParticleValueProviderID
{
	C4PV_Const,
	C4PV_Linear,
	C4PV_Random,
	C4PV_KeyFrames,
	C4PV_Direction,
	C4PV_Step,
	C4PV_Speed,
	C4PV_Wind,
	C4PV_Gravity,
};

enum C4ParticleAttachmentPropertyID
{
	C4ATTACH_None = 0,
	C4ATTACH_Front = 1,
	C4ATTACH_Back = 2,
	C4ATTACH_MoveRelative = 4
};

enum C4ParticleCollisionFuncID
{
	C4PC_Die,
	C4PC_Bounce,
	C4PC_Stop,
};

class C4ParticleDefCore;
class C4ParticleDef;
class C4ParticleList;
class C4ParticleChunk;
class C4Particle;
class C4ParticleProperties;
class C4ParticleValueProvider;

// core for particle defs
class C4ParticleDefCore
{
public:
	StdStrBuf Name;                   // name
	C4Rect GfxFace;                   // rect for graphics

	C4ParticleDefCore();          // ctor
	void CompileFunc(StdCompiler * compiler);

	bool Compile(char *particle_source, const char *name); // compile from def file
};

// one particle definition
class C4ParticleDef : public C4ParticleDefCore
{
public:
	C4ParticleDef *previous, *next; // linked list members

	StdStrBuf Filename; // path to group this particle was loaded from (for EM reloading)

	C4FacetSurface Gfx;               // graphics
	int32_t Length;                   // number of phases in gfx
	int32_t PhasesX;                  // number of phases per line in gfx
	float Aspect;                     // height:width

	C4ParticleDef();      // ctor
	~C4ParticleDef();     // dtor

	void Clear();             // free mem associated with this class

	bool Load(C4Group &group);   // load particle from group; assume file to be accessed already
	bool Reload();              // reload particle from stored position
};

typedef float (C4ParticleValueProvider::*C4ParticleValueProviderFunction) (C4Particle*);
typedef bool (C4ParticleProperties::*C4ParticleCollisionCallback) (C4Particle*);

#ifndef USE_CONSOLE
// the value providers are used to change the attributes of a particle over the lifetime
class C4ParticleValueProvider
{
protected:
	float startValue, endValue;

	// used by Random
	float currentValue;

	union
	{
		int rerollInterval; // for Random
		float delay; // for Step
		float speedFactor; // for Speed & Wind & Gravity
	};

	union
	{
		int alreadyRolled; // for Random
		int smoothing; // for KeyFrames
	};
	
	size_t keyFrameCount;
	std::vector<float> keyFrames;

	C4ParticleValueProviderFunction valueFunction;
	bool isConstant;

	std::vector<C4ParticleValueProvider*> childrenValueProviders;

	union
	{
		float C4ParticleValueProvider::*floatValueToChange;
		int C4ParticleValueProvider::*intValueToChange;
		size_t keyFrameIndex;
	};
	enum
	{
		VAL_TYPE_INT,
		VAL_TYPE_FLOAT,
		VAL_TYPE_KEYFRAMES,
	};
	int typeOfValueToChange;

public:
	void UpdatePointerValue(C4Particle *particle, C4ParticleValueProvider *parent);
	void UpdateChildren(C4Particle *particle);
	void FloatifyParameterValue(float C4ParticleValueProvider::*value, float denominator, size_t keyFrameIndex = 0);
	void SetParameterValue(int type, const C4Value &value, float C4ParticleValueProvider::*floatVal, int C4ParticleValueProvider::*intVal = 0, size_t keyFrameIndex = 0);

	bool IsConstant() const { return isConstant; }
	bool IsRandom() const { return valueFunction == &C4ParticleValueProvider::Random; }
	C4ParticleValueProvider() : startValue(0.f), endValue(0.f), currentValue(0.f), rerollInterval(0), smoothing(0), valueFunction(0), keyFrameCount(0), isConstant(true), floatValueToChange(0), typeOfValueToChange(VAL_TYPE_FLOAT) { }
	~C4ParticleValueProvider()
	{
		for (std::vector<C4ParticleValueProvider*>::iterator iter = childrenValueProviders.begin(); iter != childrenValueProviders.end(); ++iter)
			delete *iter;
	}
	C4ParticleValueProvider(const C4ParticleValueProvider &other) { *this = other; }
	C4ParticleValueProvider & operator= (const C4ParticleValueProvider &other);
	void RollRandom();

	// divides by denominator
	void Floatify(float denominator);

	void SetType(C4ParticleValueProviderID what = C4PV_Const);
	void Set(const C4Value &value);
	void Set(const C4ValueArray &fromArray);
	void Set(float to); // constant
	float GetValue(C4Particle *forParticle);

	float Linear(C4Particle *forParticle);
	float Const(C4Particle *forParticle);
	float Random(C4Particle *forParticle);
	float KeyFrames(C4Particle *forParticle);
	float Direction(C4Particle *forParticle);
	float Step(C4Particle *forParticle);
	float Speed(C4Particle *forParticle);
	float Wind(C4Particle *forParticle);
	float Gravity(C4Particle *forParticle);
};

// the properties are part of every particle and contain certain changeable attributes
class C4ParticleProperties
{
public:
	bool hasConstantColor;
	bool hasCollisionVertex;

	C4ParticleValueProvider size, stretch;
	C4ParticleValueProvider forceX, forceY;
	C4ParticleValueProvider speedDampingX, speedDampingY;
	C4ParticleValueProvider colorR, colorG, colorB, colorAlpha;
	C4ParticleValueProvider rotation;
	C4ParticleValueProvider phase;
	C4ParticleValueProvider collisionVertex;

	float bouncyness;
	C4ParticleCollisionCallback collisionCallback;
	void SetCollisionFunc(const C4Value &source);

	uint32_t blitMode;

	uint32_t attachment;

	C4ParticleProperties();

	
	void Set(C4PropList *dataSource);
	// divides ints in certain properties by 1000f and in the color properties by 255f
	void Floatify();

	
	bool CollisionDie(C4Particle *forParticle) { return false; }
	bool CollisionBounce(C4Particle *forParticle);
	bool CollisionStop(C4Particle *forParticle);
};

// one single particle
class C4Particle
{
public:

	struct DrawingData
	{
		static const int vertexCountPerParticle;

		struct Vertex
		{
			float x;
			float y;

			float u;
			float v;
			
			float r;
			float g;
			float b;
			float alpha;
		};
		Vertex *vertices;
		
		int phase;

		float currentStretch;
		float originalSize;
		float sizeX, sizeY;
		float aspect;

		float offsetX, offsetY;

		void SetOffset(float x, float y)
		{
			offsetX = x;
			offsetY = y;
		}

		void SetPointer(Vertex *startingVertex, bool initial = false)
		{
			vertices = startingVertex;

			if (initial)
			{
				vertices[0].u = 0.f; vertices[0].v = 1.f;
				vertices[1].u = 0.f; vertices[1].v = 0.f;
				vertices[2].u = 1.f; vertices[2].v = 1.f;
				vertices[3].u = 1.f; vertices[3].v = 0.f;

				SetColor(1.f, 1.f, 1.f, 1.f);

				phase = -1;
			}
		}

		void SetColor(float r, float g, float b, float a = 1.0f)
		{
			for (int vertex = 0; vertex < 4; ++vertex)
			{
				vertices[vertex].r = r;
				vertices[vertex].g = g;
				vertices[vertex].b = b;
				vertices[vertex].alpha = a;
			}
		}

		void SetPosition(float x, float y, float size, float rotation = 0.f, float stretch = 1.f);
		void SetPhase(int phase, C4ParticleDef *sourceDef);

		DrawingData() : currentStretch(1.f), originalSize(0.0001f), aspect(1.f), offsetX(0.f), offsetY(0.f)
		{
		}

	} drawingData;
protected:
	float currentSpeedX, currentSpeedY;
	float positionX, positionY;
	float lifetime, startingLifetime;

	C4ParticleProperties properties;

public:
	float GetAge() const { return startingLifetime - lifetime; }
	float GetLifetime() const { return lifetime; }
	float GetRelativeAge() const { return (startingLifetime != 0.f) ? (1.0f - (lifetime / startingLifetime)) : 0.f; }

	void Init();
	C4Particle() { Init(); }

	void SetPosition(float x, float y)
	{
		positionX = x;
		positionY = y;
		drawingData.SetPosition(positionX, positionY, properties.size.GetValue(this),  properties.rotation.GetValue(this));
	}

	bool Exec(C4Object *obj, float timeDelta, C4ParticleDef *sourceDef);

	friend class C4ParticleProperties;
	friend class C4ParticleValueProvider;
	friend class C4ParticleChunk;
	friend class C4ParticleSystem;
};

// a chunk contains all of the single particles that can be drawn with one draw call (~"have certain similar attributes")
// this is noncopyable to make sure that the OpenGL buffers are never freed multiple times
class C4ParticleChunk : public boost::noncopyable
{
private:
	C4ParticleDef *sourceDefinition;

	uint32_t blitMode;
	
	// whether the particles are translated according to the object's position
	uint32_t attachment;

	std::vector<C4Particle*> particles;
	std::vector<C4Particle::DrawingData::Vertex> vertexCoordinates;
	size_t particleCount;

	// OpenGL optimizations
	GLuint drawingDataVertexBufferObject;
	GLuint drawingDataVertexArraysObject;
	void ClearBufferObjects();

	// delete the particle at indexTo. If possible, replace it with the particle at indexFrom to keep the particles tighly packed
	void DeleteAndReplaceParticle(size_t indexToReplace, size_t indexFrom);

public:
	C4ParticleChunk() : sourceDefinition(0), blitMode(0), attachment(C4ATTACH_None), particleCount(0), drawingDataVertexBufferObject(0), drawingDataVertexArraysObject(0)
	{

	}
	~C4ParticleChunk()
	{
		Clear();
	}
	// removes all particles
	void Clear();
	bool Exec(C4Object *obj, float timeDelta);
	void Draw(C4TargetFacet cgo, C4Object *obj);
	bool IsOfType(C4ParticleDef *def, uint32_t _blitMode, uint32_t attachment) const;

	// before adding a particle, you should ReserveSpace for it
	C4Particle *AddNewParticle();
	// sets up internal data structures to be large enough for the passed amount of ADDITIONAL particles
	void ReserveSpace(uint32_t forAmount);

	friend class C4ParticleList;
};

// this class must not be copied, because deleting the contained CStdCSec twice would be fatal
// a particle list belongs to a game-world entity (objects or global particles) and contains the chunks associated with that entity
class C4ParticleList : public boost::noncopyable
{
private:
	std::list<C4ParticleChunk*> particleChunks;

	C4Object *targetObject;

	// caching..
	C4ParticleChunk *lastAccessedChunk;

	// for making sure that the list is not drawn and calculated at the same time
	CStdCSec accessMutex;

public:
	C4ParticleList(C4Object *obj = 0) : targetObject(obj), lastAccessedChunk(0)
	{

	}
	
	~C4ParticleList() { Clear(); }

	// this enables third-parties to lock the particle list (for example because a particle in the list is modified from outside)
	void Lock() { accessMutex.Enter(); }
	void Unlock() { accessMutex.Leave(); }

	// deletes all the particles
	void Clear();

	void Exec(float timeDelta = 1.f);
	void Draw(C4TargetFacet cgo, C4Object *obj);
	C4ParticleChunk *GetFittingParticleChunk(C4ParticleDef *def, uint32_t blitMode, uint32_t attachment, bool alreadyLocked);
	C4Particle *AddNewParticle(C4ParticleDef *def, uint32_t blitMode, uint32_t attachment, bool alreadyLocked, int remaining = 0);
};
#endif

// cares for the management of particle definitions
class C4ParticleSystemDefinitionList
{
#ifndef USE_CONSOLE
private:
	// pointers to the last and first element of linked list of particle definitions
	C4ParticleDef *first, *last;
public:
	C4ParticleSystemDefinitionList() : first(0), last(0) {}
	void Clear();
#endif
	C4ParticleDef *GetDef(const char *name, C4ParticleDef *exclude=0);

	friend class C4ParticleDef;
};

// the global particle system interface class
class C4ParticleSystem
{
#ifndef USE_CONSOLE
	class CalculationThread : public StdThread
	{
	protected:
		virtual void Execute();
	public:
		CalculationThread() { StdThread::Start(); }
	};
	friend class CalculationThread;

private:
#ifndef USE_CONSOLE
	// contains an array with indices for vertices, separated by a primitive restart index
	std::vector<uint32_t> primitiveRestartIndices;
	// these are fallbacks for if primitiveRestartIndex is not supported by the graphics card
	std::vector<GLsizei> multiDrawElementsCountArray;
	std::vector<uint32_t *> multiDrawElementsIndexArray;
#endif
	std::list<C4ParticleList> particleLists;

	CalculationThread calculationThread;
	CStdCSec particleListAccessMutex;
	CStdEvent frameCounterAdvancedEvent;

	int currentSimulationTime; // in game time

	// calculates the physics in all of the existing particle lists
	void ExecuteCalculation();

	C4ParticleList *globalParticles;
#endif

public:
#ifndef USE_CONSOLE
	C4ParticleSystem();
	~C4ParticleSystem();
#endif
	// called to allow the particle system the simulation of another step
	void CalculateNextStep()
	{
#ifndef USE_CONSOLE
		frameCounterAdvancedEvent.Set();
#endif
	}
	void DoInit();
	// resets the internal state of the particle system and unloads all definitions
	void Clear();
	void DrawGlobalParticles(C4TargetFacet cgo)
	{
#ifndef USE_CONSOLE
		if (globalParticles) globalParticles->Draw(cgo, 0);
#endif
	} 

	C4ParticleList *GetGlobalParticles()
	{
#ifndef USE_CONSOLE
		return globalParticles;
#else
		return 0;
#endif
	}

	C4ParticleList *GetNewParticleList(C4Object *forTarget = 0);
	// releases up to 2 lists
	void ReleaseParticleList(C4ParticleList *first, C4ParticleList *second = 0);

	// interface for particle definitions
	C4ParticleSystemDefinitionList definitions;

#ifndef USE_CONSOLE
	// on some graphics card, glPrimitiveRestartIndex might not be supported
	bool usePrimitiveRestartIndexWorkaround;
	GLsizei *GetMultiDrawElementsCountArray() { return &multiDrawElementsCountArray[0]; } 
	GLvoid **GetMultiDrawElementsIndexArray() { return reinterpret_cast<GLvoid**> (&multiDrawElementsIndexArray[0]); }

	// if true, OpenGL buffer objects will not be used (instead the slower direct calls will be made)
	bool useBufferObjectWorkaround;

	// usually, the following methods are used for drawing
	void PreparePrimitiveRestartIndices(uint32_t forSize);
	void *GetPrimitiveRestartArray() { return (void*)&primitiveRestartIndices[0]; }

	// creates a new particle
	void Create(C4ParticleDef *of_def, C4ParticleValueProvider &x, C4ParticleValueProvider &y, C4ParticleValueProvider &speedX, C4ParticleValueProvider &speedY, C4ParticleValueProvider &lifetime, C4PropList *properties, int amount = 1, C4Object *object=NULL);
	
	// removes all of the existing particles (used f.e. for scenario section loading)
	void ClearAllParticles();

	friend class C4ParticleList;

#endif

};

extern C4ParticleSystem Particles;

#endif
