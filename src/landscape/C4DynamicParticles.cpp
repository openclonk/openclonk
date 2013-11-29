/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2013 David Dormagen
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

#include <C4Include.h>
#include <C4DynamicParticles.h>

#ifndef USE_CONSOLE
#include <C4AulDefFunc.h>
#include <C4Value.h>
#include <C4ValueArray.h>
#include <C4MeshAnimation.h>
#include <C4DrawGL.h>
#include <C4Random.h>
#include <C4Landscape.h>
#include <C4Weather.h>
#endif

#ifndef USE_CONSOLE
const int C4DynamicParticle::DrawingData::vertexCountPerParticle(4);

void C4DynamicParticle::DrawingData::SetPosition(float x, float y, float size, float rotation, float stretch)
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

void C4DynamicParticle::DrawingData::SetPhase(int phase, C4ParticleDef *sourceDef)
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

C4DynamicParticleValueProvider & C4DynamicParticleValueProvider::operator= (const C4DynamicParticleValueProvider &other)
{
	startValue = other.startValue;
	endValue = other.endValue;
	currentValue = other.currentValue;
	rerollInterval = other.rerollInterval;
	smoothing = other.smoothing;
	valueFunction = other.valueFunction;
	isConstant = other.isConstant;
	keyFrameCount = other.keyFrameCount;

	keyFrames.assign(other.keyFrames.begin(), other.keyFrames.end());

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
		assert (false && "Trying to copy C4DynamicParticleValueProvider with invalid value type");
		break;
	}
	
	// copy the other's children, too
	for (std::vector<C4DynamicParticleValueProvider*>::const_iterator iter = other.childrenValueProviders.begin(); iter != other.childrenValueProviders.end(); ++iter)
	{
		childrenValueProviders.push_back(new C4DynamicParticleValueProvider(**iter)); // custom copy constructor usage
	}
	return (*this);
}

void C4DynamicParticleValueProvider::SetParameterValue(int type, const C4Value &value, float C4DynamicParticleValueProvider::*floatVal, int C4DynamicParticleValueProvider::*intVal, size_t keyFrameIndex)
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
		C4DynamicParticleValueProvider *child = new C4DynamicParticleValueProvider();
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

void C4DynamicParticleValueProvider::UpdatePointerValue(C4DynamicParticle *particle, C4DynamicParticleValueProvider *parent)
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

void C4DynamicParticleValueProvider::UpdateChildren(C4DynamicParticle *particle)
{
	for (std::vector<C4DynamicParticleValueProvider*>::iterator iter = childrenValueProviders.begin(); iter != childrenValueProviders.end(); ++iter)
	{
		(*iter)->UpdatePointerValue(particle, this);
	}
}

