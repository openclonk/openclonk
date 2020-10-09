/**
	Helpers

	Helper functions for the benchmarks.
 */

// Call from object context
global func SetPositionAndSpeed(int x, int y, int xdir, int ydir)
{
	SetPosition(x, y);
	SetXDir(xdir);
	SetYDir(ydir);
	this.was_launched = true;
}
