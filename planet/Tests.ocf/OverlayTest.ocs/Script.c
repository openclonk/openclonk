func Initialize()
{
	var effect = AddEffect("IntOverlayTest", nil, 1, 10);
}

public func InitializePlayer(int player)
{
	var crew = GetCrew(player);
	crew->SetPosition(50, 200);
}

global func FxIntOverlayTestTimer(object target, proplist effect, int timer)
{
	if (effect.delay) { effect.delay--; return FX_OK;}

	if (!effect.initialized) return InitializeTest(effect);
	if (!effect.test1) return TestSequence001(effect);
	if (!effect.test2) return TestSequence002(effect);
	if (!effect.test3) return TestSequence003(effect);
	if (!effect.test4) return TestSequence004(effect);
	if (!effect.test5) return TestSequence005(effect);
	if (!effect.test6) return TestSequence006(effect);
	if (!effect.test7) return TestSequence007(effect);
	if (!effect.test8) return TestSequence008(effect);
	if (!effect.test9) return TestSequence009(effect);
	if (!effect.test10) return TestSequence010(effect);
}

global func InitializeTest(proplist effect)
{
	// objects
	effect.dummy1 = CreateObject(Dummy, 100, LandscapeHeight() / 2);
	effect.dummy2 = CreateObject(Dummy, 200, LandscapeHeight() / 2);
	effect.dummy3 = CreateObject(Dummy, 300, LandscapeHeight() / 2);
	effect.dummy4 = CreateObject(Dummy,  10, LandscapeHeight() / 2); // object with sprite graphics
    effect.dummy5 = CreateObject(Dummy,  20, LandscapeHeight() / 2); // object with mesh graphics

	// visibility
	effect.dummy1.Visibility = VIS_All;
	effect.dummy2.Visibility = VIS_All;
	effect.dummy3.Visibility = VIS_All;
	effect.dummy4.Visibility = VIS_All;
	effect.dummy5.Visibility = VIS_All;
	
	// done
	effect.initialized = true;
	return FX_OK;
}

global func TestSequence001(proplist effect)
{
	var graphics = Amethyst;
	effect.dummy1->SetShape(-64, -64, 128, 128);
	effect.dummy1->SetGraphics(nil, graphics);
	effect.dummy1->Message("This object should use the following graphics {{%i}}", graphics);
	effect.delay = 15;
	effect.test1 = true;
	return FX_OK;
}

global func TestSequence002(proplist effect)
{
	var graphics = Loam;
	effect.dummy2->SetShape(-5, -5, 11, 11);
	effect.dummy2->SetGraphics(nil, graphics);
	effect.dummy2->Message("This object should use the following graphics {{%i}}", graphics);
	effect.delay = 15;
	effect.test2 = true;
	return FX_OK;
}

global func TestSequence003(proplist effect)
{
	var graphics = Cloth;
	effect.dummy3->SetShape(-64, -64, 128, 128);
	effect.dummy3->SetGraphics(nil, graphics);
	effect.dummy3->Message("This object should use the following graphics {{%i}}", graphics);
	effect.delay = 15;
	effect.test3 = true;
	return FX_OK;
}

global func TestSequence004(proplist effect)
{
	var graphics = GoldBar;
	effect.dummy4->SetShape(-5, -2, 10, 4);
	effect.dummy4->SetGraphics(nil, graphics);
	effect.dummy4->Message("This object should use the following graphics {{%i}}", graphics);
	effect.delay = 15;
	effect.test4 = true;
	return FX_OK;
}

global func TestSequence005(proplist effect)
{
	var graphics = Hammer;
	effect.dummy5->SetShape(-4, -6, 8, 12);
	effect.dummy5->SetGraphics(nil, Hammer);
	effect.dummy5->SetGraphics(nil, graphics);
	effect.dummy5->Message("This object should use the following graphics {{%i}}", graphics);
	effect.delay = 15;
	effect.test5 = true;
	return FX_OK;
}

