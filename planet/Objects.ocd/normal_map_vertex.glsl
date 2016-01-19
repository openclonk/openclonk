in vec3 oc_Position;
in vec3 oc_Normal;
in vec2 oc_TexCoord;

out vec3 vtxNormal;
out vec2 texcoord;

uniform mat4 projectionMatrix;
uniform mat4 modelviewMatrix;

#ifndef OPENCLONK
#define slice(x)

void main()
{
#endif

slice(texcoord)
{
  texcoord = oc_TexCoord;
}

slice(normal)
{
  // Dummy variable, it's not used in the fragment shader.
  // TODO: Improve construction of shaders so this is not needed,
  // but it's probably optimized out anyway. Also, remove
  // the oc_Normal attribute which is unused.
  vtxNormal = vec3(0.0, 0.0, 0.0);
}

slice(position)
{
  gl_Position = projectionMatrix * modelviewMatrix * vec4(oc_Position, 1.0);
}

#ifndef OPENCLONK
}
#endif
