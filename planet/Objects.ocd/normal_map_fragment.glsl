uniform sampler2D basemap;
uniform sampler2D normalTex;
uniform mat3 normalMatrix;

#ifndef OPENCLONK
varying vec2 texcoord;
#define slice(x)
#define color gl_FragColor
void main()
{
	color = vec4(1.0, 1.0, 1.0, 1.0);
#endif

slice(texture+1)
{
	color = color * texture2D(basemap, texcoord);

#ifndef OPENCLONK
	// TODO: Could apply some default lighting here, for viewing the mesh in
	// a mesh viewer
#endif
}

slice(normal+1)
{
	// TODO: This overrides what is set in slice(normal)
	// from ObjectShader.glsl. It's probably optimized out,
	// but a more elegant solution would be nice. Also maybe
	// a function in UtilShader.glsl to reduce code duplication.
	vec4 normalPx = texture2D(normalTex, texcoord);
	vec3 normalPxDir = 2.0 * (normalPx.xyz - vec3(0.5, 0.5, 0.5));
	normal = normalize(normalMatrix * normalPxDir);
}

#ifndef OPENCLONK
}
#endif
