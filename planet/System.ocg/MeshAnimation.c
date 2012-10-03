/*--
		MeshAnimation.c
		Authors: ck
		
		Wrappers for convenient calls to PlayAnimation.
--*/


global func Anim_Const(num value)
{
	return [C4AVP_Const, value];
}

global func Anim_Linear(num position, num begin, num end, num length, num ending)
{
	return [C4AVP_Linear, position, begin, end, length, ending];
}

global func Anim_X(num position, num begin, num end, num length)
{
	return [C4AVP_X, position, begin, end, length];
}

global func Anim_Y(num position, num begin, num end, num length)
{
	return [C4AVP_Y, position, begin, end, length];
}

global func Anim_R(num begin, num end)
{
	return [C4AVP_R, begin, end, this];
}

global func Anim_AbsX(num position, num begin, num end, num length)
{
	return [C4AVP_AbsX, position, begin, end, length];
}

global func Anim_AbsY(num position, num begin, num end, num length)
{
	return [C4AVP_AbsY, position, begin, end, length];
}

global func Anim_XDir(num begin, num end, num max_xdir, num prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_XDir, begin, end, max_xdir, prec];
}

global func Anim_YDir(num begin, num end, num max_ydir, num prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_YDir, begin, end, max_ydir, prec];
}

global func Anim_RDir(num begin, num end, num max_rdir, num prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_RDir, begin, end, max_rdir, prec];
}

global func Anim_CosR(num begin, num end, num offset, num prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_CosR, begin, end, offset, prec];
}

global func Anim_SinR(num begin, num end, num offset, num prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_SinR, begin, end, offset, prec];
}

global func Anim_CosV(num begin, num end, num offset, num prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_CosV, begin, end, offset, prec];
}

global func Anim_SinV(num begin, num end, num offset, num prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_SinV, begin, end, offset, prec];
}

global func Anim_Action()
{
	return [C4AVP_Action];
}
