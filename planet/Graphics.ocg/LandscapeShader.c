
#version 110

// Input textures
uniform sampler2D landscapeTex[1];
uniform sampler2D scalerTex;
uniform sampler3D materialTex;

// Resolution of the landscape texture
uniform vec2 resolution;

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

void main()
{
	// full pixel steps in the landscape texture (depends on landscape resolution)
	vec2 fullStep = vec2(1.0, 1.0) / resolution;
	vec2 fullStepX = vec2(fullStep.x, 0.0);
	vec2 fullStepY = vec2(0.0, fullStep.y);

	// calculate pixel position in landscape, find center of current pixel
	vec2 pixelCoo = gl_TexCoord[0].st * resolution;
	vec2 centerCoo = (floor(pixelCoo) + vec2(0.5, 0.5)) / resolution;

	// our pixel color (with and without interpolation)
	vec4 lpx = texture2D(landscapeTex[0], centerCoo);
	vec4 rlpx = texture2D(landscapeTex[0], gl_TexCoord[0].st);

	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;
	
#ifdef SCALER_IN_GPU
	if(texture2D(landscapeTex[0], centerCoo - fullStepX - fullStepY).r == lpx.r)
		scalerCoo += scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo             - fullStepY).r == lpx.r)
		scalerCoo += 2.0 * scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX - fullStepY).r == lpx.r)
		scalerCoo += 4.0 * scalerStepX;
		
	if(texture2D(landscapeTex[0], centerCoo - fullStepX            ).r == lpx.r)
		scalerCoo += scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX            ).r == lpx.r)
		scalerCoo += 2.0 * scalerStepY;
	
	if(texture2D(landscapeTex[0], centerCoo - fullStepX + fullStepY).r == lpx.r)
		scalerCoo += 4.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo             + fullStepY).r == lpx.r)
		scalerCoo += 8.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX + fullStepY).r == lpx.r)
		scalerCoo += 16.0 * scalerStepY;

#else

	int iScaler = f2i(lpx.a), iRow = iScaler / 8;
	scalerCoo.x += float(iScaler - iRow * 8) / 8.0;
	scalerCoo.y += float(iScaler / 8) / 32.0;
	
#endif

	// Note: scalerCoo will jump around a lot, causing some GPUs to apparantly get confused with
	//       the level-of-detail calculation. We therefore try to disable LOD.
	vec4 spx = texture2DLod(scalerTex, scalerCoo, 0.0);

	// gen3 other coordinate calculation. Still struggles a bit with 3-ways
	vec2 otherCoo = centerCoo + fullStep * floor(vec2(-0.5, -0.5) + spx.gb * 255.0 / 64.0);
	vec4 lopx = texture2D(landscapeTex[0], otherCoo);
	
	// Get material pixels
	vec2 tcoo = gl_TexCoord[0].st * resolution / materialSize;
	float mi = queryMatMap(f2i(lpx.r));
	vec4 mpx = texture3D(materialTex, vec3(tcoo, mi));
	float omi = queryMatMap(f2i(lopx.r));
	vec4 ompx = texture3D(materialTex, vec3(tcoo, omi));
	
	// Brightness
	float ambientBright = 1.0, shadeBright = 0.8;	
	vec2 normal = (2.0 * mix(rlpx.yz, lpx.yz, spx.a) - vec2(1.0, 1.0));
	vec2 normal2 = (2.0 * lopx.yz - vec2(1.0, 1.0));
	float bright = ambientBright + shadeBright * dot(normal, vec2(0.0, -1.0));
	float bright2 = ambientBright + shadeBright * dot(normal2, vec2(0.0, -1.0));

	gl_FragColor = mix(
		vec4(bright2 * ompx.rgb, ompx.a),
		vec4(bright * mpx.rgb, mpx.a),
		spx.r);
}
