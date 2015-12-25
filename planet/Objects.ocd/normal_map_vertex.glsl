uniform mat4 projectionMatrix;
uniform mat4 modelviewMatrix;

varying vec3 normalDir;

#ifndef OPENCLONK
#define slice(x)
varying vec2 texcoord;

void main()
{
#endif

slice(texcoord)
{
  texcoord = gl_MultiTexCoord0.xy;
}

slice(normal)
{
  // Dummy variable, it's not used in the fragment shader.
  // TODO: Improve construction of shaders so this is not needed.
  normalDir = vec3(0.0, 0.0, 0.0);
}

slice(position)
{
  gl_Position = projectionMatrix * modelviewMatrix * gl_Vertex;
}

#ifndef OPENCLONK
}
#endif
