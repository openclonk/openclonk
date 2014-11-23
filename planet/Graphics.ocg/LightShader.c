
// Base light calculations

uniform sampler2D lightTex;

// uncomment the following lines for debugging light directions:
// yellow: light up, blue: light down, turqoise: light right, pink: light left
// brightness: light strength
//#define LIGHT_DEBUG

// At what point of light intensity we set the "darkness" point. This
// is to compensate for the fact that the engien "smoothes" the light
// and therefore will often never arrive at 0 light intensity.
const float lightDarknessLevel = 8.0 / 256.0;

slice texture+5
{
	// Query light texture
	vec4 lightPx = texture2D(lightTex, lightCoord.st);
	float lightBright = max(0.0, lightPx.x-lightDarknessLevel);
	vec3 lightDir = extend_normal(vec2(1.0, 1.0) - lightPx.yz * 3.0);
}

slice light
{
	// Light direction
	float light = 2.0 * lightBright * dot(normal, lightDir);
#ifdef HAVE_2PX
	float light2 = 2.0 * lightBright * dot(normal2, lightDir);
#endif
}

slice color+5
{
	// Add light
	color = vec4(light * color.rgb, color.a);
#ifdef HAVE_2PX
	color2 = vec4(light2 * color2.rgb, color2.a);
#endif
}

slice finish+5
{

#ifdef LIGHT_DEBUG
	float lightYDir = lightPx.b - 1.0/3.0;
	float lightXDir = lightPx.g - 1.0/3.0;
	float lightStrength = lightPx.r;
	gl_FragColor =
	  vec4(lightStrength * vec3(1.0-1.5*(max(0.0, lightYDir) + max(0.0,lightXDir)),
	                            1.0-1.5*(max(0.0, lightYDir) + max(0.0,-lightXDir)),
	                            1.0-1.5*max(0.0, -lightYDir)),
	       1.0);
#endif

}
