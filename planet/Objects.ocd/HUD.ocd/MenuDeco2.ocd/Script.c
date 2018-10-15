/*-- Menu-Deco --*/

//func FrameDecorationBackClr() { return RGBa(25, 25, 25, 128); }
func FrameDecorationBorderTop()     { return 0; }
func FrameDecorationBorderLeft()    { return 0; }
func FrameDecorationBorderRight()   { return 0; }
func FrameDecorationBorderBottom()  { return 0; }

func Definition(def)
{
	var corner_size = 2;
	var border_size = 2;
	var border_length = 28;

	var offs = corner_size/2;
	var offs = corner_size/2;

	SetProperty("ActMap", {
		FrameDecoTopLeft = {
			Prototype = Action,
			Name = "FrameDecoTopLeft",
			X = 0,
			Y = 0,
			Wdt = corner_size,
			Hgt = corner_size,
			OffX = -offs,
			OffY = -offs,
		},
		FrameDecoTopRight = {
			Prototype = Action,
			Name = "FrameDecoTopRight",
			X = border_length + corner_size,
			Y = 0,
			Wdt = corner_size,
			Hgt = corner_size,
			OffX = 0,
			OffY = -offs,
		},
		FrameDecoBottomRight = {
			Prototype = Action,
			Name = "FrameDecoBottomRight",
			X = border_length + corner_size,
			Y = border_length + corner_size,
			Wdt = corner_size,
			Hgt = corner_size,
			OffX = 0,
			OffY = 0,
		},
		FrameDecoBottomLeft = {
			Prototype = Action,
			Name = "FrameDecoBottomLeft",
			X = 0,
			Y = border_length + corner_size,
			Wdt = corner_size,
			Hgt = corner_size,
			OffX = -offs,
			OffY = 0,
		},
		FrameDecoTop = {
			Prototype = Action,
			Name = "FrameDecoTop",
			X = corner_size,
			Y = 0,
			Wdt = border_length,
			Hgt = border_size,
			OffX = 0,
			OffY = -offs,
		},
		FrameDecoRight = {
			Prototype = Action,
			Name = "FrameDecoRight",
			X = corner_size + border_length,
			Y = corner_size,
			Wdt = border_size,
			Hgt = border_length,
			OffX = 0,
			OffY = 0,
		},
		FrameDecoBottom = {
			Prototype = Action,
			Name = "FrameDecoBottom",
			X = corner_size,
			Y = border_length + corner_size,
			Wdt = border_length,
			Hgt = border_size,
			OffX = +offs,
			OffY = 0,
		},
		FrameDecoLeft = {
			Prototype = Action,
			Name = "FrameDecoLeft",
			X = 0,
			Y = corner_size,
			Wdt = border_size,
			Hgt = border_length,
			OffX = -offs,
			OffY = 0,
		}
	});
}
