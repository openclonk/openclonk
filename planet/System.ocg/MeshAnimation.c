/*--
		MeshAnimation.c
		Authors: ck
		
		Wrappers for convenient calls to PlayAnimation.
--*/


global func Anim_Const(int value)
{
	return [C4AVP_Const, value];
}

global func Anim_Linear(int position, int begin, int end, int length, int ending)
{
	return [C4AVP_Linear, position, begin, end, length, ending];
}

global func Anim_X(int position, int begin, int end, int length)
{
	return [C4AVP_X, position, begin, end, length];
}

global func Anim_Y(int position, int begin, int end, int length)
{
	return [C4AVP_Y, position, begin, end, length];
}

global func Anim_AbsX(int position, int begin, int end, int length)
{
	return [C4AVP_AbsX, position, begin, end, length];
}

global func Anim_AbsY(int position, int begin, int end, int length)
{
	return [C4AVP_AbsY, position, begin, end, length];
}

global func Anim_XDir(int begin, int end, int max_xdir, int prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_XDir, begin, end, max_xdir, prec];
}

global func Anim_YDir(int begin, int end, int max_ydir, int prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_YDir, begin, end, max_ydir, prec];
}

global func Anim_RDir(int begin, int end, int max_rdir, int prec)
{
	if (prec == nil)
		prec = 10;
	return [C4AVP_RDir, begin, end, max_rdir, prec];
}

global func Anim_CosR(int begin, int end, int offset, int prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_CosR, begin, end, offset, prec];
}

global func Anim_SinR(int begin, int end, int offset, int prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_SinR, begin, end, offset, prec];
}

global func Anim_CosV(int begin, int end, int offset, int prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_CosV, begin, end, offset, prec];
}

global func Anim_SinV(int begin, int end, int offset, int prec)
{
	if (prec == nil)
		prec = 1;
	return [C4AVP_SinV, begin, end, offset, prec];
}

global func Anim_Action()
{
	return [C4AVP_Action];
}
