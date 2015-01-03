uniform vec4 overlayClr;
uniform sampler2D overlayTex;

slice(texture+1)
{
	// Get overlay color from overlay texture
	vec4 overlay = baseColor * overlayClr * texture2D(overlayTex, texcoord.xy);
	// Mix overlay with texture
	float alpha0 = 1.0 - (1.0 - color.a) * (1.0 - overlay.a);
	color = vec4(mix(color.rgb, overlay.rgb, overlay.a / alpha0), alpha0);
}
