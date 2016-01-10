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

#include <C4Include.h>
#include <C4Particles.h>

// headers for particle loading
#include <C4Log.h>
#include <C4Components.h>
#include <C4Config.h>

#ifndef USE_CONSOLE
// headers for particle excution
#include <C4Application.h>
#include <C4AulDefFunc.h>
#include <C4Value.h>
#include <C4ValueArray.h>
#include <C4Material.h>
#include <C4MeshAnimation.h>
#include <C4DrawGL.h>
#include <C4Random.h>
#include <C4Landscape.h>
#include <C4Weather.h>	
#endif


void C4ParticleDefCore::CompileFunc(StdCompiler * pComp)
{
	pComp->Value(mkNamingAdapt(toC4CStrBuf(Name),       "Name",       ""));
	pComp->Value(mkNamingAdapt(GfxFace,                 "Face"));
}

C4ParticleDefCore::C4ParticleDefCore()
{
	GfxFace.Default();
}

bool C4ParticleDefCore::Compile(char *particle_source, const char *name)
{
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(*this, "Particle"),
	       StdStrBuf(particle_source), name);
}

C4ParticleDef::C4ParticleDef() : C4ParticleDefCore()
{
	// zero fields
	Gfx.Default();
	// link into list
	if (!Particles.definitions.first)
	{
		previous = NULL;
		Particles.definitions.first = this;
	}
	else
	{
		previous = Particles.definitions.last;
		previous->next = this;
	}
	Particles.definitions.last = this;
	next = 0;
}