void C4DynamicParticleValueProvider::FloatifyParameterValue(float C4DynamicParticleValueProvider::*value, float denominator, size_t keyFrameIndex)
{
	if (value == 0)
		this->keyFrames[keyFrameIndex] /= denominator;
	else
		this->*value /= denominator;

	for (std::vector<C4DynamicParticleValueProvider*>::iterator iter = childrenValueProviders.begin(); iter != childrenValueProviders.end(); ++iter)
	{
		C4DynamicParticleValueProvider *child = *iter;
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

void C4DynamicParticleValueProvider::Floatify(float denominator)
{
	assert (denominator != 0.f && "Trying to floatify C4DynamicParticleValueProvider with denominator of 0");

	if (valueFunction == &C4DynamicParticleValueProvider::Direction)
	{
		FloatifyParameterValue(&C4DynamicParticleValueProvider::startValue, 1000.f);
		return;
	}

	FloatifyParameterValue(&C4DynamicParticleValueProvider::startValue, denominator);
	FloatifyParameterValue(&C4DynamicParticleValueProvider::endValue, denominator);
	FloatifyParameterValue(&C4DynamicParticleValueProvider::currentValue, denominator);

	// special treatment for keyframes
	if (valueFunction == &C4DynamicParticleValueProvider::KeyFrames)
	{
		for (size_t i = 0; i < keyFrameCount; ++i)
		{
			FloatifyParameterValue(0, 1000.f, 2 * i); // even numbers are the time values
			FloatifyParameterValue(0, denominator, 2 * i + 1); // odd numbers are the actual values
			//LogF("KF is %f @ %f", keyFrames[2 * i + 1], keyFrames[2 * i]);
		}
	}
	else if (valueFunction == &C4DynamicParticleValueProvider::Speed || valueFunction == &C4DynamicParticleValueProvider::Wind || valueFunction == &C4DynamicParticleValueProvider::Gravity)
	{
		FloatifyParameterValue(&C4DynamicParticleValueProvider::speedFactor, 1000.0f);
	}
}

void C4DynamicParticleValueProvider::RollRandom()
{
	float range = endValue - startValue;
	float rnd = (float)(rand()) / (float)(RAND_MAX); 
	currentValue = startValue + rnd * range;
}

float C4DynamicParticleValueProvider::GetValue(C4DynamicParticle *forParticle)
{
	UpdateChildren(forParticle);
	return (this->*valueFunction)(forParticle);
}

float C4DynamicParticleValueProvider::Linear(C4DynamicParticle *forParticle)
{
	return startValue + (endValue - startValue) * forParticle->GetRelativeAge();
}

float C4DynamicParticleValueProvider::Const(C4DynamicParticle *forParticle)
{
	return startValue;
}

float C4DynamicParticleValueProvider::Random(C4DynamicParticle *forParticle)
{
	if ((rerollInterval != 0 && ((int)forParticle->GetAge() % rerollInterval == 0)) || alreadyRolled == 0)
	{
		alreadyRolled = 1;
		RollRandom();
	}
	return currentValue;
}

float C4DynamicParticleValueProvider::Direction(C4DynamicParticle *forParticle)
{
	float distX = forParticle->currentSpeedX;
	float distY = forParticle->currentSpeedY;

	if (distX == 0.f) return distY > 0.f ? M_PI : 0.f;
	if (distY == 0.f) return distX < 0.f ? 3.0f * M_PI_2 : M_PI_2;

	return startValue * (atan2(distY, distX) + (float)M_PI_2);
}

float C4DynamicParticleValueProvider::Step(C4DynamicParticle *forParticle)
{
	return currentValue + startValue * forParticle->GetAge() / delay;
}

float C4DynamicParticleValueProvider::KeyFrames(C4DynamicParticle *forParticle)
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

float C4DynamicParticleValueProvider::Speed(C4DynamicParticle *forParticle)
{
	float distX = forParticle->currentSpeedX;
	float distY = forParticle->currentSpeedY;
	float speed = sqrtf((distX * distX) + (distY * distY));

	return startValue + speedFactor * speed;
}

float C4DynamicParticleValueProvider::Wind(C4DynamicParticle *forParticle)
{
	return startValue + (0.01f * speedFactor * ::Weather.GetWind((int)forParticle->positionX, (int)forParticle->positionY));
}

float C4DynamicParticleValueProvider::Gravity(C4DynamicParticle *forParticle)
{
	return startValue + (speedFactor * ::Landscape.Gravity);
}

void C4DynamicParticleValueProvider::SetType(C4ParticleValueProviderID what)
{
	switch (what)
	{
	case C4PV_Const:
		valueFunction = &C4DynamicParticleValueProvider::Const;
		break;
	case C4PV_Linear:
		valueFunction = &C4DynamicParticleValueProvider::Linear;
		break;
	case C4PV_Random:
		valueFunction = &C4DynamicParticleValueProvider::Random;
		break;
	case C4PV_Direction:
		valueFunction = &C4DynamicParticleValueProvider::Direction;
		break;
	case C4PV_Step:
		valueFunction = &C4DynamicParticleValueProvider::Step;
		break;
	case C4PV_KeyFrames:
		valueFunction = &C4DynamicParticleValueProvider::KeyFrames;
		break;
	case C4PV_Speed:
		valueFunction = &C4DynamicParticleValueProvider::Speed;
		break;
	case C4PV_Wind:
		valueFunction = &C4DynamicParticleValueProvider::Wind;
		break;
	case C4PV_Gravity:
		valueFunction = &C4DynamicParticleValueProvider::Gravity;
		break;
	default:
		assert(false && "Invalid C4DynamicParticleValueProvider ID passed");
	};

	if (what != C4PV_Const)
	{
		isConstant = false;
	}
}

void C4DynamicParticleValueProvider::Set(const C4ValueArray &fromArray)
{
	startValue = endValue = 1.0f;
	valueFunction = &C4DynamicParticleValueProvider::Const;

	size_t arraySize = (size_t) fromArray.GetSize();
	if (arraySize < 2) return;

	int type = fromArray[0].getInt();

	switch (type)
	{
	case C4PV_Const:
		if (arraySize >= 2)
		{
			SetType(C4PV_Const);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::startValue);
		}
		break;

	case C4PV_Linear:
		if (arraySize >= 3)
		{
			SetType(C4PV_Linear);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::endValue);
		}
		break;
	case C4PV_Random:
		if (arraySize >= 3)
		{
			SetType(C4PV_Random);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::endValue);
			if (arraySize >= 4)
				SetParameterValue(VAL_TYPE_INT, fromArray[3], 0, &C4DynamicParticleValueProvider::rerollInterval);
			alreadyRolled = 0;
		}
		break;
	case C4PV_Direction:
		if (arraySize >= 2)
		{
			SetType(C4PV_Direction);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::startValue);
		}
		break;
	case C4PV_Step:
		if (arraySize >= 2)
		{
			SetType(C4PV_Step);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::startValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::currentValue);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[3], &C4DynamicParticleValueProvider::delay);
			if (delay == 0.f) delay = 1.f;
		}
		break;
	case C4PV_KeyFrames:
		if (arraySize >= 5)
		{
			SetType(C4PV_KeyFrames);
			SetParameterValue(VAL_TYPE_INT, fromArray[1], 0,  &C4DynamicParticleValueProvider::smoothing);
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

			//for (int i = 0; i < keyFrameCount; ++i)
			//	LogF("KF is %f @ %d of %d", keyFrames[i * 2 + 1], int(keyFrames[i * 2]), keyFrameCount);
		}
		break;
	case C4PV_Speed:
		if (arraySize >= 3)
		{
			SetType(C4PV_Speed);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::startValue);
		}
		break;
	case C4PV_Wind:
		if (arraySize >= 3)
		{
			SetType(C4PV_Wind);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::startValue);
		}
		break;
	case C4PV_Gravity:
		if (arraySize >= 3)
		{
			SetType(C4PV_Gravity);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[1], &C4DynamicParticleValueProvider::speedFactor);
			SetParameterValue(VAL_TYPE_FLOAT, fromArray[2], &C4DynamicParticleValueProvider::startValue);
		}
		break;
	default:
		throw new C4AulExecError("invalid particle value provider supplied");
		break;
	}
}

