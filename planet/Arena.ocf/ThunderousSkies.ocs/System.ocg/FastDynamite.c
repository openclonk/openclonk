/* Everyone wins! */

#appendto Dynamite

func Initialize()
{
	this.fast = 0;
	_inherited(...);
}

func MakeFast(int f)
{
	this.fast = f;
	this.FuseTime = 140 - this.fast;
}