C4ParticleDef::~C4ParticleDef()
{
	// clear
	Clear();
	// unlink from list
	if (previous) previous->next = next; else Particles.definitions.first = next;
	if (next) next->previous = previous; else Particles.definitions.last = previous;
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
		if (!Gfx.Load(group, C4CFN_DefGraphics, C4FCT_Full, C4FCT_Full, false, C4SF_MipMap))
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
				// calc aspect
		Aspect=(float) Gfx.Hgt/Gfx.Wdt;
		
		// particle overloading
		C4ParticleDef *def_overload;
		if ((def_overload = Particles.definitions.GetDef(Name.getData(), this)))
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

#ifndef USE_CONSOLE
const int C4Particle::DrawingData::vertexCountPerParticle(4);

void C4Particle::DrawingData::SetPosition(float x, float y, float size, float rotation, float stretch)
{
	if (size != originalSize || stretch != currentStretch)
	{
		currentStretch = stretch;
		originalSize = std::max(size, 0.0001f); // a size of zero results in undefined behavior
		sizeX = originalSize / aspect;
		sizeY = originalSize * currentStretch;
	}

	if (rotation == 0.f)
	{
		vertices[0].x = x - sizeX + offsetX;
		vertices[0].y = y + sizeY + offsetY;
		vertices[1].x = x - sizeX + offsetX;
		vertices[1].y = y - sizeY + offsetY;
		vertices[2].x = x + sizeX + offsetX;
		vertices[2].y = y + sizeY + offsetY;
		vertices[3].x = x + sizeX + offsetX;
		vertices[3].y = y - sizeY + offsetY;
	}
	else
	{
		float sine = sinf(rotation);
		float cosine = cosf(rotation);

		vertices[0].x = x + ((-sizeX) * cosine - (+sizeY) * sine) + offsetX;
		vertices[0].y = y + ((-sizeX) *   sine + (+sizeY) * cosine) + offsetY;
		vertices[1].x = x + ((-sizeX) * cosine - (-sizeY) * sine) + offsetX;
		vertices[1].y = y + ((-sizeX) *   sine + (-sizeY) * cosine) + offsetY;
		vertices[2].x = x + ((+sizeX) * cosine - (+sizeY) * sine) + offsetX;
		vertices[2].y = y + ((+sizeX) *   sine + (+sizeY) * cosine) + offsetY;
		vertices[3].x = x + ((+sizeX) * cosine - (-sizeY) * sine) + offsetX;
		vertices[3].y = y + ((+sizeX) *   sine + (-sizeY) * cosine) + offsetY;
	}
}

void C4Particle::DrawingData::SetPhase(int phase, C4ParticleDef *sourceDef)
{
	this->phase = phase;
	phase = phase % sourceDef->Length;
	int offsetY = phase / sourceDef->PhasesX;
	int offsetX = phase % sourceDef->PhasesX;
	float wdt = 1.0f / (float)sourceDef->PhasesX;
	int numOfLines = sourceDef->Length / sourceDef->PhasesX;
	float hgt = 1.0f / (float)numOfLines;

	float x = wdt * (float)offsetX;
	float y = hgt * (float)offsetY;
	float xr = x + wdt;
	float yr = y + hgt;

	vertices[0].u = x; vertices[0].v = yr;
	vertices[1].u = x; vertices[1].v = y;
	vertices[2].u = xr; vertices[2].v = yr;
	vertices[3].u = xr; vertices[3].v = y;
}

C4ParticleValueProvider & C4ParticleValueProvider::operator= (const C4ParticleValueProvider &other)
{
	startValue = other.startValue;
	endValue = other.endValue;
	currentValue = other.currentValue;
	rerollInterval = other.rerollInterval;
	smoothing = other.smoothing;
	valueFunction = other.valueFunction;
	isConstant = other.isConstant;
	keyFrameCount = other.keyFrameCount;

	if (keyFrameCount > 0)
	{
		keyFrames.reserve(2 * keyFrameCount);
		keyFrames.assign(other.keyFrames.begin(), other.keyFrames.end());
	}

	typeOfValueToChange = other.typeOfValueToChange;
	switch (typeOfValueToChange)
	{
	case VAL_TYPE_FLOAT:
		floatValueToChange = other.floatValueToChange;
		break;
	case VAL_TYPE_INT:
		intValueToChange = other.intValueToChange;
		break;
	case VAL_TYPE_KEYFRAMES:
		keyFrameIndex = other.keyFrameIndex;
		break;
	default:
		assert (false && "Trying to copy C4ParticleValueProvider with invalid value type");
		break;
	}
	
	// copy the other's children, too
	for (std::vector<C4ParticleValueProvider*>::const_iterator iter = other.childrenValueProviders.begin(); iter != other.childrenValueProviders.end(); ++iter)
	{
		childrenValueProviders.push_back(new C4ParticleValueProvider(**iter)); // custom copy constructor usage
	}
	return (*this);
}

void C4ParticleValueProvider::SetParameterValue(int type, const C4Value &value, float C4ParticleValueProvider::*floatVal, int C4ParticleValueProvider::*intVal, size_t keyFrameIndex)
{
	// just an atomic data type
	if (value.GetType() == C4V_Int)
	{
		if (type == VAL_TYPE_FLOAT)
			this->*floatVal = (float)value.getInt();
		else if (type == VAL_TYPE_INT)
			this->*intVal = value.getInt();
		else if (type == VAL_TYPE_KEYFRAMES)
			this->keyFrames[keyFrameIndex] = (float)value.getInt();
	}
	else if (value.GetType() == C4V_Array)
	{
		//  might be another value provider!
		C4ParticleValueProvider *child = new C4ParticleValueProvider();
		childrenValueProviders.push_back(child);

		child->Set(*value.getArray());
		child->typeOfValueToChange = type;

		if (type == VAL_TYPE_FLOAT)
		{
			child->floatValueToChange = floatVal;
		}
		else if (type == VAL_TYPE_INT)
		{
			child->intValueToChange = intVal;
		}
		else if (type == VAL_TYPE_KEYFRAMES)
		{
			child->keyFrameIndex = keyFrameIndex;
		}

	}
	else // invalid
	{
		if (type == VAL_TYPE_FLOAT)
			this->*floatVal = 0.f;
		else if (type == VAL_TYPE_INT)
			this->*intVal = 0;
		else if (type == VAL_TYPE_KEYFRAMES)
			this->keyFrames[keyFrameIndex] = 0.f;
	}
}

void C4ParticleValueProvider::UpdatePointerValue(C4Particle *particle, C4ParticleValueProvider *parent)
{
	switch (typeOfValueToChange)
	{
	case VAL_TYPE_FLOAT:
		parent->*floatValueToChange = GetValue(particle);
		break;
	case VAL_TYPE_INT:
		parent->*intValueToChange = (int) GetValue(particle);
		break;
	case VAL_TYPE_KEYFRAMES:
		parent->keyFrames[keyFrameIndex] = GetValue(particle);
		break;
	default:
		assert (false);
	}
}

void C4ParticleValueProvider::UpdateChildren(C4Particle *particle)
{
	for (std::vector<C4ParticleValueProvider*>::iterator iter = childrenValueProviders.begin(); iter != childrenValueProviders.end(); ++iter)
	{
		(*iter)->UpdatePointerValue(particle, this);
	}
}

void C4ParticleValueProvider::FloatifyParameterValue(float C4ParticleValueProvider::*value, float denominator, size_t keyFrameIndex)
{
	if (value == 0)
		this->keyFrames[keyFrameIndex] /= denominator;
	else
		this->*value /= denominator;

	for (std::vector<C4ParticleValueProvider*>::iterator iter = childrenValueProviders.begin(); iter != childrenValueProviders.end(); ++iter)
	{
		C4ParticleValueProvider *child = *iter;
		if (value == 0)
		{
			if (child->typeOfValueToChange == VAL_TYPE_KEYFRAMES && child->keyFrameIndex == keyFrameIndex)
				child->Floatify(denominator);
		}
		else
		{
			if (child->floatValueToChange == value)
				child->Floatify(denominator);
		}
	}
	
}

void C4ParticleValueProvider::Floatify(float denominator)
{
	assert (denominator != 0.f && "Trying to floatify C4ParticleValueProvider with denominator of 0");

	if (valueFunction == &C4ParticleValueProvider::Direction)
	{
		FloatifyParameterValue(&C4ParticleValueProvider::startValue, 1000.f);
		return;
	}

	FloatifyParameterValue(&C4ParticleValueProvider::startValue, denominator);
	FloatifyParameterValue(&C4ParticleValueProvider::endValue, denominator);
	FloatifyParameterValue(&C4ParticleValueProvider::currentValue, denominator);

	// special treatment for keyframes
	if (valueFunction == &C4ParticleValueProvider::KeyFrames)
	{
		for (size_t i = 0; i < keyFrameCount; ++i)
		{
			FloatifyParameterValue(0, 1000.f, 2 * i); // even numbers are the time values
			FloatifyParameterValue(0, denominator, 2 * i + 1); // odd numbers are the actual values
		}
	}
	else if (valueFunction == &C4ParticleValueProvider::Speed || valueFunction == &C4ParticleValueProvider::Wind || valueFunction == &C4ParticleValueProvider::Gravity)
	{
		FloatifyParameterValue(&C4ParticleValueProvider::speedFactor, 1000.0f);
	}
	else if (valueFunction == &C4ParticleValueProvider::Step)
	{
		FloatifyParameterValue(&C4ParticleValueProvider::maxValue, denominator);
	}
	else if (valueFunction == &C4ParticleValueProvider::Sin)
	{
		FloatifyParameterValue(&C4ParticleValueProvider::parameterValue, 1.0f);
		FloatifyParameterValue(&C4ParticleValueProvider::maxValue, denominator);
	}
}

void C4ParticleValueProvider::RollRandom()
{
	float range = endValue - startValue;
	float rnd = (float)(rand()) / (float)(RAND_MAX); 
	currentValue = startValue + rnd * range;
}

float C4ParticleValueProvider::GetValue(C4Particle *forParticle)
{
	UpdateChildren(forParticle);
	return (this->*valueFunction)(forParticle);
}

float C4ParticleValueProvider::Linear(C4Particle *forParticle)
{
	return startValue + (endValue - startValue) * forParticle->GetRelativeAge();
}

float C4ParticleValueProvider::Const(C4Particle *forParticle)
{
	return startValue;
}

float C4ParticleValueProvider::Random(C4Particle *forParticle)
{
	if ((rerollInterval != 0 && ((int)forParticle->GetAge() % rerollInterval == 0)) || alreadyRolled == 0)
	{
		alreadyRolled = 1;
		RollRandom();
	}
	return currentValue;
}

float C4ParticleValueProvider::Direction(C4Particle *forParticle)
{
	float distX = forParticle->currentSpeedX;
	float distY = forParticle->currentSpeedY;

	if (distX == 0.f) return distY > 0.f ? M_PI : 0.f;
	if (distY == 0.f) return distX < 0.f ? 3.0f * M_PI_2 : M_PI_2;

	return startValue * (atan2(distY, distX) + (float)M_PI_2);
}

float C4ParticleValueProvider::Step(C4Particle *forParticle)
{
	float value = currentValue + startValue * forParticle->GetAge() / delay;
	if (maxValue != 0.0f && value > maxValue) value = maxValue;
	return value;
}

float C4ParticleValueProvider::KeyFrames(C4Particle *forParticle)
{
	float age = forParticle->GetRelativeAge();
	// todo, implement smoothing
	//if (smoothing == 0) // linear
	{
		for (size_t i = 0; i < keyFrameCount; ++i)
		{
			if (age > keyFrames[i * 2]) continue;
			assert(i >= 1);

			float x1 = keyFrames[(i - 1) * 2];
			float x2 = keyFrames[i * 2];
			float y1 = keyFrames[(i - 1) * 2 + 1];
			float y2 = keyFrames[i * 2 + 1];
			float position = (age - x1) / (x2 - x1);
			float totalRange = (y2 - y1);

			float value = position * totalRange + y1;
			return value;
		}
	}

	return startValue;
}

float C4ParticleValueProvider::Sin(C4Particle *forParticle)
{
	return sin(parameterValue * M_PI / 180.0f) * maxValue + startValue;
}

float C4ParticleValueProvider::Speed(C4Particle *forParticle)
{
	float distX = forParticle->currentSpeedX;
	float distY = forParticle->currentSpeedY;
	float speed = sqrtf((distX * distX) + (distY * distY));

	return startValue + speedFactor * speed;
}

float C4ParticleValueProvider::Wind(C4Particle *forParticle)
{
	return startValue + (0.01f * speedFactor * ::Weather.GetWind((int)forParticle->positionX, (int)forParticle->positionY));
}

float C4ParticleValueProvider::Gravity(C4Particle *forParticle)
{
	return startValue + (speedFactor * ::Landscape.Gravity);
}

void C4ParticleValueProvider::SetType(C4ParticleValueProviderID what)
{
	switch (what)
	{
	case C4PV_Const:
		valueFunction = &C4ParticleValueProvider::Const;
		break;
	case C4PV_Linear:
		valueFunction = &C4ParticleValueProvider::Linear;
		break;
	case C4PV_Random:
		valueFunction = &C4ParticleValueProvider::Random;
		break;
	case C4PV_Direction:
		valueFunction = &C4ParticleValueProvider::Direction;
		break;
	case C4PV_Step:
		valueFunction = &C4ParticleValueProvider::Step;
		break;
	case C4PV_KeyFrames:
		valueFunction = &C4ParticleValueProvider::KeyFrames;
		break;
	case C4PV_Sin:
		valueFunction = &C4ParticleValueProvider::Sin;
		break;
	case C4PV_Speed:
		valueFunction = &C4ParticleValueProvider::Speed;
		break;
	case C4PV_Wind:
		valueFunction = &C4ParticleValueProvider::Wind;
		break;
	case C4PV_Gravity:
		valueFunction = &C4ParticleValueProvider::Gravity;
		break;
	default:
		assert(false && "Invalid C4ParticleValueProvider ID passed");
	};

	if (what != C4PV_Const)
	{
		isConstant = false;
	}
}

void C4ParticleValueProvider::Set(const C4ValueArray &fromArray)
{
	startValue = endValue = 1.0f;
	valueFunction = &C4ParticleValueProvider::Const;

	size_t arraySize = (size_t) fromArray.GetSize();
	if (arraySize < 2) return;

	int type = fromArray[0].getInt();

	switch (type)
	{
	case C4PV_Const:
		if (arraySize >= 2)
		{
			SetType(C4PV_Const);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::startValue);
		}
		break;

	case C4PV_Linear:
		if (arraySize >= 3)
		{
			SetType(C4PV_Linear);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::endValue);
		}
		break;
	case C4PV_Random:
		if (arraySize >= 3)
		{
			SetType(C4PV_Random);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::endValue);
			if (arraySize >= 4)
				SetParameterValue(VAL_TYPE_INT, fromArray[3], 0, &C4ParticleValueProvider::rerollInterval);
			alreadyRolled = 0;
		}
		break;
	case C4PV_Direction:
		if (arraySize >= 2)
		{
			SetType(C4PV_Direction);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::startValue);
		}
		break;
	case C4PV_Step:
		if (arraySize >= 4)
		{
			SetType(C4PV_Step);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::currentValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[3], &C4ParticleValueProvider::delay);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[4], &C4ParticleValueProvider::maxValue);
			if (delay == 0.f) delay = 1.f;
		}
		break;
	case C4PV_KeyFrames:
		if (arraySize >= 5)
		{
			SetType(C4PV_KeyFrames);
			SetParameterValue(VAL_TYPE_INT, fromArray[1], 0,  &C4ParticleValueProvider::smoothing);
			keyFrames.resize(arraySize + 4 - 1); // 2*2 additional information floats at the beginning and ending, offset the first array item, though

			keyFrameCount = 0;
			const size_t startingOffset = 2;
			size_t i = startingOffset;
			for (; i < arraySize; ++i)
			{
				SetParameterValue(VAL_TYPE_KEYFRAMES, fromArray[(int32_t)i], 0, 0, 2 + i - startingOffset);
			}
			keyFrameCount = (i - startingOffset) / 2 + 2;

			startValue = keyFrames[2 + 1];
			endValue = keyFrames[2 * keyFrameCount - 1];

			// add two points for easier interpolation at beginning and ending
			keyFrames[0] = -500.f;
			keyFrames[1] = keyFrames[2 + 1];
			keyFrames[2 * keyFrameCount - 2] = 1500.f;
			keyFrames[2 * keyFrameCount - 1] = keyFrames[keyFrameCount - 1 - 2];

		}
		break;
	case C4PV_Sin:
		if (arraySize >= 3)
		{
			SetType(C4PV_Sin); // Sin(parameterValue) * maxValue + startValue
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::parameterValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::maxValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[3], &C4ParticleValueProvider::startValue);
		}
		break;
	case C4PV_Speed:
		if (arraySize >= 3)
		{
			SetType(C4PV_Speed);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::startValue);
		}
		break;
	case C4PV_Wind:
		if (arraySize >= 3)
		{
			SetType(C4PV_Wind);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::startValue);
		}
		break;
	case C4PV_Gravity:
		if (arraySize >= 3)
		{
			SetType(C4PV_Gravity);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4ParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4ParticleValueProvider::startValue);
		}
		break;
	default:
		throw C4AulExecError("invalid particle value provider supplied");
		break;
	}
}

