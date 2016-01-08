uniform sampler2D basemap;
uniform sampler2D normalTex;
uniform mat3 normalMatrix;

#ifndef OPENCLONK
in vec2 texcoord;
out vec4 fragColor;
#define slice(x)
void main()
{
	fragColor = vec4(1.0, 1.0, 1.0, 1.0);
#endif

slice(texture+1)
{
	fragColor = fragColor * texture(basemap, texcoord);

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
	vec4 normalPx = texture(normalTex, texcoord);
	vec3 normalPxDir = 2.0 * (normalPx.xyz - vec3(0.5, 0.5, 0.5));
	normal = normalize(normalMatrix * normalPxDir);
}

#ifndef OPENCLONK
}
#endif
