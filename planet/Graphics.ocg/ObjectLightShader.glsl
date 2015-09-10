uniform mat3x2 lightTransform;
#ifdef OC_WITH_NORMALMAP
uniform sampler2D normalTex;
#endif

#ifdef OC_MESH
varying vec3 normalDir;
#endif

slice(texture+4)
{
#ifdef OC_DYNAMIC_LIGHT
	// prepare texture coordinate for light lookup in LightShader.c
	// Extra .xy since some old intel drivers return a vec3
	vec2 lightCoord = (lightTransform * vec3(gl_FragCoord.xy, 1.0)).xy;
#endif
}

slice(normal)
{
#ifdef OC_WITH_NORMALMAP
	vec4 normalPx = texture2D(normalTex, texcoord.xy);
	vec3 normalPxDir = 2.0 * (normalPx.xyz - vec3(0.5, 0.5, 0.5));
	vec3 normal = normalize(gl_NormalMatrix * normalPxDir);
#else
#ifdef OC_MESH
	vec3 normal = normalDir; // Normal matrix is already applied in vertex shader
#else
	vec3 normal = vec3(0.0, 0.0, 1.0);
#endif
#endif
}