void C4ParticleValueProvider::Set(const C4Value &value)
{
	C4ValueArray *valueArray= value.getArray();

	if (valueArray != 0)
		Set(*valueArray);
	else
		Set((float)value.getInt());
}

void C4ParticleValueProvider::Set(float to)
{
	SetType(C4PV_Const);
	startValue = endValue = to;
}

C4ParticleProperties::C4ParticleProperties()
{
	blitMode = 0;
	attachment = C4ATTACH_None;
	hasConstantColor = false;
	hasCollisionVertex = false;
	collisionCallback = 0;
	bouncyness = 0.f;

	// all values in pre-floatified range (f.e. 0..255 instead of 0..1)
	collisionDensity.Set(static_cast<float>(C4M_Solid));
	collisionVertex.Set(0.f);
	size.Set(8.f);
	stretch.Set(1000.f);
	forceX.Set(0.f);
	forceY.Set(0.f);
	speedDampingX.Set(1000.f);
	speedDampingY.Set(1000.f);
	colorR.Set(255.f);
	colorG.Set(255.f);
	colorB.Set(255.f);
	colorAlpha.Set(255.f);
	rotation.Set(0.f);
	phase.Set(0.f);
}

void C4ParticleProperties::Floatify()
{
	bouncyness /= 1000.f;

	collisionDensity.Floatify(1.f);
	collisionVertex.Floatify(1000.f);
	size.Floatify(2.f);
	stretch.Floatify(1000.f);
	forceX.Floatify(100.f);
	forceY.Floatify(100.f);
	speedDampingX.Floatify(1000.f);
	speedDampingY.Floatify(1000.f);
	colorR.Floatify(255.f);
	colorG.Floatify(255.f);
	colorB.Floatify(255.f);
	colorAlpha.Floatify(255.f);
	rotation.Floatify(180.0f / (float)M_PI);
	phase.Floatify(1.f);

	hasConstantColor = colorR.IsConstant() && colorG.IsConstant() && colorB.IsConstant() && colorAlpha.IsConstant();
}

