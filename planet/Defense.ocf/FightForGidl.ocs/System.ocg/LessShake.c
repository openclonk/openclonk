/* Overload viewport shake function to have lower range */

global func ShakeViewport(int level, int x_off, int y_off, range)
{
	return inherited(level, x_off, y_off, Min(range ?? 700, level*2));
}
