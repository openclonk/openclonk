/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2015, The OpenClonk Team and contributors
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
#include "C4FoWDrawStrategy.h"
#include "C4FoWLight.h"
#include "C4FoWRegion.h"
#include "C4DrawGL.h"


void C4FoWDrawLightTextureStrategy::Begin(int32_t passPar)
{
	pass = passPar;

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	glBlendFunc(GL_ONE, GL_ONE);
	if(pass == 0)
	{
		glBlendEquation( GL_FUNC_ADD );
	}
	else if(pass == 1)
	{
		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	}

}

void C4FoWDrawLightTextureStrategy::End(int32_t pass)
{
	glBlendEquation( GL_FUNC_ADD );
}

void C4FoWDrawLightTextureStrategy::DrawVertex(float x, float y, bool shadeLight)
{
	if(pass == 0)
	{
		float dx = x - light->getX();
		float dy = y - light->getY();
		float dist = sqrt(dx*dx+dy*dy);
		float mult = Min(0.5f / light->getNormalSize(), 0.5f / dist);
		float normX = Clamp(0.5f + dx * mult, 0.0f, 1.0f) / 1.5f;
		float normY = Clamp(0.5f + dy * mult, 0.0f, 1.0f) / 1.5f;
		if(shadeLight)  glColor3f(0.5f, normX, normY);
		else            glColor3f(0.0f, 0.5f/1.5f, 0.5f/1.5f);
	}
	else
	{
		glColor3f(0.0f, 0.5f/1.5f, 0.5f/1.5f);
	}

	// global coords -> region coords
	x += -region->getRegion().x;
	y += -region->getRegion().y;

	glVertex2f(x,y);
}

void C4FoWDrawLightTextureStrategy::DrawDarkVertex(float x, float y)
{
	DrawVertex(x,y, false);
}

void C4FoWDrawLightTextureStrategy::DrawLightVertex(float x, float y)
{
	DrawVertex(x,y, true);
}

void C4FoWDrawWireframeStrategy::Begin(int32_t pass)
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

void C4FoWDrawWireframeStrategy::End(int32_t pass)
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glBlendEquation( GL_FUNC_ADD );
}

void C4FoWDrawWireframeStrategy::DrawVertex(float x, float y)
{
	// global coords -> screen pos and zoom
	x += screen->X - screen->TargetX;
	y += screen->Y - screen->TargetY;
	pGL->ApplyZoom(x,y);
	glVertex2f(x,y);
}

void C4FoWDrawWireframeStrategy::DrawDarkVertex(float x, float y)
{
	if(!draw) return;
	glColor3f(0.5f, 0.5f, 0.0f);
	DrawVertex(x, y);
}

void C4FoWDrawWireframeStrategy::DrawLightVertex(float x, float y)
{
	if(!draw) return;
	glColor3f(1.0f, 0.0f, 0.0f);
	DrawVertex(x, y);
}

