
#version 130

// Input textures
uniform sampler2D landscapeTex[1];
uniform sampler2D scalerTex;
uniform sampler3D materialTex;

// Resolution of the landscape texture
uniform vec2 resolution;

// Texture map
uniform float matTexMap[256];

// Expected parameters for the scaler
const vec2 scalerStepX = vec2(1.0 / 8.0, 0.0);
const vec2 scalerStepY = vec2(0.0, 1.0 / 32.0);
const vec2 scalerOffset = vec2(0.0, 0.0) + scalerStepX / 3.0 + scalerStepY / 3.0;
const vec2 scalerPixel = vec2(scalerStepX.x, scalerStepY.y) / 3.0;

// Our input and output
in vec4 gl_TexCoord[];
out vec4 gl_FragColor;

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
	vec4 lpx = texture(landscapeTex[0], centerCoo);
	vec4 rlpx = texture(landscapeTex[0], gl_TexCoord[0].st);
	
	// gen2 other coordinate calculation (TODO: scaler map)
	vec2 otherCoo = centerCoo + fullStep * round(normalize(mod(pixelCoo, vec2(1.0, 1.0)) - vec2(0.5, 0.5)));
	vec4 lopx = texture(landscapeTex[0], otherCoo);

	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;
	
	if(texture(landscapeTex[0], centerCoo - fullStepX - fullStepY).r == lpx.r)
		scalerCoo += scalerStepX;
	if(texture(landscapeTex[0], centerCoo             - fullStepY).r == lpx.r)
		scalerCoo += 2.0 * scalerStepX;
	if(texture(landscapeTex[0], centerCoo + fullStepX - fullStepY).r == lpx.r)
		scalerCoo += 4.0 * scalerStepX;
		
	if(texture(landscapeTex[0], centerCoo - fullStepX            ).r == lpx.r)
		scalerCoo += scalerStepY;
	if(texture(landscapeTex[0], centerCoo + fullStepX            ).r == lpx.r)
		scalerCoo += 2.0 * scalerStepY;
	
	if(texture(landscapeTex[0], centerCoo - fullStepX + fullStepY).r == lpx.r)
		scalerCoo += 4.0 * scalerStepY;
	if(texture(landscapeTex[0], centerCoo             + fullStepY).r == lpx.r)
		scalerCoo += 8.0 * scalerStepY;
	if(texture(landscapeTex[0], centerCoo + fullStepX + fullStepY).r == lpx.r)
		scalerCoo += 16.0 * scalerStepY;

	vec4 spx = textureLod(scalerTex, scalerCoo, 0.0);

	// Material pixel
	float mix = matTexMap[int(lpx.r * 255.0)];
	vec4 mpx = texture(materialTex, vec3(gl_TexCoord[0].st * resolution / vec2(512.0, 512.0) * vec2(3.0, 3.0), mix));
	float omix = matTexMap[int(lopx.r * 255.0)];
	vec4 ompx = texture(materialTex, vec3(gl_TexCoord[0].st * resolution / vec2(512.0, 512.0) * vec2(3.0, 3.0), omix));
	
	// Brightness
	vec2 normal = (1.5 * rlpx.yz - vec2(1.0, 1.0));
	float ambientBright = 1.0;
	float bright = ambientBright * (1.0 + dot(normal, vec2(0.0, -1.0)));
	float bright2 = ambientBright;

	gl_FragColor = vec4(
		bright * spx.r * mpx.rgb + bright2 * (1.0-spx.r) * ompx.rgb,
		spx.r * mpx.a + (1.0-spx.r) * ompx.a);
}