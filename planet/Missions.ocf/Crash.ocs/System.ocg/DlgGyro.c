// NPC Gyro: Someone talking about catapult usage

#appendto Dialogue

func Dlg_Gyro_1(object clonk)
{
	MessageBox("$Gyro1$", clonk, clonk); // That's a weird spot for a clonk to stand in.
	return true;
}

func Dlg_Gyro_2(object clonk)
{
	MessageBox("$Gyro2$", clonk); // Yeah, I know! I'm here at this weird place because one of my experiments went horribly wrong...
	return true;
}

func Dlg_Gyro_3(object clonk)
{
	MessageBox("$Gyro3$", clonk, clonk); // What happend?
	return true;
}

func Dlg_Gyro_4(object clonk)
{
	MessageBox("$Gyro4$", clonk); // You see, fuel has gotten pretty rare and expensive recently. To save money, I tried to reach remote places by launching myself with a catapult!
	return true;
}

func Dlg_Gyro_5(object clonk)
{
	MessageBox("$Gyro5$", clonk, clonk); // That sounds pretty dangerous.
	return true;
}

func Dlg_Gyro_6(object clonk)
{
	MessageBox("$Gyro6$", clonk); // Bah, if your aim is good, you can just hop into a catapult and off you go! It's a really good way to reach distant locations quickly. Only a few people seem to know this secret! And even fewer actually do it!
	return true;
}

func Dlg_Gyro_7(object clonk)
{
	MessageBox("$Gyro7$", clonk, clonk); // I wonder why...
	return true;
}

func Dlg_Gyro_8(object clonk)
{
	MessageBox("$Gyro8$", clonk); // Well, to be fair... it is pretty dangerous. I almost flung myself into the lava lake because I couldn't find my glasses. Fortunately I missed it by just a few inches and landed here.
	return true;
}

func Dlg_Gyro_9(object clonk)
{
	MessageBox("$Gyro9$", clonk); // Stubbed my toe pretty bad though. Should have put on my safety-sandals...
	SetDialogueProgress(0);
	return StopDialogue();
}
