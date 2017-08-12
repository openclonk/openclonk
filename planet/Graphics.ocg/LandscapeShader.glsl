
// Interpolated texture coordinates
in vec2 landscapeTexCoord;
#ifdef OC_DYNAMIC_LIGHT
in vec2 lightTexCoord;
#endif

// Input textures
uniform sampler2D landscapeTex[2];
uniform sampler2D scalerTex;
uniform sampler2DArray materialTex;

// Resolution of the landscape texture
uniform vec2 resolution;

// Center position
uniform vec2 center;
// Texture map
uniform sampler1D matMapTex;
uniform float materialDepth;
uniform vec2 materialSize;

// for SetMatAdjust
uniform vec4 clrMod;

out vec4 fragColor;

// Expected parameters for the scaler
const vec2 scalerStepX = vec2(1.0 / 8.0, 0.0);
const vec2 scalerStepY = vec2(0.0, 1.0 / 32.0);
const vec2 scalerOffset = scalerStepX / 3.0 + scalerStepY / 3.0;
const vec2 scalerPixel = vec2(scalerStepX.x, scalerStepY.y) / 3.0;

vec4 queryMatMap(int pix)
{
	return texture(matMapTex, float(pix) / 2.0 / 256.0 + 0.5 / 2.0 / 256.0);
}

slice(init)
{
	// How much % the normals from the normal map are added up to the
	// landscape normal. The higher the strength, the more structure
	// within the material is visible.
	//  0.0 = just textures
	//  1.0 = just material borders
	const float normalMapStrengthMin = 0.4;
	const float normalMapStrengthMax = 0.6;

	// Depth assumed for landscape normals. This decides how deep the
	// material "borders" appear to be. Lower means sharper normals
	// means more of a 3D look.
	float landscapeNormalDepth = 0.1;
}

slice(coordinate)
{
	// full pixel steps in the landscape texture (depends on landscape resolution)
	vec2 fullStep = vec2(1.0, 1.0) / resolution;
	vec2 fullStepX = vec2(fullStep.x, 0.0);
	vec2 fullStepY = vec2(0.0, fullStep.y);

	vec2 texCoo = landscapeTexCoord;

	// calculate pixel position in landscape, find center of current pixel
	vec2 pixelCoo = texCoo * resolution;
	vec2 centerCoo = (floor(pixelCoo) + vec2(0.5, 0.5)) / resolution;

	// Texture coordinate for material
	vec2 materialCoo = texCoo * resolution / materialSize;
}

slice(texture)
{
	// our pixel color (without/with interpolation)
	vec4 landscapePx = texture(landscapeTex[0], centerCoo);
	vec4 realLandscapePx = texture(landscapeTex[0], texCoo);

	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;
	int iScaler = f2i(landscapePx.a), iRow = iScaler / 8;
	scalerCoo.x += float(iScaler - iRow * 8) / 8.0;
	scalerCoo.y += float(iScaler / 8) / 32.0;

	// query scaler texture
	vec4 scalerPx = texture(scalerTex, scalerCoo);

	// Get "second" landscape pixel
	vec2 centerCoo2 = centerCoo + fullStep * floor(vec2(-0.5, -0.5) +
	                                               scalerPx.gb * 255.0 / 64.0);
	vec4 landscapePx2 = texture(landscapeTex[0], centerCoo2);

}

slice(texture+4)
{
#ifdef OC_DYNAMIC_LIGHT
	vec2 lightCoord = lightTexCoord;
#endif
}

slice(material)
{

	// Get material properties from material map
	int matMapIx = f2i(landscapePx.r);
	vec4 matMap = queryMatMap(2*matMapIx);
	vec4 matMapX = queryMatMap(2*matMapIx+1);
	float materialIx = int(matMap.a * (materialDepth - 1) + 0.5);
	vec3 matEmit = matMap.rgb;
	vec3 matSpot = matMapX.rgb * 255.9f / 16.0f;
	float matAngle = matMapX.a;

	// Query material texture pixels
	vec4 materialPx = texture(materialTex, vec3(materialCoo, materialIx));
	vec4 normalPx = texture(materialTex, vec3(materialCoo, materialIx+0.5 * materialDepth));
	// Same for second pixel
	int matMapIx2 = f2i(landscapePx2.r);
	vec4 matMap2 = queryMatMap(2*matMapIx2);
	vec4 matMapX2 = queryMatMap(2*matMapIx2+1);
	float materialIx2 = int(matMap2.a * (materialDepth - 1) + 0.5);
	vec3 matEmit2 = matMap2.rgb;
	vec3 matSpot2 = matMapX2.rgb * 255.9f / 16.0f;
	float matAngle2 = matMapX2.a;

	// Query material texture pixels
	vec4 materialPx2 = texture(materialTex, vec3(materialCoo, materialIx2));
	vec4 normalPx2 = texture(materialTex, vec3(materialCoo, materialIx2+0.5 * materialDepth));
}

slice(normal)
{
	// Normal calculation. At edges, take landscape normals
	// more into account (#1418).
	vec2 landscapeNormalPx = mix(realLandscapePx.yz, landscapePx.yz, scalerPx.a) - vec2(0.5, 0.5);
	vec3 landscapeNormal = normalize(vec3(landscapeNormalPx, landscapeNormalDepth));
	vec3 textureNormal = 2.0*(normalPx.xyz - vec3(0.5,0.5,0.5));
	textureNormal.y *= -1.0;
	float edgeFactor = 2.0 * length(landscapeNormalPx);
	float edgeStrength = normalMapStrengthMin + (normalMapStrengthMax - normalMapStrengthMin) * edgeFactor;
	vec3 normal = mix(textureNormal, landscapeNormal, edgeStrength);

	vec2 landscapeNormalPx2 = landscapePx2.yz - vec2(0.5, 0.5);
	vec3 landscapeNormal2 = normalize(vec3(landscapeNormalPx2, landscapeNormalDepth));
	vec3 textureNormal2 = 2.0*(normalPx2.xyz - vec3(0.5,0.5,0.5));
	textureNormal2.y *= -1.0;
	float edgeFactor2 = 2.0 * length(landscapeNormalPx2);
	float edgeStrength2 = normalMapStrengthMin + (normalMapStrengthMax - normalMapStrengthMin) * edgeFactor2;
	vec3 normal2 = mix(textureNormal2, landscapeNormal2, edgeStrength2);

}

slice(color) {
	fragColor = materialPx;
	vec4 color2 = materialPx2;
}

slice(color+10) {
	// Mix second color into main color according to scaler
	fragColor = mix(color2, fragColor, scalerPx.r);
	// Apply modulation
	fragColor *= clrMod;
}
