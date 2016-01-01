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

// Default Vertex Shader for the landscape.

uniform mat4 projectionMatrix;

slice(position)
{
	// model-view matrix is always the identity matrix
	gl_Position = projectionMatrix * gl_Vertex;
}
