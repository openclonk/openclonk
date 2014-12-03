varying vec2 texcoord;

#ifndef OPENCLONK
#define slice(x)
void main()
{
#endif

slice(texcoord)
{
  texcoord = gl_MultiTexCoord0.xy;
}

slice(position)
{
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

#ifndef OPENCLONK
}
#endif