void C4DynamicParticleValueProvider::Set(const C4Value &value)
{
	C4ValueArray *valueArray= value.getArray();

	if (valueArray != 0)
		Set(*valueArray);
	else
		Set((float)value.getInt());
}

void C4DynamicParticleValueProvider::Set(float to)
{
	SetType(C4PV_Const);
	startValue = endValue = to;
}

C4DynamicParticleProperties::C4DynamicParticleProperties()
{
	blitMode = 0;
	attachment = C4ATTACH_None;
	hasConstantColor = false;
	hasCollisionVertex = false;
	collisionCallback = 0;
	bouncyness = 0.f;

	// all values in pre-floatified range (f.e. 0..255 instead of 0..1)
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

void C4DynamicParticleProperties::Floatify()
{
	bouncyness /= 1000.f;

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

void C4DynamicParticleProperties::Set(C4PropList *dataSource)
{
	if (!dataSource) return;

	// get properties from proplist
	// todo: make sure to delete the array later, we took ownership
	C4ValueArray *properties = dataSource->GetProperties();

	for (int32_t i = 0; i < properties->GetSize(); ++i)
	{
		const C4Value &entry = properties->GetItem(i);
		C4String *key = entry.getStr();
		assert(key && "PropList returns non-string as key");

		C4Value property;
		dataSource->GetPropertyByS(key, &property);

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
		else if(&Strings.P[P_OnCollision] == key)
		{
			SetCollisionFunc(property);
		}
	}

	delete properties;
}

void C4DynamicParticleProperties::SetCollisionFunc(const C4Value &source)
{
	C4ValueArray *valueArray;
	if (!(valueArray = source.getArray())) return;

	int arraySize = valueArray->GetSize();
	if (arraySize < 1) return;

	int type = (*valueArray)[0].getInt();

	switch (type)
	{
	case C4PC_Die:
		collisionCallback = &C4DynamicParticleProperties::CollisionDie;
		break;
	case C4PC_Bounce:
		collisionCallback = &C4DynamicParticleProperties::CollisionBounce;
		bouncyness = 1.f;
		if (arraySize >= 2)
			bouncyness = ((float)(*valueArray)[1].getInt());
		break;
	case C4PC_Stop:
		collisionCallback = &C4DynamicParticleProperties::CollisionStop;
		break;
	default:
		assert(false);
		break;
	}
}

bool C4DynamicParticleProperties::CollisionBounce(C4DynamicParticle *forParticle)
{
	forParticle->currentSpeedX = -forParticle->currentSpeedX * bouncyness;
	forParticle->currentSpeedY = -forParticle->currentSpeedY * bouncyness;
	return true;
}

bool C4DynamicParticleProperties::CollisionStop(C4DynamicParticle *forParticle)
{
	forParticle->currentSpeedX = 0.f;
	forParticle->currentSpeedY = 0.f;
	return true;
}

void C4DynamicParticle::Init()
{
	currentSpeedX = currentSpeedY = 0.f;
	positionX = positionY = 0.f;
	lifetime = startingLifetime = 5.f * 38.f;
}

bool C4DynamicParticle::Exec(C4Object *obj, float timeDelta, C4ParticleDef *sourceDef)
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
			if (GBackSolid(positionX + size_x + timeDelta * currentSpeedX, positionY + size_y + timeDelta * currentSpeedY))
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

void C4DynamicParticleChunk::Clear()
{
	for (size_t i = 0; i < particleCount; ++i)
	{
		delete particles[i];
	}
	particleCount = 0;
	particles.clear();
	vertexCoordinates.clear();
}

void C4DynamicParticleChunk::DeleteAndReplaceParticle(size_t indexToReplace, size_t indexFrom)
{
	C4DynamicParticle *oldParticle = particles[indexToReplace];

	// try to replace the soon-to-be empty slot in the array
	if (indexFrom != indexToReplace) // false when "replacing" the last one
	{
		std::copy(&vertexCoordinates[indexFrom * C4DynamicParticle::DrawingData::vertexCountPerParticle], &vertexCoordinates[indexFrom * C4DynamicParticle::DrawingData::vertexCountPerParticle] + C4DynamicParticle::DrawingData::vertexCountPerParticle, &vertexCoordinates[indexToReplace * C4DynamicParticle::DrawingData::vertexCountPerParticle]);
		particles[indexToReplace] = particles[indexFrom];
		particles[indexToReplace]->drawingData.SetPointer(&vertexCoordinates[indexToReplace * C4DynamicParticle::DrawingData::vertexCountPerParticle]);
	}

	delete oldParticle;
}

bool C4DynamicParticleChunk::Exec(C4Object *obj, float timeDelta)
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

void C4DynamicParticleChunk::Draw(C4TargetFacet cgo, C4Object *obj)
{
	if (particleCount == 0) return;
	const int stride = sizeof(C4DynamicParticle::DrawingData::Vertex);
	assert(sourceDefinition && "No source definition assigned to particle chunk.");
	C4TexRef *textureRef = (*sourceDefinition->Gfx.GetFace().ppTex);
	assert(textureRef != 0 && "Particle definition had no texture assigned.");

	

	// use a relative offset?
	bool resetMatrix(false);
	if ((attachment & C4ATTACH_MoveRelative) && (obj != 0))
	{
		resetMatrix = true;
		glPushMatrix();
		glTranslatef((float)obj->GetX(), (float)obj->GetY(), 0.0f);
	}

	glBlendFunc(GL_SRC_ALPHA, (blitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureRef->texName);

	glVertexPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].x));
	glTexCoordPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].u));
	glColorPointer(4, GL_FLOAT, stride, &(vertexCoordinates[0].r));

	if (!DynamicParticles.usePrimitiveRestartIndexWorkaround)
	{
		glDrawElements(GL_TRIANGLE_STRIP, (GLsizei) (5 * particleCount), GL_UNSIGNED_INT, ::DynamicParticles.GetPrimitiveRestartArray());
	}
	else
	{
		glMultiDrawElements(GL_TRIANGLE_STRIP, ::DynamicParticles.GetMultiDrawElementsCountArray(), GL_UNSIGNED_INT, (const GLvoid**) ::DynamicParticles.GetMultiDrawElementsIndexArray(), (GLsizei) particleCount);
	}
	if (resetMatrix)
		glPopMatrix();
}

