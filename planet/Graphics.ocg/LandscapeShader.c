
#version 110

// Input textures
uniform sampler2D landscapeTex[1];
uniform sampler2D scalerTex;
uniform sampler3D materialTex;

// Resolution of the landscape texture
uniform vec2 resolution;

// Texture map
uniform float matMap[256];
uniform sampler1D matMapTex;
uniform int materialDepth;

// Expected parameters for the scaler
const vec2 scalerStepX = vec2(1.0 / 8.0, 0.0);
const vec2 scalerStepY = vec2(0.0, 1.0 / 32.0);
const vec2 scalerOffset = vec2(0.0, 0.0) + scalerStepX / 3.0 + scalerStepY / 3.0;
const vec2 scalerPixel = vec2(scalerStepX.x, scalerStepY.y) / 3.0;

float queryMatMap(int pix)
{
#ifdef BROKEN_ARRAYS_WORKAROUND
	return texture1D(matMapTex, float(pix) / 256.0 + 0.5 / 256.0).r * 255.0 / float(materialDepth) + 0.5 / float(materialDepth);
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
	
	// gen2 other coordinate calculation (TODO: scaler map)
	vec2 otherCoo = centerCoo + fullStep * floor(normalize(mod(pixelCoo, vec2(1.0, 1.0)) - vec2(0.5, 0.5)) + vec2(0.5, 0.5));
	vec4 lopx = texture2D(landscapeTex[0], otherCoo);

	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;
	
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

	vec4 spx = texture2D(scalerTex, scalerCoo);

	// Get material pixels
	float mi = queryMatMap(int(lpx.r * 255.0));
	vec4 mpx = texture3D(materialTex, vec3(gl_TexCoord[0].st * resolution / vec2(512.0, 512.0) * vec2(3.0, 3.0), mi));
	float omi = queryMatMap(int(lopx.r * 255.0));
	vec4 ompx = texture3D(materialTex, vec3(gl_TexCoord[0].st * resolution / vec2(512.0, 512.0) * vec2(3.0, 3.0), omi));
	
	// Brightness
	vec2 normal = (1.5 * rlpx.yz - vec2(1.0, 1.0));
	float ambientBright = 1.0;
	float bright = ambientBright * (1.0 + dot(normal, vec2(0.0, -1.0)));
	float bright2 = ambientBright;

	gl_FragColor = mix(
		vec4(bright2 * ompx.rgb, ompx.a),
		vec4(bright * mpx.rgb, mpx.a),
		spx.r);
}
