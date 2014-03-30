/*--- Ruby Chunk ---*/

#include Library_CarryHeavy

public func GetCarryMode(clonk) { return CARRY_BothHands; }
public func GetCarryPhase() { return 800; }

// returns the color of the gem (used for effects)
func GetGemColor()
{
	return RGB(255, 20, 20);
}

func Initialize()
{
	AddEffect("Sparkle", this, 1, 30 + RandomX(-3, 3), this);
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
	if(this()->Contained() || !Random(2)) return FX_OK;
	CreateParticle("MagicRing", 0, 0, 0, 0, effect.Interval, effect.particles, 1);
	return FX_OK;
}

func IsValuable() { return true; }

func QueryOnSell()
{
	// Inform goal of gem sale
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (goal) goal->OnGemSold();
	return false; // do perform selling
}

func Hit()
{
	Sound("GlassHit*");
}

public func IsChunk() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(30,0,0,1),Trans_Rotate(-30,1,0,0),Trans_Scale(1300)),def);
}

local Name = "$Name$";
local Description = "$Description$";
local Touchable = 2;
local Plane = 480;