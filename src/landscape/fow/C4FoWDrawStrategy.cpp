/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2016, The OpenClonk Team and contributors
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
#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE

#include "landscape/fow/C4FoWDrawStrategy.h"
#include "landscape/fow/C4FoWLight.h"
#include "landscape/fow/C4FoWRegion.h"
#include "graphics/C4DrawGL.h"

C4FoWDrawTriangulator::C4FoWDrawTriangulator() = default;

void C4FoWDrawTriangulator::Fan()
{
	FinishPrimitive();
	mode = M_Fan;
}

void C4FoWDrawTriangulator::Quads()
{
	FinishPrimitive();
	mode = M_Quads;
}

void C4FoWDrawTriangulator::FinishPrimitive()
{
	// Check the current primitive is complete
	switch (mode)
	{
	case M_Fan:
		assert(cur_vertices == 0 || cur_vertices >= 3);
		break;
	case M_Quads:
		assert(cur_vertices % 4 == 0);
		break;
	default:
		assert(false);
		break;
	}

	// Reset primitve counter
	begin_vertices += cur_vertices;
	cur_vertices = 0;
}

void C4FoWDrawTriangulator::AddVertex()
{
	switch (mode)
	{
	case M_Fan:
		++cur_vertices;
		if (cur_vertices == 3)
		{
			indices.push_back(begin_vertices);
			indices.push_back(begin_vertices + 1);
			indices.push_back(begin_vertices + 2);
		}
		else if (cur_vertices > 3)
		{
			indices.push_back(begin_vertices);
			indices.push_back(begin_vertices + cur_vertices - 2);
			indices.push_back(begin_vertices + cur_vertices - 1);
		}
		break;
	case M_Quads:
		++cur_vertices;
		if (cur_vertices % 4 == 0)
		{
			// upper tri
			indices.push_back(begin_vertices + cur_vertices - 4);
			indices.push_back(begin_vertices + cur_vertices - 3);
			indices.push_back(begin_vertices + cur_vertices - 2);

			// lower tri
			indices.push_back(begin_vertices + cur_vertices - 4);
			indices.push_back(begin_vertices + cur_vertices - 2);
			indices.push_back(begin_vertices + cur_vertices - 1);
		}
		break;
	default:
		assert(false);
	}
}

void C4FoWDrawTriangulator::Reset()
{
	FinishPrimitive();
	begin_vertices = 0;

	// Assume this keeps capacity
	indices.resize(0);
}

C4FoWDrawLightTextureStrategy::C4FoWDrawLightTextureStrategy(const C4FoWLight* light)
 : light(light), region(nullptr), vbo_size(0), ibo_size(0)
{
	bo[0] = bo[1] = 0u;
}

C4FoWDrawLightTextureStrategy::~C4FoWDrawLightTextureStrategy()
{
	if (bo[0])
	{
		glDeleteBuffers(2, bo);
		pGL->FreeVAOID(vaoids[2]);
		pGL->FreeVAOID(vaoids[1]);
		pGL->FreeVAOID(vaoids[0]);
	}
}

void C4FoWDrawLightTextureStrategy::Begin(const C4FoWRegion* regionPar)
{
	region = regionPar;
	if (!bo[0])
	{
		// lazy-init buffer objects
		glGenBuffers(2, bo);
		vaoids[0] = pGL->GenVAOID();
		vaoids[1] = pGL->GenVAOID();
		vaoids[2] = pGL->GenVAOID();
	}
}