void C4ParticleProperties::Set(C4PropList *dataSource)
{
	if (!dataSource) return;

	C4PropList::Iterator iter = dataSource->begin(), end = dataSource->end();

	for (;iter != end; ++iter)
	{
		const C4Property * p = *iter;
		C4String *key = p->Key;
		assert(key && "PropList returns non-string as key");
		const C4Value &property = p->Value;

		if(&Strings.P[P_R] == key)
		{
			colorR.Set(property);
		}
		else if(&Strings.P[P_G] == key)
		{
			colorG.Set(property);
		}
		else if(&Strings.P[P_B] == key)
		{
			colorB.Set(property);
		}
		else if(&Strings.P[P_Alpha] == key)
		{
			colorAlpha.Set(property);
		}
		else if(&Strings.P[P_ForceX] == key)
		{
			forceX.Set(property);
		}
		else if(&Strings.P[P_ForceY] == key)
		{
			forceY.Set(property);
		}
		else if(&Strings.P[P_DampingX] == key)
		{
			speedDampingX.Set(property);
		}
		else if(&Strings.P[P_DampingY] == key)
		{
			speedDampingY.Set(property);
		}
		else if(&Strings.P[P_Size] == key)
		{
			size.Set(property);
		}
		else if(&Strings.P[P_Stretch] == key)
		{
			stretch.Set(property);
		}
		else if(&Strings.P[P_Rotation] == key)
		{
			rotation.Set(property);
		}
		else if(&Strings.P[P_BlitMode] == key)
		{
			// needs to be constant
			blitMode = (uint32_t) property.getInt();
		}
		else if(&Strings.P[P_Attach] == key)
		{
			// needs to be constant
			attachment = (uint32_t) property.getInt();
		}
		else if(&Strings.P[P_Phase] == key)
		{
			phase.Set(property);
		}
		else if(&Strings.P[P_CollisionVertex] == key)
		{
			collisionVertex.Set(property);
			if (property.GetType() != C4V_Nil)
				hasCollisionVertex = true;
		}
		else if (&Strings.P[P_CollisionDensity] == key)
		{
			collisionDensity.Set(property);
		}
		else if(&Strings.P[P_OnCollision] == key)
		{
			SetCollisionFunc(property);
		}
	}

}

void C4ParticleProperties::SetCollisionFunc(const C4Value &source)
{
	C4ValueArray *valueArray;
	if (!(valueArray = source.getArray())) return;

	int arraySize = valueArray->GetSize();
	if (arraySize < 1) return;

	int type = (*valueArray)[0].getInt();

	switch (type)
	{
	case C4PC_Die:
		collisionCallback = &C4ParticleProperties::CollisionDie;
		break;
	case C4PC_Bounce:
		collisionCallback = &C4ParticleProperties::CollisionBounce;
		bouncyness = 1.f;
		if (arraySize >= 2)
			bouncyness = ((float)(*valueArray)[1].getInt());
		break;
	case C4PC_Stop:
		collisionCallback = &C4ParticleProperties::CollisionStop;
		break;
	default:
		assert(false);
		break;
	}
}

bool C4ParticleProperties::CollisionBounce(C4Particle *forParticle)
{
	forParticle->currentSpeedX = -forParticle->currentSpeedX * bouncyness;
	forParticle->currentSpeedY = -forParticle->currentSpeedY * bouncyness;
	return true;
}

bool C4ParticleProperties::CollisionStop(C4Particle *forParticle)
{
	forParticle->currentSpeedX = 0.f;
	forParticle->currentSpeedY = 0.f;
	return true;
}

void C4Particle::Init()
{
	currentSpeedX = currentSpeedY = 0.f;
	positionX = positionY = 0.f;
	lifetime = startingLifetime = 5.f * 38.f;
}

