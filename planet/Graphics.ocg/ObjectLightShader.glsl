uniform mat3x2 lightTransform;
#ifdef HAVE_NORMALMAP
uniform sampler2D normalTex;
#endif

#ifdef MESH
varying vec3 normalDir;
#endif

slice(texture+4)
{
#ifdef HAVE_LIGHT
	// prepare texture coordinate for light lookup in LightShader.c
	// Extra .xy since some old intel drivers return a vec3
	vec2 lightCoord = (lightTransform * vec3(gl_FragCoord.xy, 1.0)).xy;
#endif
}

slice(normal)
{
#ifdef HAVE_NORMALMAP
	vec4 normalPx = texture2D(normalTex, texcoord.xy);
	vec3 normalPxDir = 2.0 * (normalPx.xyz - vec3(0.5, 0.5, 0.5));
#ifdef MESH
	// For meshes, the normal matrix is typically provided in Clonk
	// coordinates, but the normal matrix incorporates a component that
	// transforms from Ogre to Clonk coordinates. Therefore, we need to
	// reverse that transformation for meshes.
	// TODO: This could be optimized since the matrix is so simple that
	// we don't need to do a full matrix multiplication.
	mat3 c2o = mat3(0.0, 1.0, 0.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0);
	vec3 normal = normalize(c2o * gl_NormalMatrix * normalPxDir);
#else
	vec3 normal = normalize(gl_NormalMatrix * normalPxDir);
#endif
#else
#ifdef MESH
	vec3 normal = normalDir; // Normal matrix is already applied in vertex shader
#else
	vec3 normal = vec3(0.0, 0.0, 1.0);
#endif
#endif
}