void C4FoWDrawLightTextureStrategy::End(C4ShaderCall& call)
{
	// If we have nothing to draw (e.g. directly after initialization), abort early.
	if (vertices.empty()) return;

	const GLuint vbo = bo[0];
	const GLuint ibo = bo[1];

	// Upload vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	if (vbo_size < vertices.size())
	{
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_DYNAMIC_DRAW);
		vbo_size = vertices.size();
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Upload indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	if (ibo_size < triangulator.GetNIndices())
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangulator.GetNIndices() * sizeof(GLuint), triangulator.GetIndices(), GL_DYNAMIC_DRAW);
		ibo_size = triangulator.GetNIndices();
	}
	else
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, triangulator.GetNIndices() * sizeof(GLuint), triangulator.GetIndices());
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Region dimensions
	const float width = region->getSurfaceWidth();
	const float height = region->getSurfaceHeight() / 2.0;

	// Set Y offset for vertex
	float y_offset[] = { 0.0f, 0.0f };
	call.SetUniform2fv(C4FoWRSU_VertexOffset, 1, y_offset);

	// Enable scissor test to only draw in upper or lower half of texture
	glEnable(GL_SCISSOR_TEST);
	glScissor(0, height, width, height);

	// Setup state for 1st pass
	GLuint vao1, vao2, vao3;
	const bool has_vao1 = pGL->GetVAO(vaoids[0], vao1);
	glBindVertexArray(vao1);
	if (!has_vao1)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Position));
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Color));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Position), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, x)));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Color), 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, r1)));
	}

	// Set up blend equation, see C4FoWDrawLightTextureStrategy::DrawVertex
	// for details.
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);

	// Render 1st pass
	glDrawElements(GL_TRIANGLES, triangulator.GetNIndices(), GL_UNSIGNED_INT, nullptr);

	// Prepare state for 2nd pass
	//glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);

	const bool has_vao2 = pGL->GetVAO(vaoids[1], vao2);
	glBindVertexArray(vao2);
	if (!has_vao2)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Position));
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Color));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Position), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, x)));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Color), 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, r2)));
	}
	
	// Render 2nd pass
	glDrawElements(GL_TRIANGLES, triangulator.GetNIndices(), GL_UNSIGNED_INT, nullptr);

	// Prepare state for 3rd pass (color pass)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glScissor(0, 0, width, height);
	y_offset[1] = height;
	call.SetUniform2fv(C4FoWRSU_VertexOffset, 1, y_offset);

	const bool has_vao3 = pGL->GetVAO(vaoids[2], vao3);
	glBindVertexArray(vao3);
	if (!has_vao3)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Position));
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Color));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Position), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, x)));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Color), 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, r3)));
	}
	
	// Render 3rd pass
	glDrawElements(GL_TRIANGLES, triangulator.GetNIndices(), GL_UNSIGNED_INT, nullptr);

	// Reset GL state
	glBindVertexArray(0);
	glDisable(GL_SCISSOR_TEST);

	// Assume the capacity stays the same:
	vertices.resize(0);
	C4FoWDrawStrategy::End(call);
}

void C4FoWDrawLightTextureStrategy::DrawVertex(float x, float y, bool shadow)
{

	// Here's the master plan for updating the lights texture. We want to
	//
	//  1. sum up the normals, weighted by intensity
	//  2. take over intensity maximum as new intensity
	//
	// where intensity is in the A channel and normals are in the GB channels.
	// Normals are obviously meant to be though of as signed, though,
	// so the equation we want would be something like
	//
	//  A_new = max(A_old, A);
	//  G_new = BoundBy(G_old + G - 0.5, 0.0, 1.0);
	//  B_new = BoundBy(B_old + B - 0.5, 0.0, 1.0);
	//
	// It seems we can't get that directly though - glBlendFunc only talks
	// about two operands. Even if we make two passes, we have to take
	// care that that we don't over- or underflow in the intermediate pass.
	//
	// Therefore, we store G/1.5 instead of G, losing a bit of accuracy,
	// but allowing us to formulate the following approximation without
	// overflows:
	//
	//  G_new = BoundBy(BoundBy(G_old + G / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)
	//  B_new = BoundBy(BoundBy(B_old + B / 1.5), 0.0, 1.0) - 0.5 / 1.5, 0.0, 1.0)

	vertices.emplace_back();
	Vertex& vtx = vertices.back();
	vtx.x = x; vtx.y = y;

	// global coords -> region coords
	// TODO: We could also do this in the shader...
	vtx.x += -region->getRegion().x;
	vtx.y += -region->getRegion().y;

	// First pass color
	if (shadow)
	{
		float dx = x - light->getX();
		float dy = y - light->getY();
		float dist = sqrt(dx*dx+dy*dy);
		float bright = light->getBrightness();
		float mult = std::min(0.5f / light->getNormalSize(), 0.5f / dist);
		float normX = Clamp(0.5f + dx * mult, 0.0f, 1.0f) / 1.5f;
		float normY = Clamp(0.5f + dy * mult, 0.0f, 1.0f) / 1.5f;
		vtx.r1 = 0.0f; vtx.g1 = normX; vtx.b1 = normY; vtx.a1 = bright;
	}
	else
	{
		vtx.r1 = 0.0f; vtx.g1 = 0.5f / 1.5f; vtx.b1 = 0.5f / 1.5f; vtx.a1 = 0.0f;
	}

	// Second pass color
	vtx.r2 = 0.0f; vtx.g2 = 0.5f / 1.5f; vtx.b2 = 0.5f / 1.5f; vtx.a2 = 0.0f;

	// Third pass color
	float alpha; // 0.0 == fully transparent (takes old color), 1.0 == solid color (takes new color)
	if (shadow)
	{
		alpha = 0.3 + 0.6 * light->getValue() * light->getLightness();
	}
	else // draw the edge of the light
	{
		alpha = 0.0;
	}

	vtx.r3 = light->getR();
	vtx.g3 = light->getG();
	vtx.b3 = light->getB();
	vtx.a3 = alpha;
}