bool C4DynamicParticleChunk::IsOfType(C4ParticleDef *def, uint32_t _blitMode, uint32_t _attachment) const
{
	return def == sourceDefinition && blitMode == _blitMode && attachment == _attachment;
}

C4DynamicParticle *C4DynamicParticleChunk::AddNewParticle()
{
	size_t currentIndex = particleCount++;
	::DynamicParticles.PreparePrimitiveRestartIndices(particleCount);

	if (currentIndex < particles.size())
	{
		particles[currentIndex] = new C4DynamicParticle();
	}
	else
	{
		particles.push_back(new C4DynamicParticle());
		// resizing the points vector is relatively costly, hopefully we only do it rarely
		while (vertexCoordinates.capacity() <= particleCount * C4DynamicParticle::DrawingData::vertexCountPerParticle)
		{
			vertexCoordinates.reserve(C4DynamicParticle::DrawingData::vertexCountPerParticle + vertexCoordinates.capacity() * 2);

			// update all existing particles' pointers..
			for (size_t i = 0; i < currentIndex; ++i)
				particles[i]->drawingData.SetPointer(&vertexCoordinates[i * C4DynamicParticle::DrawingData::vertexCountPerParticle]);
		}
		vertexCoordinates.resize(vertexCoordinates.size() + C4DynamicParticle::DrawingData::vertexCountPerParticle);
	}
	C4DynamicParticle *newParticle = particles[currentIndex];
	newParticle->drawingData.SetPointer(&vertexCoordinates[currentIndex * C4DynamicParticle::DrawingData::vertexCountPerParticle], true);
	return newParticle;
}

