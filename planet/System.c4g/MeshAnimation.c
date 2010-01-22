
#strict 2

// Wrappers for conventient calls to PlayAnimation

global func Anim_Const(int Value)
{
	return [C4AVP_Const, Value];
}

global func Anim_Linear(int Position, int Begin, int End, int Length, int Ending)
{
	return [C4AVP_Linear, Position, Begin, End, Length, Ending];
}

global func Anim_X(int Position, int Begin, int End, int Length)
{
	return [C4AVP_X, Position, Begin, End, Length];
}

global func Anim_Y(int Position, int Begin, int End, int Length)
{
	return [C4AVP_Y, Position, Begin, End, Length];
}

global func Anim_AbsX(int Position, int Begin, int End, int Length)
{
	return [C4AVP_AbsX, Position, Begin, End, Length];
}

global func Anim_AbsY(int Position, int Begin, int End, int Length)
{
	return [C4AVP_AbsY, Position, Begin, End, Length];
}

global func Anim_XDir(int Begin, int End, int MaxXDir, int Prec)
{
	if(Prec == nil) Prec = 10;
	return [C4AVP_XDir, Begin, End, MaxXDir, Prec];
}

global func Anim_YDir(int Begin, int End, int MaxYDir, int Prec)
{
	if(Prec == nil) Prec = 10;
	return [C4AVP_YDir, Begin, End, MaxYDir, Prec];
}

global func Anim_RDir(int Begin, int End, int MaxRDir, int Prec)
{
	if(Prec == nil) Prec = 10;
	return [C4AVP_RDir, Begin, End, MaxRDir, Prec];
}

global func Anim_CosR(int Begin, int End, int Offset, int Prec)
{
	if(Prec == nil) Prec = 1;
	return [C4AVP_CosR, Begin, End, Offset, Prec];
}

global func Anim_SinR(int Begin, int End, int Offset, int Prec)
{
	if(Prec == nil) Prec = 1;
	return [C4AVP_SinR, Begin, End, Offset, Prec];
}

global func Anim_CosV(int Begin, int End, int Offset, int Prec)
{
	if(Prec == nil) Prec = 1;
	return [C4AVP_CosV, Begin, End, Offset, Prec];
}

global func Anim_SinV(int Begin, int End, int Offset, int Prec)
{
	if(Prec == nil) Prec = 1;
	return [C4AVP_SinV, Begin, End, Offset, Prec];
}
