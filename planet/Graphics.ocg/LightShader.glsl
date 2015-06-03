
// Base light calculations

#ifdef HAVE_LIGHT
uniform sampler2D lightTex;
#endif

// uncomment the following lines for debugging light directions:
// yellow: light up, blue: light down, turqoise: light right, pink: light left
// brightness: light strength
//#define LIGHT_DEBUG

// At what point of light intensity we set the "darkness" point. This
// is to compensate for the fact that the engien "smoothes" the light
// and therefore will often never arrive at 0 light intensity.
const float lightDarknessLevel = 8.0 / 256.0;

slice(texture+5)
{
#ifdef HAVE_LIGHT
	// Query light texture
	vec4 lightPx = texture2D(lightTex, lightCoord.st);
	float lightBright = max(0.0, lightPx.x-lightDarknessLevel);
	vec3 lightDir = extend_normal(vec2(1.0, 1.0) - lightPx.yz * 3.0);
#else
	// When lighting is disabled, put a light source coming from the camera.
	// Note that in most cases this does not actually matter, since in the
	// case with lighting disabled, ambient lighting takes fully over.
	float lightBright = 0.5;
	vec3 lightDir = vec3(0.0, 0.0, 1.0);
#endif
}

slice(light)
{
	float light = 2.0 * lightBright * max(dot(normal, lightDir), 0.0);
#ifdef HAVE_2PX
	float light2 = 2.0 * lightBright * max(dot(normal2, lightDir), 0.0);
#endif
}

slice(color+5)
{
	// Add light
	color = vec4(light * color.rgb, color.a);
#ifdef HAVE_2PX
	color2 = vec4(light2 * color2.rgb, color2.a);
#endif
}

slice(finish+5)
{

#ifdef LIGHT_DEBUG
#ifdef HAVE_LIGHT
	float lightYDir = lightPx.b - 1.0/3.0;
	float lightXDir = lightPx.g - 1.0/3.0;
	float lightStrength = lightPx.r;
	color =
	  vec4(lightStrength * vec3(1.0-1.5*(max(0.0, lightYDir) + max(0.0,lightXDir)),
	                            1.0-1.5*(max(0.0, lightYDir) + max(0.0,-lightXDir)),
	                            1.0-1.5*max(0.0, -lightYDir)),
	       1.0);
#else
    color = vec4(0.0, 0.0, 0.0, 0.0); // invisible
#endif
#endif

}
