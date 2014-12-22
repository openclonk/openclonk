uniform sampler2D basemap;
uniform sampler2D normalTex;

#ifndef OPENCLONK
#define slice(x)
#define color gl_FragColor
varying vec2 texcoord;
void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
#endif

slice(init+1)
{
	// This picks up the normal map lookup in ObjectLightShader.c:
#define HAVE_NORMALMAP

	color = color * texture2D(basemap, texcoord);

#ifndef OPENCLONK
	// TODO: Could apply some default lighting here, for viewing the mesh in
	// a mesh viewer
#endif
}

#ifndef OPENCLONK
}
#endif