global func TestSequence006(proplist effect)
{
	// the effect.dummy2 object gets a lot of overlays
	effect.dummy2->SetGraphics(nil, Firestone, 1, GFXOV_MODE_Base); // sprite graphics as overlay
	effect.dummy2->SetGraphics(nil, Axe, 2, GFXOV_MODE_Base); // mesh graphics as overlay
	effect.dummy2->SetGraphics(nil, nil, 3, GFXOV_MODE_Object, nil, nil, effect.dummy4); // sprite graphics from other object
	effect.dummy2->SetGraphics(nil, nil, 4, GFXOV_MODE_Object, nil, nil, effect.dummy5); // mesh graphics from other object
	effect.dummy2->SetObjDrawTransform(1000, 0, -10000, 0, 1000, -32000, 0);
	effect.dummy2->SetObjDrawTransform(1000, 0, -10000, 0, 1000, -24000, 1);
	effect.dummy2->SetObjDrawTransform(1000, 0, -10000, 0, 1000, -16000, 2);
	effect.dummy2->SetObjDrawTransform(1000, 0, -10000, 0, 1000,  -8000, 3);
	effect.dummy2->SetObjDrawTransform(1000, 0, -10000, 0, 1000,      0, 4);
	effect.dummy2->Message("This object should have the graphics in this order now:|{{Loam}}|{{Firestone}}|{{Axe}}|{{GoldBar}}|{{Hammer}}");
	effect.dummy4->Message("*!*");
	effect.dummy5->Message("*!*");
	effect.delay = 25;
	effect.test6 = true;
	return FX_OK;
}

global func TestSequence007(proplist effect)
{
	// the effect.dummy1 object gets the effect.dummy2 as object overlay
	effect.dummy1->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, nil, effect.dummy2);
	effect.dummy1->Message("Should have the graphics of Nr.2 as overlay now");
	effect.dummy2->Message("> Nr.2");
	effect.test7 = true;
	effect.delay = 15;
	return FX_OK;
}

global func TestSequence008(proplist effect)
{
	// the effect.dummy3 object gets the effect.dummy2 as object overlay
	effect.dummy3->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, nil, effect.dummy2);
	effect.dummy3->Message("Should have the graphics of Nr.2 as overlay now");
	effect.dummy2->Message("> Nr.2");
	effect.test8 = true;
	effect.delay = 15;
	return FX_OK;
}

global func TestSequence009(proplist effect)
{
	// remove old objects
    effect.dummy1->RemoveObject();
    effect.dummy2->RemoveObject();
    effect.dummy3->RemoveObject();
    effect.dummy4->RemoveObject();
    effect.dummy5->RemoveObject();

	// objects
    effect.dummy1 = CreateObject(Rock, 100, LandscapeHeight() / 2);
    effect.dummy2 = CreateObject(Rock, 200, LandscapeHeight() / 2);
    effect.dummy3 = CreateObject(Rock, 300, LandscapeHeight() / 2);
    effect.dummy4 = CreateObject(Rock,  10, LandscapeHeight() / 2); // object with sprite graphics
    effect.dummy5 = CreateObject(Rock,  20, LandscapeHeight() / 2); // object with mesh graphics

	// done
	effect.test9 = true;
	effect.test1 = false;
	effect.test2 = false;
	effect.test3 = false;
	effect.test4 = false;
	effect.test5 = false;
	effect.test6 = false;
	effect.test7 = false;
	effect.test8 = false;
	effect.delay = 10;
	return FX_OK;
}

global func TestSequence010(proplist effect)
{
	// remove old objects
    effect.dummy1->RemoveObject();
    effect.dummy2->RemoveObject();
    effect.dummy3->RemoveObject();
    effect.dummy4->RemoveObject();
    effect.dummy5->RemoveObject();

	// objects
    effect.dummy1 = CreateObject(Hammer, 100, LandscapeHeight() / 2);
    effect.dummy2 = CreateObject(Hammer, 200, LandscapeHeight() / 2);
    effect.dummy3 = CreateObject(Hammer, 300, LandscapeHeight() / 2);
    effect.dummy4 = CreateObject(Hammer,  10, LandscapeHeight() / 2); // object with sprite graphics
    effect.dummy5 = CreateObject(Hammer,  20, LandscapeHeight() / 2); // object with mesh graphics

	// done
	effect.test10 = true;
	effect.test1 = false;
	effect.test2 = false;
	effect.test3 = false;
	effect.test4 = false;
	effect.test5 = false;
	effect.test6 = false;
	effect.test7 = false;
	effect.test8 = false;
	effect.delay = 10;
	return FX_OK;
}
