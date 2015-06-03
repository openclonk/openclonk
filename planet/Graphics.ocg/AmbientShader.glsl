
// Ambient light calculation

#ifdef HAVE_LIGHT
uniform sampler2D ambientTex;

uniform mat3x2 ambientTransform;
uniform float ambientBrightness;
#endif

slice(texture+6)
{
#ifdef HAVE_LIGHT
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
#ifdef LANDSCAPE
	// For landscape, ambient brightness is coming from top
	vec3 ambientDir = vec3(0.0, -1.0, 0.0);
	light = mix(light, 1.0 + 1.0 * dot(normal, ambientDir), ambient);
#ifdef HAVE_2PX
	light2 = mix(light2, 1.0 + 1.0 * dot(normal2, ambientDir), ambient);
#endif
#else
	// For objects, ambient brightness is coming from the front
	vec3 ambientDir = vec3(0.0, 0.0, 1.0);
	light = mix(light, max(dot(normal, ambientDir), 0.0), ambient);
#endif
}
