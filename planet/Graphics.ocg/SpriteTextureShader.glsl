uniform sampler2D baseTex;

slice(texture)
{
	color = baseColor * texture2D(baseTex, texcoord.xy);
}
