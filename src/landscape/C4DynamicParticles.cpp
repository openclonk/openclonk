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

#include <C4Value.h>
#include <C4ValueArray.h>
#include <C4MeshAnimation.h>
#include <C4DrawGL.h>
#include <C4Random.h>
#include <C4Landscape.h>

const int C4DynamicParticle::DrawingData::vertexCountPerParticle(4);

void C4DynamicParticle::DrawingData::SetPosition(float x, float y, float size, float rotation, float stretch)
{
	if (size != originalSize || stretch != currentStretch)
	{
		currentStretch = stretch;
		originalSize = size;
		sizeX = originalSize / aspect;
		sizeY = originalSize * currentStretch;
	}

	if (rotation == 0.f)
	{
		vertices[0].x = x - sizeX;
		vertices[0].y = y + sizeY;
		vertices[1].x = x - sizeX;
		vertices[1].y = y - sizeY;
		vertices[2].x = x + sizeX;
		vertices[2].y = y + sizeY;
		vertices[3].x = x + sizeX;
		vertices[3].y = y - sizeY;
	}
	else
	{
		float sine = sinf(rotation);
		float cosine = cosf(rotation);

		vertices[0].x = x + ((-sizeX) * cosine - (+sizeY) * sine);
		vertices[0].y = y + ((-sizeX) *   sine + (+sizeY) * cosine);
		vertices[1].x = x + ((-sizeX) * cosine - (-sizeY) * sine);
		vertices[1].y = y + ((-sizeX) *   sine + (-sizeY) * cosine);
		vertices[2].x = x + ((+sizeX) * cosine - (+sizeY) * sine);
		vertices[2].y = y + ((+sizeX) *   sine + (+sizeY) * cosine);
		vertices[3].x = x + ((+sizeX) * cosine - (-sizeY) * sine);
		vertices[3].y = y + ((+sizeX) *   sine + (-sizeY) * cosine);
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

	if (other.keyFrames != 0)
	{
		int size = keyFrameCount * 2 * sizeof(float);
		keyFrames = (float*) malloc(size);
		memcpy(keyFrames, other.keyFrames, size);
	}
	else
		keyFrames = 0;
	return (*this);
}

void C4DynamicParticleValueProvider::Floatify(float denominator)
{
	assert (denominator != 0.f && "Trying to floatify C4DynamicParticleValueProvider with denominator of 0");

	if (valueFunction == &C4DynamicParticleValueProvider::Direction)
	{
		startValue /= 1000.f;
		return;
	}

	startValue /= denominator;
	endValue /= denominator;
	currentValue /= denominator;

	// special treatment for keyframes
	if (valueFunction == &C4DynamicParticleValueProvider::KeyFrames)
	{
		for (int i = 0; i < keyFrameCount; ++i)
		{
			keyFrames[2 * i] /= 1000.0f; // even numbers are the time values
			keyFrames[2 * i + 1] /= denominator; // odd numbers are the actual values
			//LogF("KF is %f @ %f", keyFrames[2 * i + 1], keyFrames[2 * i]);
		}
	}
	else
		if (valueFunction == &C4DynamicParticleValueProvider::Random)
			RollRandom();
}

void C4DynamicParticleValueProvider::RollRandom()
{
	int range = (int)(100.f * (endValue - startValue));
	int randomValue = SafeRandom(range);
	currentValue = startValue + (float)randomValue / 100.0f;
}

float C4DynamicParticleValueProvider::GetValue(C4DynamicParticle *forParticle)
{
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
	if (rerollInterval != 0 && ((int)forParticle->GetAge() % rerollInterval == 0))
		RollRandom();
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
	if (smoothing == 0) // linear
	{
		for (int i = 0; i < keyFrameCount; ++i)
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

void C4DynamicParticleValueProvider::Set(float _startValue, float _endValue, C4ParticleValueProviderID what)
{
	startValue = _startValue;
	endValue = _endValue;

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
	default:
		assert(false);
	};

	if (what != C4PV_Const)
	{
		isConstant = false;
	}
}

void C4DynamicParticleValueProvider::Set(C4ValueArray *fromArray)
{
	startValue = endValue = 1.0f;
	valueFunction = &C4DynamicParticleValueProvider::Const;
	if (!fromArray) return;
	int arraySize = fromArray->GetSize();
	if (arraySize < 2) return;

	int type = (*fromArray)[0].getInt();

	switch (type)
	{
	case C4PV_Const:
		if (arraySize >= 2)
			Set((float)(*fromArray)[1].getInt(), 0.f, C4PV_Const);
		break;

	case C4PV_Linear:
		if (arraySize >= 3)
		{
			Set((float)(*fromArray)[1].getInt(), (float)(*fromArray)[2].getInt(), C4PV_Linear);
		}
		break;
	case C4PV_Random:
		if (arraySize >= 3)
		{
			Set((float)(*fromArray)[1].getInt(), (float)(*fromArray)[2].getInt(), C4PV_Random);
			if (arraySize >= 4)
				rerollInterval = (*fromArray)[3].getInt();
			RollRandom();
		}
		break;
	case C4PV_Direction:
		if (arraySize >= 2)
		{
			Set((float)(*fromArray)[1].getInt(), 0.f, C4PV_Direction);
		}
		break;
	case C4PV_Step:
		if (arraySize >= 2)
		{
			Set((float)(*fromArray)[1].getInt(), 0.f, C4PV_Step);
			currentValue = (float)(*fromArray)[2].getInt();
			delay = (float)(*fromArray)[3].getInt();
			if (delay == 0.f) delay = 1.f;
		}
		break;
	case C4PV_KeyFrames:
		if (arraySize >= 5)
		{
			smoothing = (*fromArray)[1].getInt();
			keyFrames = (float*) malloc(sizeof(float) * (arraySize + 3)); // 2 additional information floats at the beginning and ending
			keyFrameCount = 0;
			const int startingOffset = 2;
			int i = startingOffset;
			for (; i < arraySize; ++i)
			{
				keyFrames[2 + i - startingOffset] = (float)(*fromArray)[i].getInt();
			}
			keyFrameCount = (i - startingOffset) / 2 + 2;
			Set(keyFrames[2 + 1], keyFrames[2 * keyFrameCount - 1], C4PV_KeyFrames);

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
			Set((float)(*fromArray)[2].getInt(), 0.f, C4PV_Speed);
			speedFactor = (float)(*fromArray)[1].getInt() / 1000.f;
		}
		break;
	default:
		break;
	}
}

void C4DynamicParticleValueProvider::Set(const C4Value &value)
{
	C4ValueArray *valueArray;
	if (valueArray = value.getArray())
	{
		Set(valueArray);
		return;
	}

	int32_t valueInt = value.getInt();

	Set((float)valueInt, (float)valueInt, C4PV_Const); 
}

C4DynamicParticleProperties::C4DynamicParticleProperties()
{
	blitMode = 0;
	hasConstantColor = false;
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
	// make sure to delete the array later, we took ownership
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
			blitMode = property.getInt();
		}
		else if(&Strings.P[P_Phase] == key)
		{
			phase.Set(property);
		}
		else if(&Strings.P[P_CollisionVertex] == key)
		{
			collisionVertex.Set(property);
		}
		else if(&Strings.P[P_OnCollision] == key)
		{
			SetCollisionFunc(property);
		}
	}
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

void C4DynamicParticle::Init()
{
	currentSpeedX = currentSpeedX = 0.f;
	positionX = positionY = 0.f;
	lifetime = startingLifetime = 5.f * 38.f;
}

bool C4DynamicParticle::Exec(C4Object *obj, float timeDelta, C4ParticleDef *sourceDef)
{
	// die of old age? :<
	lifetime -= timeDelta;
	if (lifetime <= 0.f) return false;

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
		float collisionPoint = properties.collisionVertex.GetValue(this);
		if (collisionPoint >= 0.001f && GBackSolid(positionX + timeDelta * currentSpeedX * collisionPoint * (1.f + size), positionY + timeDelta * currentSpeedY * collisionPoint * (1.f + size)))
		{
			// exec collision func
			if (properties.collisionCallback != 0 && !(properties.*properties.collisionCallback)(this)) return false;
		}
		else
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
	for (int i = 0; i < particleCount; ++i)
	{
		delete particles[i];
	}
	particleCount = 0;
	particles.clear();
	vertexCoordinates.clear();
}

void C4DynamicParticleChunk::ReplaceParticle(int indexTo, int indexFrom)
{
	C4DynamicParticle *oldParticle = particles[indexTo];

	if (indexFrom != indexTo) // false when "replacing" the last one
	{
		memcpy(&vertexCoordinates[indexTo * C4DynamicParticle::DrawingData::vertexCountPerParticle], &vertexCoordinates[indexFrom * C4DynamicParticle::DrawingData::vertexCountPerParticle], sizeof(C4DynamicParticle::DrawingData::Vertex) * C4DynamicParticle::DrawingData::vertexCountPerParticle);
		particles[indexTo] = particles[indexFrom];
		particles[indexTo]->drawingData.SetPointer(&vertexCoordinates[indexTo * C4DynamicParticle::DrawingData::vertexCountPerParticle]);
	}

	delete oldParticle;
}

bool C4DynamicParticleChunk::Exec(C4Object *obj, float timeDelta)
{
	for (int i = 0; i < particleCount; ++i)
	{
		if (!particles[i]->Exec(obj, timeDelta, sourceDefinition))
		{
			ReplaceParticle(i, particleCount - 1);
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

	glBlendFunc(GL_SRC_ALPHA, (blitMode & C4GFXBLIT_ADDITIVE) ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureRef->texName);

	glVertexPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].x));
	glTexCoordPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].u));
	glColorPointer(4, GL_FLOAT, stride, &(vertexCoordinates[0].r));
	glDrawElements(GL_TRIANGLE_STRIP, 5 * particleCount, GL_UNSIGNED_INT, ::DynamicParticles.GetPrimitiveRestartArray());
}

