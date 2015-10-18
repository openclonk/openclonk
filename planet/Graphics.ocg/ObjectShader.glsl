
uniform mat3x2 lightTransform;

#ifdef OC_WITH_NORMALMAP
uniform sampler2D normalTex;
#endif

#ifdef OC_MESH
varying vec3 normalDir;
#endif

uniform vec4 clrMod;

#ifdef OC_HAVE_BASE
uniform sampler2D baseTex;
#endif

#ifdef OC_HAVE_OVERLAY
uniform vec4 overlayClr;
uniform sampler2D overlayTex;
#endif

slice(material)
{
	// Default material properties. TODO: Populate them?
	vec3 matEmit = vec3(0.0,0.0,0.0);
	vec3 matSpot = vec3(1.0,1.0,1.0);
	float matAngle = 1.0;
}

slice(texture)
{
#define color gl_FragColor

#ifdef OC_MESH
	// TODO: Add emission part of the material. Note we cannot just
	// add this to the color, but it would need to be handled separately,
	// such that it is independent from the incident light direction.
	color = gl_FrontMaterial.diffuse;
#else
	vec4 baseColor = gl_Color;
	color = baseColor;
#endif

#ifdef OC_HAVE_BASE
	color = baseColor * texture2D(baseTex, texcoord.xy);
#endif

#ifdef OC_HAVE_OVERLAY
	// Get overlay color from overlay texture
	vec4 overlay = baseColor * overlayClr * texture2D(overlayTex, texcoord.xy);
	// Mix overlay with texture
	float alpha0 = 1.0 - (1.0 - color.a) * (1.0 - overlay.a);
	color = vec4(mix(color.rgb, overlay.rgb, overlay.a / alpha0), alpha0);
#endif
}

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

slice(color)
{
	// TODO: Instead of a conditional, we could compute the color as
	// color = A + B*color + C*clrMod + D*color*clrMod;
	// Then, mod2 would be A=-0.5, B=1, C=1, D=0
	// and default would be A=0,B=0,C=0,D=1
	// Would allow to avoid conditionsals and #ifdefs, but need 4 uniforms...

	// Could also try some sort of 3x3 matrix:
	// out = (color, clrmod, 1) * (A,B,C,D,E,F,0,0,G) * (color, clrmod, 1)

#ifdef OC_CLRMOD_MOD2
	color = vec4(clamp(color.rgb + clrMod.rgb - 0.5, 0.0, 1.0), color.a * clrMod.a);
#else
	color = color * clrMod;
#endif
}
