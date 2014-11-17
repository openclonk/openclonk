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
  vec3 normal = normalize((texture2D(normalmap, texcoord).rgb * 2.0 - 1.0)); // TODO: This might be normalized already
  vec4 diffuse;

  if(oc_UseLight != 0)
  {
    vec4 lightPx = texture2D(oc_Light, (gl_TextureMatrix[2] * gl_FragCoord).xy);
    vec3 lightDir = normalize(vec3(vec2(1.0, 1.0) - lightPx.gb * 3.0, 0.3));
    float lightIntensity = 2.0 * lightPx.r;
    float ambient = texture2D(oc_Ambient, (gl_TextureMatrix[3] * gl_FragCoord).xy).r;
    diffuse = ambient * (gl_FrontMaterial.emission + gl_FrontMaterial.diffuse * max(dot(normalize(normal), vec3(0.0, 0.0, 1.0)), 0.0)) + (1.0 - ambient) * lightIntensity * (gl_FrontMaterial.emission + gl_FrontMaterial.diffuse * max(dot(normalize(normal), lightDir), 0.0));
  }
  else
  {
    vec3 lightDir = vec3(0.0, 0.0, 1.0);
    diffuse = gl_FrontMaterial.emission + gl_FrontMaterial.diffuse * max(dot(normal, lightDir), 0.0);
  }

  // Modulate with base texture
  vec4 finalColor = diffuse * texture2D(basemap, texcoord);

  // Apply openclonk blit parameters
  if(oc_Mod2 != 0)
    gl_FragColor = clamp(2.0 * finalColor * oc_ColorModulation - 0.5, 0.0, 1.0);
  else
    gl_FragColor = finalColor * oc_ColorModulation;
}