bool C4DynamicParticleChunk::IsOfType(C4ParticleDef *def, int _blitMode)
{
	return def == sourceDefinition && blitMode == _blitMode;
}

C4DynamicParticle *C4DynamicParticleChunk::AddNewParticle()
{
	int currentIndex = particleCount++;
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
			for (int i = 0; i < currentIndex; ++i)
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

	glPrimitiveRestartIndex(0xffffffff);

	// apply zoom
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(cgo.X, cgo.Y, 0.0f);
	glScalef(cgo.Zoom, cgo.Zoom, 1.0f);
	glTranslatef(-cgo.TargetX, -cgo.TargetY, 0.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnable(GL_PRIMITIVE_RESTART);

	accessMutex.Enter();

	for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
	{
		iter->Draw(cgo, obj);
	}

	accessMutex.Leave();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_PRIMITIVE_RESTART);

	glDisable(GL_TEXTURE_2D);
}

void C4DynamicParticleList::Clear()
{
	accessMutex.Enter();
	particleChunks.clear();
	accessMutex.Leave();
}

C4DynamicParticle *C4DynamicParticleList::AddNewParticle(C4ParticleDef *def, int blitMode)
{
	accessMutex.Enter();

	// if not cached, find correct chunk in list
	C4DynamicParticleChunk *chunk = 0;
	if (lastAccessedChunk && lastAccessedChunk->IsOfType(def, blitMode))
		chunk = lastAccessedChunk;
	else
	{
		for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
		{
			if (!iter->IsOfType(def, blitMode)) continue;
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

void C4DynamicParticleSystem::ExecuteCalculation()
{
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
	Sleep(1000 / 38);
}

C4DynamicParticleList *C4DynamicParticleSystem::GetNewParticleList(C4Object *forObject)
{
	C4DynamicParticleList *newList = 0;

	particleListAccessMutex.Enter();
	particleLists.push_back(C4DynamicParticleList(forObject));
	newList = &particleLists.back();
	particleListAccessMutex.Leave();

	return newList;
}

void C4DynamicParticleSystem::ReleaseParticleList(C4DynamicParticleList *first, C4DynamicParticleList *second)
{
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
}

C4DynamicParticle *C4DynamicParticleSystem::Create(C4ParticleDef *of_def, float x, float y, C4DynamicParticleValueProvider speedX, C4DynamicParticleValueProvider speedY, C4DynamicParticleValueProvider size, float lifetime, C4PropList *properties, C4DynamicParticleList *pxList, C4Object *object)
{
	// todo: check amount etc

	if (!pxList) 
	{
		pxList = globalParticles;
	}

	if (pxList == globalParticles && globalParticles == 0)
	{
		globalParticles = GetNewParticleList();
		pxList = globalParticles;
	}

	C4DynamicParticleProperties particleProperties;
	particleProperties.Set(properties);

	C4DynamicParticle *particle = pxList->AddNewParticle(of_def, particleProperties.blitMode);
	particle->properties = particleProperties;
	if (!(size.IsConstant() && size.GetValue(particle) == 0.f))
		particle->properties.size = size;
	particle->properties.Floatify();

	particle->lifetime = particle->startingLifetime = lifetime;
	particle->currentSpeedX = speedX.GetValue(particle);
	particle->currentSpeedY = speedY.GetValue(particle);
	particle->drawingData.aspect = of_def->Aspect;
	particle->SetPosition(x, y);

	return particle;
}

void C4DynamicParticleSystem::PreparePrimitiveRestartIndices(int forAmount)
{
	const uint32_t PRI = 0xffffffff;
	int neededAmount = 5 * forAmount;

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
		int oldSize = primitiveRestartIndices.size();
		primitiveRestartIndices.resize(neededAmount);

		for (int i = oldSize; i < neededAmount; ++i)
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

void C4DynamicParticleSystem::Clear()
{
	currentSimulationTime = 0;

	particleListAccessMutex.Enter();
	particleLists.clear();
	particleListAccessMutex.Leave();
}

C4DynamicParticleSystem DynamicParticles;
