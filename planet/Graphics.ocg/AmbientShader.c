
// Ambient light calculation

uniform sampler2D ambientTex;

// Factor between landscape coordinates and ambient map coordinates
uniform vec2 ambientScale;

slice texture+6
{
	// Ambient light
	float ambient = texture2D(ambientTex, ambientScale * texCoo).r;
}

slice light+1
{
	// Add ambience to brightness
	vec3 ambientDir = vec3(0.0, -1.0, 0.0);
	light = mix(light, 1.0 + 1.0 * dot(normal, ambientDir), ambient);
#ifdef HAVE_2PX
	light2 = mix(light2, 1.0 + 1.0 * dot(normal2, ambientDir), ambient);
#endif
}