void C4DynamicParticleList::Exec(float timeDelta)
{
	if (particleChunks.empty()) return;

	accessMutex.Enter();

	for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end();)
	{
		if (iter->Exec(targetObject, timeDelta))
		{
			++iter;
		}
		else
		{
			iter = particleChunks.erase(iter);
			lastAccessedChunk = 0;
		}
	}

	accessMutex.Leave();
}

void C4DynamicParticleList::Draw(C4TargetFacet cgo, C4Object *obj)
{
	if (particleChunks.empty()) return;

	//glDisable(GL_DEPTH_TEST);
	//if (additiveBlit)
	//	pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
	pDraw->DeactivateBlitModulation();
	pDraw->ResetBlitMode();
	
	glEnable(GL_TEXTURE_2D);

	if (!DynamicParticles.usePrimitiveRestartIndexWorkaround)
	{
		glPrimitiveRestartIndex(0xffffffff);
		glEnable(GL_PRIMITIVE_RESTART);
	}
	// apply zoom
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(cgo.X, cgo.Y, 0.0f);
	glScalef(cgo.Zoom, cgo.Zoom, 1.0f);
	glTranslatef(-cgo.TargetX, -cgo.TargetY, 0.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	

	accessMutex.Enter();

	for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
	{
		iter->Draw(cgo, obj);
	}

	accessMutex.Leave();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!DynamicParticles.usePrimitiveRestartIndexWorkaround)
	{
		glDisable(GL_PRIMITIVE_RESTART);
	}
	glDisable(GL_TEXTURE_2D);
}

void C4DynamicParticleList::Clear()
{
	accessMutex.Enter();
	particleChunks.clear();
	accessMutex.Leave();
}

C4DynamicParticle *C4DynamicParticleList::AddNewParticle(C4ParticleDef *def, uint32_t blitMode, uint32_t attachment)
{
	accessMutex.Enter();

	// if not cached, find correct chunk in list
	C4DynamicParticleChunk *chunk = 0;
	if (lastAccessedChunk && lastAccessedChunk->IsOfType(def, blitMode, attachment))
		chunk = lastAccessedChunk;
	else
	{
		for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
		{
			if (!iter->IsOfType(def, blitMode, attachment)) continue;
			chunk = &(*iter);
			break;
		}
	}

	// add new chunk?
	if (!chunk)
	{
		particleChunks.push_back(C4DynamicParticleChunk());
		chunk = &particleChunks.back();
		chunk->sourceDefinition = def;
		chunk->blitMode = blitMode;
		chunk->attachment = attachment;
	}

	assert(chunk && "No suitable particle chunk could be found or created.");
	lastAccessedChunk = chunk;

	accessMutex.Leave();
	return chunk->AddNewParticle();
}

