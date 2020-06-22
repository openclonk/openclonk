/*--- Ruby ---*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 480;
local graphics_index = 0;

// returns the color of the gem (used for effects)
func GetGemColor()
{
	return RGB(255, 20, 20);
}

func Initialize()
{
	AddEffect("Sparkle", this, 1, 30 + RandomX(-3, 3), this);
	graphics_index = Random(4);
	if (graphics_index) SetGraphics(Format("%d", graphics_index + 1));
	return true;
}

func FxSparkleStart(target, effect, temp)
{
	if (temp) return;
	var color = this->~GetGemColor() ?? RGB(255, 20, 20);
	effect.particles =
	{
		Prototype = Particles_MagicRing(),
		R = (color >> 16) & 0xff,
		G = (color >>  8) & 0xff,
		B = (color >>  0) & 0xff,
	};
}

func FxSparkleTimer(target, effect, effect_time)
{
	if (this->Contained() || !Random(2)) return FX_OK;
	CreateParticle("MagicRing", 0, 0, 0, 0, effect.Interval, effect.particles, 1);
	return FX_OK;
}

func IsValuable() { return true; }
func QueryRejectRebuy() { return true; }

func OnSale(int to_player, object sale_base)
{
	// Inform goal of gem sale
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (goal) goal->OnGemSold();
	return true;
}

func Hit()
{
	Sound("Hits::Materials::Glass::GlassHit*");
	return true;
}