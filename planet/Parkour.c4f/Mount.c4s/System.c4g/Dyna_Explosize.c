/* larger dynamite explosion */

#appendto DYNA

func Explode(size)
{
  return _inherited(size+3, ...);
}