void C4FoWDrawLightTextureStrategy::DrawDarkVertex(float x, float y)
{
	DrawVertex(x,y, false);
	C4FoWDrawStrategy::DrawDarkVertex(x, y);
}

void C4FoWDrawLightTextureStrategy::DrawLightVertex(float x, float y)
{
	DrawVertex(x,y, true);
	C4FoWDrawStrategy::DrawLightVertex(x, y);
}

C4FoWDrawWireframeStrategy::C4FoWDrawWireframeStrategy(const C4FoWLight* light, const C4TargetFacet *screen) :
  screen(screen), vbo_size(0), ibo_size(0)
{
	glGenBuffers(2, bo);
	vaoid = pGL->GenVAOID();
}

C4FoWDrawWireframeStrategy::~C4FoWDrawWireframeStrategy()
{
	glDeleteBuffers(2, bo);
	pGL->FreeVAOID(vaoid);
}

void C4FoWDrawWireframeStrategy::Begin(const C4FoWRegion* region)
{
}

void C4FoWDrawWireframeStrategy::End(C4ShaderCall& call)
{
	// If we have nothing to draw (e.g. directly after initialization), abort early.
	if (vertices.empty()) return;

	const GLuint vbo = bo[0];
	const GLuint ibo = bo[1];

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Upload vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	if (vbo_size < vertices.size())
	{
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STREAM_DRAW);
		vbo_size = vertices.size();
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Upload indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	if (ibo_size < triangulator.GetNIndices())
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangulator.GetNIndices() * sizeof(GLuint), triangulator.GetIndices(), GL_STREAM_DRAW);
		ibo_size = triangulator.GetNIndices();
	}
	else
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, triangulator.GetNIndices() * sizeof(GLuint), triangulator.GetIndices());
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLuint vao;
	const bool has_vao = pGL->GetVAO(vaoid, vao);
	glBindVertexArray(vao);
	if (!has_vao)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Position));
		glEnableVertexAttribArray(call.GetAttribute(C4FoWRSA_Color));

		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Position), 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, x)));
		glVertexAttribPointer(call.GetAttribute(C4FoWRSA_Color), 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const uint8_t*>(offsetof(Vertex, r)));
	}

	// Set Y offset for vertex
	const float y_offset[] = { 0.0f, 0.0f };
	call.SetUniform2fv(C4FoWRSU_VertexOffset, 1, y_offset);

	glDrawElements(GL_TRIANGLES, triangulator.GetNIndices(), GL_UNSIGNED_INT, nullptr);

	// Reset GL state
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Assume the capacity stays the same:
	vertices.resize(0);
	C4FoWDrawStrategy::End(call);
}

void C4FoWDrawWireframeStrategy::DrawVertex(Vertex& vtx)
{
	// global coords -> screen pos and zoom
	// TODO: We could do this in the shader...
	vtx.x += screen->X - screen->TargetX;
	vtx.y += screen->Y - screen->TargetY;
	pGL->ApplyZoom(vtx.x, vtx.y);
}

void C4FoWDrawWireframeStrategy::DrawDarkVertex(float x, float y)
{
	vertices.emplace_back();
	Vertex& vtx = vertices.back();
	vtx.x = x; vtx.y = y;

	switch(phase)
	{
	case P_None:         return;
	case P_Fade:         vtx.r = 0.0f; vtx.g = 0.5f; vtx.b = 0.0f; vtx.a = 1.0f; break;
	case P_Intermediate: vtx.r = 0.0f; vtx.g = 0.0f; vtx.b = 0.5f; vtx.a = 1.0f; break;
	default:             assert(false); // only fade has dark vertices
	}

	DrawVertex(vtx);
	C4FoWDrawStrategy::DrawDarkVertex(x, y);
}

void C4FoWDrawWireframeStrategy::DrawLightVertex(float x, float y)
{
	vertices.emplace_back();
	Vertex& vtx = vertices.back();
	vtx.x = x; vtx.y = y;

	switch(phase)
	{
	case P_None:         return;
	case P_Fan:          vtx.r = 1.0f; vtx.g = 0.0f; vtx.b = 0.0f; vtx.a = 1.0f; break;
	case P_FanMaxed:     vtx.r = 1.0f; vtx.g = 1.0f; vtx.b = 0.0f; vtx.a = 1.0f; break;
	case P_Fade:         vtx.r = 0.0f; vtx.g = 1.0f; vtx.b = 0.0f; vtx.a = 1.0f; break;
	case P_Intermediate: vtx.r = 0.0f; vtx.g = 0.0f; vtx.b = 1.0f; vtx.a = 1.0f; break;
	default:             assert(false);
	}

	DrawVertex(vtx);
	C4FoWDrawStrategy::DrawLightVertex(x, y);
}

#endif
