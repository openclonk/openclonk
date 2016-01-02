/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015, The OpenClonk Team and contributors
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

// Default Vertex Shader for objects and sprites.

uniform mat4 projectionMatrix;
uniform mat4 modelviewMatrix;

attribute vec2 oc_Position;
attribute vec4 oc_Color;

#ifdef OC_HAVE_BASE
attribute vec2 oc_TexCoord;
#endif

varying vec4 vtxColor;

#ifdef OC_HAVE_BASE
varying vec2 texcoord;
#endif

slice(position)
{
	// model-view matrix is always the identity matrix
	gl_Position = projectionMatrix * modelviewMatrix * vec4(oc_Position, 0.0, 1.0);
}

slice(texcoord)
{
#ifdef OC_HAVE_BASE
	texcoord = oc_TexCoord;
#endif
}

slice(color)
{
	vtxColor = oc_Color;
}