void C4DynamicParticleSystem::CalculationThread::Execute()
{
	DynamicParticles.ExecuteCalculation();
}

C4DynamicParticleSystem::C4DynamicParticleSystem() : frameCounterAdvancedEvent(false)
{
	currentSimulationTime = 0;
	globalParticles = 0;
	usePrimitiveRestartIndexWorkaround = false;
}

C4DynamicParticleSystem::~C4DynamicParticleSystem()
{
	calculationThread.SignalStop();
	CalculateNextStep();

	for (std::vector<uint32_t *>::iterator iter = multiDrawElementsIndexArray.begin(); iter != multiDrawElementsIndexArray.end(); ++iter)
		delete (*iter);
}

void C4DynamicParticleSystem::DoInit()
{
#ifndef USE_CONSOLE
	// we use features that are only supported from 3.1 upwards. Check whether the graphics card supports that and - if not - use workarounds
	if (!GLEW_VERSION_3_1 || (&glPrimitiveRestartIndex == 0))
	{
		usePrimitiveRestartIndexWorkaround = true;
		Log("WARNING (particle system): Your graphics card does not support glPrimitiveRestartIndex - a (slower) fallback will be used!");
	}
#endif
}

void C4DynamicParticleSystem::ExecuteCalculation()
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

		for (std::list<C4DynamicParticleList>::iterator iter = particleLists.begin(); iter != particleLists.end(); ++iter)
		{
			iter->Exec(timeDelta);
		}

		particleListAccessMutex.Leave();
	}
}
#endif

C4DynamicParticleList *C4DynamicParticleSystem::GetNewParticleList(C4Object *forObject)
{
#ifdef USE_CONSOLE
	return 0;
#else
	C4DynamicParticleList *newList = 0;

	particleListAccessMutex.Enter();
	particleLists.emplace_back(forObject);
	newList = &particleLists.back();
	particleListAccessMutex.Leave();

	return newList;
#endif
}

void C4DynamicParticleSystem::ReleaseParticleList(C4DynamicParticleList *first, C4DynamicParticleList *second)
{
#ifndef USE_CONSOLE
	particleListAccessMutex.Enter();

	for(std::list<C4DynamicParticleList>::iterator iter = particleLists.begin(); iter != particleLists.end();)
	{
		C4DynamicParticleList *list = &(*iter);
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
void C4DynamicParticleSystem::Create(C4ParticleDef *of_def, C4DynamicParticleValueProvider &x, C4DynamicParticleValueProvider &y, C4DynamicParticleValueProvider &speedX, C4DynamicParticleValueProvider &speedY, C4DynamicParticleValueProvider &lifetime, C4PropList *properties, int amount, C4Object *object)
{
	// todo: check amount etc

	C4DynamicParticleList * pxList(globalParticles);


	// initialize the particle properties
	// this is done here, because it would also be the right place to implement caching
	C4DynamicParticleProperties particleProperties;
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
			pxList = object->DynamicFrontParticles;
		}

		// figure out particle list to use
		if (particleProperties.attachment & C4ATTACH_Front) pxList = object->DynamicFrontParticles;
		else if (particleProperties.attachment & C4ATTACH_Back) pxList = object->DynamicBackParticles;
	}

	// sanity for global particles - might have to be created first
	if (pxList == globalParticles && globalParticles == 0)
	{
		globalParticles = GetNewParticleList();
		pxList = globalParticles;
	}

	while (amount--)
	{
		if (x.IsRandom()) x.RollRandom();
		if (y.IsRandom()) y.RollRandom();
		if (speedX.IsRandom()) speedX.RollRandom();
		if (speedY.IsRandom()) speedY.RollRandom();
		if (lifetime.IsRandom()) lifetime.RollRandom();
		
		// create a particle in the fitting chunk
		C4DynamicParticle *particle = pxList->AddNewParticle(of_def, particleProperties.blitMode, particleProperties.attachment);

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
}

void C4DynamicParticleSystem::PreparePrimitiveRestartIndices(uint32_t forAmount)
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

void C4DynamicParticleSystem::Clear()
{
#ifndef USE_CONSOLE
	currentSimulationTime = 0;

	particleListAccessMutex.Enter();
	particleLists.clear();
	particleListAccessMutex.Leave();
#endif
}

C4DynamicParticleSystem DynamicParticles;
