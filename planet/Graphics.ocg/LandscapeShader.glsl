

// Input textures
uniform sampler2D landscapeTex[2];
uniform sampler2D scalerTex;
uniform sampler3D materialTex;

// Resolution of the landscape texture
uniform vec2 resolution;

// Center position
uniform vec2 center;
// Texture map
#ifndef NO_BROKEN_ARRAYS_WORKAROUND
uniform sampler1D matMapTex;
#else
uniform float matMap[256];
#endif
uniform int materialDepth;
uniform vec2 materialSize;

// Expected parameters for the scaler
const vec2 scalerStepX = vec2(1.0 / 8.0, 0.0);
const vec2 scalerStepY = vec2(0.0, 1.0 / 32.0);
const vec2 scalerOffset = scalerStepX / 3.0 + scalerStepY / 3.0;
const vec2 scalerPixel = vec2(scalerStepX.x, scalerStepY.y) / 3.0;

// Parameters

// how much % the normals from the normal map are added up to the landscape normal. The higher the strength, the more
// structure within the material is visible but also the less the borders between the different materials stand out.
const float normalMapStrength = 0.75;

float queryMatMap(int pix)
{
#ifndef NO_BROKEN_ARRAYS_WORKAROUND
	int idx = f2i(texture1D(matMapTex, float(pix) / 256.0 + 0.5 / 256.0).r);
	return float(idx) / 256.0 + 0.5 / float(materialDepth);
#else
	return matMap[pix];
#endif
}

slice(coordinate)
{
	// full pixel steps in the landscape texture (depends on landscape resolution)
	vec2 fullStep = vec2(1.0, 1.0) / resolution;
	vec2 fullStepX = vec2(fullStep.x, 0.0);
	vec2 fullStepY = vec2(0.0, fullStep.y);

	vec2 texCoo = gl_TexCoord[0].st;

	// calculate pixel position in landscape, find center of current pixel
	vec2 pixelCoo = texCoo * resolution;
	vec2 centerCoo = (floor(pixelCoo) + vec2(0.5, 0.5)) / resolution;

	// Texture coordinate for material
	vec2 materialCoo = texCoo * resolution / materialSize;
}

slice(texture)
{
	// our pixel color (without/with interpolation)
	vec4 landscapePx = texture2D(landscapeTex[0], centerCoo);
	vec4 realLandscapePx = texture2D(landscapeTex[0], texCoo);
}

slice(material)
{

	// Get material pixels
	float materialIx = queryMatMap(f2i(landscapePx.r));
	vec4 materialPx = texture3D(materialTex, vec3(materialCoo, materialIx));
	vec4 normalPx = texture3D(materialTex, vec3(materialCoo, materialIx+0.5));

	// Same for second pixel, but we'll simply use the first normal
#ifdef HAVE_2PX
	float materialIx2 = queryMatMap(f2i(landscapePx2.r));
	vec4 materialPx2 = texture3D(materialTex, vec3(materialCoo, materialIx2));
	vec4 normalPx2 = texture3D(materialTex, vec3(materialCoo, materialIx2+0.5));
#endif
}

slice(normal)
{
	// Normal calculation
	vec3 normal = extend_normal(mix(realLandscapePx.yz, landscapePx.yz, scalerPx.a)
								- vec2(0.5, 0.5));
	vec3 textureNormal = normalPx.xyz - vec3(0.5,0.5,0.5);
	normal = normal + textureNormal * normalMapStrength;

#ifdef HAVE_2PX
	vec3 normal2 = extend_normal(landscapePx2.yz - vec2(0.5, 0.5));
	vec3 textureNormal2 = normalPx2.xyz - vec3(0.5,0.5,0.5);
	normal2 = normal2 + textureNormal2 * normalMapStrength;
#endif

}

slice(color) {
#define color gl_FragColor
	color = materialPx;
#ifdef HAVE_2PX
	vec4 color2 = materialPx2;
#endif
}
