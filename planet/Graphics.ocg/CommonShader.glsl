
// uncomment the following lines for debugging light directions:
// yellow: light up, blue: light down, turqoise: light right, pink: light left
// brightness: light strength
//#define LIGHT_DEBUG

// uncomment the following lines for debugging light color:
// the light will always come from the front and have a uniform brightness.
//#define LIGHT_DEBUG_COLOR

// uncomment the following lines to set the light color to pink for all lights for debugging:
//#define LIGHT_DEBUG_PINK

#ifdef OC_DYNAMIC_LIGHT
uniform sampler2D ambientTex;

uniform mat3x2 ambientTransform;
uniform float ambientBrightness;

uniform sampler2D lightTex;
#endif

// Gamma uniforms
uniform vec3 gamma;

// 0 if backface culling is enabled, 1 if it is disabled
uniform float cullMode;

// Light dot product, taking cull mode into account
float lightDot(vec3 normal, vec3 lightDir) {
	return abs(max(-cullMode, dot(normalize(normal), lightDir)));
}

// Converts the pixel range 0.0..1.0 into the integer range 0..255
int f2i(float x) {
	return int(x * 255.9);
}

slice(init)
{

	// At what point of light intensity we set the "darkness" point. This
	// is to compensate for the fact that the engine "smoothes" the light
	// and therefore will often never arrive at 0 light intensity.
	float lightDarknessLevel = 8.0 / 256.0;

	// "Height" of the light in front of the screen. The lower this
	// value, the sharper the incoming light angles are. For
	// orientation: A value of 1 means 45 degrees maximum.
	float lightDepth = 0.5;

	// Position of the ambient light. Note that for normal directional
	// lights we have |X| <= 1.0 and |Y| <= 1.0, so it might be a good
	// idea to keep within this values.
	vec3 ambientLightPos = vec3(1.0, -1.0, lightDepth);

	// Amount of ambience we put into the ambient light. The higher
	// this is, the less directional ambient lighting is, and the
	// "flatter" the shading appears.
	float ambientAmbience = 1.0;

	// The total brightness assigned by the ambient shader to a
	// texture facing the viewer.
	float maxAmbientBrightness = 1.0;

	// The total brightness assigned by the ambient shader to a
	// texture facing the viewer.
	float maxLightBrightness = 3.0;

}

slice(texture+5)
{
#ifdef OC_DYNAMIC_LIGHT

	// Query light texture
	vec2 lightDirCoord = lightCoord;

	vec4  lightPx = texture2D(lightTex, lightDirCoord);
	float lightBright = maxLightBrightness * max(0.0, (lightPx.a-lightDarknessLevel)/(1.0-lightDarknessLevel));
	vec3  lightDir = normalize(vec3(vec2(1.0, 1.0) - lightPx.yz * 3.0, lightDepth));

	// Query light color texture (part of the light texture)
	vec2 lightColorCoord = lightCoord - vec2(0.0, 0.5); // subtract offset for the color texture

	vec3 lightColor = texture2D(lightTex, lightColorCoord.st).rgb;

	// Normalise light colour
	#ifdef LIGHT_DEBUG_COLOR
		lightBright = 0.5;
		lightColor = vec4(1.0, 0.0, 1.0, 1.0);
	#endif

	// Ambient light
	// Edxtra .xy since some old intel drivers return a vec3
	float ambient = texture2D(ambientTex, (ambientTransform * vec3(gl_FragCoord.xy, 1.0)).xy).r;
	ambient *= ambientBrightness;
#ifdef OC_SKY
	ambient = 0.999; // TODO: = 1.0 causes bugs?
#endif
#else
	// When lighting is disabled, put a light source coming from the camera.
	// Note that in most cases this does not actually matter, since in the
	// case with lighting disabled, ambient lighting takes fully over.
	float lightBright = 1.0;
	vec3  lightDir = vec3(0.0, 0.0, 1.0);
	vec3  lightColor = vec3(1.0, 1.0, 1.0);
	float ambient = 1.0;
#endif
}

slice(light)
{

	// Light dot product, taking backface culling into account
	float light = lightDot(normal, lightDir);
	// Amount of reflection, depending on the angle where material reflects most
	light = min(light / matAngle, 2.0 - light / matAngle);

#ifdef OC_LANDSCAPE
	normal2 = normalize(normal2);
	float light2 = lightDot(normal2, lightDir);
	light2 = min(light2 / matAngle2, 2.0 - light2 / matAngle2);
#endif

	// For landscape, ambient brightness is coming from top
	vec3 ambientDir = normalize(ambientLightPos);
	float ambientMul = maxAmbientBrightness / (ambientAmbience + lightDot(vec3(0.0,0.0,1.0), ambientDir));
	// Add ambience to brightness
	lightBright = mix(lightBright, 1.0, ambient);
	light = mix(light, ambientMul * (ambientAmbience + lightDot(normal, ambientDir)), ambient);
#ifdef OC_LANDSCAPE
	light2 = mix(light2, ambientMul * (ambientAmbience + lightDot(normal2, ambientDir)), ambient);
#endif
	lightColor = mix(lightColor, vec3(1.0,1.0,1.0), ambient);
}

slice(color+5)
{
	// Normalize light colour
	vec3 lightColorNorm = sqrt(3.0) * normalize(lightColor);

	// Add light. Depending on material properties, we make it more
	// "spotty" and allow the material to self-illuminate. The light
	// brightness overrules everything though (= FoW is last factor).
	vec3 spotLight = pow(vec3(light,light,light), matSpot);
	color.rgb = lightBright * color.rgb * (matEmit + lightColorNorm * spotLight);
#ifdef OC_LANDSCAPE
	vec3 spotLight2 = pow(vec3(light2,light2,light2), matSpot2);
	color2.rgb = lightBright * color2.rgb * (matEmit2 + lightColorNorm * spotLight2);
#endif
}

slice(finish+5)
{

#ifdef LIGHT_DEBUG
#ifdef OC_DYNAMIC_LIGHT
	float lightYDir = lightPx.b - 1.0/3.0;
	float lightXDir = lightPx.g - 1.0/3.0;
	float lightStrength = lightPx.a;
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

slice(finish+10) {
	color = vec4(pow(color.rgb, gamma), color.a);
}