bool C4Particle::Exec(C4Object *obj, float timeDelta, C4ParticleDef *sourceDef)
{
	// die of old age? :<
	lifetime -= timeDelta;
	// check only if we had a maximum lifetime to begin with (for permanent particles)
	if (startingLifetime > 0.f)
	{
		if (lifetime <= 0.f) return false;
	}

	// movement
	float currentForceX = properties.forceX.GetValue(this);
	float currentForceY = properties.forceY.GetValue(this);

	currentSpeedX += currentForceX;
	currentSpeedY += currentForceY;

	if (currentSpeedX != 0.f || currentSpeedY != 0.f)
	{
		float currentDampingX = properties.speedDampingX.GetValue(this);
		float currentDampingY = properties.speedDampingY.GetValue(this);
		float size = properties.size.GetValue(this);

		currentSpeedX *= currentDampingX;
		currentSpeedY *= currentDampingY;

		// move & collision check
		// note: accessing Landscape.GetDensity here is not protected by locks
		// it is assumed that the particle system is cleaned up before, f.e., the landscape memory is freed
		bool collided = false;
		if (properties.hasCollisionVertex)
		{
			float collisionPoint = properties.collisionVertex.GetValue(this);
			float size_x = (currentSpeedX > 0.f ? size : -size) * 0.5f * collisionPoint;
			float size_y = (currentSpeedY > 0.f ? size : -size) * 0.5f * collisionPoint;
			float density = static_cast<float>(GBackDensity(positionX + size_x + timeDelta * currentSpeedX, positionY + size_y + timeDelta * currentSpeedY));
			
			if (density + 0.5f >= properties.collisionDensity.GetValue(this)) // Small offset against floating point insanities.
			{
				// exec collision func
				if (properties.collisionCallback != 0 && !(properties.*properties.collisionCallback)(this)) return false;
				collided = true;
			}
		}

		if (!collided)
		{
			positionX += timeDelta * currentSpeedX;
			positionY += timeDelta * currentSpeedY;
		}
		drawingData.SetPosition(positionX, positionY, size, properties.rotation.GetValue(this), properties.stretch.GetValue(this));

	}
	else if(!properties.size.IsConstant() || !properties.rotation.IsConstant() || !properties.stretch.IsConstant())
	{
		drawingData.SetPosition(positionX, positionY, properties.size.GetValue(this), properties.rotation.GetValue(this), properties.stretch.GetValue(this));
	}

	// adjust color
	if (!properties.hasConstantColor)
	{
		drawingData.SetColor(properties.colorR.GetValue(this), properties.colorG.GetValue(this), properties.colorB.GetValue(this), properties.colorAlpha.GetValue(this));
	}

	int currentPhase = (int)(properties.phase.GetValue(this) + 0.5f);
	if (currentPhase != drawingData.phase)
		drawingData.SetPhase(currentPhase, sourceDef);

	return true;
}

void C4ParticleChunk::Clear()
{
	for (size_t i = 0; i < particleCount; ++i)
	{
		delete particles[i];
	}
	particleCount = 0;
	particles.clear();
	vertexCoordinates.clear();

	ClearBufferObjects();
}

void C4ParticleChunk::DeleteAndReplaceParticle(size_t indexToReplace, size_t indexFrom)
{
	C4Particle *oldParticle = particles[indexToReplace];

	// try to replace the soon-to-be empty slot in the array
	if (indexFrom != indexToReplace) // false when "replacing" the last one
	{
		std::copy(&vertexCoordinates[indexFrom * C4Particle::DrawingData::vertexCountPerParticle], &vertexCoordinates[indexFrom * C4Particle::DrawingData::vertexCountPerParticle] + C4Particle::DrawingData::vertexCountPerParticle, &vertexCoordinates[indexToReplace * C4Particle::DrawingData::vertexCountPerParticle]);
		particles[indexToReplace] = particles[indexFrom];
		particles[indexToReplace]->drawingData.SetPointer(&vertexCoordinates[indexToReplace * C4Particle::DrawingData::vertexCountPerParticle]);
	}

	delete oldParticle;
}

bool C4ParticleChunk::Exec(C4Object *obj, float timeDelta)
{
	for (size_t i = 0; i < particleCount; ++i)
	{
		if (!particles[i]->Exec(obj, timeDelta, sourceDefinition))
		{
			DeleteAndReplaceParticle(i, particleCount - 1);
			--particleCount;
		}
	}
	return particleCount > 0;
}

#if defined(__APPLE__)
#undef glGenVertexArrays
#undef glBindVertexArray
#undef glDeleteVertexArrays

#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#endif

