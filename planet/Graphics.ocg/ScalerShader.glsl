
#define HAVE_2PX

slice(texture+5)
{
	// find scaler coordinate
	vec2 scalerCoo = scalerOffset + mod(pixelCoo, vec2(1.0, 1.0)) * scalerPixel;

#ifdef SCALER_IN_GPU
	if(texture2D(landscapeTex[0], centerCoo - fullStepX - fullStepY).r == landscapePx.r)
		scalerCoo += scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo				- fullStepY).r == landscapePx.r)
		scalerCoo += 2.0 * scalerStepX;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX - fullStepY).r == landscapePx.r)
		scalerCoo += 4.0 * scalerStepX;

	if(texture2D(landscapeTex[0], centerCoo - fullStepX			   ).r == landscapePx.r)
		scalerCoo += scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX			   ).r == landscapePx.r)
		scalerCoo += 2.0 * scalerStepY;

	if(texture2D(landscapeTex[0], centerCoo - fullStepX + fullStepY).r == landscapePx.r)
		scalerCoo += 4.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo				+ fullStepY).r == landscapePx.r)
		scalerCoo += 8.0 * scalerStepY;
	if(texture2D(landscapeTex[0], centerCoo + fullStepX + fullStepY).r == landscapePx.r)
		scalerCoo += 16.0 * scalerStepY;
#else
	int iScaler = f2i(landscapePx.a), iRow = iScaler / 8;
	scalerCoo.x += float(iScaler - iRow * 8) / 8.0;
	scalerCoo.y += float(iScaler / 8) / 32.0;
#endif

	// Note: scalerCoo will jump around a lot, causing some GPUs to
	//		 apparantly get confused with the level-of-detail
	//		 calculation. We therefore try to disable LOD.
	vec4 scalerPx = texture2DLod(scalerTex, scalerCoo, 0.0);

	// gen3 other coordinate calculation. Still struggles a bit with 3-ways
	vec2 centerCoo2 = centerCoo + fullStep * floor(vec2(-0.5, -0.5) +
	                                               scalerPx.gb * 255.0 / 64.0);
	vec4 landscapePx2 = texture2D(landscapeTex[0], centerCoo2);

}

slice(color+10) {
	// Mix second color into main color according to scaler
	color = mix(color2, color, scalerPx.r);
}
