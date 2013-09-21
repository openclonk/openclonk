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

const int C4DynamicParticle::DrawingData::vertexCountPerParticle(4);


void C4DynamicParticleValueProvider::Floatify(float denominator)
{
	assert (denominator != 0.f && "Trying to floatify C4DynamicParticleValueProvider with denominator of 0");
	startValue /= denominator;
	endValue /= denominator;

	if (valueFunction == &C4DynamicParticleValueProvider::Random)
		RollRandom();
}

void C4DynamicParticleValueProvider::RollRandom()
{
	currentValue = startValue + SafeRandom((int)(1000.f * (endValue - startValue)) / 1000.0f);
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
	default:
		assert(false);
	};

	if (what != C4PV_Const)
	{
		isConstant = false;
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
			Set((float)(*fromArray)[1].getInt(), (float)(*fromArray)[2].getInt(), C4PV_Linear);
		break;
	case C4PV_Random:
		if (arraySize >= 4)
		{
			Set((float)(*fromArray)[1].getInt(), (float)(*fromArray)[2].getInt(), C4PV_Random);
			rerollInterval = (*fromArray)[3].getInt();
			RollRandom();
			
		}
	default:
		break;
	}
}

C4DynamicParticleProperties::C4DynamicParticleProperties()
{
	// all values in pre-floatified range (f.e. 0..255 instead of 0..1)
	size.Set(8.f);
	forceX.Set(0.f);
	forceY.Set(0.f);
	speedDampingX.Set(1000.f);
	speedDampingY.Set(1000.f);
	colorR.Set(255.f);
	colorG.Set(255.f);
	colorB.Set(255.f);
	colorAlpha.Set(255.f);
	hasConstantColor = false;
}

void C4DynamicParticleProperties::Floatify()
{
	forceX.Floatify(10.f);
	forceY.Floatify(10.f);
	speedDampingX.Floatify(1000.f);
	speedDampingY.Floatify(1000.f);
	colorR.Floatify(255.f);
	colorG.Floatify(255.f);
	colorB.Floatify(255.f);
	colorAlpha.Floatify(255.f);

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
	}
}

void C4DynamicParticle::Init()
{
	currentSpeedX = currentSpeedX = 0.f;
	positionX = positionY = 0.f;
	lifetime = startingLifetime = 5.f * 38.f;
}

bool C4DynamicParticle::Exec(C4Object *obj)
{
	// die of old age? :<
	lifetime -= 1.f;
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

		currentSpeedX *= currentDampingX;
		currentSpeedY *= currentDampingY;

		// todo: collision check
		
		positionX += currentSpeedX;
		positionY += currentSpeedY;
		drawingData.SetPosition(positionX, positionY, properties.size.GetValue(this));
		
	}
	else if(!properties.size.IsConstant())
	{
		drawingData.SetPosition(positionX, positionY, properties.size.GetValue(this));
	}

	// adjust color
	if (!properties.hasConstantColor)
	{
		drawingData.SetColor(properties.colorR.GetValue(this), properties.colorG.GetValue(this), properties.colorB.GetValue(this), properties.colorAlpha.GetValue(this));
	}
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

bool C4DynamicParticleChunk::Exec(C4Object *obj)
{
	for (int i = 0; i < particleCount; ++i)
	{
		if (!particles[i]->Exec(obj))
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

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureRef->texName);

	glVertexPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].x));
	glTexCoordPointer(2, GL_FLOAT, stride, &(vertexCoordinates[0].u));
	glColorPointer(4, GL_FLOAT, stride, &(vertexCoordinates[0].r));
	glDrawElements(GL_TRIANGLE_STRIP, 5 * particleCount, GL_UNSIGNED_INT, ::DynamicParticles.GetPrimitiveRestartArray());
}

bool C4DynamicParticleChunk::IsOfType(C4ParticleDef *def)
{
	return def == sourceDefinition;
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

void C4DynamicParticleList::Exec(C4Object *obj)
{
	for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end();)
	{
		if (iter->Exec(obj))
		{
			++iter;
		}
		else
		{
			iter = particleChunks.erase(iter);
		}
	}
}

void C4DynamicParticleList::Draw(C4TargetFacet cgo, C4Object *obj)
{
	if (particleChunks.empty()) return;

	//glDisable(GL_DEPTH_TEST);
	//if (additiveBlit)
	//	pDraw->SetBlitMode(C4GFXBLIT_ADDITIVE);
	pDraw->DeactivateBlitModulation();
	pDraw->ResetBlitMode();
	pDraw->SetTexture();
	
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

	for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
	{
		iter->Draw(cgo, obj);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_PRIMITIVE_RESTART);

	//pDraw->ResetBlitMode();
	pDraw->ResetTexture();
}

C4DynamicParticle *C4DynamicParticleList::AddNewParticle(C4ParticleDef *def, bool additive)
{
	// if not cached, find correct chunk in list
	C4DynamicParticleChunk *chunk = 0;
	if (lastAccessedChunk && lastAccessedChunk->IsOfType(def))
		chunk = lastAccessedChunk;
	else
	{
		for (std::list<C4DynamicParticleChunk>::iterator iter = particleChunks.begin(); iter != particleChunks.end(); ++iter)
		{
			if (!iter->IsOfType(def)) continue;
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
		chunk->additiveBlit = additive;
	}

	assert(chunk && "No suitable particle chunk could be found or created.");
	lastAccessedChunk = chunk;
	return chunk->AddNewParticle();
}

C4DynamicParticle *C4DynamicParticleSystem::Create(C4ParticleDef *of_def, float x, float y, C4DynamicParticleValueProvider speedX, C4DynamicParticleValueProvider speedY, C4DynamicParticleValueProvider size, float lifetime, C4PropList *properties, C4DynamicParticleList *pxList, C4Object *object)
{
	// todo: check amount etc

	if (!pxList) 
		pxList = &globalParticles;

	C4DynamicParticle *particle = pxList->AddNewParticle(of_def, true);
	particle->properties.Set(properties);
	if (!(size.IsConstant() && size.GetValue(particle) == 0.f))
		particle->properties.size = size;
	particle->properties.Floatify();

	particle->lifetime = particle->startingLifetime = lifetime;
	particle->SetPosition(x, y);
	particle->currentSpeedX = speedX.GetValue(particle);
	particle->currentSpeedY = speedY.GetValue(particle);

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

C4DynamicParticleSystem DynamicParticles;
