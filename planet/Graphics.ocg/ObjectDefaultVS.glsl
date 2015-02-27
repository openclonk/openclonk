varying vec3 normalDir;
uniform mat4 bones[128];

// For more performance, this should be set by the engine, and this shader
// should be compiled three times: with BONE_COUNT set to 0, 4, and 8,
// respectively. (Or we could split it even further.)
#define BONE_COUNT 8

attribute vec4 oc_BoneIndices0;
attribute vec4 oc_BoneWeights0;

#if BONE_COUNT > 4
attribute vec4 oc_BoneIndices1;
attribute vec4 oc_BoneWeights1;
#endif

vec4 merge_bone(vec4 vertex, vec4 original, mat4 bone, float weight)
{
	return (bone * original) * weight + vertex;
}

slice(position)
{
#if BONE_COUNT == 0
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
#else
	vec4 vertex = vec4(0, 0, 0, 0);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices0.x)], oc_BoneWeights0.x);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices0.y)], oc_BoneWeights0.y);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices0.z)], oc_BoneWeights0.z);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices0.w)], oc_BoneWeights0.w);
#if BONE_COUNT > 4
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices1.x)], oc_BoneWeights1.x);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices1.y)], oc_BoneWeights1.y);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices1.z)], oc_BoneWeights1.z);
	vertex = merge_bone(vertex, gl_Vertex, bones[int(oc_BoneIndices1.w)], oc_BoneWeights1.w);
#endif
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
#endif
}

slice(texcoord)
{
	texcoord = gl_MultiTexCoord0.st;
}

slice(normal)
{
#if BONE_COUNT == 0
	normalDir = normalize(gl_NormalMatrix * gl_Normal);
#else
	vec4 base_normal = vec4(gl_Normal, 0.0);
	vec4 normal = base_normal;
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
	normalDir = normalize(gl_NormalMatrix * normal.xyz);
#endif
}