void C4ParticleChunk::Draw(C4TargetFacet cgo, C4Object *obj, C4ShaderCall& call, int texUnit, const StdProjectionMatrix& modelview)
{
	if (particleCount == 0) return;
	const int stride = sizeof(C4Particle::DrawingData::Vertex);
	assert(sourceDefinition && "No source definition assigned to particle chunk.");
	C4TexRef *textureRef = &sourceDefinition->Gfx.GetFace().textures[0];
	assert(textureRef != 0 && "Particle definition had no texture assigned.");

	// use a relative offset?
	// (note the normal matrix is unaffected by this)
	if ((attachment & C4ATTACH_MoveRelative) && (obj != 0))
	{
		StdProjectionMatrix new_modelview(modelview);
		Translate(new_modelview, fixtof(obj->GetFixedX()), fixtof(obj->GetFixedY()), 0.0f);
		call.SetUniformMatrix4x4(C4SSU_ModelViewMatrix, new_modelview);
	}
	else
	{
		call.SetUniformMatrix4x4(C4SSU_ModelViewMatrix, modelview);
	}

	// enable additive blending for particles with that blit mode
	glBlendFunc(GL_SRC_ALPHA, (blitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, textureRef->texName);

	// generate the buffer as necessary
	if (drawingDataVertexBufferObject == 0)
	{
		// clear up old data
		ClearBufferObjects();
		// generate new buffer objects
		glGenBuffers(1, &drawingDataVertexBufferObject);
		assert (drawingDataVertexBufferObject != 0 && "Could not generate OpenGL buffer object.");
		// Immediately bind the buffer.
		// glVertexAttribPointer requires a valid GL_ARRAY_BUFFER to be bound and we need the buffer to be created for glObjectLabel.
		glBindBuffer(GL_ARRAY_BUFFER, drawingDataVertexBufferObject);

#ifdef GL_KHR_debug
		if (glObjectLabel)
			glObjectLabel(GL_BUFFER, drawingDataVertexBufferObject, -1, "<particles>/VBO");
#endif

		// generate new vertex arrays object
		if (!Particles.useVAOWorkaround)
		{
			glGenVertexArrays(1, &drawingDataVertexArraysObject);
			assert (drawingDataVertexArraysObject != 0 && "Could not generate OpenGL vertex arrays object.");
			
			// set up the vertex array structure once
			glBindVertexArray(drawingDataVertexArraysObject);

#ifdef GL_KHR_debug
			if (glObjectLabel)
				glObjectLabel(GL_VERTEX_ARRAY, drawingDataVertexArraysObject, -1, "<particles>/VAO");
#endif

			glEnableVertexAttribArray(call.GetAttribute(C4SSA_Position));
			glEnableVertexAttribArray(call.GetAttribute(C4SSA_Color));
			glEnableVertexAttribArray(call.GetAttribute(C4SSA_TexCoord));
			glVertexAttribPointer(call.GetAttribute(C4SSA_Position), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, x)));
			glVertexAttribPointer(call.GetAttribute(C4SSA_TexCoord), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, u)));
			glVertexAttribPointer(call.GetAttribute(C4SSA_Color), 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, r)));
			glBindVertexArray(0);
		}
	}

	assert ((Particles.useVAOWorkaround || drawingDataVertexArraysObject != 0) && "No vertex arrays object has been created yet.");
	assert ((drawingDataVertexBufferObject != 0) && "No buffer object has been created yet.");

	// bind the VBO and push the new vertex data
	// this has to be done before binding the vertex arrays object
	glBindBuffer(GL_ARRAY_BUFFER, drawingDataVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(C4Particle::DrawingData::Vertex) * particleCount, &vertexCoordinates[0], GL_DYNAMIC_DRAW);

	// bind VAO and set correct state
	if (!Particles.useVAOWorkaround)
	{
		glBindVertexArray(drawingDataVertexArraysObject);
	}
	else
	{
		glVertexAttribPointer(call.GetAttribute(C4SSA_Position), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, x)));
		glVertexAttribPointer(call.GetAttribute(C4SSA_TexCoord), 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, u)));
		glVertexAttribPointer(call.GetAttribute(C4SSA_Color), 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<GLvoid*>(offsetof(C4Particle::DrawingData::Vertex, r)));
	}

	if (!Particles.usePrimitiveRestartIndexWorkaround)
	{
		glDrawElements(GL_TRIANGLE_STRIP, static_cast<GLsizei> (5 * particleCount), GL_UNSIGNED_INT, ::Particles.GetPrimitiveRestartArray());
	}
	else
	{
		glMultiDrawElements(GL_TRIANGLE_STRIP, ::Particles.GetMultiDrawElementsCountArray(), GL_UNSIGNED_INT, const_cast<const GLvoid**>(::Particles.GetMultiDrawElementsIndexArray()), static_cast<GLsizei> (particleCount));
	}

	// reset buffer data
	if (!Particles.useVAOWorkaround)
	{
		glBindVertexArray(0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool C4ParticleChunk::IsOfType(C4ParticleDef *def, uint32_t _blitMode, uint32_t _attachment) const
{
	return def == sourceDefinition && blitMode == _blitMode && attachment == _attachment;
}

void C4ParticleChunk::ClearBufferObjects()
{
	if (drawingDataVertexBufferObject != 0) // the value 0 as a buffer index is reserved and will never be returned by glGenBuffers
		glDeleteBuffers(1, &drawingDataVertexBufferObject);
	if (drawingDataVertexArraysObject != 0)
		glDeleteVertexArrays(1, &drawingDataVertexArraysObject);

	drawingDataVertexArraysObject = 0;
	drawingDataVertexBufferObject = 0;
}

void C4ParticleChunk::ReserveSpace(uint32_t forAmount)
{
	uint32_t newSize = static_cast<uint32_t>(particleCount) + forAmount + 1;
	::Particles.PreparePrimitiveRestartIndices(newSize);
	if (particles.capacity() < newSize)
		particles.reserve(std::max<uint32_t>(newSize, particles.capacity() * 2));

	// resizing the points vector is relatively costly, hopefully we only do it rarely
	while (vertexCoordinates.capacity() <= newSize * C4Particle::DrawingData::vertexCountPerParticle)
	{
		vertexCoordinates.reserve(std::max<uint32_t>(C4Particle::DrawingData::vertexCountPerParticle * newSize, vertexCoordinates.capacity() * 2));

		// update all existing particles' pointers..
		for (size_t i = 0; i < particleCount; ++i)
			particles[i]->drawingData.SetPointer(&vertexCoordinates[i * C4Particle::DrawingData::vertexCountPerParticle]);
	}
}

C4Particle *C4ParticleChunk::AddNewParticle()
{
	size_t currentIndex = particleCount++;

	if (currentIndex < particles.size())
	{
		particles[currentIndex] = new C4Particle();
	}
	else
	{
		particles.push_back(new C4Particle());
		vertexCoordinates.resize(vertexCoordinates.size() + C4Particle::DrawingData::vertexCountPerParticle);
	}

	C4Particle *newParticle = particles[currentIndex];
	newParticle->drawingData.SetPointer(&vertexCoordinates[currentIndex * C4Particle::DrawingData::vertexCountPerParticle], true);
	return newParticle;
}

void C4ParticleList::Exec(float timeDelta)
{
	if (particleChunks.empty()) return;

	accessMutex.Enter();

	for (std::list<C4ParticleChunk*>::iterator iter = particleChunks.begin(); iter != particleChunks.end();++iter)
	{
		C4ParticleChunk *chunk = *iter;
		chunk->Exec(targetObject, timeDelta);
	}

	accessMutex.Leave();
}

void C4ParticleList::Draw(C4TargetFacet cgo, C4Object *obj)
{
	if (particleChunks.empty()) return;

	pDraw->DeactivateBlitModulation();
	pDraw->ResetBlitMode();
	
	if (!Particles.usePrimitiveRestartIndexWorkaround)
	{
		glPrimitiveRestartIndex(0xffffffff);
		glEnable(GL_PRIMITIVE_RESTART);
	}

	// enable shader
	C4ShaderCall call(pGL->GetSpriteShader(true, false, false));
	// apply zoom and upload shader uniforms
	StdProjectionMatrix modelview = StdProjectionMatrix::Identity();
	pGL->SetupMultiBlt(call, NULL, 0, 0, 0, 0, &modelview);
	// go to correct output position (note the normal matrix is unaffected
	// by this)
	Translate(modelview, cgo.X-cgo.TargetX, cgo.Y-cgo.TargetY, 0.0f);
	// allocate texture unit for particle texture, and remember allocated
	// texture unit. Will be used for each particle chunk to bind
	// their texture to this unit.
	const GLint texUnit = call.AllocTexUnit(C4SSU_BaseTex);
	// Texture coordinates are always associated to texture unit 0, since
	// there is only one set of texture coordinates
	glClientActiveTexture(GL_TEXTURE0);

	if (Particles.useVAOWorkaround)
	{
		glEnableVertexAttribArray(call.GetAttribute(C4SSA_Position));
		glEnableVertexAttribArray(call.GetAttribute(C4SSA_Color));
		glEnableVertexAttribArray(call.GetAttribute(C4SSA_TexCoord));
	}

	accessMutex.Enter();

	for (std::list<C4ParticleChunk*>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); )
	{
		if ((*iter)->IsEmpty())
		{
			delete *iter;
			iter = particleChunks.erase(iter);
			lastAccessedChunk = 0;
		}
		else
		{
			(*iter)->Draw(cgo, obj, call, texUnit, modelview);
			++iter;
		}
	}

	accessMutex.Leave();

	if (Particles.useVAOWorkaround)
	{
		glDisableVertexAttribArray(call.GetAttribute(C4SSA_Position));
		glDisableVertexAttribArray(call.GetAttribute(C4SSA_Color));
		glDisableVertexAttribArray(call.GetAttribute(C4SSA_TexCoord));
	}

	if (!Particles.usePrimitiveRestartIndexWorkaround)
	{
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

void C4ParticleList::Clear()
{
	accessMutex.Enter();

	for (std::list<C4ParticleChunk*>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
		delete *iter;
	particleChunks.clear();

	if (targetObject)
	{
		if (this == targetObject->FrontParticles) targetObject->FrontParticles = NULL;
		else if (this == targetObject->BackParticles) targetObject->BackParticles = NULL;
	}
	else
		if(this == ::Particles.globalParticles) ::Particles.globalParticles = NULL;

	accessMutex.Leave();
}

C4ParticleChunk *C4ParticleList::GetFittingParticleChunk(C4ParticleDef *def, uint32_t blitMode, uint32_t attachment, bool alreadyLocked)
{
	if (!alreadyLocked)
		accessMutex.Enter();

	// if not cached, find correct chunk in list
	C4ParticleChunk *chunk = 0;
	if (lastAccessedChunk && lastAccessedChunk->IsOfType(def, blitMode, attachment))
		chunk = lastAccessedChunk;
	else
	{
		for (std::list<C4ParticleChunk*>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
		{
			C4ParticleChunk *current = *iter;
			if (!current->IsOfType(def, blitMode, attachment)) continue;
			chunk = current;
			break;
		}
	}

	// add new chunk?
	if (!chunk)
	{
		particleChunks.push_back(new C4ParticleChunk());
		chunk = particleChunks.back();
		chunk->sourceDefinition = def;
		chunk->blitMode = blitMode;
		chunk->attachment = attachment;
	}

	assert(chunk && "No suitable particle chunk could be found or created.");
	lastAccessedChunk = chunk;

	if (!alreadyLocked)
		accessMutex.Leave();
	
	return chunk;
}

void C4ParticleSystem::CalculationThread::Execute()
{
	Particles.ExecuteCalculation();
}

C4ParticleSystem::C4ParticleSystem() : frameCounterAdvancedEvent(false)
{
	currentSimulationTime = 0;
	globalParticles = 0;
	usePrimitiveRestartIndexWorkaround = false;
	useVAOWorkaround = false;
}

C4ParticleSystem::~C4ParticleSystem()
{
	Clear();

	calculationThread.SignalStop();
	CalculateNextStep();

	for (std::vector<uint32_t *>::iterator iter = multiDrawElementsIndexArray.begin(); iter != multiDrawElementsIndexArray.end(); ++iter)
		delete[] (*iter);
}

void C4ParticleSystem::DoInit()
{
	// we use features that are only supported from 3.1 upwards. Check whether the graphics card supports that and - if not - use workarounds
	if (!GLEW_VERSION_3_1 || (glPrimitiveRestartIndex == 0))
	{
		usePrimitiveRestartIndexWorkaround = true;
		LogSilent("WARNING (particle system): Your graphics card does not support glPrimitiveRestartIndex - a (slower) fallback will be used!");
	}

	assert (glGenBuffers != 0 && "Your graphics card does not seem to support buffer objects.");
	useVAOWorkaround = false;

#ifndef USE_WIN32_WINDOWS
	// Every window in developers' mode has an own OpenGL context at the moment. Certain objects are not shared between contexts.
	// In that case we can just use the slower workaround without VAOs to allow the developer to view particles in every viewport.
	// The best solution would obviously be to make all windows use a single OpenGL context. This has to be considered as a workaround.
	if (Application.isEditor)
		useVAOWorkaround = true;
#endif
}

void C4ParticleSystem::ExecuteCalculation()
{
	frameCounterAdvancedEvent.WaitFor(INFINITE);
	frameCounterAdvancedEvent.Reset();

	int gameTime = Game.FrameCounter;
	if (currentSimulationTime < gameTime)
	{
		float timeDelta = 1.f;
		if (currentSimulationTime != 0)
			timeDelta = (float)(gameTime - currentSimulationTime);
		currentSimulationTime = gameTime;

		particleListAccessMutex.Enter();

		for (std::list<C4ParticleList>::iterator iter = particleLists.begin(); iter != particleLists.end(); ++iter)
		{
			iter->Exec(timeDelta);
		}

		particleListAccessMutex.Leave();
	}
}
#else // ifdef USE_CONSOLE
void C4ParticleSystem::DoInit() {}
#endif

C4ParticleList *C4ParticleSystem::GetNewParticleList(C4Object *forObject)
{
#ifdef USE_CONSOLE
	return 0;
#else
	C4ParticleList *newList = 0;

	particleListAccessMutex.Enter();
	particleLists.emplace_back(forObject);
	newList = &particleLists.back();
	particleListAccessMutex.Leave();

	return newList;
#endif
}

void C4ParticleSystem::ReleaseParticleList(C4ParticleList *first, C4ParticleList *second)
{
#ifndef USE_CONSOLE
	particleListAccessMutex.Enter();

	for(std::list<C4ParticleList>::iterator iter = particleLists.begin(); iter != particleLists.end();)
	{
		C4ParticleList *list = &(*iter);
		if (list == first || list == second)
		{
			iter = particleLists.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	particleListAccessMutex.Leave();
#endif
}

#ifndef USE_CONSOLE
void C4ParticleSystem::Create(C4ParticleDef *of_def, C4ParticleValueProvider &x, C4ParticleValueProvider &y, C4ParticleValueProvider &speedX, C4ParticleValueProvider &speedY, C4ParticleValueProvider &lifetime, C4PropList *properties, int amount, C4Object *object)
{
	// todo: check amount etc

	C4ParticleList * pxList(0);


	// initialize the particle properties
	// this is done here, because it would also be the right place to implement caching
	C4ParticleProperties particleProperties;
	particleProperties.Set(properties);

	speedX.Floatify(10.f);
	speedY.Floatify(10.f);

	// position offset that will be added to the particle
	float xoff(0.f), yoff(0.f);
	
	// offset only for the drawing position - this is needed so that particles relative to an object work correctly
	float drawingOffsetX(0.f), drawingOffsetY(0.f);

	if (object != 0)
	{
		// for all types of particles add object's offset (mainly for collision etc.)
		xoff = object->GetX();
		yoff = object->GetY();

		if (particleProperties.attachment & C4ATTACH_MoveRelative)
		{
			drawingOffsetX = -xoff;
			drawingOffsetY = -yoff;

			// move relative implies that the particle needs to be in the object's particle list (back OR front)
			// just select the front particles here - will be overwritten below if necessary
			if (!(particleProperties.attachment & C4ATTACH_Front) && !(particleProperties.attachment & C4ATTACH_Back))
				particleProperties.attachment |= C4ATTACH_Front;
		}

		// figure out particle list to use
		if (particleProperties.attachment & C4ATTACH_Front) 
		{
			if (!object->FrontParticles) object->FrontParticles = GetNewParticleList(object);
			pxList = object->FrontParticles;
		}
		else if (particleProperties.attachment & C4ATTACH_Back)
		{
			if (!object->BackParticles) object->BackParticles = GetNewParticleList(object);
			pxList = object->BackParticles;
		}
	}

	// no assigned list implies that we are going to use the global particles
	if (!pxList)
	{
		if (!globalParticles) globalParticles = GetNewParticleList();
		pxList = globalParticles;
	}

	// It is necessary to lock the particle list, because we will have it create a particle first that we are going to modify.
	// Inbetween creation of the particle and modification, the particle list's calculations should not be executed
	// (this could f.e. lead to the particle being removed before it was fully instantiated).
	pxList->Lock();

	// retrieve the fitting chunk for the particle (note that we tell the particle list, we already locked it)
	C4ParticleChunk *chunk = pxList->GetFittingParticleChunk(of_def, particleProperties.blitMode, particleProperties.attachment, true);
	
	// set up chunk to be able to contain enough particles
	chunk->ReserveSpace(static_cast<uint32_t>(amount));

	while (amount--)
	{
		if (x.IsRandom()) x.RollRandom();
		if (y.IsRandom()) y.RollRandom();
		if (speedX.IsRandom()) speedX.RollRandom();
		if (speedY.IsRandom()) speedY.RollRandom();
		if (lifetime.IsRandom()) lifetime.RollRandom();
		
		// create a particle in the fitting chunk (note that we tell the particle list, we already locked it)
		C4Particle *particle = chunk->AddNewParticle();

		// initialize some more properties
		particle->properties = particleProperties;
		// this will adjust the initial values of the (possibly cached) particle properties
		particle->properties.Floatify();

		// setup some more non-property attributes of the particle
		float lifetime_value = lifetime.GetValue(particle);
		if (lifetime_value < 0.0f) lifetime_value = 0.0f; // negative values not allowed (would crash later); using a value of 0 is most likely visible to the scripter
		particle->lifetime = particle->startingLifetime = lifetime_value;

		particle->currentSpeedX = speedX.GetValue(particle);
		particle->currentSpeedY = speedY.GetValue(particle);
		particle->drawingData.aspect = of_def->Aspect;
		particle->drawingData.SetOffset(drawingOffsetX, drawingOffsetY);
		particle->SetPosition(x.GetValue(particle) + xoff, y.GetValue(particle) + yoff);
		particle->drawingData.SetColor(particle->properties.colorR.GetValue(particle), particle->properties.colorG.GetValue(particle), particle->properties.colorB.GetValue(particle), particle->properties.colorAlpha.GetValue(particle));
		particle->drawingData.SetPhase((int)(particle->properties.phase.GetValue(particle) + 0.5f), of_def);
	}

	pxList->Unlock();
}

void C4ParticleSystem::PreparePrimitiveRestartIndices(uint32_t forAmount)
{
	if (!usePrimitiveRestartIndexWorkaround)
	{
		// prepare array with indices, separated by special primitive restart index
		const uint32_t PRI = 0xffffffff;
		size_t neededAmount = 5 * forAmount;

		if (primitiveRestartIndices.size() < neededAmount)
		{
			uint32_t oldValue = 0;

			if (primitiveRestartIndices.size() > 2)
			{
				oldValue = primitiveRestartIndices[primitiveRestartIndices.size()-1];
				if (oldValue == PRI)
					oldValue = primitiveRestartIndices[primitiveRestartIndices.size()-2];
				++oldValue;
			}
			size_t oldSize = primitiveRestartIndices.size();
			primitiveRestartIndices.resize(neededAmount);

			for (size_t i = oldSize; i < neededAmount; ++i)
			{
				if (((i+1) % 5 == 0) && (i != 0))
				{
					primitiveRestartIndices[i] = PRI;
				}
				else
				{
					primitiveRestartIndices[i] = oldValue++;
				}
			}
		}
	}
	else
	{
		// prepare arrays for glMultiDrawElements
		if (multiDrawElementsCountArray.size() <= forAmount)
		{
			multiDrawElementsCountArray.resize(forAmount, 4);
		}

		if (multiDrawElementsIndexArray.size() <= forAmount)
		{
			uint32_t oldSize = multiDrawElementsIndexArray.size();
			multiDrawElementsIndexArray.resize(forAmount);

			for (; oldSize < forAmount; ++oldSize)
			{
				multiDrawElementsIndexArray[oldSize] = new uint32_t[4];
				for (uint32_t i = 0; i < 4; ++i)
					multiDrawElementsIndexArray[oldSize][i] = 4 * oldSize + i;	
			}
		}
	}
}
#endif

void C4ParticleSystem::Clear()
{
#ifndef USE_CONSOLE
	currentSimulationTime = 0;
	ClearAllParticles();
#endif
	// clear definitions even in console mode
	definitions.Clear();
}

void C4ParticleSystem::ClearAllParticles()
{
#ifndef USE_CONSOLE
	particleListAccessMutex.Enter();
	particleLists.clear();
	particleListAccessMutex.Leave();
#endif
}

C4ParticleDef *C4ParticleSystemDefinitionList::GetDef(const char *name, C4ParticleDef *exclude)
{
#ifndef USE_CONSOLE
	// seek list
	for (C4ParticleDef *def = first; def != 0; def=def->next)
		if (def != exclude && def->Name == name)
			return def;
#endif
	// nothing found
	return 0;
}

void C4ParticleSystemDefinitionList::Clear()
{
	// the particle definitions update the list in their destructor
	while (first)
		delete first;
}
C4ParticleSystem Particles;
