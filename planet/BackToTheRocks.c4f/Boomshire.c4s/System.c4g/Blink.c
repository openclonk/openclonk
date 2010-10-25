/* larger dynamite explosion */

#appendto SpinWheel


public func ControlUp(object clonk)
{
	if(GetEffect("SparklingAttention")) RemoveEffect("SparklingAttention");
	if (GetAction() == "Still" && targetdoor)
	{
		targetdoor->OpenGateDoor();
		SetAction("SpinLeft");
		Sound("Chain.ogg");
	}
}

