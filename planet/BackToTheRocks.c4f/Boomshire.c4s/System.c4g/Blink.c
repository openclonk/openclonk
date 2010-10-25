/* larger dynamite explosion */

#appendto SpinWheel


public func ControlUp(object clonk)
{
	if(GetEffect("SparklingAttention",this)) RemoveEffect("SparklingAttention",this);
	if (GetAction() == "Still" && targetdoor)
	{
		targetdoor->OpenGateDoor();
		SetAction("SpinLeft");
		Sound("Chain.ogg");
	}
}

