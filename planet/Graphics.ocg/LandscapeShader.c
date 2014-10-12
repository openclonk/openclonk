
#version 110

// Input textures
uniform sampler2D landscapeTex[2];
uniform sampler2D lightTex;
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

#ifdef NO_TEXTURE_LOD_IN_FRAGMENT
#define texture1DLod(t,c,l) texture1D(t,c)
#define texture2DLod(t,c,l) texture2D(t,c)
#endif

// Converts the pixel range 0.0..1.0 into the integer range 0..255
int f2i(float x) {
	return int(x * 255.9);
}

float queryMatMap(int pix)
{
#ifndef NO_BROKEN_ARRAYS_WORKAROUND
	int idx = f2i(texture1D(matMapTex, float(pix) / 256.0 + 0.5 / 256.0).r);
	return float(idx) / 256.0 + 0.5 / float(materialDepth);
#else
	return matMap[pix];
#endif
}

vec3 extend_normal(vec2 v)
{
	return normalize(vec3(v, 0.3));
}

void main()
{

	// full pixel steps in the landscape texture (depends on landscape resolution)
	vec2 fullStep = vec2(1.0, 1.0) / resolution;
	vec2 fullStepX = vec2(fullStep.x, 0.0);
	vec2 fullStepY = vec2(0.0, fullStep.y);

	vec2 texCoo = gl_TexCoord[0].st;
	
	// calculate pixel position in landscape, find center of current pixel
	vec2 pixelCoo = texCoo * resolution;
	vec2 centerCoo = (floor(pixelCoo) + vec2(0.5, 0.5)) / resolution;

	// our pixel color (without/with interpolation)
	vec4 landscapePx = texture2D(landscapeTex[0], centerCoo);
	vec4 realLandscapePx = texture2D(landscapeTex[0], texCoo);

	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;

#ifdef SCALER_IN_GPU
	if(texture2D(landscapeTex[0], centerCoo - fullStepX - fullStepY).r == landscapePx.r)
		scalerCoo += scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo             - fullStepY).r == landscapePx.r)
		scalerCoo += 2.0 * scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX - fullStepY).r == landscapePx.r)
		scalerCoo += 4.0 * scalerStepX;
		
	if(texture2D(landscapeTex[0], centerCoo - fullStepX            ).r == landscapePx.r)
		scalerCoo += scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX            ).r == landscapePx.r)
		scalerCoo += 2.0 * scalerStepY;
	
	if(texture2D(landscapeTex[0], centerCoo - fullStepX + fullStepY).r == landscapePx.r)
		scalerCoo += 4.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo             + fullStepY).r == landscapePx.r)
		scalerCoo += 8.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX + fullStepY).r == landscapePx.r)
		scalerCoo += 16.0 * scalerStepY;

#else

	int iScaler = f2i(landscapePx.a), iRow = iScaler / 8;
	scalerCoo.x += float(iScaler - iRow * 8) / 8.0;
	scalerCoo.y += float(iScaler / 8) / 32.0;
	
#endif

	// Note: scalerCoo will jump around a lot, causing some GPUs to apparantly get confused with
	//       the level-of-detail calculation. We therefore try to disable LOD.
	vec4 scalerPx = texture2DLod(scalerTex, scalerCoo, 0.0);

	// gen3 other coordinate calculation. Still struggles a bit with 3-ways
	vec2 otherCoo = centerCoo + fullStep * floor(vec2(-0.5, -0.5) + scalerPx.gb * 255.0 / 64.0);
	vec4 otherLandscapePx = texture2D(landscapeTex[0], otherCoo);

	// Get material pixels
	float materialIx = queryMatMap(f2i(landscapePx.r));
	vec2 tcoo = texCoo * resolution / materialSize;
	vec4 materialPx = texture3D(materialTex, vec3(tcoo, materialIx));
	vec4 normalPx = texture3D(materialTex, vec3(tcoo, materialIx+0.5));
	float otherMaterialIx = queryMatMap(f2i(otherLandscapePx.r));
	vec4 otherMaterialPx = texture3D(materialTex, vec3(tcoo, otherMaterialIx));

	// Brightness
	vec4 lightPx = texture2D(lightTex, gl_TexCoord[1].st);
	float ambientBright = lightPx.r, shadeBright = ambientBright;

	// Normal calculation
	vec3 landscapeNormal = extend_normal(mix(realLandscapePx.yz, landscapePx.yz, scalerPx.a) - vec2(0.5, 0.5));
	vec3 landscapeNormal2 = extend_normal(otherLandscapePx.yz - vec2(0.5, 0.5));
	vec3 textureNormal = normalPx.xyz - vec3(0.5,0.5,0.5);
	vec3 normal = landscapeNormal + textureNormal;
	vec3 normal2 = landscapeNormal2 + textureNormal;

	// Light calculation
	vec3 lightDir = extend_normal(vec2(1.0, 1.0) - lightPx.yz * 3.0);
	float bright = 2.0 * shadeBright * dot(normal, lightDir);
	float bright2 = 2.0 * shadeBright * dot(normal2, lightDir);

	gl_FragColor = mix(
		vec4(bright2 * otherMaterialPx.rgb, otherMaterialPx.a),
		vec4(bright * materialPx.rgb, materialPx.a),
		scalerPx.r);
	
	// uncomment the following lines for debugging light directions:
	// yellow: light up, blue: light down, turqoise: light right, pink: light left, opacity: light strength
	//float lightYDir = lightPx.b - 1.0/3.0;
	//float lightXDir = lightPx.g - 1.0/3.0;
	//float lightStrength = lightPx.r;
	//gl_FragColor = vec4(
	//	1.0-1.5*(max(0.0, lightYDir) + max(0.0,lightXDir)),
	//	1.0-1.5*(max(0.0, lightYDir) + max(0.0,-lightXDir)),
	//	1.0-1.5*max(0.0, -lightYDir),
	//	lightStrength);
}
