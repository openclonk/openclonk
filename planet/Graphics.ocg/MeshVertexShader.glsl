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

// Default Vertex Shader for mesh-based objects.

// Input uniforms, if OC_WA_LOW_MAX_VERTEX_UNIFORM_COMPONENTS is NOT defined:
//   bones: array of 4x3 bone transformation matrices.

// Input uniforms, if OC_WA_LOW_MAX_VERTEX_UNIFORM_COMPONENTS is defined:
//   bones: array of 3x4 transposed bone transformation matrices.

// Input vertex attributes:
//   oc_BoneWeights0 and oc_BoneWeight1: vectors of bone influence weights.
//     The sum of weights must be 1. Each component of these vectors will be
//     matched up with its corresponding component of the oc_BoneIndices0 or
//     oc_BoneIndices1 vectors to specify a vertex bone assignment.
//  oc_BoneIndices0 and oc_BoneIndices1: vectors of bone influence indices.
//    The integer part of every component of these vectors selects one of the
//    bones from the bone matrix array to influence the current vertex.

// If the vertex is not influenced by any bones, it must use a dummy entry
// inside the bone matrix array that contains the identity matrix, with a bone
// weight of 1.0.

#define MAX_BONE_COUNT 80

in vec3 oc_Position;
in vec3 oc_Normal;
in vec2 oc_TexCoord;

out vec3 vtxNormal;
out vec2 texcoord;

uniform mat4 projectionMatrix;
uniform mat4 modelviewMatrix;
uniform mat3 normalMatrix;

#ifndef OC_WA_LOW_MAX_VERTEX_UNIFORM_COMPONENTS
uniform mat4x3 bones[MAX_BONE_COUNT];
#else
uniform mat3x4 bones[MAX_BONE_COUNT];
#endif

// For more performance, this should be set by the engine, and this shader
// should be compiled three times: with BONE_COUNT set to 0, 4, and 8,
// respectively. (Or we could split it even further.)
#ifndef OC_WA_FORCE_SOFTWARE_TRANSFORM
#define BONE_COUNT 8
#else
#define BONE_COUNT 0
#endif

in vec4 oc_BoneIndices0;
in vec4 oc_BoneWeights0;

#if BONE_COUNT > 4
in vec4 oc_BoneIndices1;
in vec4 oc_BoneWeights1;
#endif

#ifndef OC_WA_LOW_MAX_VERTEX_UNIFORM_COMPONENTS
vec4 merge_bone(vec4 vertex, vec4 original, mat4x3 bone, float weight)
{
	return (mat4(bone) * original) * weight + vertex;
}
#else
vec4 merge_bone(vec4 vertex, vec4 original, mat3x4 bone, float weight)
{
	return (mat4(transpose(bone)) * original) * weight + vertex;
}
#endif

slice(position)
{
	vec4 origVertex = vec4(oc_Position, 1.0);

#if BONE_COUNT == 0
	gl_Position = projectionMatrix * modelviewMatrix * origVertex;
#else
	vec4 vertex = vec4(0, 0, 0, 0);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices0.x)], oc_BoneWeights0.x);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices0.y)], oc_BoneWeights0.y);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices0.z)], oc_BoneWeights0.z);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices0.w)], oc_BoneWeights0.w);
#if BONE_COUNT > 4
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices1.x)], oc_BoneWeights1.x);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices1.y)], oc_BoneWeights1.y);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices1.z)], oc_BoneWeights1.z);
	vertex = merge_bone(vertex, origVertex, bones[int(oc_BoneIndices1.w)], oc_BoneWeights1.w);
#endif
	gl_Position = projectionMatrix * modelviewMatrix * vertex;
#endif
}

slice(texcoord)
{
	texcoord = oc_TexCoord;
}

slice(normal)
{
#if BONE_COUNT == 0
	vtxNormal = normalize(normalMatrix * oc_Normal);
#else
	vec4 base_normal = vec4(oc_Normal, 0.0);
	vec4 normal = vec4(0, 0, 0, 0);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices0.x)], oc_BoneWeights0.x);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices0.y)], oc_BoneWeights0.y);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices0.z)], oc_BoneWeights0.z);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices0.w)], oc_BoneWeights0.w);
#if BONE_COUNT > 4
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices1.x)], oc_BoneWeights1.x);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices1.y)], oc_BoneWeights1.y);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices1.z)], oc_BoneWeights1.z);
	normal = merge_bone(normal, base_normal, bones[int(oc_BoneIndices1.w)], oc_BoneWeights1.w);
#endif
	vtxNormal = normalize(normalMatrix * normal.xyz);
#endif
}
