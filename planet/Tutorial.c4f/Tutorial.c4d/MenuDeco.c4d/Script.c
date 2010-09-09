/*-- Menu-Deco --*/

func FrameDecorationBackClr() { return RGBa(63,63,0,128); }
func FrameDecorationBorderTop()     { return 5; }
func FrameDecorationBorderLeft()    { return 5; }
func FrameDecorationBorderRight()   { return 5; }
func FrameDecorationBorderBottom()  { return 5; }

local Name = "MenuDeco";

local ActMap = {

		FrameDecoTopLeft = {
			Prototype = Action,
			Name = "FrameDecoTopLeft",
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 0,
			Wdt = 10,
			Hgt = 10,
			OffX = -5,
			OffY = -5,
		},

		FrameDecoTop = {
			Prototype = Action,
			Name = "FrameDecoTop",
			Length = 1,
			Delay = 0,
			X = 10,
			Y = 0,
			Wdt = 44,
			Hgt = 10,
			OffX = 0,
			OffY = -5,
		},

		FrameDecoTopRight = {
			Prototype = Action,
			Name = "FrameDecoTopRight",
			Length = 1,
			Delay = 0,
			X = 54,
			Y = 0,
			Wdt = 10,
			Hgt = 10,
			OffX = 0,
			OffY = -5,
		},

		FrameDecoRight = {
			Prototype = Action,
			Name = "FrameDecoRight",
			Length = 1,
			Delay = 0,
			X = 54,
			Y = 10,
			Wdt = 10,
			Hgt = 44,
			OffX = 0,
			OffY = 0,
		},

		FrameDecoBottomRight = {
			Prototype = Action,
			Name = "FrameDecoBottomRight",
			Length = 1,
			Delay = 0,
			X = 54,
			Y = 54,
			Wdt = 10,
			Hgt = 10,
			OffX = 0,
			OffY = 0,
		},

		FrameDecoBottom = {
			Prototype = Action,
			Name = "FrameDecoBottom",
			Length = 1,
			Delay = 0,
			X = 10,
			Y = 54,
			Wdt = 44,
			Hgt = 10,
			OffX = 0,
			OffY = 0,
		},

		FrameDecoBottomLeft = {
			Prototype = Action,
			Name = "FrameDecoBottomLeft",
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 54,
			Wdt = 10,
			Hgt = 10,
			OffX = -5,
			OffY = 0,
		},

		FrameDecoLeft = {
			Prototype = Action,
			Name = "FrameDecoLeft",
			Length = 1,
			Delay = 0,
			X = 0,
			Y = 10,
			Wdt = 10,
			Hgt = 44,
			OffX = -5,
			OffY = 0,
		},

		FrameDecoWall = {
			Prototype = Action,
			Name = "FrameDecoWall",
			Length = 1,
			Delay = 0,
			X = 11,
			Y = 11,
			Wdt = 42,
			Hgt = 42,
			OffX = 0,
			OffY = 0,
		}
};
