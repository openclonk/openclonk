/* larger dynamite explosion */

#appendto Dynamite

func Explode(size)
{
	return _inherited(size+3, ...);
}
