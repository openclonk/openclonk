/**
	Colors.c
	All sorts of operations on colors.
	
	@author Tyron	
*/

static const RGBA_ALPHA = 0;
static const RGBA_RED = 1;
static const RGBA_GREEN = 2;
static const RGBA_BLUE = 3;

// documented in /docs/sdk/script/fn
global func HSL(int h, int s, int l)  { return HSL2RGB(RGB(h, s, l)); }
global func HSLa(int h, int s, int l, int a) { return  HSL2RGB(RGB(h, s, l)) ^ ~(a & 255) << 24; }

// documented in /docs/sdk/script/fn
global func RGB(int r, int g, int b) { return (255 << 24) | (r & 255) << 16 | (g & 255) << 8 | (b & 255); }
global func RGBa (int r, int g, int b, int a) { return (a & 255) << 24 | (r & 255) << 16 | (g & 255) << 8 | (b & 255); }

// documented in /docs/sdk/script/fn
global func GetRGBaValue(int val, int sel) { return val >> ((3 - sel) * 8) & 255; }
global func DoRGBaValue(int val, int chng, int sel) { return val + (chng << ((3 - sel) * 8)); }

// documented in /docs/sdk/script/fn
global func SetRGBaValue(int val, int newval, int sel)
{
	// 'delete' old color
	val = val & ~(255 << ((3 - sel) * 8));
	// add new
	return val | newval << ((3 - sel) * 8);
}

// documented in /docs/sdk/script/fn
global func SplitRGBaValue(int rgb)
{
	return {
		R = GetRGBaValue(rgb, RGBA_RED), 
		G = GetRGBaValue(rgb, RGBA_GREEN), 
		B = GetRGBaValue(rgb, RGBA_BLUE),
		Alpha = GetRGBaValue(rgb, RGBA_ALPHA)
	};
}

// documented in /docs/sdk/script/fn
global func HSL2RGB(int hsl)
{
	var hue = GetRGBaValue(hsl, 1), sat = GetRGBaValue(hsl, 2), lightness = GetRGBaValue(hsl, 3);
	var red, green, blue;
	var var1, var2;
	
	if (sat == 0)
	{
		red = green = blue = lightness;
	}
	else
	{
		if (lightness < 128)
			var2 = lightness * (255 + sat) / 255;
		else
			var2 = lightness + sat - lightness * sat / 255;
			
		var1 = 2 * lightness - var2;
			
		red = Hue_2_RGB(var1, var2, hue + 85);
		green = Hue_2_RGB(var1, var2, hue);
		blue = Hue_2_RGB(var1, var2, hue - 85);
	}
	
	return RGB(red, green, blue);
}

global func Hue_2_RGB(int var1, int var2, int hue)
{
	if (hue < 0)
		hue += 255;
	if (hue > 255)
		hue -= 255;
	if (6 * hue < 255)
		return var1 + (var2 - var1) * 6 * hue / 255;
	if (2 * hue < 255)
		return var2;
	if (3 * hue < 510)
		return var1 + (var2 - var1) * ( 510 / 3 - hue ) * 6 / 255;
	return var1;
}

// documented in /docs/sdk/script/fn
global func RGB2HSL(int rgb)
{
	var red = GetRGBaValue(rgb, RGBA_RED), green = GetRGBaValue(rgb, RGBA_GREEN), blue = GetRGBaValue(rgb, RGBA_BLUE);
	var min_val = Min(red, Min(green, blue)), max_val = Max(red, Max(green, blue));
	var diff_val = max_val - min_val;
	var lightness = (max_val + min_val) / 2;
	var hue, sat, diff_red, diff_green, diff_blue;
	
	if (diff_val==0)
	{
		hue = 0;
		sat = 0;
	}
	else
	{
		if (lightness < 128)
			sat = 255 * diff_val / (max_val + min_val);
		else
			sat = 255 * diff_val / (510 - (max_val + min_val));

		diff_red = ((255 * (max_val - red)) / 6 + (255 * diff_val) / 2) / diff_val;
		diff_green = ((255 * (max_val - green)) / 6 + (255 * diff_val) / 2) / diff_val;
		diff_blue = ((255 * (max_val - blue )) / 6 + (255 * diff_val) / 2) / diff_val;
			
		if (red == max_val)
			hue = diff_blue -diff_green;
		else if (green == max_val)
			hue = 255 / 3 + diff_red - diff_blue;
		else if (blue == max_val)
			hue = 510 / 3 + diff_green - diff_red;
		
		if (hue < 0)
			hue += 255;
		if (hue > 255)
			hue -= 255;
	}
	
	return RGB(hue, sat, lightness);
}
