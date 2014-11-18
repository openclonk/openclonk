varying vec2 texcoord;
uniform sampler2D basemap;
uniform sampler2D normalmap;

uniform int oc_Mod2;
uniform vec4 oc_ColorModulation;
uniform int oc_UseLight;
uniform sampler2D oc_Light;
uniform sampler2D oc_Ambient;

// This is mostly copied from C4DrawMeshGL.cpp -- only the calculation
// of the normal has been replaced
void main()
{
  vec4 lightClr;
  vec3 normalDir = normalize((texture2D(normalmap, texcoord).rgb * 2.0 - 1.0)); // TODO: This might be normalized already
  if(oc_UseLight != 0)
  {
    vec4 lightPx = texture2D(oc_Light, (gl_TextureMatrix[2] * gl_FragCoord).xy);
    vec3 lightDir = normalize(vec3(vec2(1.0, 1.0) - lightPx.gb * 3.0, 0.3));
    float lightIntensity = 2.0 * lightPx.r;
    float ambient = texture2D(oc_Ambient, (gl_TextureMatrix[3] * gl_FragCoord).xy).r;
    lightClr = ambient * (gl_FrontMaterial.emission + vec4(gl_FrontMaterial.diffuse.rgb * (0.25 + 0.75 * max(dot(normalDir, vec3(0.0, 0.0, 1.0)), 0.0)), gl_FrontMaterial.diffuse.a)) + (1.0 - ambient) * lightIntensity * (gl_FrontMaterial.emission + vec4(gl_FrontMaterial.diffuse.rgb * (0.25 + 0.75 * max(dot(normalDir, lightDir), 0.0)), gl_FrontMaterial.diffuse.a));
  }
  else
  {
    vec3 lightDir = vec3(0.0, 0.0, 1.0);
    lightClr = gl_FrontMaterial.emission + vec4(gl_FrontMaterial.diffuse.rgb * (0.25 + 0.75 * max(dot(normalDir, lightDir), 0.0)), gl_FrontMaterial.diffuse.a);
  }

  // Modulate with base texture
  vec4 finalColor = lightClr * texture2D(basemap, texcoord);

  // Apply openclonk blit parameters
  if(oc_Mod2 != 0)
    gl_FragColor = clamp(2.0 * finalColor * oc_ColorModulation - 0.5, 0.0, 1.0);
  else
    gl_FragColor = finalColor * oc_ColorModulation;
}
