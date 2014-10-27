varying vec2 texcoord;
uniform sampler2D base;
uniform sampler2D normalmap;

uniform int oc_Mod2;
uniform vec4 oc_ColorModulation;
uniform int oc_UseClrModMap;
uniform sampler2D oc_ClrModMap;

void main()
{
  // Do the lights calculation (based on normal map)
  vec3 normal = normalize((texture2D(normalmap, texcoord).rgb * 2.0 - 1.0)); // TODO: This might be normalized already
  vec3 lightDir = normalize(gl_LightSource[0].position.xyz);
  vec4 lightColor = clamp(gl_FrontLightModelProduct.sceneColor + gl_FrontLightProduct[0].ambient + gl_FrontLightProduct[0].diffuse * max(0.0, dot(normal, lightDir)), 0.0, 1.0);

  // Modulate with base texture
  vec4 finalColor = lightColor * texture2D(base, texcoord);

  // Apply openclonk blit parameters
  vec4 clrModMapClr = vec4(1.0, 1.0, 1.0, 1.0);
  if(oc_UseClrModMap != 0)
    clrModMapClr = texture2D(oc_ClrModMap, gl_FragCoord.xy);
  if(oc_Mod2 != 0)
    gl_FragColor = clamp(2.0 * finalColor * oc_ColorModulation * clrModMapClr - 0.5, 0.0, 1.0);
  else
    gl_FragColor = finalColor * oc_ColorModulation * clrModMapClr;
}
