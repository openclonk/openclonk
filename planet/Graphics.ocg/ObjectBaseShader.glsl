uniform vec4 clrMod;

slice(init)
{
#define color gl_FragColor

#ifdef MESH
	// TODO: Add emission part of the material. Note we cannot just
	// add this to the color, but it would need to be handled separately,
	// such that it is independent from the incident light direction.
	color = gl_FrontMaterial.diffuse;
#else
	vec4 baseColor = gl_Color;
	color = baseColor;
#endif

}

slice(color)
{
	// TODO: Instead of a conditional, we could compute the color as
	// color = A + B*color + C*clrMod + D*color*clrMod;
	// Then, mod2 would be A=-0.5, B=1, C=1, D=0
	// and default would be A=0,B=0,C=0,D=1
	// Would allow to avoid conditionsals and #ifdefs, but need 4 uniforms...

	// Could also try some sort of 3x3 matrix:
	// out = (color, clrmod, 1) * (A,B,C,D,E,F,0,0,G) * (color, clrmod, 1)

#ifdef CLRMOD_MOD2
	color = clamp(color + clrMod - 0.5, 0.0, 1.0);
#else
	color = color * clrMod;
#endif
}
