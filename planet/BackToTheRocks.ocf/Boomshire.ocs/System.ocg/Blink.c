// Remove the shining-effect on spinwheels when its turned.

#appendto SpinWheel


public func ControlUp(object clonk)
{
	if(GetEffect("SparklingAttention",this)) RemoveEffect("SparklingAttention",this);
	if (GetAction() == "Still" && targetdoor)
	{
		targetdoor->OpenDoor();
		SetAction("SpinLeft");
		Sound("Chain.ogg");
	}
}

