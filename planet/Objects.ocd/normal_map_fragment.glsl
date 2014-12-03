uniform sampler2D basemap;

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
	// This will make ObjectLightShader.glsl pick up the path that
	// Looks up the direction from the normal map.
#define HAVE_NORMALMAP

	color = color * gl_FrontMaterial.diffuse * texture2D(basemap, texcoord);

#ifndef OPENCLONK
	// TODO: Could apply some default lighting here, for viewing the mesh in
	// a mesh viewer
#endif
}

#ifndef OPENCLONK
}
#endif
