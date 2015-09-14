
// Ambient light calculation

#ifdef OC_DYNAMIC_LIGHT
uniform sampler2D ambientTex;

uniform mat3x2 ambientTransform;
uniform float ambientBrightness;
#endif
//uniform float cullMode; // 0 if backface culling is enabled, 1 if it is disabled
// Already declared in LightShader.glsl

slice(texture+6)
{
#ifdef OC_DYNAMIC_LIGHT
	// Ambient light
	// Extra .xy since some old intel drivers return a vec3
	float ambient = texture2D(ambientTex, (ambientTransform * vec3(gl_FragCoord.xy, 1.0)).xy).r * ambientBrightness;
#else
	// Lighting disabled: Ambient light everywhere
	float ambient = 1.0;
#endif
}

slice(light+1)
{
	// Add ambience to brightness
#ifdef OC_LANDSCAPE
	// For landscape, ambient brightness is coming from top
	vec3 ambientDir = vec3(0.0, -1.0, 0.0);
	light = mix(light, 1.0 + 1.0 * dot(normal, ambientDir), ambient);
	#ifdef OC_HAVE_2PX
		light2 = mix(light2, 1.0 + 1.0 * dot(normal2, ambientDir), ambient);
	#endif
#else
	#ifdef OC_SKY
		// For sky, ambient brightness is coming from the front
		vec3 ambientDir = vec3(0.0, 0.0, 1.0);
		light = mix(light, max(max(dot(normal, ambientDir), 0.0), cullMode * max(dot(-normal, ambientDir), 0.0)), ambient);
	#else
		// For objects, ambient brightness is coming from the sky
		vec3 ambientDir = normalize(vec3(0.5, -0.5, 0.5));
		float ambientLight = max(dot(normal, ambientDir), 0.8);
		// Also, objects do have a rimlight
		vec3 rimDir = vec3(0.0, 0.0, 1.0);
		float rimLight = 1.0 - dot(normal, rimDir);
		ambientLight += ambientLight * rimLight;

		light = mix(light, ambientLight, ambient);
	#endif
#endif
}
